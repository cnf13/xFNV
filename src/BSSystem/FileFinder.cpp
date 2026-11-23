#include "BSSystem/FileFinder.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/Archive.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "SDK/MSVC/string.h"

// The first finder built becomes the engine's finder; NiFile opens and tests files through the finder.
FileFinder::FileFinder(
    [[maybe_unused]] const char* apBasePath,
    [[maybe_unused]] int32_t aiFileLogging,
    bool abUseDDXTextures
)
  : kPaths(0) {
    if (!FileFinder::pFinder) {
        FileFinder::pFinder = this;
    }
    NiFile::SetFileCreateFunc(&FileFinder::GetFile);
    NiFile::SetFileAccessFunc(&FileFinder::AccessFile);
    FileFinder::bUseDDXTextures = abUseDDXTextures;
}

// Frees the finder's own path copies (AddPath); kPaths's destructor frees its buffer.
FileFinder::~FileFinder() {
    for (uint32_t auiIndex = kPaths.iSize; auiIndex--; kPaths.RemoveFast(auiIndex, 1u)) {
        MemoryManager::Instance().Deallocate(const_cast<char*>(kPaths.pBuffer[auiIndex]));
    }
    if (FileFinder::pFinder == this) {
        FileFinder::pFinder = nullptr;
    }
}

// Looks the file up in the archives, the working directory, the alternate subdirectories and then the search paths;
// a found path is copied back over apFilePath.
FileFinder::FILE_LOCATION FileFinder::LookForFile(char* apFilePath, uint32_t aiFlags, uint32_t aiArchiveType) {
    if ((aiFlags & SKIP_ARCHIVE) == 0 &&
        ArchiveManager::FindFile(apFilePath, static_cast<ARCHIVE_TYPE>(aiArchiveType))) {
        return ARCHIVE;
    }
    // Drops the caller's flags unless the archives are in use and older files are not invalidated.
    if (!ArchiveManager::bUseArchives || ArchiveManager::bInvalidateOlderFiles) {
        aiFlags = SKIP_NONE;
    }
    if (SearchCWD(aiFlags, aiArchiveType) && rt::_access(apFilePath, 0) != -1) {
        return HARDDRIVE;
    }
    if (!SearchAlternateSubdirectories(aiFlags, aiArchiveType)) {
        return NOTFOUND;
    }
    // The path count is read once before the loop; the buffer is reloaded through the object every iteration.
    const uint32_t uiNumPaths = kPaths.iSize;
    for (uint32_t i = 0; i < uiNumPaths; ++i) {
        const char* pcPath = kPaths.pBuffer[i];
        if (!pcPath) {
            continue;
        }
        const size_t uiPathLength = std::strlen(pcPath);
        // A path the file name already starts with is not tried again.
        if (!_strnicmp(apFilePath, pcPath, uiPathLength)) {
            continue;
        }
        char acFileName[260];
        strcpy_s(acFileName, 260, pcPath);
        strcat_s(acFileName, 260, apFilePath);
        if (rt::_access(acFileName, 0) == -1) {
            continue;
        }
        strcpy_s(apFilePath, 260, acFileName);
        return HARDDRIVE;
    }
    return NOTFOUND;
}

FileFinder::FILE_LOCATION
FileFinder::Exist(const char* apName, char* apFilePath, uint32_t aiFlags, uint32_t aiArchiveType) {
    char Str1[268];
    if (aiArchiveType == -1) {
        aiArchiveType = ARCHIVE_TYPE_ALL;
    }
    if (apName[1] == 58) {
        aiFlags |= SKIP_ARCHIVE | SKIP_PATHS;
    }
    else {
        if (*apName == 46 && apName[1] == 92) {
            aiFlags |= SKIP_PATHS;
            apName += 2;
        }
    }
    if (*apName == 92) {
        strcpy_s(Str1, 260, apName + 1);
    }
    else {
        strcpy_s(Str1, 260, apName);
    }
    FILE_LOCATION eres = LookForFile(Str1, aiFlags, aiArchiveType);
    if (eres) {
        if (apFilePath) {
            strcpy_s(apFilePath, 260, Str1);
        }
    }
    else {
        if (apFilePath) {
            *apFilePath = 0;
        }
    }
    return eres;
}

void FileFinder::AddPath(const char* apPath) {
    if (apPath) {
        size_t uiLength = std::strlen(apPath);
        bool bAddSlash = apPath[uiLength - 1] != 92;
        size_t Size = std::strlen(apPath) + bAddSlash + 1;
        char* pPathCopy = static_cast<char*>(MemoryManager::Instance().Allocate(Size));
        strcpy_s(pPathCopy, Size, apPath);
        if (bAddSlash) {
            pPathCopy[uiLength] = 92;
            pPathCopy[uiLength + 1] = 0;
        }
        kPaths.Add(pPathCopy);
    }
}

int32_t FileFinder::LookForFile(char* Str1, uint32_t aiFlags) {
    return LookForFile(Str1, aiFlags, ARCHIVE_TYPE_ALL);
}

// The same lookup as the four-argument GetFile, for every archive type.
NiFile* FileFinder::GetFile(const char* apName, NiFile::OpenMode aeMode, uint32_t aiSize) {
    return FileFinder::GetFile(apName, aeMode, aiSize, ARCHIVE_TYPE_ALL);
}

// A file to write is always a new loose BSFile; otherwise the finder locates the name: an archived file (2) comes from
// the archives, a loose one is a new BSFile opened through its Open function.
NiFile* FileFinder::GetFile(const char* apName, NiFile::OpenMode aeMode, uint32_t aiSize, uint32_t aiArchiveType) {
    char acFilePath[0x310];
    BSFile* pFile = nullptr;
    if (!FileFinder::pFinder) {
        return pFile;
    }
    if (aeMode == NiFile::WRITE_ONLY) {
        pFile = new BSFile(apName, aeMode, aiSize, false);
        return pFile;
    }
    int32_t iFound = FileFinder::pFinder->Exist(apName, acFilePath, SKIP_NONE, aiArchiveType);
    if (iFound) {
        // pFile is still null here.
        if (iFound == 2) {
            pFile = ArchiveManager::GetFile(acFilePath, aiSize, static_cast<ARCHIVE_TYPE>(aiArchiveType));
        }
        else {
            pFile = new BSFile(acFilePath, aeMode, aiSize, false);
            pFile->Open(false, false);
        }
    }
    return pFile;
}

// apBase's directory without its trailing backslash; each leading "..\" of apAddition drops the directory's last
// component. When no backslash is left, the directory pointer is set to an empty string.
void FileFinder::CombineFileNames(const char* apBase, const char* apAddition, char* apOutput) {
    char acDir[260];
    *apOutput = 0;
    _splitpath_s(apBase, nullptr, 0, acDir, 260, nullptr, 0, nullptr, 0);
    char* pcDir = acDir;
    size_t uiLength = std::strlen(pcDir);
    if (pcDir[uiLength - 1] == '\\') {
        pcDir[uiLength - 1] = 0;
    }
    bool bParent = false;
    while (!std::strncmp(apAddition, "..\\", 3)) {
        bParent = true;
        char* pcSeparator = pcDir;
        uiLength = std::strlen(pcDir);
        for (int32_t i = uiLength - 1; i > 0; --i) {
            if (pcDir[i] == '\\') {
                pcSeparator = &pcDir[i];
                break;
            }
        }
        if (pcSeparator) {
            *pcSeparator = 0;
            apAddition += 3;
        }
        else {
            pcDir = const_cast<char*>("");
        }
    }
    if (bParent) {
        strcat_s(apOutput, 260, pcDir);
        strcat_s(apOutput, 260, "\\");
        strcat_s(apOutput, 260, apAddition);
    }
    else {
        strcpy_s(apOutput, 260, apAddition);
    }
}

FileFinder::FILE_LOCATION
FileFinder::Locate(const char* apName, char* apFilePath, uint32_t aiFlags, uint32_t aiArchiveType) {
    if (FileFinder::pFinder) {
        return FileFinder::pFinder->Exist(apName, apFilePath, aiFlags, aiArchiveType);
    }

    return NOTFOUND;
}

// A name without wildcards is looked up alone; otherwise the loose files (unless only the archives count) and the
// archives' files matching it are added to the list, which is freed and null when nothing matched.
BSSimpleList<const char*>* FileFinder::BuildFileList(
    const char* apSearchName,
    const char* apBaseFilename,
    uint32_t aiArchiveType,
    BSSimpleList<const char*>* apFileList
) {
    if (!apSearchName || !apBaseFilename) {
        return nullptr;
    }
    if (!std::strstr(apSearchName, "*") && !std::strstr(apSearchName, "?")) {
        return FileFinder::BuildSingleFileList(apSearchName, apBaseFilename, aiArchiveType, apFileList);
    }
    BSSimpleList<const char*>* plist = apFileList;
    if (!apFileList) {
        BSSimpleList<const char*>* pNewList;
        BSSimpleList<const char*>* pAllocated = static_cast<BSSimpleList<const char*>*>(
            MemoryManager::Instance().Allocate(sizeof(BSSimpleList<const char*>))
        );
        if (pAllocated) {
            pAllocated->m_item = 0;
            pAllocated->m_pkNext = nullptr;
            pNewList = pAllocated;
        }
        else {
            pNewList = nullptr;
        }
        plist = pNewList;
    }
    if (!ArchiveManager::bUseArchives || ArchiveManager::bInvalidateOlderFiles) {
        FileFinder::PlatformBuildFileList(apSearchName, apBaseFilename, plist);
    }
    // aiArchiveType is an integer; ArchiveManager takes the enum.
    ArchiveManager::AddToFileList(plist, apSearchName, apBaseFilename, static_cast<ARCHIVE_TYPE>(aiArchiveType));
    if (!plist->GetNext() && !plist->m_item) {
        if (plist) {
            plist->RemoveAll();
            MemoryManager::Instance().Deallocate(plist);
        }
        return nullptr;
    }
    return plist;
}

FileFinder* FileFinder::GetSingleton() {
    return FileFinder::pFinder;
}

void FileFinder::operator delete(void* apMemory) {
    ourFree(apMemory);
}

bool FileFinder::AccessFile(const char* apName, NiFile::OpenMode aeMode) {
    if (aeMode) {
        NiFile tempFile(apName, aeMode, 0u);
        bool bOpened = tempFile;
        return bOpened;
    }

    int32_t iExists;
    if (FileFinder::pFinder) {
        iExists = FileFinder::pFinder->Exist(apName, nullptr, 0, 0xFFFFFFFFu);
    }
    else {
        iExists = 0;
    }
    return iExists != 0;
}

BSSimpleList<const char*>* FileFinder::BuildFileList_(const char* apSearchName, const char* apBaseFilename) {
    return FileFinder::BuildFileList(apSearchName, apBaseFilename, 0xFFFFu, nullptr);
}

BSSimpleList<const char*>* FileFinder::BuildSingleFileList(
    const char* apSearchName,
    const char* apBaseFilename,
    BSSimpleList<const char*>* apFileList
) {
    return FileFinder::BuildFileList(apSearchName, apBaseFilename, ARCHIVE_TYPE_ALL, apFileList);
}

// Adds the base file name's directory joined with the search name's file name to the list (a new one when it is
// null) when the file exists; the list is freed and null when it stays empty.
BSSimpleList<const char*>* FileFinder::BuildSingleFileList(
    const char* apSearchName,
    const char* apBaseFilename,
    uint32_t aiArchiveType,
    BSSimpleList<const char*>* apFileList
) {
    BSSimpleList<const char*>* plist = apFileList;
    if (!apFileList) {
        BSSimpleList<const char*>* pNewList;
        BSSimpleList<const char*>* pAllocated = static_cast<BSSimpleList<const char*>*>(
            MemoryManager::Instance().Allocate(sizeof(BSSimpleList<const char*>))
        );
        if (pAllocated) {
            pAllocated->m_item = 0;
            pAllocated->m_pkNext = nullptr;
            pNewList = pAllocated;
        }
        else {
            pNewList = nullptr;
        }
        plist = pNewList;
    }
    bool bfound = false;
    if (!ArchiveManager::bUseArchives || ArchiveManager::bInvalidateOlderFiles) {
        bfound = FileFinder::PlatformFindSingleFile(apSearchName);
    }
    if (!bfound) {
        // aiArchiveType is an integer; ArchiveManager takes the enum.
        bfound = ArchiveManager::FindFile(apSearchName, static_cast<ARCHIVE_TYPE>(aiArchiveType));
    }
    if (bfound) {
        size_t ibaseLength = std::strlen(apBaseFilename);
        const char* pbaseSlash = std::strrchr(apBaseFilename, '\\');
        const char* psearchSlash = std::strrchr(apSearchName, '\\');
        if (pbaseSlash && psearchSlash) {
            ibaseLength -= std::strlen(pbaseSlash + 1);
            size_t ilen = ibaseLength + std::strlen(psearchSlash + 1) + 1;
            char* pfound = static_cast<char*>(MemoryManager::Instance().Allocate(ilen));
            rt::memcpy(pfound, apBaseFilename, ibaseLength);
            pfound[ibaseLength] = 0;
            strcat_s(pfound, ilen, psearchSlash + 1);
            const char* pcfound = pfound;
            plist->AddHead(pcfound);
        }
    }
    if (!plist->GetNext() && !plist->m_item) {
        if (plist) {
            plist->RemoveAll();
            MemoryManager::Instance().Deallocate(plist);
        }
        return nullptr;
    }
    return plist;
}

char FileFinder::GetFileTime(const char* apFileName, _FILETIME& arFileTime) {
    // GetLastWriteTimeForFile takes char* for the name it only reads.
    return ArchiveManager::GetLastWriteTimeForFile(const_cast<char*>(apFileName), &arFileTime, ARCHIVE_TYPE_ALL);
}
