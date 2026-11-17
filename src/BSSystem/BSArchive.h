#pragma once

#include "BSSystem/BSHash.h"
#include "Reconstruction.hpp"

// The archive's flag word, as read from the BSA header.
class ArchiveFlag {
public:
	void Endian();
	// The exe's copy is the shared getter BSArchive::GetFlag (0x8C1BE0), registered there: iFlag & aiFlag
	// (thiscall, ret 4).
	uint32_t CheckFlag(uint32_t aiFlag);

	uint32_t iFlag;
};

// The header at the start of a BSA file.
class BSArchiveHeader {
public:
	BSArchiveHeader();

	void Endian();

	uint32_t iTag;
	uint32_t iVersion;
	uint32_t iHeaderSize;
	ArchiveFlag oFlags;
	uint32_t iDirectories;
	uint32_t iFiles;
	uint32_t iDirectoryNamesLength;
	uint32_t iFileNamesLength;
	// The ARCHIVE_TYPE bits of the file groups the archive holds.
	uint16_t iArchiveType;
};

// A file's entry in an archive directory: the name hash, then the size and offset of its data.
class BSFileEntry : public BSHash {
public:
	uint32_t GetEntryOffset();
	// iSize without its flag bits (thiscall, ret 0; defined in ModelLoader.cpp, the PDB's file).
	uint32_t Size();
	void Endian();

	uint32_t iSize;
	uint32_t iOffset;
};

// A directory of an archive: the directory hash and its file entries.
class BSDirectoryEntry : public BSHash {
public:
	// Whether apFileEntry lies in this directory's file entries.
	bool IsFileEntryInDirectory(BSFileEntry* apFileEntry);
	void Endian();

	uint32_t iFiles;
	BSFileEntry* pFiles;
};

// The directory table of a BSA archive.
class BSArchive : public BSArchiveHeader {
public:
	// The bits of the header's oFlags.
	enum ARCHIVE_FLAGS : int32_t {
		ARCHIVE_DIRSTRINGS = 0x1,
		ARCHIVE_FILESTRINGS = 0x2,
		ARCHIVE_COMPRESSED = 0x4,
		ARCHIVE_RETAIN_DIRECTORY_NAMES = 0x8,
		ARCHIVE_RETAIN_FILE_NAMES = 0x10,
		ARCHIVE_RETAIN_FILE_NAME_OFFSETS = 0x20,
		ARCHIVE_XBOX_ARCHIVE = 0x40,
		ARCHIVE_RETAIN_STRINGS_DURING_STARTUP = 0x80,
		ARCHIVE_EMBEDDED_FILE_NAMES = 0x100,
		ARCHIVE_XBOX_COMPRESSED = 0x200,
		ARCHIVE_NEXT_FLAG = 0x400,
	};

	// BSArchiveHeader(), then no directory table yet.
	BSArchive() : pDirectories(nullptr) {}

	// Whether apFileEntry lies in this archive's file entries.
	bool IsFileEntryInArchive(BSFileEntry* apFileEntry);
	int32_t GetFlag(int32_t a2);
	// oFlags.CheckFlag(aeFlag).
	int32_t GetFlag_(ARCHIVE_FLAGS aeFlag);
	int32_t HasEmbeddedFileNames();

	BSDirectoryEntry* pDirectories;
};
