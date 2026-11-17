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
        MemoryManager_Singleton.Deallocate(const_cast<char*>(kPaths.pBuffer[auiIndex]));
    }
    if (FileFinder::pFinder == this) {
        FileFinder::pFinder = 0;
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
    char v10 = 0;
    int32_t result = 0;
    if (apName[1] == 58) {
        aiFlags |= 5u;
    }
    else {
        if (*apName == 46 && apName[1] == 92) {
            aiFlags |= 4u;
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
        size_t v4 = std::strlen(apPath);
        bool v3 = apPath[v4 - 1] != 92;
        size_t Size = std::strlen(apPath) + v3 + 1;
        char* a2 = static_cast<char*>(MemoryManager_Singleton.Allocate(Size));
        strcpy_s(a2, Size, apPath);
        if (v3) {
            a2[v4] = 92;
            a2[v4 + 1] = 0;
        }
        kPaths.Add(a2);
    }
}

int32_t FileFinder::LookForFile(char* Str1, uint32_t a2) {
    return LookForFile(Str1, a2, ARCHIVE_TYPE_ALL);
}

// The same lookup as the four-argument GetFile, for every archive type.
NiFile* FileFinder::GetFile(const char* apName, NiFile::OpenMode aeMode, uint32_t aiSize) {
    return FileFinder::GetFile(apName, aeMode, aiSize, ::ARCHIVE_TYPE_ALL);
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
        void* pMemory = MemoryManager_Singleton.Allocate(sizeof(BSFile));
        BSFile* pNew;
        if (pMemory) {
            pNew = ::new (pMemory) BSFile(apName, aeMode, aiSize, false);
        }
        else {
            pNew = nullptr;
        }
        pFile = pNew;
        return pFile;
    }
    int32_t iFound = FileFinder::pFinder->Exist(apName, acFilePath, SKIP_NONE, aiArchiveType);
    if (iFound) {
        // pFile is still null here.
        if (iFound == 2) {
            pFile = ArchiveManager::GetFile(acFilePath, aiSize, static_cast<ARCHIVE_TYPE>(aiArchiveType));
        }
        else {
            void* pMemory = MemoryManager_Singleton.Allocate(sizeof(BSFile));
            BSFile* pNew;
            if (pMemory) {
                pNew = ::new (pMemory) BSFile(acFilePath, aeMode, aiSize, false);
            }
            else {
                pNew = nullptr;
            }
            pFile = pNew;
            pFile->Open(0, false);
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
    else {
        return NOTFOUND;
    }
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
        return 0;
    }
    if (!std::strstr(apSearchName, "*") && !std::strstr(apSearchName, "?")) {
        return FileFinder::BuildSingleFileList(apSearchName, apBaseFilename, aiArchiveType, apFileList);
    }
    BSSimpleList<const char*>* plist = apFileList;
    if (!apFileList) {
        BSSimpleList<const char*>* v6;
        BSSimpleList<const char*>* v7 = static_cast<BSSimpleList<const char*>*>(
            MemoryManager_Singleton.Allocate(sizeof(BSSimpleList<const char*>))
        );
        if (v7) {
            v7->m_item = 0;
            v7->m_pkNext = 0;
            v6 = v7;
        }
        else {
            v6 = 0;
        }
        plist = v6;
    }
    if (!ArchiveManager::bUseArchives || ArchiveManager::bInvalidateOlderFiles) {
        FileFinder::PlatformBuildFileList(apSearchName, apBaseFilename, plist);
    }
    // aiArchiveType is an integer; ArchiveManager takes the enum.
    ArchiveManager::AddToFileList(plist, apSearchName, apBaseFilename, static_cast<ARCHIVE_TYPE>(aiArchiveType));
    if (!plist->GetNext() && !plist->m_item) {
        if (plist) {
            plist->RemoveAll();
            MemoryManager_Singleton.Deallocate(plist);
        }
        return 0;
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
        bool v4 = tempFile.operator bool();
        return v4;
    }
    else {
        int32_t v3;
        if (FileFinder::pFinder) {
            v3 = FileFinder::pFinder->Exist(apName, 0, 0, 0xFFFFFFFFu);
        }
        else {
            v3 = 0;
        }
        return v3 != 0;
    }
}

BSSimpleList<const char*>* FileFinder::BuildFileList_(const char* apSearchName, const char* apBaseFilename) {
    return FileFinder::BuildFileList(apSearchName, apBaseFilename, 0xFFFFu, 0);
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
        BSSimpleList<const char*>* v6;
        BSSimpleList<const char*>* v8 = static_cast<BSSimpleList<const char*>*>(
            MemoryManager_Singleton.Allocate(sizeof(BSSimpleList<const char*>))
        );
        if (v8) {
            v8->m_item = 0;
            v8->m_pkNext = 0;
            v6 = v8;
        }
        else {
            v6 = 0;
        }
        plist = v6;
    }
    bool bfound = 0;
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
            char* pfound = static_cast<char*>(MemoryManager_Singleton.Allocate(ilen));
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
            MemoryManager_Singleton.Deallocate(plist);
        }
        return 0;
    }
    return plist;
}

char FileFinder::GetFileTime(const char* apFileName, _FILETIME& arFileTime) {
    // GetLastWriteTimeForFile takes char* for the name it only reads.
    return ArchiveManager::GetLastWriteTimeForFile(const_cast<char*>(apFileName), &arFileTime, ARCHIVE_TYPE_ALL);
}
