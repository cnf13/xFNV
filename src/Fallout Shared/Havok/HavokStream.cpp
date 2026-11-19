#include "Fallout Shared/Havok/HavokStream.h"

#include "BSCore/BSCoreMessage.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/MessageHandler.h"
#include "BSSystem/BSFile.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/HavokError.h"
#include "Fallout Shared/Havok/HavokFileStreambufReader.h"
#include "Fallout Shared/Havok/HavokFileStreambufWriter.h"
#include "Fallout Shared/Havok/HavokStreambufFactory.h"
#include "Fallout Shared/Havok/HitCaster.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "SDK/MSVC/string.h"

#include <cstdint>
#include <new>
#include <xmmintrin.h>

SettingT<INISettingCollection> gs_iSimType_HAVOK("iSimType:HAVOK", 1);

// The two path separators hkGetFileBaseName strips, "/" then "\".
using HavokError_Separators_t = const char[3];
static HavokError_Separators_t HavokError_Separators{"/\\"};

void KillHavokSystem() {
    bhkWorld::QuitSystem();
}

void hkDeallocateChars(void* apPtr) {
    hkMemoryRouter* instance = &hkMemoryRouter::getInstance();
    hkMemoryRouter::easyFree(*static_cast<hkMemoryAllocator*>(instance->GetHeap()), apPtr);
}

void destructElements(int32_t a1, int32_t a2) {
    int32_t i;
    for (i = 0; i < a2; ++i) {
        ;
    }
}

uint32_t insert(void* apThis, [[maybe_unused]] void* apEdx, uint32_t key, uint32_t val) {
    hkContainerHeapAllocator v6 = hkContainerHeapAllocator{};
    hkMemoryAllocator* v3 = &(&v6)->get(apThis);
    return static_cast<hkUlongMapBase*>(apThis)->insert(*v3, key, val);
}

HavokError::HavokError() {
    constexpr uint32_t kDisabledIds[] = {
        0x6E8D163B, 0x6CEE9071, 0xF0DE4355, 0x2FF8C16F, 0xF0FE4356, 0x275EC1FD, 0x1ADAAD0E, 0x70DC41CB, 0x1FF88F0E,
        0x475D86B1, 0x6FE84A9B, 0x21C8AB2A, 0x309314D9, 0xAD67FA3A, 0xAD67FA3A, 0xF032DE34, 0x34DF5494, 0xF02E32E1,
        0xF02E32E2, 0xF02E32DF, 0xF043D534, 0xF02184ED, 0xF02DE43E, 0xF02E32E0, 0xF02132DF, 0xF02132FF, 0x54006467,
        0x717C3B1D, 0xF032DF30, 0x0F0213DE, 0xAD8966AA, 0x2A1DB936, 0x68C4E1DC, 0x38F1276D, 0xF03243DF, 0x2FF8C16D,
        0xABBAEFBF, 0x761EC287, 0x53FE2EBC, 0x1F0690F0, 0x56F80833,
    };
    for (uint32_t uiId : kDisabledIds) {
        HavokError::setEnabled(static_cast<int32_t>(uiId), hkBool(false));
    }
    constexpr int32_t kDisabledLines[] = {0x26, 0x56, 0x5D, 0x6A, 0x6E, 0x73, 0x7E, 0x85, 0xBF, 0xF6, 0x115};
    for (int32_t iLine : kDisabledLines) {
        HavokError::disableReport("hkVisualDebugger.cpp", iLine);
    }
    pRootDir[0] = 0;
}

HavokError::~HavokError() {
    KillArray(m_disabledReports);
}

int32_t HavokError::message(hkError::Message m, int32_t id, const char* description, const char* file, int32_t line) {
    if (id == -1) {
        if (!isReportEnabled(file, line)) {
            return 0;
        }
    }
    else {
        hkBool v9 = isEnabled(id);
        if (!v9.operator bool()) {
            return 0;
        }
    }
    switch (m) {
        case 0: {
            programFlowMessage("Report", id, description, file, line);
            break;
        }
        case 1: {
            warningMessage("Warning", id, description, file, line);
            break;
        }
        case 2:
        case 3: {
            assertMessage("Assert", id, description, file, line);
            break;
        }
        default: {
            errorMessage("Error", id, description, file, line);
            break;
        }
    }
    return 1;
}

void HavokError::setEnabled(int32_t id, hkBool enabled) {
    if (enabled.operator bool()) {
        m_disabledAssertIds.remove(id);
    }
    else {
        m_disabledAssertIds.insert(id, 1);
    }
}

hkBool HavokError::isEnabled(int32_t aiId) {
    int32_t v3 = m_disabledAssertIds.getWithDefault(aiId, 0);
    return v3 == 0;
}

void HavokError::enableAll() {
    m_disabledAssertIds.clear();
}

void HavokError::disableReport(const char* apFile, int32_t aiLine) {
    HavokError::hkFileLine ap;
    char src[132];
    char Src[32];
    BSsplitpath(apFile, nullptr, 0, nullptr, 0, src, 128, Src, 32);
    BSstrcat(src, 128, Src);
    ap.m_file = hkString::strDup(src);
    ap.m_line = aiLine;
    m_disabledReports.pushBack<HavokError::hkFileLine>(ap);
}

void HavokError::enableReport(const char* apFile, int32_t aiLine) {
    char Dst[132];
    char Src[32];
    BSsplitpath(apFile, nullptr, 0, nullptr, 0, Dst, 128, Src, 32);
    BSstrcat(Dst, 128, Src);
    for (int32_t i = 0; i < m_disabledReports.getSize(); ++i) {
        hkFileLine ap = m_disabledReports[i];
        if (!hkString::strCasecmp(Dst, ap.m_file) && aiLine == ap.m_line) {
            // m_file is const char*, and the deallocator takes a char*, hence the cast.
            hkDeallocateChars(const_cast<char*>(ap.m_file));
            m_disabledReports.removeAt(i);
            return;
        }
    }
}

// Builds "type : description", rebases a file under the source tree's TES4 folder (or a "..\" path into the Havok
// SDK's include folder) onto pRootDir, and hands it to the message handler; answering 5 switches the id off.
void HavokError::assertMessage(
    const char* apWhat,
    int32_t aiID,
    const char* apDescription,
    const char* apFile,
    int32_t aiLine
) {
    char cMessage[1024];
    char cPath[260];
    if (apWhat) {
        BSstrcpy(cMessage, 1024, apWhat);
    }
    else {
        cMessage[0] = 0;
    }
    if (apDescription) {
        BSstrcat(cMessage, 1024, " : ");
        BSstrcat(cMessage, 1024, apDescription);
    }
    int32_t iLine = aiLine;
    const char* pFile = nullptr;
    if (pRootDir[0]) {
        pFile = std::strstr(apFile, "TES4");
        if (!pFile) {
            pFile = std::strstr(apFile, "tes4");
        }
        if (pFile) {
            BSstrcpy(cPath, 260, pRootDir);
            BSstrcat(cPath, 260, pFile);
            pFile = cPath;
        }
        else if (apFile[0] == '.' && apFile[1] == '.') {
            BSstrcpy(cPath, 260, pRootDir);
            BSstrcat(cPath, 260, "TES4\\Havok\\SDK\\Include\\");
            BSstrcat(cPath, 260, apFile + 3);
            pFile = cPath;
        }
    }
    if (!pFile) {
        pFile = apFile;
        iLine = aiLine;
    }
    BSCoreMessage* pInstance = BSCoreMessage::GetInstance();
    if (pInstance->Assert(pFile, iLine, cMessage) == 5) {
        hkBool bEnabled;
        bEnabled.~hkBool();
        ::new (&bEnabled) hkBool(false);
        setEnabled(aiID, bEnabled);
    }
}

void HavokError::errorMessage(
    const char* apWhat,
    int32_t aiID,
    const char* apDescription,
    const char* apFile,
    int32_t aiLine
) {
    MessageHandler::Handler();
    Warning(nullptr, "%s:%d: %s : %s'", apFile, aiLine, apWhat, apDescription);
}

void HavokError::warningMessage(
    const char* apWhat,
    int32_t aiID,
    const char* apDescription,
    const char* apFile,
    int32_t aiLine
) {
    MessageHandler::Handler();
    Warning(WARN_COMBAT, "%s:%d: %s : %s'", apFile, aiLine, apWhat, apDescription);
}

void HavokError::programFlowMessage(
    const char* apWhat,
    int32_t aiID,
    const char* apDescription,
    const char* apFile,
    int32_t aiLine
) {
    MessageHandler::Handler();
    Warning(WARN_ANIMATION, "%s:%d: %s : %s'", apFile, aiLine, apWhat, apDescription);
}

void HavokError::showString(const char* apStr) {
    MessageHandler::Handler();
    Warning(WARN_SCRIPTS, apStr);
}

// The text after the last "/" and then after the last "\"; a separator that is missing leaves null, as the game does.
const char* HavokError::hkGetFileBaseName(const char* apPath) {
    const char* pSeparators = HavokError_Separators;
    for (int32_t i = 0; i < 2; i++) {
        const char* pFound = hkString::strRchr(apPath, pSeparators[i]);
        apPath = pFound ? pFound + 1 : pFound;
    }
    return apPath;
}

void HavokError::KillArray(hkArray<hkFileLine>& arArray) {
    for (int32_t i = 0; i < arArray.getSize(); ++i) {
        hkFileLine ap = arArray[i];
        // m_file is const char*, and the deallocator takes a char*, hence the cast.
        hkDeallocateChars(const_cast<char*>(ap.m_file));
    }
}

void HavokError::SetRootDir(const char* apRoot) {
    const char* pfileName = std::strstr(apRoot, "TES4");
    if (!pfileName) {
        pfileName = std::strstr(apRoot, "tes4");
    }
    if (pfileName) {
        BSstrncpy(pRootDir, apRoot, pfileName - apRoot);
        pRootDir[pfileName - apRoot] = 0;
    }
}

bool HavokError::isReportEnabled(const char* apFile, int32_t aiLine) {
    if (m_disabledReports.getSize()) {
        char Dst[132];
        char Src[32];
        BSsplitpath(apFile, nullptr, 0, nullptr, 0, Dst, 128, Src, 32);
        BSstrcat(Dst, 128, Src);
        for (int32_t i = 0; i < m_disabledReports.getSize(); ++i) {
            hkFileLine ap = m_disabledReports[i];
            if (!hkString::strCasecmp(Dst, ap.m_file) && aiLine == ap.m_line) {
                return false;
            }
        }
    }
    return true;
}

void HavokError::operator delete(void* apMemory) {
    hkReferencedObject::deleteObject(apMemory);
}

HavokFileStreambufReader::HavokFileStreambufReader([[maybe_unused]] const char* apPath) : bOwned(true) {}

HavokFileStreambufReader::~HavokFileStreambufReader() {
    if (pFile && bOwned) {
        delete pFile;
    }
}

hkBool HavokFileStreambufReader::isOk() const {
    uint8_t isGood;
    if (pFile) {
        isGood = (uint8_t)pFile->FileIsGood();
        return isGood;
    }
    else {
        return false;
    }
}

int32_t HavokFileStreambufReader::read(void* apBuf, int32_t aiBytes) {
    return (int32_t)pFile->NiBinaryStream::Read(apBuf, aiBytes);
}

hkResult HavokFileStreambufReader::seek(int32_t aiOffset, uint32_t aeWhence) {
    int32_t v4 = 1;
    if (pFile) {
        pFile->Seek(aiOffset, (int32_t)aeWhence);
        return HK_SUCCESS;
    }
    return static_cast<hkResult>(v4);
}

int32_t HavokFileStreambufReader::tell() {
    if (pFile) {
        return (int32_t)pFile->Tell();
    }

    return 0;
}

int32_t HavokFileStreambufReader::sysGetFileSize() {
    if (pFile) {
        return (int32_t)pFile->GetSize();
    }

    return (-1);
}

void HavokFileStreambufReader::operator delete(void* apMemory) {
    hkReferencedObject::deleteObject(apMemory);
}

HavokFileStreambufWriter::HavokFileStreambufWriter(const char* apFilename)
  : pFile(new BSFile(apFilename, NiFile::WRITE_ONLY, 0x4000u, false)), bOwned(true) {}

HavokFileStreambufWriter::~HavokFileStreambufWriter() {
    if (pFile && bOwned) {
        delete pFile;
    }
}

hkBool HavokFileStreambufWriter::isOk() const {
    uint8_t isGood;
    if (pFile) {
        isGood = (uint8_t)pFile->FileIsGood();
        return isGood;
    }
    else {
        return false;
    }
}

int32_t HavokFileStreambufWriter::write(const void* apBuf, int32_t aiBytes) {
    return pFile->WriteHere(const_cast<void*>(apBuf), aiBytes);
}

hkResult HavokFileStreambufWriter::seek(int32_t aiOffset, uint32_t aeWhence) {
    if (pFile) {
        pFile->Seek(aiOffset, (int32_t)aeWhence);
        return HK_SUCCESS;
    }
    return HK_FAILURE;
}

int32_t HavokFileStreambufWriter::tell() {
    if (pFile) {
        return (int32_t)pFile->Tell();
    }

    return 0;
}

void HavokFileStreambufWriter::operator delete(void* apMemory) {
    hkReferencedObject::deleteObject(apMemory);
}

HavokStreambufFactory::HavokStreambufFactory() {}

HavokStreambufFactory::~HavokStreambufFactory() {}

hkStreamReader* HavokStreambufFactory::openReader(const char* apName) {
    void* apMemory = hkReferencedObject::newObject(sizeof(HavokFileStreambufReader));
    if (apMemory) {
        return ::new (apMemory) HavokFileStreambufReader(apName);
    }

    return nullptr;
}

hkStreamWriter* HavokStreambufFactory::openWriter(const char* apName) {
    HavokFileStreambufWriter* v4 =
        static_cast<HavokFileStreambufWriter*>(hkReferencedObject::newObject(sizeof(HavokFileStreambufWriter)));
    if (v4) {
        return ::new (v4) HavokFileStreambufWriter(apName);
    }

    return nullptr;
}

void HavokStreambufFactory::operator delete(void* apMemory) {
    hkReferencedObject::deleteObject(apMemory);
}

hkFileSystem::hkFileSystem() {}

hkFileSystem::~hkFileSystem() {}

hkResult hkFileSystem::listDirectory(const char* basePath, hkFileSystem::DirectoryListing& listingOut) {
    return HK_FAILURE;
}

hkStreamReader::hkStreamReader() {}

hkStreamReader::~hkStreamReader() {}

hkStreamWriter::hkStreamWriter() {}
