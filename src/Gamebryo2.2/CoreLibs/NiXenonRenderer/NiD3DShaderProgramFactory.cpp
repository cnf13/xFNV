#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramFactory.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreator.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DUtility.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"

void* GetNext(NiTListIterator& arIterator) {
    NiTListItem<void*>* pkItem = static_cast<NiTListItem<void*>*>(arIterator);
    if (!pkItem) {
        return 0;
    }
    arIterator = pkItem->m_pkNext;
    return pkItem->m_element;
}

// Both shader maps have 59 buckets and do not copy their keys.
NiD3DShaderProgramFactory::NiD3DShaderProgramFactory()
  : m_pkVertexShaderMap(new NiTStringPointerMap<NiD3DVertexShader*>(59, false)),
    m_pkPixelShaderMap(new NiTStringPointerMap<NiD3DPixelShader*>(59, false)) {}

// Empties and deletes both shader maps, then the program directories.
NiD3DShaderProgramFactory::~NiD3DShaderProgramFactory() {
    if (m_pkVertexShaderMap) {
        m_pkVertexShaderMap->RemoveAll();
    }
    delete m_pkVertexShaderMap;
    if (m_pkPixelShaderMap) {
        m_pkPixelShaderMap->RemoveAll();
    }
    delete m_pkPixelShaderMap;
    RemoveAllProgramDirectories();
}

// The length is computed once, since replacing '/' with '\\' keeps the length.
void NiD3DShaderProgramFactory::AddProgramDirectory(const char* pacDirectory) {
    if (!pacDirectory || pacDirectory[0] == '\0') {
        return;
    }
    char acSourcePath[0x104];
    strcpy_s(acSourcePath, 0x104, pacDirectory);
    size_t uiLength = 0;
    while (acSourcePath[uiLength]) {
        ++uiLength;
    }
    for (size_t ui = 0; ui < uiLength; ++ui) {
        if (acSourcePath[ui] == '/') {
            acSourcePath[ui] = '\\';
        }
    }
    for (NiTListItem<char*>* pkItem = m_kProgramDirectories.m_pkHead; pkItem;) {
        char* pcCheckName = pkItem->m_element;
        pkItem = pkItem->m_pkNext;
        if (pcCheckName && NiStricmp(pcCheckName, acSourcePath) == 0) {
            return;
        }
    }
    size_t uiLen = uiLength + 1;
    char* pacNewAdd = static_cast<char*>(_NiMalloc(uiLen));
    strcpy_s(pacNewAdd, uiLen, acSourcePath);
    // EXE-INLINED none: NiTPointerListBase<char*>::AddHead
    // SWEEP-INLINE 0x43a010
    NiTListItem<char*>* pkNode = m_kProgramDirectories.NewItem();
    pkNode->m_element = pacNewAdd;
    pkNode->m_pkPrev = nullptr;
    pkNode->m_pkNext = m_kProgramDirectories.m_pkHead;
    if (m_kProgramDirectories.m_pkHead) {
        m_kProgramDirectories.m_pkHead->m_pkPrev = pkNode;
    }
    else {
        m_kProgramDirectories.m_pkTail = pkNode;
    }
    ++m_kProgramDirectories.m_uiCount;
    m_kProgramDirectories.m_pkHead = pkNode;
}

// Removes pacDirectory from the program directories: '/' becomes '\\' and the compare ignores case.
void NiD3DShaderProgramFactory::RemoveProgramDirectory(const char* pcDirectory) {
    if (!pcDirectory || pcDirectory[0] == '\0') {
        return;
    }
    char acSourcePath[0x104];
    strcpy_s(acSourcePath, 0x104, pcDirectory);
    size_t uiLength = 0;
    while (acSourcePath[uiLength]) {
        ++uiLength;
    }
    for (size_t ui = 0; ui < uiLength; ++ui) {
        if (acSourcePath[ui] == '/') {
            acSourcePath[ui] = '\\';
        }
    }
    for (NiTListItem<char*>* pkItem = m_kProgramDirectories.m_pkHead; pkItem; pkItem = pkItem->m_pkNext) {
        char* pcCheckName = pkItem->m_element;
        if (pcCheckName && NiStricmp(pcCheckName, acSourcePath) == 0) {
            m_kProgramDirectories.Remove(pcCheckName);
            return;
        }
    }
}

// Frees every program directory.
void NiD3DShaderProgramFactory::RemoveAllProgramDirectories() {
    using PoolAllocator = NiTAbstractPoolAllocator<uint32_t>;
    NiTListItem<char*>* pkIter = m_kProgramDirectories.m_pkHead;
    while (pkIter) {
        char* pacProgDir = pkIter->m_element;
        pkIter = pkIter->m_pkNext;
        if (!pacProgDir) {
            continue;
        }
        // EXE-INLINED 0xE7DB70
        NiTListItem<char*>* pkHead = m_kProgramDirectories.m_pkHead;
        NiTListItem<char*>* pkPos = nullptr;
        for (NiTListItem<char*>* pkItem = pkHead; pkItem; pkItem = pkItem->m_pkNext) {
            if (pkItem->m_element == pacProgDir) {
                pkPos = pkItem;
                break;
            }
        }
        if (pkPos) {
            if (pkPos == pkHead) {
                m_kProgramDirectories.RemoveHead();
            }
            else if (pkPos == m_kProgramDirectories.m_pkTail) {
                m_kProgramDirectories.RemoveTail();
            }
            else {
                NiTListItem<char*>* pkPrev = pkPos->m_pkPrev;
                NiTListItem<char*>* pkNext = pkPos->m_pkNext;
                if (pkPrev) {
                    pkPrev->m_pkNext = pkNext;
                }
                if (pkNext) {
                    pkNext->m_pkPrev = pkPrev;
                }
                pkPos->m_element = nullptr;
                // EXE-INLINED 0x45CEE0
                PoolAllocator::Lock();
                // Overlays the list item's untyped memory, as the game does.
                PoolAllocator::AllocNode* pkFreed = reinterpret_cast<PoolAllocator::AllocNode*>(pkPos);
                pkFreed->m_pkData = nullptr;
                pkFreed->m_pkNext = PoolAllocator::ms_pkFreeMem;
                bool bLastLock = --PoolAllocator::ms_kSpinLock.uiLockCount == 0;
                PoolAllocator::ms_pkFreeMem = pkFreed;
                if (bLastLock) {
                    PoolAllocator::ms_kSpinLock.uiOwningThread = 0;
                }
                --m_kProgramDirectories.m_uiCount;
            }
        }
        _NiFree(pacProgDir);
    }
}

const char* NiD3DShaderProgramFactory::GetFirstProgramDirectory(NiTListIterator& kIter) {
    NiTListItem<char*>* pkHead = m_kProgramDirectories.m_pkHead;
    kIter = pkHead;
    if (pkHead) {
        kIter = pkHead->m_pkNext;
        return pkHead->m_element;
    }
    return nullptr;
}

void NiD3DShaderProgramFactory::InsertVertexShaderIntoMap(NiD3DVertexShader* pkShader) {
    const char* v3;
    if (pkShader) {
        if (m_pkVertexShaderMap) {
            v3 = pkShader->GetName();
            m_pkVertexShaderMap->SetAt(v3, pkShader);
        }
    }
}

void NiD3DShaderProgramFactory::InsertPixelShaderIntoMap(NiD3DPixelShader* pkShader) {
    const char* v3;
    if (pkShader) {
        if (m_pkPixelShaderMap) {
            v3 = pkShader->GetName();
            m_pkPixelShaderMap->SetAt(v3, pkShader);
        }
    }
}

NiD3DVertexShader* NiD3DShaderProgramFactory::GetVertexShader(const char* pcShaderName) {
    NiTStringPointerMap<NiD3DVertexShader*>* pkMap = m_pkVertexShaderMap;
    if (!pkMap) {
        return nullptr;
    }
    NiD3DVertexShader* pkVertexShader = nullptr;
    bool bFound = pkMap->GetAt(pcShaderName, pkVertexShader);
    return bFound ? pkVertexShader : nullptr;
}

NiD3DPixelShader* NiD3DShaderProgramFactory::GetPixelShader(const char* pcShaderName) {
    NiTStringPointerMap<NiD3DPixelShader*>* pkMap = m_pkPixelShaderMap;
    if (!pkMap) {
        return nullptr;
    }
    NiD3DPixelShader* pkPixelShader = nullptr;
    bool bFound = pkMap->GetAt(pcShaderName, pkPixelShader);
    return bFound ? pkPixelShader : nullptr;
}

void NiD3DShaderProgramFactory::ReleaseVertexShader(NiD3DVertexShader* pkShader) {
    if (ms_pkFactory && pkShader) {
        const char* pcName = pkShader->GetName();
        if (pcName && ms_pkFactory->m_pkVertexShaderMap) {
            ms_pkFactory->m_pkVertexShaderMap->RemoveAt(pcName);
        }
    }
}

void NiD3DShaderProgramFactory::ReleasePixelShader(NiD3DPixelShader* pkShader) {
    if (ms_pkFactory && pkShader) {
        const char* pcName = pkShader->GetName();
        if (pcName && ms_pkFactory->m_pkPixelShaderMap) {
            ms_pkFactory->m_pkPixelShaderMap->RemoveAt(pcName);
        }
    }
}

// Walks the directory list (GetFirstProgramDirectory/GetNextProgramDirectory); a null directory ends the search. Error
// 1 is NISHADERERR_UNKNOWN.
bool NiD3DShaderProgramFactory::ResolveShaderFileName(
    const char* pcOriginalName,
    char* pcTrueName,
    uint32_t uiStringLen
) {
    if (NiFile::Access(pcOriginalName, NiFile::READ_ONLY)) {
        strcpy_s(pcTrueName, uiStringLen, pcOriginalName);
        return true;
    }
    if (!ms_pkFactory) {
        NiShaderFactory::ReportError(1, false, "No valid shader program factory\n");
        return false;
    }
    char acDrive[3];
    char acDir[260];
    char acFileName[260];
    char acFileExt[256];
    NiD3DUtility::GetSplitPath(pcOriginalName, acDrive, acDir, acFileName, acFileExt);
    NiTListItem<char*>* pkItem = ms_pkFactory->m_kProgramDirectories.m_pkHead;
    if (!pkItem) {
        return false;
    }
    const char* pacProgDir = pkItem->m_element;
    NiTListItem<char*>* pkNext = pkItem->m_pkNext;
    while (pacProgDir) {
        if (pacProgDir[0] == '\0') {
            NiShaderFactory::ReportError(1, false, "Invalid or no shader program directory\n");
        }
        else {
            size_t uiLen = 0;
            while (pacProgDir[uiLen]) {
                ++uiLen;
            }
            const char* pcFormat;
            if (pacProgDir[uiLen - 1] == '/' || pacProgDir[uiLen - 1] == '\\') {
                pcFormat = "%s%s%s";
            }
            else {
                pcFormat = "%s\\%s%s";
            }
            NiSprintf(pcTrueName, uiStringLen, pcFormat, pacProgDir, acFileName, acFileExt);
            if (NiFile::Access(pcTrueName, NiFile::READ_ONLY)) {
                return true;
            }
            NiShaderFactory::ReportError(
                1, true, "Shader program file not found %s in directory %s\n", pcOriginalName, pacProgDir
            );
        }
        if (!pkNext) {
            return false;
        }
        pacProgDir = pkNext->m_element;
        pkNext = pkNext->m_pkNext;
    }
    return false;
}

bool NiD3DShaderProgramFactory::RecreatePixelShader(NiD3DPixelShader* pkPixelShader) {
    if (!ms_pkFactory || !pkPixelShader) {
        return false;
    }
    if (pkPixelShader->GetShaderHandle()) {
        return true;
    }
    NiD3DShaderProgramCreator* pkCreator = pkPixelShader->GetCreator();
    if (!pkCreator) {
        return false;
    }
    return pkCreator->RecreatePixelShader(pkPixelShader);
}

bool NiD3DShaderProgramFactory::RecreateVertexShader(NiD3DVertexShader* pkVertexShader) {
    if (!ms_pkFactory || !pkVertexShader) {
        return false;
    }
    if (pkVertexShader->GetShaderHandle()) {
        return true;
    }
    NiD3DShaderProgramCreator* pkCreator = pkVertexShader->GetCreator();
    if (!pkCreator) {
        return false;
    }
    return pkCreator->RecreateVertexShader(pkVertexShader);
}

// Returns the shader already in the map under pcShaderName, else has the creator registered for pcExt build it and maps
// it.
NiD3DVertexShader* NiD3DShaderProgramFactory::CreateVertexShaderFromBuffer(
    const void* pvBuffer,
    uint32_t uiBufferSize,
    const char* pcExt,
    const char* pcShaderName,
    const char* pcEntryPoint,
    const char* pcShaderTarget,
    IDirect3DVertexDeclaration9* hDecl,
    uint32_t uiUsage,
    int32_t bSoftwareVP,
    int32_t bRecoverable
) {
    if (!NiD3DShaderProgramFactory::ms_pkFactory) {
        return nullptr;
    }
    if (!pvBuffer || !uiBufferSize) {
        return nullptr;
    }
    NiD3DVertexShader* pkVertexShader = NiD3DShaderProgramFactory::ms_pkFactory->GetVertexShader(pcShaderName);
    if (pkVertexShader) {
        return pkVertexShader;
    }
    NiD3DShaderProgramCreator* pkCreator = NiD3DShaderProgramFactory::GetShaderCreator(pcExt);
    if (!pkCreator) {
        return nullptr;
    }
    pkVertexShader = pkCreator->CreateVertexShaderFromBuffer(
        pvBuffer, uiBufferSize, pcShaderName, pcEntryPoint, pcShaderTarget, hDecl, uiUsage, bSoftwareVP, bRecoverable
    );
    NiD3DShaderProgramFactory::ms_pkFactory->InsertVertexShaderIntoMap(pkVertexShader);
    return pkVertexShader;
}

// Returns the shader already in the map under pcShaderName, else has the creator registered for pcExt build it and maps
// it.
NiD3DPixelShader* NiD3DShaderProgramFactory::CreatePixelShaderFromBuffer(
    const void* pvBuffer,
    uint32_t uiBufferSize,
    const char* pcExt,
    const char* pcShaderName,
    const char* pcEntryPoint,
    const char* pcShaderTarget,
    int32_t bRecoverable
) {
    if (!NiD3DShaderProgramFactory::ms_pkFactory) {
        return nullptr;
    }
    if (!pvBuffer || !uiBufferSize) {
        return nullptr;
    }
    NiD3DPixelShader* pkPixelShader = NiD3DShaderProgramFactory::ms_pkFactory->GetPixelShader(pcShaderName);
    if (pkPixelShader) {
        return pkPixelShader;
    }
    NiD3DShaderProgramCreator* pkCreator = NiD3DShaderProgramFactory::GetShaderCreator(pcExt);
    if (!pkCreator) {
        return nullptr;
    }
    pkPixelShader = pkCreator->CreatePixelShaderFromBuffer(
        pvBuffer, uiBufferSize, pcShaderName, pcEntryPoint, pcShaderTarget, bRecoverable
    );
    NiD3DShaderProgramFactory::ms_pkFactory->InsertPixelShaderIntoMap(pkPixelShader);
    return pkPixelShader;
}

// Creates the shader creator map when the factory has none.
void NiD3DShaderProgramFactory::_SDMInit() {
    if (!ms_pkShaderCreators) {
        CreateShaderCreatorMap();
    }
}

// Empties the creator map, then deletes it. RemoveAll is called before the pointer is tested, deliberately, as the game
// does.
void NiD3DShaderProgramFactory::_SDMShutdown() {
    ms_pkShaderCreators->RemoveAll();
    if (ms_pkShaderCreators) {
        delete ms_pkShaderCreators;
    }
    ms_pkShaderCreators = nullptr;
}

// The map is constructed with 37 buckets and copies its keys.
void NiD3DShaderProgramFactory::CreateShaderCreatorMap() {
    NiTStringPointerMap<NiD3DShaderProgramCreator*>* v0 = static_cast<NiTStringPointerMap<NiD3DShaderProgramCreator*>*>(
        MemoryManager_Singleton.Allocate(sizeof(NiTStringPointerMap<NiD3DShaderProgramCreator*>))
    );
    if (v0) {
        NiD3DShaderProgramFactory::ms_pkShaderCreators =
            ::new (v0) NiTStringPointerMap<NiD3DShaderProgramCreator*>(37, true);
    }
    else {
        NiD3DShaderProgramFactory::ms_pkShaderCreators = 0;
    }
}

void NiD3DShaderProgramFactory::SetD3DRenderer(NiDX9Renderer* pkD3DRenderer) {
    IDirect3DDevice9* m_pkD3DDevice9;
    NiD3DShaderProgramFactory::ms_pkD3DRenderer = pkD3DRenderer;
    if (pkD3DRenderer) {
        m_pkD3DDevice9 = pkD3DRenderer->m_pkD3DDevice9;
        if (NiD3DShaderProgramFactory::ms_pkD3DDevice) {
            NiD3DShaderProgramFactory::ms_pkD3DDevice->Release();
        }
        NiD3DShaderProgramFactory::ms_pkD3DDevice = m_pkD3DDevice9;
        if (m_pkD3DDevice9) {
            m_pkD3DDevice9->AddRef();
        }
        NiD3DShaderProgramFactory::ms_pkD3DRenderState = NiD3DShaderProgramFactory::ms_pkD3DRenderer->m_pkRenderState;
    }
    else {
        NiD3DShaderProgramFactory::ms_pkD3DRenderState = nullptr;
        if (NiD3DShaderProgramFactory::ms_pkD3DDevice) {
            NiD3DShaderProgramFactory::ms_pkD3DDevice->Release();
        }
        NiD3DShaderProgramFactory::ms_pkD3DDevice = nullptr;
    }
}

void NiD3DShaderProgramFactory::Shutdown() {
    delete ms_pkFactory;
    ms_pkFactory = nullptr;
}

NiD3DShaderProgramFactory* NiD3DShaderProgramFactory::GetInstance() {
    return NiD3DShaderProgramFactory::ms_pkFactory;
}

void NiD3DShaderProgramFactory::UnregisterShaderCreator(NiD3DShaderProgramCreator* pkCreator) {
    NiD3DShaderProgramCreator* pkMapCreator;
    const char* pcExt;
    NiTMapItem<const char*, NiD3DShaderProgramCreator*>* kIter;
    NiTStringPointerMap<NiD3DShaderProgramCreator*>* v1 = NiD3DShaderProgramFactory::ms_pkShaderCreators;
    if (NiD3DShaderProgramFactory::ms_pkShaderCreators) {
        uint32_t m_uiHashSize = NiD3DShaderProgramFactory::ms_pkShaderCreators->m_uiHashSize;
        uint32_t v3 = 0;
        if (m_uiHashSize) {
            NiTMapItem<const char*, NiD3DShaderProgramCreator*>** m_ppkHashTable =
                NiD3DShaderProgramFactory::ms_pkShaderCreators->m_ppkHashTable;
            while (!*m_ppkHashTable) {
                ++v3;
                ++m_ppkHashTable;
                if (v3 >= m_uiHashSize) {
                    goto LABEL_6;
                }
            }
            kIter = NiD3DShaderProgramFactory::ms_pkShaderCreators->m_ppkHashTable[v3];
        }
        else {
        LABEL_6:;
            kIter = 0;
        }
        if (kIter) {
            while (1) {
                pcExt = 0;
                pkMapCreator = 0;
                v1->GetNext(kIter, pcExt, pkMapCreator);
                if (pkCreator == pkMapCreator) {
                    NiD3DShaderProgramFactory::ms_pkShaderCreators->RemoveAt(pcExt);
                }
                if (!kIter) {
                    break;
                }
                v1 = NiD3DShaderProgramFactory::ms_pkShaderCreators;
            }
        }
    }
}

// KEEP: GetFileExtension writes only the extension and its terminator, so for a name without one the exe tests
// acFileExt[1], an uninitialised stack byte, and may lower-case stack garbage from there.
NiD3DShaderProgramCreator* NiD3DShaderProgramFactory::GetShaderCreator(const char* pcFilename) {
    NiD3DShaderProgramCreator* pkCreator;
    char acFileExt[256];
    if (!NiD3DShaderProgramFactory::ms_pkShaderCreators || (!pcFilename || !*pcFilename)) {
        return 0;
    }
    NiD3DUtility::GetFileExtension(pcFilename, acFileExt, 256u);
    const char* pcSrc = &acFileExt[1];
    if (!acFileExt[1]) {
        pcSrc = pcFilename;
    }
    uint32_t i = 0;
    do {
        char v4 = std::tolower(pcSrc[i]);
        acFileExt[i] = v4;
        if (!v4) {
            break;
        }
        ++i;
    }
    while (i < 256);
    pkCreator = 0;
    bool v5 = NiD3DShaderProgramFactory::ms_pkShaderCreators->GetAt(acFileExt, pkCreator);
    return v5 ? pkCreator : 0;
}

// KEEP: the lower-case copy stops at 256 characters without a terminator, so for an extension that long the exe's
// SetAt hashes and copies acFileExt past its end, into uninitialised stack bytes. The exe tests the source character
// (not tolower's result) for the end.
bool NiD3DShaderProgramFactory::RegisterShaderCreator(const char* pcExt, NiD3DShaderProgramCreator* pkCreator) {
    char acFileExt[256];
    if (!pcExt || std::strchr(pcExt, '.')) {
        return false;
    }
    for (uint32_t i = 0; i < 256; ++i) {
        char cLower = std::tolower(pcExt[i]);
        bool bEnd = pcExt[i] == '\0';
        acFileExt[i] = cLower;
        if (bEnd) {
            break;
        }
    }
    if (!NiD3DShaderProgramFactory::ms_pkShaderCreators) {
        NiD3DShaderProgramFactory::CreateShaderCreatorMap();
    }
    NiD3DShaderProgramFactory::ms_pkShaderCreators->SetAt(acFileExt, pkCreator);
    return true;
}
