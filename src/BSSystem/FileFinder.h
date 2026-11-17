#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"

#include <stdint.h>

// Finds game files in the search paths, the working directory and the archives.
class FileFinder {
public:
    // Places a lookup skips (combined as bits).
    enum LOOKIN_FLAGS : uint32_t {
        SKIP_NONE = 0x0,
        SKIP_ARCHIVE = 0x1,
        SKIP_CWD = 0x2,
        SKIP_PATHS = 0x4,
    };

    // Where a lookup found a file (LookForFile's, Exist's and Locate's result).
    enum FILE_LOCATION : int32_t {
        NOTFOUND = 0,
        HARDDRIVE = 1,
        ARCHIVE = 2,
        CACHE = 3,
        PC = 4,
        DVD = 5,
        DVDTOHD = 6,
    };

    FileFinder(const char* apBasePath, int32_t aiFileLogging, bool abUseDDXTextures);
    ~FileFinder();

    // Where apFilePath was found: ARCHIVE, HARDDRIVE in the working directory or an alternate subdirectory, or
    // NOTFOUND; a found path is copied back over apFilePath.
    FILE_LOCATION LookForFile(char* apFilePath, uint32_t aiFlags, uint32_t aiArchiveType);
    // Where apName was found (copying the found path to apFilePath when given); NOTFOUND when it was not.
    FILE_LOCATION Exist(const char* apName, char* apFilePath, uint32_t aiFlags, uint32_t aiArchiveType);
    void AddPath(const char* apPath);

protected:
    bool SearchCWD(int32_t aiFlags, int32_t archiveType);
    bool SearchAlternateSubdirectories(int32_t aiFlags, int32_t archiveType);

public:
    int32_t LookForFile(char* Str1, uint32_t a2);

    // The same lookup as the four-argument GetFile, for every archive type.
    static NiFile* GetFile(const char* apName, NiFile::OpenMode aeMode, uint32_t aiSize);
    // Opens apName from the search paths or the archives of aiArchiveType with an aiSize-byte buffer; null when it
    // was not found.
    static NiFile* GetFile(const char* apName, NiFile::OpenMode aeMode, uint32_t aiSize, uint32_t aiArchiveType);
    // Joins apAddition onto apBase's directory into apOutput.
    static void CombineFileNames(const char* apBase, const char* apAddition, char* apOutput);
    // Where apName was found through the engine's finder (copying the found path to apFilePath); NOTFOUND when it
    // was not.
    static FILE_LOCATION Locate(const char* apName, char* apFilePath, uint32_t aiFlags, uint32_t aiArchiveType);
    // Adds the names of the files matching apSearchName in apBaseFilename's directory to apFileList (a new list
    // when it is null), from the archives of aiArchiveType too; returns the list.
    static BSSimpleList<const char*>* BuildFileList(
        const char* apSearchName,
        const char* apBaseFilename,
        uint32_t aiArchiveType,
        BSSimpleList<const char*>* apFileList
    );
    static FileFinder* GetSingleton();
    static void
    PlatformBuildFileList(const char* apSearchName, const char* apBaseFilename, BSSimpleList<char const*>* apFileList);
    static void AddPath_0(const char* apPath);
    static void Close();
    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);
    static bool AccessFile(const char* apName, NiFile::OpenMode aeMode);
    // Two-argument BuildFileList: BuildFileList over every archive type into a new list.
    static BSSimpleList<const char*>* BuildFileList_(const char* apSearchName, const char* apBaseFilename);
    static BSSimpleList<const char*>*
    BuildSingleFileList(const char* apSearchName, const char* apBaseFilename, BSSimpleList<const char*>* apFileList);
    static BSSimpleList<const char*>* BuildSingleFileList(
        const char* apSearchName,
        const char* apBaseFilename,
        uint32_t aiArchiveType,
        BSSimpleList<const char*>* apFileList
    );
    static char GetFileTime(const char* apFileName, _FILETIME& arFileTime);
    static bool PlatformFindSingleFile(const char* apSearchName);
    static bool PlatformGetFileTime(const char* apFileName, _FILETIME& arFileTime);

    // The search paths, tried in order by LookForFile.
    BSSimpleArray<const char*> kPaths;

    // The archive type that matches every archive (-1, as callers pass it), unlike ARCHIVE_TYPE_ALL (0xFFFF).
    static constexpr int32_t ARCHIVE_TYPE_ANY = -1;

    // The engine's file finder.
    static inline FileFinder* pFinder;
    static inline bool bUseDDXTextures;
};
