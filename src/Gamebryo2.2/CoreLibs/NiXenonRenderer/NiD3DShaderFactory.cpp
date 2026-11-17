#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderFactory.h"

#include "BSCore/BSCoreMessage.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderLibrary.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DGPUProgramCache.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderInterface.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderLibrary.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramFactory.h"
#include "SDK/MSVC/string.h"

#include <cstdarg>

// The shader map copies its keys, the library map does not. The lock's constructor initialises its section, and its
// destructor deletes it after this destructor's body.
NiD3DShaderFactory::NiD3DShaderFactory() : m_pkRenderer(nullptr) {
    m_pkShaderMap = new NiTStringPointerMap<NiShader*>(59, true);
    m_pkLibraryMap = new NiTStringPointerMap<NiPointer<NiShaderLibrary>>(37, false);
}

NiD3DShaderFactory::~NiD3DShaderFactory() {
    if (m_pkShaderMap) {
        m_pkShaderMap->RemoveAll();
    }
    NiD3DShaderFactory::RemoveAllLibraries();
    delete m_pkShaderMap;
    delete m_pkLibraryMap;
    m_pkShaderMap = nullptr;
    m_pkLibraryMap = nullptr;
    m_pkRenderer = nullptr;
}

NiShader* NiD3DShaderFactory::RetrieveShader(const char* pcName, uint32_t uiImplementation, bool bReportNotFound) {
    if (!m_pkRenderer) {
        if (BSCoreMessage::pHandler) {
            BSCoreMessage::pHandler->Message(
                BSCoreMessage::MS_DEBUGSTRING, "Attempting to RetrieveShader w/out a valid renderer!\n", nullptr, 0, 0
            );
        }
        return nullptr;
    }
    if (!pcName || *pcName == '\0') {
        NiShaderFactory::ReportError(NISHADERERR_UNKNOWN, false, "Attempting to RetrieveShader w/out a valid name!\n");
        return nullptr;
    }
    NiShader* pkShader = this->FindShader(pcName, uiImplementation);
    if (!pkShader) {
        m_kShaderMapLock.Lock();
        NiShaderLibrary* pkLibrary = this->GetFirstLibrary();
        pkShader = this->FindShader(pcName, uiImplementation);
        if (!pkShader && pkLibrary) {
            for (;;) {
                pkShader = pkLibrary->GetShader(m_pkRenderer, pcName, uiImplementation);
                if (pkShader) {
                    this->InsertShader(pkShader, uiImplementation);
                    NiD3DShaderInterface* pkD3DShader = static_cast<NiD3DShaderInterface*>(pkShader);
                    m_pkRenderer->RegisterD3DShader(pkD3DShader);
                    pkD3DShader->SetD3DRenderer(m_pkRenderer);
                    break;
                }
                pkLibrary = this->GetNextLibrary();
                if (!pkLibrary) {
                    break;
                }
            }
        }
        m_kShaderMapLock.Unlock();
    }
    if (bReportNotFound && !pkShader) {
        NiShaderFactory::ReportError(
            NISHADERERR_UNKNOWN, false, "Failed to find shader %s, Implementation %d\n", pcName, uiImplementation
        );
    }
    return pkShader;
}

NiShader* NiD3DShaderFactory::FindShader(const char* pcName, uint32_t uiImplementation) {
    char acFullName[260];
    if (!m_pkShaderMap) {
        return 0;
    }
    NiShader* pkShader = 0;
    NiSprintf(acFullName, 260u, "%s%d", pcName, uiImplementation);
    bool v4 = m_pkShaderMap->GetAt(acFullName, pkShader);
    return v4 ? pkShader : 0;
}

// The second FindShader asks for the default implementation; the shader's implementation id is then compared with the
// one wanted. Returns 1 whatever nonzero value ReleaseShader gave.
bool NiD3DShaderFactory::ReleaseShaderFromLibrary(const char* pcName, uint32_t uiImplementation) {
    if (!pcName || *pcName == '\0') {
        return false;
    }
    if (!this->FindShader(pcName, uiImplementation)) {
        NiShader* pkShader = this->FindShader(pcName, NiShader::DEFAULT_IMPLEMENTATION);
        if (!pkShader || pkShader->GetImplementation() != uiImplementation) {
            return false;
        }
    }
    m_kShaderMapLock.Lock();
    NiShaderLibrary* pkLibrary = this->GetFirstLibrary();
    bool bReleased = false;
    while (pkLibrary) {
        if (pkLibrary->ReleaseShader(pcName, uiImplementation)) {
            bReleased = true;
            break;
        }
        pkLibrary = this->GetNextLibrary();
    }
    m_kShaderMapLock.Unlock();
    return bReleased;
}

bool NiD3DShaderFactory::ReleaseShaderFromLibrary(NiShader* pkShader) {
    if (!pkShader) {
        return 0;
    }
    NiShader* v4 = FindShader((&pkShader->m_kName)->GetHandle(), pkShader->m_uiImplementation);
    if (!v4 || v4 != pkShader) {
        NiShader* pkTestShader = FindShader((&pkShader->m_kName)->GetHandle(), NiShader::DEFAULT_IMPLEMENTATION);
        if (!pkTestShader || pkTestShader != pkShader) {
            return 0;
        }
    }
    m_kShaderMapLock.Lock();
    NiShaderLibrary* pkLibrary = GetFirstLibrary();
    bool bRet = 0;
    if (pkLibrary) {
        while (!pkLibrary->ReleaseShader(pkShader)) {
            pkLibrary = GetNextLibrary();
            if (!pkLibrary) {
                m_kShaderMapLock.Unlock();
                return 0;
            }
        }
        bRet = 1;
    }
    m_kShaderMapLock.Unlock();
    return bRet;
}

void NiD3DShaderFactory::InsertShader(NiShader* pkShader, uint32_t uiImplementation) {
    if (pkShader) {
        char acFullName[260];
        NiSprintf(acFullName, 260u, "%s%d", (&pkShader->m_kName)->GetHandle(), uiImplementation);
        NiTStringPointerMap<NiShader*>* m_pkShaderMap = this->m_pkShaderMap;
        if (m_pkShaderMap) {
            m_pkShaderMap->SetAt(acFullName, pkShader);
        }
    }
}

void NiD3DShaderFactory::RemoveShader(const char* pcName, uint32_t uiImplementation) {
    int8_t pcDest[260];
    if (m_pkShaderMap) {
        NiSprintf(static_cast<char*>(static_cast<void*>(pcDest)), 0x104u, "%s%d", pcName, uiImplementation);
        m_pkShaderMap->RemoveAt(static_cast<const char*>(static_cast<const void*>(pcDest)));
    }
}

void NiD3DShaderFactory::RemoveAllShaders() {
    NiTStringPointerMap<NiShader*>* pkShaderMap = m_pkShaderMap;
    if (pkShaderMap) {
        pkShaderMap->RemoveAll();
    }
}

// A D3D shader's default implementation is its best one.
bool NiD3DShaderFactory::IsDefaultImplementation(NiShader* pkShader) {
    if (!pkShader) {
        return false;
    }
    NiD3DShaderInterface* pkD3DShader = NiD3DShaderInterface::DynamicCast(pkShader, &NiD3DShaderInterface::ms_RTTI);
    if (!pkD3DShader) {
        return false;
    }
    return pkD3DShader->GetIsBestImplementation();
}

// The NiPointer argument is passed by value; the map's SetAt releases it.
void NiD3DShaderFactory::InsertLibrary(NiShaderLibrary* pkLibrary) {
    if (pkLibrary) {
        m_pkLibraryMap->SetAt(pkLibrary->m_pcName, NiPointer<NiShaderLibrary>(pkLibrary));
    }
}

// The map is not tested for null. The returned library is the map's: the local reference is released first.
NiShaderLibrary* NiD3DShaderFactory::FindLibrary(const char* pcName) {
    NiPointer<NiShaderLibrary> spLibrary;
    if (m_pkLibraryMap->GetAt(pcName, spLibrary)) {
        return spLibrary;
    }
    return nullptr;
}

// Removes the library only when the map and this lookup hold its only references.
void NiD3DShaderFactory::RemoveLibrary(const char* pcName) {
    NiPointer<NiShaderLibrary> spLibrary;
    if (m_pkLibraryMap->GetAt(pcName, spLibrary) && spLibrary->m_uiRefCount == 2) {
        m_pkLibraryMap->RemoveAt(pcName);
    }
}

void NiD3DShaderFactory::RemoveAllLibraries() {
    if (m_pkLibraryMap) {
        NiShaderLibrary* pkLibrary = NiD3DShaderFactory::GetFirstLibrary();
        while (pkLibrary) {
            m_pkLibraryMap->RemoveAt(pkLibrary->m_pcName);
            // EXE-INLINED 0xE92BE0
            if (!m_pkLibraryMap || !m_kLibraryIter) {
                break;
            }
            const char* pcName;
            NiPointer<NiShaderLibrary> spLibrary;
            m_pkLibraryMap->GetNext(m_kLibraryIter, pcName, spLibrary);
            pkLibrary = spLibrary;
        }
        m_pkLibraryMap->RemoveAll();
    }
}

NiShaderLibrary* NiD3DShaderFactory::GetFirstLibrary() {
    if (!m_pkLibraryMap) {
        return nullptr;
    }
    m_kLibraryIter = m_pkLibraryMap->GetFirstPos();
    if (!m_kLibraryIter) {
        return nullptr;
    }
    const char* pcName;
    NiPointer<NiShaderLibrary> spLibrary;
    m_pkLibraryMap->GetNext(m_kLibraryIter, pcName, spLibrary);
    return spLibrary;
}

NiShaderLibrary* NiD3DShaderFactory::GetNextLibrary() {
    if (!m_pkLibraryMap || !m_kLibraryIter) {
        return nullptr;
    }
    const char* pcName;
    NiPointer<NiShaderLibrary> spLibrary;
    m_pkLibraryMap->GetNext(m_kLibraryIter, pcName, spLibrary);
    return spLibrary;
}

uint32_t NiD3DShaderFactory::GetMinorPixelShaderVersion(const uint32_t uiVersion) {
    return uiVersion & 0xFF;
}

uint32_t NiD3DShaderFactory::GetMajorPixelShaderVersion(const uint32_t uiVersion) {
    return (uiVersion >> 8) & 0xFF;
}

uint32_t NiD3DShaderFactory::CreatePixelShaderVersion(const uint32_t uiMajorVersion, const uint32_t uiMinorVersion) {
    return ((uiMajorVersion | 0xFFFFFF00) << 8) | uiMinorVersion;
}

uint32_t NiD3DShaderFactory::GetMinorVertexShaderVersion(const uint32_t uiVersion) {
    return uiVersion & 0xFF;
}

uint32_t NiD3DShaderFactory::GetMajorVertexShaderVersion(const uint32_t uiVersion) {
    return (uiVersion >> 8) & 0xFF;
}

uint32_t NiD3DShaderFactory::CreateVertexShaderVersion(const uint32_t uiMajorVersion, const uint32_t uiMinorVersion) {
    return ((uiMajorVersion | 0xFFFFFE00) << 8) | uiMinorVersion;
}

const char* NiD3DShaderFactory::GetRendererString() const {
    return "DX9";
}

// The libraries are walked through the factory's own virtuals; the source casts each to the D3D library to hand it
// the renderer.
void NiD3DShaderFactory::SetRenderer(NiDX9Renderer* pkRenderer) {
    if (m_pkRenderer != pkRenderer) {
        m_pkRenderer = pkRenderer;
        NiD3DShaderLibrary* pkLibrary = static_cast<NiD3DShaderLibrary*>(GetFirstLibrary());
        while (pkLibrary) {
            pkLibrary->SetRenderer(pkRenderer);
            pkLibrary = static_cast<NiD3DShaderLibrary*>(GetNextLibrary());
        }
    }
}

void NiD3DShaderFactory::PurgeAllShaderRendererData() {
    if (m_pkRenderer) {
        m_pkRenderer->PurgeAllD3DShaders();
    }
}

void NiD3DShaderFactory::RestoreAllShaderRendererData() {
    NiShader* pkShader;
    NiTMapItem<const char*, NiShader*>* kIter;
    const char* pcName;
    if (m_pkRenderer) {
        NiTStringPointerMap<NiShader*>* m_pkShaderMap = this->m_pkShaderMap;
        if (m_pkShaderMap) {
            uint32_t m_uiHashSize = m_pkShaderMap->m_uiHashSize;
            uint32_t v4 = 0;
            pkShader = 0;
            if (m_uiHashSize) {
                NiTMapItem<const char*, NiShader*>** m_ppkHashTable = m_pkShaderMap->m_ppkHashTable;
                NiTMapItem<const char*, NiShader*>** v6 = m_ppkHashTable;
                while (!*v6) {
                    ++v4;
                    ++v6;
                    if (v4 >= m_uiHashSize) {
                        goto LABEL_7;
                    }
                }
                kIter = m_ppkHashTable[v4];
            }
            else {
            LABEL_7:;
                kIter = 0;
            }
            while (kIter) {
                this->m_pkShaderMap->GetNext(kIter, pcName, pkShader);
                // The source casts the map's shader to the D3D shader interface.
                NiD3DShaderInterface* pkD3DShader = static_cast<NiD3DShaderInterface*>(pkShader);
                if (pkD3DShader) {
                    if (!pkD3DShader->IsInitialized()) {
                        pkD3DShader->SetD3DRenderer(m_pkRenderer);
                        pkD3DShader->RecreateRendererData();
                        m_pkRenderer->RegisterD3DShader(pkD3DShader);
                    }
                }
            }
        }
    }
}

// Delegates to the program factory's GetFirstProgramDirectory.
const char* NiD3DShaderFactory::GetFirstProgramDirectory(NiTListIterator& kIter) {
    NiD3DShaderProgramFactory* pkSPFactory = NiD3DShaderProgramFactory::GetInstance();
    if (!pkSPFactory) {
        return nullptr;
    }
    return pkSPFactory->GetFirstProgramDirectory(kIter);
}

// The program factory's next program directory, or null without a factory.
const char* NiD3DShaderFactory::GetNextProgramDirectory(NiTListIterator& kIter) {
    NiD3DShaderProgramFactory* pkSPFactory = NiD3DShaderProgramFactory::GetInstance();
    if (!pkSPFactory) {
        return nullptr;
    }
    return static_cast<const char*>(GetNext(kIter));
}

void NiD3DShaderFactory::AddProgramDirectory(const char* pacDirectory) {
    NiD3DShaderProgramFactory* instance = NiD3DShaderProgramFactory::GetInstance();
    if (instance) {
        instance->AddProgramDirectory(const_cast<char*>(pacDirectory));
    }
}

void NiD3DShaderFactory::RemoveProgramDirectory(const char* pacDirectory) {
    NiD3DShaderProgramFactory* instance = NiD3DShaderProgramFactory::GetInstance();
    if (instance) {
        instance->RemoveProgramDirectory(const_cast<char*>(pacDirectory));
    }
}

void NiD3DShaderFactory::RemoveAllProgramDirectories() {
    NiD3DShaderProgramFactory* pkFactory = NiD3DShaderProgramFactory::GetInstance();
    if (pkFactory) {
        pkFactory->RemoveAllProgramDirectories();
    }
}

NiD3DShaderFactory* NiD3DShaderFactory::GetD3DShaderFactory() {
    return NiD3DShaderFactory::ms_pkD3DShaderFactory;
}

void NiD3DShaderFactory::_SDMShutdown() {
    if (NiD3DShaderFactory::ms_pkD3DShaderFactory) {
        delete NiD3DShaderFactory::ms_pkD3DShaderFactory;
        NiD3DShaderFactory::ms_pkD3DShaderFactory = nullptr;
    }
}

// Allocates the factory with NiNew, constructs it, and publishes it in both statics. The second test is on
// NiShaderFactory's static, not the D3D one: a factory installed by someone else leaves it null, and the failure is
// then reported through an empty stub.
void NiD3DShaderFactory::_SDMInit() {
    if (!NiD3DShaderFactory::ms_pkD3DShaderFactory) {
        void* pvMemory = NiMemObject::operator new(sizeof(NiD3DShaderFactory));
        NiD3DShaderFactory* pkFactory = pvMemory ? ::new (pvMemory) NiD3DShaderFactory : nullptr;
        NiD3DShaderFactory::ms_pkD3DShaderFactory = pkFactory;
        NiShaderFactory::ms_pkShaderFactory = pkFactory;
    }
    if (!NiShaderFactory::ms_pkShaderFactory) {
        // NiRenderer::Error is an empty stub, so this message is never read.
        NiRenderer::Error("Failed to create shader factory!");
    }
}

// Formats into pcDest with the CRT's vsnprintf, then always terminates the last byte (vsnprintf returns -1 without
// terminating when the text does not fit); returns 1 without writing for a zero size.
int32_t NiSprintf(char* pcDest, size_t stDestSize, const char* pcFormat, ...) {
    if (stDestSize == 0) {
        return 1;
    }
    va_list kArgs;
    va_start(kArgs, pcFormat);
    int32_t iRet = vsprintf_s(pcDest, stDestSize, pcFormat, kArgs);
    pcDest[stDestSize - 1] = '\0';
    return iRet;
}
