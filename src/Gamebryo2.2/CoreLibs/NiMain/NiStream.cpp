#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBezRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTFixedStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiPath.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiProcessorAffinity.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiThread.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cstdint>
#include <cstring>

NiCriticalSection NiStream::ms_kCleanupCriticalSection;
NiSemaphore NiStream::m_kSemaphore(0, 1);

// Reads the file's class names into a table of create functions, then creates the object of each block from its class
// index. Unlike the reference source, a pre-20.1.0.1 file naming NiControllerManager fails here, and that return leaves
// the table allocated, as the game does; bhkConvexListShape blocks are never created.
// KEEP: niread: a short read leaves usRTTICount (the table's size and loop bound) and each uiLength (a read size and
// an index into aucRTTI) unwritten, and the exe uses them.
bool NiStream::LoadRTTI() {
    uint16_t usRTTICount;
    uint32_t uiSize = 2;
    m_pkIstr->BinaryRead(&usRTTICount, 2, &uiSize, 1);
    CreateFunction* ppfnCreate = static_cast<CreateFunction*>(_NiMalloc(sizeof(CreateFunction) * usRTTICount));
    CreateFunction* ppfnEntry = ppfnCreate;
#if XFNV_64BIT
    // x64 departure: a Havok block (class name starting "bhk") holds its creation info as the 32-bit program's
    // structure, which the 64-bit layouts do not match. The classes below read it through its serialized form
    // (BSHavok/bhkSerialized.h) or read nothing width-dependent: every bhk class the game's own model files use. The
    // blocks of every other bhk class are skipped by the size table, as the exe skips bhkConvexListShape, and every
    // link to them reads as null (docs/FOLLOWUPS.md "x64 port").
    static const char* const s_apcHavokClassesRead[] = {
        "bhkBlendCollisionObject",
        "bhkBlendController",
        "bhkBoxShape",
        "bhkBreakableConstraint",
        "bhkCapsuleShape",
        "bhkCollisionObject",
        "bhkConvexTransformShape",
        "bhkConvexVerticesShape",
        "bhkHingeConstraint",
        "bhkLimitedHingeConstraint",
        "bhkLiquidAction",
        "bhkListShape",
        "bhkMalleableConstraint",
        "bhkMoppBvTreeShape",
        "bhkNiTriStripsShape",
        "bhkOrientHingedBodyAction",
        "bhkPCollisionObject",
        "bhkPackedNiTriStripsShape",
        "bhkPrismaticConstraint",
        "bhkRagdollConstraint",
        "bhkRigidBody",
        "bhkRigidBodyT",
        "bhkSPCollisionObject",
        "bhkSimpleShapePhantom",
        "bhkSphereShape",
        "bhkStiffSpringConstraint",
        "bhkTransformShape",
    };
    bool* pbHavokClass = static_cast<bool*>(_NiMalloc(usRTTICount ? usRTTICount : 1));
#endif
    for (uint32_t i = 0; i < usRTTICount; ++i) {
        char aucRTTI[256];
        uint32_t uiLength;
        uiSize = 4;
        m_pkIstr->BinaryRead(&uiLength, 4, &uiSize, 1);
        uiSize = 1;
        m_pkIstr->BinaryRead(aucRTTI, uiLength, &uiSize, 1);
        aucRTTI[uiLength] = 0;
        if (m_uiNifFileVersion < GetVersion(20, 1, 0, 1) && !strcmp(aucRTTI, "NiControllerManager")) {
            m_uiLastError = OLDER_VERSION;
            strcpy_s(m_acLastErrorMessage, 0x104, "Older files may not contain animation data. Reexport required.");
#if XFNV_64BIT
            _NiFree(pbHavokClass);
#endif
            return false;
        }
        if (!ms_pkLoaders->GetAt(aucRTTI, *ppfnEntry)) {
            m_uiLastError = NO_CREATE_FUNCTION;
            strcpy_s(m_acLastErrorMessage, 0x104, aucRTTI);
            strcat_s(m_acLastErrorMessage, 0x104, ": cannot find create function.");
            *ppfnEntry = nullptr;
        }
        if (aucRTTI[0] == 'b' && aucRTTI[1] == 'h' && !strcmp(aucRTTI, "bhkConvexListShape")) {
            *ppfnEntry = nullptr;
        }
#if XFNV_64BIT
        pbHavokClass[i] = aucRTTI[0] == 'b' && aucRTTI[1] == 'h' && aucRTTI[2] == 'k';
        for (const char* pcRead : s_apcHavokClassesRead) {
            if (pbHavokClass[i] && !strcmp(aucRTTI, pcRead)) {
                pbHavokClass[i] = false;
            }
        }
        if (pbHavokClass[i]) {
            *ppfnEntry = nullptr;
        }
#endif
        ++ppfnEntry;
    }
    for (uint32_t i = 0; i < m_kObjects.m_uiMaxSize; ++i) {
        uint16_t usRTTI;
        bool bSkippable = false;
        uiSize = 2;
        m_pkIstr->BinaryRead(&usRTTI, 2, &uiSize, 1);
        if (m_uiNifFileVersion >= GetVersion(20, 2, 0, 5) && (usRTTI & 0x8000) != 0) {
            usRTTI &= 0x7FFF;
            bSkippable = true;
        }
#if XFNV_64BIT
        // Skipping needs the object size table (files from 20.2.0.5 on, every file the game ships).
        bSkippable = bSkippable || (pbHavokClass[usRTTI] && m_uiNifFileVersion >= GetVersion(20, 2, 0, 5));
#endif
        CreateFunction* ppfnClass = &ppfnCreate[usRTTI];
        if (*ppfnClass) {
            NiPointer<NiObject> spObject((*ppfnClass)());
            m_kObjects.SetAt(i, spObject);
        }
        else {
            if (!bSkippable) {
                _NiFree(ppfnCreate);
#if XFNV_64BIT
                _NiFree(pbHavokClass);
#endif
                return false;
            }
            NiPointer<NiObject> spNull;
            m_kObjects.SetAt(i, spNull);
        }
    }
    _NiFree(ppfnCreate);
#if XFNV_64BIT
    _NiFree(pbHavokClass);
#endif
    return true;
}

NiStream::NiStream()
  : m_kGroups(0, 1), m_uiNifFileVersion(ms_uiNifMaxVersion),
    m_uiNifFileUserDefinedVersion(ms_uiNifMaxUserDefinedVersion), m_bSaveLittleEndian(true),
    m_bSourceIsLittleEndian(true), m_kObjects(0, 1024), m_kObjectSizes(0, 1), m_kTopObjects(0, 1),
    m_kFixedStrings(0, 128), m_pkIstr(nullptr), m_pkOstr(nullptr), m_kLinkIDs(0), m_uiLinkIndex(0), m_kLinkIDBlocks(0),
    m_uiLinkBlockIndex(0), m_kRegisterMap(37), m_eBackgroundLoadStatus(IDLE), m_pkThread(nullptr),
    m_pkBGLoadProc(nullptr), m_ePriority(NiThread::BELOW_NORMAL), m_uiLastError(STREAM_OKAY) {
    m_BSStreamHeader.uiVersion = 34;
    m_BSStreamHeader.cAuthor[0] = 0;
    m_BSStreamHeader.cProcessScript[0] = 0;
    m_BSStreamHeader.cExportScript[0] = 0;
    m_acFileName[0] = 0;
    m_kAffinity.m_eIdealProcessor = NiProcessorAffinity::PROCESSOR_DONT_CARE;
    m_kAffinity.m_uiAffinityMask = -1;
    m_acLastErrorMessage[0] = 0;
    bUseDefaultPath = true;
}

NiStream::~NiStream() {
    delete m_pkIstr;
    delete m_pkOstr;
    RemoveAllObjects();
    delete m_pkBGLoadProc;
}

bool NiStream::Load(const char* pcFileName) {
    char* m_acFileName = this->m_acFileName;
    strcpy_s(this->m_acFileName, 260, pcFileName);
    NiPath::Standardize(m_acFileName);
    if (m_acFileName && *m_acFileName) {
        strncpy_s(m_acFilePath, 260u, m_acFileName, 259u);
        NiPath::Standardize(m_acFilePath);
    }
    else {
        m_acFilePath[0u] = 0;
    }
    NiFile* kIst = NiFile::GetFile(m_acFileName, NiFile::READ_ONLY, 0x8000u);
    if (kIst) {
        if (kIst->operator bool()) {
            bool bResult = Load(kIst);
            delete kIst;
            return bResult;
        }
        delete kIst;
    }
    m_uiLastError = NiStream::FILE_NOT_LOADED;
    strcpy_s(m_acLastErrorMessage, 260, "Cannot open file.");
    return false;
}

bool NiStream::Load(char* pcBuffer, int32_t iBufferSize) {
    bool v4;
    {
        NiMemStream apThis(pcBuffer, static_cast<uint32_t>(iBufferSize));
        v4 = Load(&apThis);
    }
    return v4;
}

bool NiStream::Load(NiBinaryStream* pkIstr) {
    bool bResult = static_cast<bool>(*pkIstr);
    if (bResult) {
        m_pkIstr = pkIstr;
        bResult = LoadStream();
    }
    m_pkIstr = nullptr;
    return bResult;
}

bool NiStream::Save(const char* pcFileName) {
    char* m_acFileName = this->m_acFileName;
    strcpy_s(this->m_acFileName, 260, pcFileName);
    NiPath::Standardize(m_acFileName);
    if (m_acFileName && *m_acFileName) {
        strncpy_s(m_acFilePath, 260u, m_acFileName, 259u);
        NiPath::Standardize(m_acFilePath);
    }
    else {
        m_acFilePath[0u] = 0;
    }
    NiFile* pkOstr = NiFile::GetFile(m_acFileName, NiFile::WRITE_ONLY, 0x8000u);
    if (pkOstr) {
        if (pkOstr->operator bool()) {
            bool bResult = Save(pkOstr);
            delete pkOstr;
            return bResult;
        }
        delete pkOstr;
    }
    return false;
}

// Builds a default NiMemStream in a local buffer, saves through Save(NiBinaryStream*), hands out the buffer and its end
// offset, then destroys the stream.
bool NiStream::Save(char*& pcBuffer, int32_t& iBufferSize) {
    alignas(NiMemStream) uint8_t aucStream[sizeof(NiMemStream)];
    NiMemStream* pkMemStream = ::new (aucStream) NiMemStream;
    bool bResult = Save(pkMemStream);
    iBufferSize = static_cast<int32_t>(pkMemStream->m_uiEnd);
    pcBuffer = static_cast<char*>(pkMemStream->Str());
    pkMemStream->~NiMemStream();
    return bResult;
}

bool NiStream::Save(NiBinaryStream* pkOstr) {
    m_pkOstr = pkOstr;
    NiStream* pkStream = this;
    bool bResult = pkStream->SaveStream();
    m_pkOstr = nullptr;
    return bResult;
}

// Adds kString to the table unless an equal handle is already there; the null string is always added.
bool NiStream::RegisterFixedString(const NiFixedString& kString) {
    const char* pcHandle = kString.GetHandle();
    if (pcHandle != NiGlobalStringTable::NULL_STRING) {
        uint32_t uiSize = m_kFixedStrings.m_uiSize;
        NiFixedString* pkBase = m_kFixedStrings.m_pBase;
        for (uint32_t i = 0; i < uiSize; ++i) {
            if (pcHandle == pkBase[i].GetHandle()) {
                return true;
            }
        }
    }
    m_kFixedStrings.AddFirstEmpty(kString);
    return true;
}

// The Win32 interlocked imports interpret the reference count's raw 32-bit storage as a signed LONG.
bool NiStream::RegisterSaveObject(NiObject* pkObject) {
    uint32_t uiLinkID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pkObject));
    bool bFound;
    bFound = m_kRegisterMap.GetAt(pkObject, uiLinkID);
    if (bFound) {
        return false;
    }
    m_kRegisterMap.SetAt(pkObject, m_kObjects.m_uiSize);
    NiPointer<NiObject> spObject;
    spObject.m_pObject = pkObject;
    if (pkObject) {
        pkObject->IncRefCount();
    }
    uint32_t uiIndex = m_kObjects.m_uiSize;
    if (uiIndex >= m_kObjects.m_uiMaxSize) {
        m_kObjects.SetSize(uiIndex + m_kObjects.m_uiGrowBy);
    }
    m_kObjects.SetAt(uiIndex, spObject);
    spObject.m_pObject = nullptr;
    if (pkObject) {
        pkObject->DecRefCount();
    }
    return true;
}

// The object being loaded is replaced in place; the array holds its own reference.
void NiStream::ChangeObject(NiObject* pkNewObject) {
    m_kObjects.SetAt(m_uiLoad, pkNewObject);
}

uint32_t NiStream::GetLinkIDFromObject(const NiObject* pkObject) {
    if (!pkObject) {
        return 0xFFFFFFFF;
    }

    uint32_t uiLinkID = 0xFFFFFFFF;
    m_kRegisterMap.GetAt(pkObject, uiLinkID);
    return uiLinkID;
}

// The three BSStreamHeader string lengths are read through the 1-byte transfer, NiStreamLoadBinary<bool>, and passed on
// as the raw byte, so each length is a bool here too.
bool NiStream::LoadHeader() {
    char acLine[128];
    m_pkIstr->GetLine(acLine, 128);
    if (!std::strstr(acLine, "File Format")) {
        m_uiLastError = NOT_NIF_FILE;
        strcpy_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), "Not a NIF file");
        return false;
    }

    m_pkIstr->SetEndianSwap(false);
    NiStreamLoadBinary(*this, m_uiNifFileVersion);
    if (m_uiNifFileVersion < ms_uiNifMinVersion) {
        m_uiLastError = OLDER_VERSION;
        strcpy_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), "NIF version is too old.");
        return false;
    }
    if (m_uiNifFileVersion > ms_uiNifMaxVersion) {
        m_uiLastError = LATER_VERSION;
        strcpy_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), "Unknown NIF version.");
        return false;
    }

    m_bSourceIsLittleEndian = true;
    if (m_uiNifFileVersion >= GetVersion(20, 0, 0, 3)) {
        NiStreamLoadBinary(*this, m_bSourceIsLittleEndian);
    }
    // The loaded byte is tested against 1; a bool reads the same for 0 and 1.
    if (!m_bSourceIsLittleEndian && NiBinaryStream::GetEndianMatchHint()) {
        m_uiLastError = ENDIAN_MISMATCH;
        NiStrcpy(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), "Endian mismatch.");
        return false;
    }

    if (m_uiNifFileVersion >= GetVersion(10, 0, 1, 8)) {
        NiStreamLoadBinary(*this, m_uiNifFileUserDefinedVersion);
        if (m_uiNifFileVersion == GetVersion(10, 0, 1, 101)) {
            m_uiNifFileUserDefinedVersion = 1;
        }
    }
    if (m_uiNifFileUserDefinedVersion < ms_uiNifMinUserDefinedVersion) {
        m_uiLastError = OLDER_VERSION;
        strcpy_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), "NIF user defined version is too old.");
        return false;
    }
    if (m_uiNifFileUserDefinedVersion > ms_uiNifMaxUserDefinedVersion) {
        m_uiLastError = LATER_VERSION;
        NiStrcpy(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), "Unknown NIF user defined version.");
        return false;
    }

    uint32_t uiObjects;
    NiStreamLoadBinary(*this, uiObjects);
    m_kObjects.SetSize(uiObjects);

    m_BSStreamHeader.uiVersion = 0;
    uint32_t uiVersion = m_uiNifFileVersion;
    if (uiVersion > GetVersion(10, 0, 1, 0) &&
        (uiVersion <= GetVersion(10, 0, 1, 8) || m_uiNifFileUserDefinedVersion)) {
        if (uiVersion > GetVersion(10, 0, 1, 1)) {
            NiStreamLoadBinary(*this, m_BSStreamHeader.uiVersion);
            if (m_uiNifFileVersion == GetVersion(10, 0, 1, 2)) {
                m_uiNifFileVersion = GetVersion(10, 0, 1, 1);
            }
        }
        uint8_t ucLength;
        NiStreamLoadBinary(*this, ucLength);
        NiStreamLoadBinary(*this, m_BSStreamHeader.cAuthor, ucLength);
        NiStreamLoadBinary(*this, ucLength);
        NiStreamLoadBinary(*this, m_BSStreamHeader.cProcessScript, ucLength);
        NiStreamLoadBinary(*this, ucLength);
        NiStreamLoadBinary(*this, m_BSStreamHeader.cExportScript, ucLength);
        if (m_BSStreamHeader.uiVersion < 21) {
            m_uiLastError = OLDER_VERSION;
            NiStrcpy(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), "NIF BSversion is too old");
            return false;
        }
    }

    m_pkIstr->SetEndianSwap(!m_bSourceIsLittleEndian);
    return true;
}

// The text line, the version, the endianness flag, the user version and the object count, then (with a user version)
// Bethesda's header: its version and three length-prefixed strings. Each transfer is a BinaryWrite with a one-component
// size local, and writes with no endian swap until the end, where the swap is set from the save endianness (the
// platform is little-endian).
void NiStream::SaveHeader() {
    m_pkOstr->SetEndianSwap(false);
    m_pkOstr->PutS("Gamebryo File Format, Version 20.2.0.7\n");
    uint32_t uiSize = 4;
    m_pkOstr->BinaryWrite(&ms_uiNifMaxVersion, 4, &uiSize, 1);
    uiSize = 1;
    m_pkOstr->BinaryWrite(&m_bSaveLittleEndian, 1, &uiSize, 1);
    uiSize = 4;
    m_pkOstr->BinaryWrite(&ms_uiNifMaxUserDefinedVersion, 4, &uiSize, 1);
    uint32_t uiObjects = m_kObjects.m_uiSize;
    uiSize = 4;
    m_pkOstr->BinaryWrite(&uiObjects, 4, &uiSize, 1);
    if (ms_uiNifMaxUserDefinedVersion) {
        uiSize = 4;
        m_pkOstr->BinaryWrite(&m_BSStreamHeader.uiVersion, 4, &uiSize, 1);
        const char* apcStrings[3] = {
            m_BSStreamHeader.cAuthor, m_BSStreamHeader.cProcessScript, m_BSStreamHeader.cExportScript
        };
        for (const char* pcString : apcStrings) {
            uint8_t ucLength = static_cast<uint8_t>(strlen(pcString) + 1);
            uiSize = 1;
            m_pkOstr->BinaryWrite(&ucLength, 1, &uiSize, 1);
            uiSize = 1;
            m_pkOstr->BinaryWrite(pcString, ucLength, &uiSize, 1);
        }
    }
    m_pkOstr->SetEndianSwap(!m_bSaveLittleEndian);
}

// Loads the header, the RTTI, object-size and fixed-string tables and the groups the file's version has, then each
// object (by its RTTI, or by LoadObject for files without one), the top-level objects, the links and post-links, and
// runs every post-process function on each top-level object. A pausing background load suspends its thread before each
// object; a cancelled one drops what was loaded.
bool NiStream::LoadStream() {
    bool bResult = LoadHeader();
    if (!bResult) {
        return bResult;
    }
    m_uiPostLink = 0;
    m_uiLink = 0;
    m_uiLoad = 0;
    RemoveAllObjects();
    bool bHasRTTI = m_uiNifFileVersion >= GetVersion(5, 0, 0, 1);
    if ((bHasRTTI && !LoadRTTI()) || (m_uiNifFileVersion >= GetVersion(20, 2, 0, 5) && !LoadObjectSizeTable()) ||
        (m_uiNifFileVersion >= GetVersion(20, 1, 0, 1) && !LoadFixedStringTable())) {
        return false;
    }
    if (m_uiNifFileVersion >= GetVersion(5, 0, 0, 6)) {
        LoadObjectGroups();
    }

    uint32_t uiObjects = m_kObjects.m_uiMaxSize;
    for (; m_uiLoad < uiObjects; ++m_uiLoad) {
        NiStream::ThreadStatus eStatus = m_eBackgroundLoadStatus;
        if (eStatus == NiStream::CANCELLING) {
            goto Cancelled;
        }
        if (eStatus == NiStream::PAUSING) {
            NiThread* pkThread = m_pkThread;
            m_eBackgroundLoadStatus = NiStream::PAUSED;
            pkThread->Suspend();
        }
        if (bHasRTTI) {
            NiObject* pkObject = m_kObjects.m_pBase[m_uiLoad].m_pObject;
            if (pkObject) {
                pkObject->LoadBinary(*this);
            }
            else {
                // A block without a create function is skipped by its size from the size table; the position read first
                // is unused.
                m_pkIstr->GetPosition();
                m_pkIstr->Seek(static_cast<int32_t>(m_kObjectSizes.m_pBase[m_uiLoad]));
            }
        }
        else if (!LoadObject()) {
            FreeLoadData();
            return false;
        }
    }
    LoadTopLevelObjects();
    for (; m_uiLink < uiObjects; ++m_uiLink) {
        if (m_eBackgroundLoadStatus == NiStream::PAUSING) {
            NiThread* pkThread = m_pkThread;
            m_eBackgroundLoadStatus = NiStream::PAUSED;
            pkThread->Suspend();
        }
        NiObject* pkObject = m_kObjects.m_pBase[m_uiLink].m_pObject;
        if (pkObject) {
            pkObject->LinkObject(*this);
        }
    }
    for (; m_uiPostLink < uiObjects; ++m_uiPostLink) {
        if (m_eBackgroundLoadStatus == NiStream::PAUSING) {
            NiThread* pkThread = m_pkThread;
            m_eBackgroundLoadStatus = NiStream::PAUSED;
            pkThread->Suspend();
        }
        NiObject* pkObject = m_kObjects.m_pBase[m_uiPostLink].m_pObject;
        if (pkObject) {
            pkObject->PostLinkObject(*this);
        }
    }
    {
        NiStream::ThreadStatus eStatus = m_eBackgroundLoadStatus;
        if (eStatus == NiStream::CANCELLING) {
            goto Cancelled;
        }
        if (eStatus == NiStream::PAUSING) {
            NiThread* pkThread = m_pkThread;
            m_eBackgroundLoadStatus = NiStream::PAUSED;
            pkThread->Suspend();
        }
    }
    // Tests the array's effective size once, then walks its whole size for each top-level object.
    if (ms_pkPostProcessFunctions->m_usESize > 0) {
        for (uint32_t i = 0; i < m_kTopObjects.m_uiSize; ++i) {
            NiObject* pkTopObject = m_kTopObjects.m_pBase[i].m_pObject;
            if (!pkTopObject) {
                continue;
            }
            for (uint32_t j = 0; j < ms_pkPostProcessFunctions->m_usSize; ++j) {
                PostProcessFunction pfnPostProcess = ms_pkPostProcessFunctions->m_pBase[j];
                if (pfnPostProcess) {
                    pfnPostProcess(*this, pkTopObject);
                }
            }
        }
    }
    if (m_eBackgroundLoadStatus == NiStream::CANCELLING) {
    Cancelled:
        FreeLoadData();
        return false;
    }
    SetSelectiveUpdateFlagsForOldVersions();
    FreeLoadData();
    return true;
}

bool NiStream::SaveStream() {
    NiStream* pkStream = this;
    pkStream->RegisterObjects();
    m_kObjectSizes.SetSize(m_kObjects.m_uiSize);
    pkStream->SaveHeader();
    SaveRTTI();
    uint32_t uiStartingOffset = pkStream->PreSaveObjectSizeTable();
    SaveFixedStringTable();
    UpdateObjectGroups();
    SaveObjectGroups();
    for (uint32_t i = 0; i < m_kObjects.m_uiSize; ++i) {
        NiObject* pkObject = m_kObjects.m_pBase[i].m_pObject;
        uint32_t uiStartInBytes = m_pkOstr->GetPosition();
        pkObject->SaveBinary(*this);
        uint32_t uiSize = m_pkOstr->GetPosition() - uiStartInBytes;
        m_kObjectSizes.SetAt(i, uiSize);
    }
    pkStream->SaveTopLevelObjects();
    pkStream->SaveObjectSizeTable(uiStartingOffset);
    m_kObjects.RemoveAll();
    m_kRegisterMap.RemoveAll();
    return true;
}

void NiStream::RegisterObjects() {
    for (uint32_t i = 0; i < m_kTopObjects.m_uiSize; ++i) {
        NiObject* pkObject = m_kTopObjects.m_pBase[i].m_pObject;
        pkObject->RegisterStreamables(*this);
    }
}

void NiStream::LoadTopLevelObjects() {
    uint32_t uiNumTopObjects;
    NiStreamLoadBinary(*m_pkIstr, uiNumTopObjects);
    m_kTopObjects.SetSize(uiNumTopObjects);
    for (uint32_t i = 0; i < uiNumTopObjects; ++i) {
        uint32_t uiLinkID;
        NiStreamLoadBinary(*m_pkIstr, uiLinkID);
        NiObject* pkObject = uiLinkID == 0xFFFFFFFF ? nullptr : m_kObjects.GetAt(uiLinkID).m_pObject;
        m_kTopObjects.SetAt(i, pkObject);
    }
}

void NiStream::SaveTopLevelObjects() {
    uint32_t uiNumTopObjects = m_kTopObjects.m_uiSize;
    uint32_t uiComponentSize = 4;
    m_pkOstr->BinaryWrite(&uiNumTopObjects, 4, &uiComponentSize, 1);
    NiStream* pkStream = this;
    for (uint32_t i = 0; i < uiNumTopObjects; ++i) {
        pkStream->SaveLinkID(m_kTopObjects.m_pBase[i].m_pObject);
    }
}

// Loads one object: its class name, the create function the loader map holds for it, then the object's own data. The
// created object is kept in a smart-pointer local, the array is given a reference, the local's reference is dropped and
// only then is LoadBinary called.
bool NiStream::LoadObject() {
    CreateFunction pfnCreate;
    char acRTTI[256];
    LoadRTTIString(acRTTI);
    if (!ms_pkLoaders->GetAt(acRTTI, pfnCreate)) {
        m_uiLastError = NO_CREATE_FUNCTION;
        strcpy_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), acRTTI);
        strcat_s(m_acLastErrorMessage, sizeof(m_acLastErrorMessage), ": cannot find create function.");
        return false;
    }

    NiObject* pkObject = pfnCreate();
    NiPointer<NiObject> spObject;
    spObject.m_pObject = pkObject;
    if (pkObject) {
        pkObject->IncRefCount();
    }
    uint32_t uiIndex = m_kObjects.m_uiSize;
    if (uiIndex >= m_kObjects.m_uiMaxSize) {
        m_kObjects.SetSize(uiIndex + m_kObjects.m_uiGrowBy);
    }
    m_kObjects.SetAt(uiIndex, spObject);
    spObject.m_pObject = nullptr;
    if (pkObject) {
        pkObject->DecRefCount();
    }
    pkObject->LoadBinary(*this);
    return true;
}

uint32_t NiStream::PreSaveObjectSizeTable() {
    uint32_t uiStartOffset = m_pkOstr->GetPosition();
    uint32_t uiSize = 0;
    for (uint32_t i = 0; i < m_kObjects.m_uiSize; ++i) {
        uint32_t uiComponentSize = 4;
        m_pkOstr->BinaryWrite(&uiSize, 4, &uiComponentSize, 1);
    }
    return uiStartOffset;
}

bool NiStream::SaveObjectSizeTable(uint32_t uiStartOffset) {
    uint32_t uiPosition = m_pkOstr->GetPosition();
    m_pkOstr->Seek(static_cast<int32_t>(uiStartOffset - uiPosition));
    for (uint32_t uiSize : m_kObjectSizes) {
        uint32_t uiComponentSize = 4;
        m_pkOstr->BinaryWrite(&uiSize, 4, &uiComponentSize, 1);
    }
    return true;
}

bool NiStream::LoadObjectSizeTable() {
    m_kObjectSizes.SetSize(m_kObjects.GetSize());
    for (uint32_t i = 0; i < m_kObjects.GetSize(); ++i) {
        // Clears the size before reading it.
        uint32_t uiSize = 0;
        NiStreamLoadBinary(*m_pkIstr, uiSize);
        m_kObjectSizes.SetAt(i, uiSize);
    }
    return true;
}

// Drops the loaded objects and resets the link ID sets, under the cleanup lock.
void NiStream::FreeLoadData() {
    ms_kCleanupCriticalSection.Lock();
    m_kObjects.RemoveAll();
    m_kLinkIDs.m_uiUsed = 0;
    m_kLinkIDBlocks.m_uiUsed = 0;
    m_uiLinkIndex = 0;
    m_uiLinkBlockIndex = 0;
    ms_kCleanupCriticalSection.Unlock();
}

// Reads a length, then that many characters into pcString.
// KEEP: niread: a short first read leaves uiLength unwritten, and the exe passes it on as the second read's size and
// indexes pcString with it.
void NiStream::LoadRTTIString(char* pcString) {
    uint32_t uiLength;
    NiStreamLoadBinary(*this, uiLength);
    NiStreamLoadBinary(*this, pcString, uiLength);
    pcString[uiLength] = '\0';
}

void NiStream::ReadLinkID() {
    uint32_t uiLinkID;
    NiStreamLoadBinary(*this, uiLinkID);
    m_kLinkIDs.Add(uiLinkID);
}

NiObject* NiStream::ResolveLinkID() {
    uint32_t uiLinkID;
    NiStreamLoadBinary(*this, uiLinkID);
    if (uiLinkID == 0xFFFFFFFF) {
        return nullptr;
    }

    return m_kObjects.GetAt(uiLinkID);
}

NiObject* NiStream::GetObjectFromLinkID() {
    uint32_t uiLinkID = m_kLinkIDs.GetAt(m_uiLinkIndex++);
    if (uiLinkID == 0xFFFFFFFF) {
        return nullptr;
    }

    return m_kObjects.GetAt(uiLinkID);
}

uint32_t NiStream::ReadMultipleLinkIDs() {
    uint32_t uiNumLinks;
    NiStreamLoadBinary(*m_pkIstr, uiNumLinks);
    m_kLinkIDBlocks.Add(uiNumLinks);
    for (uint32_t i = 0; i < uiNumLinks; ++i) {
        ReadLinkID();
    }
    return uiNumLinks;
}

// The count queued by ReadMultipleLinkIDs (or SetNumberOfLinkIDs).
uint32_t NiStream::GetNumberOfLinkIDs() {
    return m_kLinkIDBlocks.GetAt(m_uiLinkBlockIndex++);
}

NiObjectGroup* NiStream::GetGroupFromID(uint32_t uiID) const {
    return m_kGroups.GetAt(uiID);
}

uint32_t NiStream::GetIDFromGroup(NiObjectGroup* pkGroup) {
    uint32_t uiSize = m_kGroups.m_usSize;
    for (uint32_t i = 0; i < uiSize; ++i) {
        if (m_kGroups.m_pBase[i] == pkGroup) {
            return i;
        }
    }
    return 0;
}

// Reads a 4-byte index into the file's string table; -1 empties the string.
void NiStream::LoadFixedString(NiFixedString& kString) {
    int32_t a;
    int32_t uiStringID;
    NiBinaryStream* m_pkIstr = this->Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
    a = 4;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &uiStringID, 4, reinterpret_cast<uint32_t*>(&a), 1);
    if (uiStringID == -1) {
        const char* m_kHandle = (&kString)->GetHandle();
        if ((&kString)->GetHandle()) {
            kString.m_kHandle = NiGlobalStringTable::AddString(nullptr);
            NiGlobalStringTable::ReleaseHandle(m_kHandle);
        }
    }
    else {
        const char* v6 = m_kFixedStrings.m_pBase[uiStringID].GetHandle();
        if ((&kString)->GetHandle() != v6) {
            NiGlobalStringTable::AcquireHandle(v6);
            NiGlobalStringTable::ReleaseHandle((&kString)->GetHandle());
            kString.m_kHandle = v6;
        }
    }
}

void NiStream::LoadCStringAsFixedString(NiFixedString& kString) {
    uint32_t v14;
    int32_t iLength;
    NiBinaryStream* m_pkIstr = this->Istr();
    uint32_t (*m_pfnRead)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = m_pkIstr->m_pfnRead;
    v14 = 4;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &iLength, 4, &v14, 1);
    if (iLength < 1024) {
        const char* m_kHandle;
        const char* v13;
        if (iLength <= 0) {
            m_kHandle = (&kString)->GetHandle();
            if (!(&kString)->GetHandle()) {
                return;
            }
            v13 = NiGlobalStringTable::AddString(nullptr);
        }
        else {
            char acString[1024];
            NiBinaryStream* v10 = this->Istr();
            uint32_t (*v11)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v10->m_pfnRead;
            v14 = 1;
            v10->m_uiAbsoluteCurrentPos += v11(v10, acString, iLength, &v14, 1);
            m_kHandle = (&kString)->GetHandle();
            acString[iLength] = 0;
            if (m_kHandle == acString) {
                return;
            }
            v13 = NiGlobalStringTable::AddString(acString);
        }
        kString.m_kHandle = v13;
        NiGlobalStringTable::ReleaseHandle(m_kHandle);
    }
    else {
        // Gamebryo's NiAlloc(char, iLength + 1): the int count converts to the allocator's size.
        char* pcString = static_cast<char*>(_NiMalloc(static_cast<size_t>(iLength + 1)));
        NiBinaryStream* v6 = this->Istr();
        uint32_t (*v7)(NiBinaryStream*, void*, uint32_t, uint32_t*, uint32_t) = v6->m_pfnRead;
        v14 = 1;
        v6->m_uiAbsoluteCurrentPos += v7(v6, pcString, iLength, &v14, 1);
        pcString[iLength] = 0;
        const char* v9 = (&kString)->GetHandle();
        if ((&kString)->GetHandle() != pcString) {
            kString.m_kHandle = NiGlobalStringTable::AddString(pcString);
            NiGlobalStringTable::ReleaseHandle(v9);
        }
        _NiFree(pcString);
    }
}

// Writes the string's 4-byte table index, or NULL_LINKID for the null string.
void NiStream::SaveFixedString(const NiFixedString& kString) {
    const uint32_t* puiID;
    uint32_t uiID;
    if (kString.GetHandle() == NiGlobalStringTable::NULL_STRING) {
        puiID = &NULL_LINKID;
    }
    else {
        uiID = GetStringID(kString);
        puiID = &uiID;
    }
    NiBinaryStreamSave(*m_pkOstr, puiID, 1);
}

void NiStream::LoadCString(char*& pcString) {
    int32_t iLength;
    NiStreamLoadBinary(*this, iLength);
    if (iLength > 0) {
        pcString = static_cast<char*>(_NiMalloc(static_cast<size_t>(iLength) + 1));
        NiStreamLoadBinary(*this, pcString, static_cast<uint32_t>(iLength));
        pcString[iLength] = '\0';
    }
    else {
        pcString = nullptr;
    }
}

void NiStream::SaveCString(const char* pcString) {
    uint32_t uiLength = pcString ? static_cast<uint32_t>(strlen(pcString)) : 0;
    NiStreamSaveBinary(*this, uiLength);
    if (uiLength > 0) {
        NiStreamSaveBinary(*this, pcString, uiLength);
    }
}

// KEEP: niread: a short read leaves uiNumLinks (the Add value, the loop bound and the returned count) unwritten, and
// the exe uses it.
void NiStream::SetNumberOfLinkIDs(uint32_t uiLinks) {
    m_kLinkIDBlocks.Add(uiLinks);
}

void NiStream::RemoveAllObjects() {
    ms_kCleanupCriticalSection.Lock();
    m_kTopObjects.RemoveAll();
    m_kFixedStrings.RemoveAll();
    m_kObjectSizes.RemoveAll();
    ms_kCleanupCriticalSection.Unlock();
}

// Hands the texture to BSTexturePalette with no file entry; this is unused.
void NiStream::SetTexture(NiTexture* pkTexture) {
    BSTexturePalette::SetTexture(pkTexture, nullptr);
}

void NiStream::GetTexture(const char* pcURL, NiPointer<NiTexture>& arTexture) {
    BSTexturePalette::GetTexture(pcURL, arTexture);
}

// Passes the fixed NI_MAX_PATH sizes, not uiStringLen. Tests the whole byte of bUseDefaultPath.
bool NiStream::GetNextSearchPath(char* pcPath, uint32_t uiStringLen) {
    (void)uiStringLen;
    if (*reinterpret_cast<const uint8_t*>(&bUseDefaultPath) == 0) {
        return false;
    }

    strncpy_s(pcPath, 260, m_acFilePath, 259);
    bUseDefaultPath = false;
    return true;
}

void NiStream::SetFilePath(const char* pcFilePath) {
    if (pcFilePath && *pcFilePath) {
        strncpy_s(m_acFilePath, sizeof(m_acFilePath), pcFilePath, sizeof(m_acFilePath) - 1);
        NiPath::Standardize(m_acFilePath);
    }
    else {
        m_acFilePath[0] = '\0';
    }
}

// The Win32 interlocked imports interpret the reference count's raw 32-bit storage as a signed LONG.
void NiStream::InsertObject(NiObject* pkObject) {
    NiPointer<NiObject> spObject;
    spObject.m_pObject = pkObject;
    if (pkObject) {
        pkObject->IncRefCount();
    }
    uint32_t uiIndex = m_kTopObjects.m_uiSize;
    if (uiIndex >= m_kTopObjects.m_uiMaxSize) {
        m_kTopObjects.SetSize(uiIndex + m_kTopObjects.m_uiGrowBy);
    }
    m_kTopObjects.SetAt(uiIndex, spObject);
    spObject.m_pObject = nullptr;
    if (pkObject) {
        pkObject->DecRefCount();
    }
}

void NiStream::SetLastError(uint32_t error) {
    m_uiLastError = static_cast<NiStream::ErrorMessages>(error);
}

uint32_t NiStream::GetStringID(const NiFixedString& kString) {
    if ((&kString)->GetHandle() == NiGlobalStringTable::NULL_STRING) {
        return -1;
    }
    uint32_t m_uiSize = m_kFixedStrings.m_uiSize;
    uint32_t result = 0;
    if (!m_uiSize) {
        return -1;
    }
    for (NiFixedString* i = m_kFixedStrings.m_pBase; (&kString)->GetHandle() != i->GetHandle(); ++i) {
        if (++result >= m_uiSize) {
            return -1;
        }
    }
    return result;
}

void NiStream::SetSelectiveUpdateFlagsForOldVersions() {
    if (m_uiNifFileVersion >= GetVersion(4, 1, 0, 12)) {
        return;
    }

    for (NiObject* pkObject : m_kObjects) {
        if (!pkObject) {
            continue;
        }

        // NiIsKindOf(NiAVObject, pkObject): walks the RTTI chain.
        for (const NiRTTI* pkRTTI = pkObject->GetRTTI(); pkRTTI; pkRTTI = pkRTTI->m_pkBaseRTTI) {
            if (pkRTTI == &NiAVObject::ms_RTTI) {
                NiAVObject* pkAVObject = static_cast<NiAVObject*>(pkObject);
                if (!pkAVObject->m_pkParent) {
                    SetSelectiveUpdateFlagsTTTFRecursive(pkAVObject);
                }
                break;
            }
        }
    }
}

// Marks the subtree for selective update of transforms and property controllers, not rigid, and recurses into every
// child of a node.
void NiStream::SetSelectiveUpdateFlagsTTTFRecursive(NiAVObject* pkAVObject) {
    pkAVObject->m_uFlags = (pkAVObject->m_uFlags & ~static_cast<uint32_t>(NiAVObject::SELECTIVE_UPDATE_RIGID)) |
        NiAVObject::SELECTIVE_UPDATE | NiAVObject::SELECTIVE_UPDATE_TRANSFORMS |
        NiAVObject::SELECTIVE_UPDATE_CONTROLLER;
    NiNode* pkNode = pkAVObject->IsNode();
    if (pkNode) {
        for (NiAVObject* pkChild : pkNode->m_kChildren) {
            if (pkChild) {
                SetSelectiveUpdateFlagsTTTFRecursive(pkChild);
            }
        }
    }
}

void NiStream::LoadObjectGroups() {
    uint32_t uiNumGroups;
    NiStreamLoadBinary(*m_pkIstr, uiNumGroups);
    // Group 0 is the null group.
    ++uiNumGroups;
    m_kGroups.SetSize(uiNumGroups);
    m_kGroups.SetAt(0, nullptr);
    for (uint32_t i = 1; i < uiNumGroups; ++i) {
        uint32_t uiBytes;
        NiStreamLoadBinary(*m_pkIstr, uiBytes);
        NiObjectGroup* pkGroup = new NiObjectGroup(uiBytes);
        m_kGroups.SetAt(i, pkGroup);
    }
}

void NiStream::SaveObjectGroups() {
    uint32_t uiNumGroups = static_cast<uint32_t>(m_kGroups.m_usSize) - 1;
    uint32_t uiComponentSize = 4;
    m_pkOstr->BinaryWrite(&uiNumGroups, 4, &uiComponentSize, 1);
    for (uint32_t i = 1; i < m_kGroups.m_usSize; ++i) {
        uint32_t uiSize = m_kGroups.m_pBase[i]->m_uiSize;
        uiComponentSize = 4;
        m_pkOstr->BinaryWrite(&uiSize, 4, &uiComponentSize, 1);
    }
}

void NiStream::UpdateObjectGroups() {
    m_kGroups.SetSize(1);
    m_kGroups.SetAt(0, nullptr);
    for (NiObject* pkObject : m_kObjects) {
        if (!pkObject) {
            continue;
        }
        NiObjectGroup* pkGroup = pkObject->GetGroup();
        if (!pkGroup) {
            continue;
        }
        uint32_t uiSize = m_kGroups.m_usSize;
        uint32_t uiID = 0;
        for (uint32_t j = 0; j < uiSize; ++j) {
            if (m_kGroups.m_pBase[j] == pkGroup) {
                uiID = j;
                break;
            }
        }
        if (uiID) {
            continue;
        }
        m_kGroups.Add(pkGroup);
        pkGroup->m_uiSize = 0;
    }
    for (NiObject* pkObject : m_kObjects) {
        if (!pkObject) {
            continue;
        }
        NiObjectGroup* pkGroup = pkObject->GetGroup();
        if (pkGroup) {
            uint32_t uiSize = pkGroup->m_uiSize;
            pkGroup->m_uiSize = uiSize + pkObject->GetBlockAllocationSize();
        }
    }
}

// The string count, the longest string's length, then each string as its length and its characters (a null string as
// length 0).
void NiStream::SaveFixedStringTable() {
    uint32_t uiNumStrings = m_kFixedStrings.m_uiSize;
    uint32_t uiComponentSize = 4;
    m_pkOstr->BinaryWrite(&uiNumStrings, 4, &uiComponentSize, 1);

    uint32_t uiCount = m_kFixedStrings.m_uiSize;
    uint32_t uiMaxLength = 0;
    if (uiCount) {
        const NiFixedString* pkStrings = m_kFixedStrings.m_pBase;
        const char* pcNull = NiGlobalStringTable::NULL_STRING;
        uint16_t i = 0;
        do {
            const char* pcHandle = pkStrings[i].GetHandle();
            if (pcHandle != pcNull) {
                uint32_t uiLength = NiGlobalStringTable::GetStringHeader(pcHandle)->m_uiLength;
                if (uiLength > uiMaxLength) {
                    uiMaxLength = uiLength;
                }
            }
            ++i;
        }
        while (i < uiCount);
    }
    uiComponentSize = 4;
    m_pkOstr->BinaryWrite(&uiMaxLength, 4, &uiComponentSize, 1);

    for (uint16_t i = 0; i < m_kFixedStrings.m_uiSize; ++i) {
        const char* pcHandle = m_kFixedStrings.m_pBase[i].GetHandle();
        uint32_t uiLength = pcHandle == NiGlobalStringTable::NULL_STRING
            ? 0
            : NiGlobalStringTable::GetStringHeader(pcHandle)->m_uiLength;
        uiComponentSize = 4;
        m_pkOstr->BinaryWrite(&uiLength, 4, &uiComponentSize, 1);
        uiComponentSize = 1;
        m_pkOstr->BinaryWrite(pcHandle, uiLength, &uiComponentSize, 1);
    }
}

// Writes the class names of the objects to save, each once, and then each object's index into them (with 0x8000 set for
// an object the reader may skip). The name map has 37 buckets and does not copy its keys.
void NiStream::SaveRTTI() {
    using RTTIMapBase = NiTMapBase<NiTPointerAllocator<size_t>, const char*, uint16_t>;
    using RTTIMap = NiTStringPointerMap<uint16_t>;
    RTTIMap kRTTIMap(37, false);

    for (NiObject* pkObject : m_kObjects) {
        const char* pcName = pkObject->GetStreamableRTTI()->GetName();
        uint16_t usIndex;
        if (!kRTTIMap.GetAt(pcName, usIndex)) {
            kRTTIMap.RTTIMapBase::SetAt(pcName, static_cast<uint16_t>(kRTTIMap.GetCount()));
        }
    }

    // The count is a 2-byte value, held zero-extended in 32 bits, and only its low 16 bits are written.
    uint32_t uiNumRTTI = static_cast<uint16_t>(kRTTIMap.GetCount());
    const char** ppcRTTI = static_cast<const char**>(_NiMalloc(sizeof(const char*) * uiNumRTTI));
    NiTMapItem<const char*, uint16_t>* pkPos = kRTTIMap.GetFirstPos();
    while (pkPos) {
        const char* pcName;
        uint16_t usIndex;
        kRTTIMap.GetNext(pkPos, pcName, usIndex);
        ppcRTTI[usIndex] = pcName;
    }
    uint32_t uiSize = 2;
    m_pkOstr->BinaryWrite(&uiNumRTTI, 2, &uiSize, 1);
    for (uint32_t i = 0; i < static_cast<uint16_t>(uiNumRTTI); ++i) {
        SaveCString(ppcRTTI[i]);
    }
    _NiFree(ppcRTTI);

    // uiRTTI is shared by every object, so a name the map lacks (never the case, every name was added above) would
    // leave the previous object's index, as the game does.
    uint16_t uiRTTI;
    for (NiObject* pkObject : m_kObjects) {
        const char* pcName = pkObject->GetStreamableRTTI()->GetName();
        kRTTIMap.GetAt(pcName, uiRTTI);
        if (pkObject->StreamCanSkip()) {
            uiRTTI |= 0x8000;
        }
        uiSize = 2;
        m_pkOstr->BinaryWrite(&uiRTTI, 2, &uiSize, 1);
    }
    // kRTTIMap's destructor runs here, at the end of its scope.
}

// Reads the string count and the longest string's size, then each string into one buffer and from there into the
// fixed-string table.
// KEEP: niread: a short read leaves uiStringCount (the loop bound) and uiMaxStringSize (the buffer size) unwritten,
// and the exe uses both.
bool NiStream::LoadFixedStringTable() {
    uint32_t uiStringCount;
    uint32_t uiSize = 4;
    m_pkIstr->BinaryRead(&uiStringCount, 4, &uiSize, 1);
    m_kFixedStrings.SetSize(uiStringCount);
    uint32_t uiMaxStringSize;
    uiSize = 4;
    m_pkIstr->BinaryRead(&uiMaxStringSize, 4, &uiSize, 1);
    char* pcString = static_cast<char*>(_NiMalloc(uiMaxStringSize + 1));
    for (uint16_t us = 0; us < uiStringCount; ++us) {
        uint32_t uiLength = 0;
        uiSize = 4;
        m_pkIstr->BinaryRead(&uiLength, 4, &uiSize, 1);
        uiSize = 1;
        m_pkIstr->BinaryRead(pcString, uiLength, &uiSize, 1);
        pcString[uiLength] = 0;
        m_kFixedStrings.SetAt(us, pcString);
    }
    _NiFree(pcString);
    return true;
}

// Removes the class's create function from the loader map.
void NiStream::UnregisterLoader(const char* pcClassName) {
    ms_pkLoaders->RemoveAt(pcClassName);
}

int32_t NiStream::RegisterLoader(const char* pcName, CreateFunction pfnFunc) {
    ms_pkLoaders->SetAt(pcName, pfnFunc);
    return 0;
}

// Makes an empty object of the class registered under pcRTTI; null when none is.
NiObject* NiStream::CreateObjectByRTTI(const char* pcRTTI) {
    CreateFunction pfnCreate;
    if (!ms_pkLoaders->GetAt(pcRTTI, pfnCreate)) {
        return nullptr;
    }
    return pfnCreate();
}

// The array keeps its size unless the last slot is the one emptied.
void NiStream::UnregisterPostProcessFunction(PostProcessFunction pfnFunc) {
    for (uint32_t ui = 0; ui < ms_pkPostProcessFunctions->m_usSize; ++ui) {
        if (ms_pkPostProcessFunctions->m_pBase[ui] == pfnFunc) {
            ms_pkPostProcessFunctions->RemoveAt(ui);
            break;
        }
    }
}

// Constructs both tables: the loader map (59 buckets, no copies of the keys) and the post-process array (grows by 3).
// Each object comes from MemoryManager::Allocate.
void NiStream::_SDMInit() {
    NiTStringPointerMap<CreateFunction>* pkLoaders = static_cast<NiTStringPointerMap<CreateFunction>*>(
        MemoryManager_Singleton.Allocate(sizeof(NiTStringPointerMap<CreateFunction>))
    );
    if (pkLoaders) {
        ms_pkLoaders = ::new (pkLoaders) NiTStringPointerMap<CreateFunction>(59, false);
    }
    else {
        ms_pkLoaders = nullptr;
    }

    PostProcessFunctionArray* pkPostProcessFunctions =
        static_cast<PostProcessFunctionArray*>(MemoryManager_Singleton.Allocate(sizeof(PostProcessFunctionArray)));
    if (pkPostProcessFunctions) {
        ms_pkPostProcessFunctions = ::new (pkPostProcessFunctions) PostProcessFunctionArray(0, 3);
    }
    else {
        ms_pkPostProcessFunctions = nullptr;
    }
    bUseDefaultPath = true;
}

void NiStream::_SDMShutdown() {
    if (ms_pkLoaders) {
        delete ms_pkLoaders;
    }
    if (ms_pkPostProcessFunctions) {
        delete ms_pkPostProcessFunctions;
    }
}

void NiStream::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

uint32_t NiStream::GetVersionFromString(const char* pcVersionString) {
    char* Context;
    char String[16];
    strcpy_s(String, 16, pcVersionString);
    int32_t v1 = 24;
    uint32_t v2 = 0;
    for (char* i = strtok_s(String, ".", &Context); i; i = strtok_s(nullptr, ".", &Context)) {
        int32_t v4 = std::atoi(i) << v1;
        v1 -= 8;
        v2 |= v4;
    }
    return v2;
}

void NiStream::RegisterPostProcessFunction(void (*pfnFunc)(NiStream&, NiObject*)) {
    ms_pkPostProcessFunctions->AddFirstEmpty(pfnFunc);
}

// NiStreamSaveBinary for a byte (BSValueNode::m_uFlags and others).
void NiStreamSaveByte(NiStream* a1, const uint8_t* a2) {
    NiStreamSaveBinary(*a1, *a2);
}
