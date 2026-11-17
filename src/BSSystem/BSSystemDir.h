#pragma once

#include "BSSystem/BSSystemFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"
#include "platform/Paths.h"

#include <vector>

// A walk over the entries of a directory: platform::FindFiles once, then one entry per NextEntry. The error codes
// seen in the exe are 0 (none), 2 (null path) and 4 (the search failed). The constructor and destructor are not
// declared.
class BSSystemDir {
public:
	// The local the exe constructs in place (a plain local, reconstructed later): the reconstruct runs the destructor
	// first, which frees Handle_'s entries, so it starts with none.
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
	// Not named in the PDB or IDA; name chosen here. Returns this (left in eax).
	BSSystemDir* ClearFindAttributeByte();

	// Creates the directory; 0 when it was made, 4 when it was not.
	static BSSystemFile::ErrorCode MakeDir(const char* apcPath);

	std::vector<platform::FileEntry>* Handle_;
	_WIN32_FIND_DATAA FindData_;
	char DirPath_[260];
	BSSystemFile::ErrorCode eLastError;
	// The PC build's padding before the 8-byte aligned entry position (IDA's dword250).
	uint8_t padding24C[4];
	// The index in Handle_ of the entry FindData_ holds.
	uint64_t uiEntryPos;
};
