#pragma once

#include "BSSystem/BSSystemFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/Paths.h"

#include <vector>

// A walk over the entries of a directory: platform::FindFiles once, then one entry per NextEntry. The error codes are 0
// (none), 2 (null path) and 4 (the search failed).
class BSSystemDir {
public:
    BSSystemDir() : Handle_(nullptr), eLastError(BSSystemFile::EC_NONE), uiEntryPos(0) {}
    BSSystemDir(const char* apcPath);
    ~BSSystemDir();

    // Moves to the next entry (the first one on the first call); abOutDone is set once there is none left.
    BSSystemFile::ErrorCode NextEntry(bool& abIsComplete);
    bool QRegular();
    bool QDirectory();
    int32_t IsDirectory();

protected:
    // Keeps the path with "\*" appended and clears the handle; 2 for a null path.
    BSSystemFile::ErrorCode DoOpen(const char* apcPath);
    // Closes the handle unless it is the invalid one; the last error code.
    BSSystemFile::ErrorCode DoClose();

public:
    // Returns this.
    BSSystemDir* ClearFindAttributeByte();

    // Creates the directory; 0 when it was made, 4 when it was not.
    static BSSystemFile::ErrorCode MakeDir(const char* apcPath);

    std::vector<platform::FileEntry>* Handle_;
    _WIN32_FIND_DATAA FindData_;
    char DirPath_[260];
    BSSystemFile::ErrorCode eLastError;
    // Padding before the 8-byte aligned entry position.
    uint8_t padding24C[4];
    // The index in Handle_ of the entry FindData_ holds.
    uint64_t uiEntryPos;
};
