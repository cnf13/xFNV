#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstdio>

// The engine's raw OS file. Only some of its members are declared.
class BSSystemFile {
public:
	enum ErrorCode : int32_t {
		EC_NONE = 0,
		EC_NOT_EXIST = 1,
		EC_INVALID_PATH = 2,
		EC_NO_PERMISSION = 3,
		EC_FAILED = 4,
	};

	// How the file is opened for access (eAccessMode); DoOpen maps it to the stream's mode, and DoRead, DoWrite and
	// DoFlush refuse what CreateFileA access would refuse.
	enum AccessMode : uint32_t {
		AM_RDONLY = 0,
		AM_RDWR = 1,
		AM_WRONLY = 2,
	};

	// What opening does with an existing or missing file (eOpenMode); DoOpen maps it to the creation disposition.
	enum OpenMode : uint32_t {
		OM_NONE = 0,
		OM_APPEND = 1,
		OM_CREAT = 2,
		OM_TRUNC = 3,
		OM_EXCL = 4,
	};

	// Where a seek's offset counts from (DoSeek's aeWhence); its values equal SetFilePointerEx's FILE_BEGIN,
	// FILE_CURRENT and FILE_END move methods; DoSeek maps it to SEEK_SET, SEEK_CUR, SEEK_END.
	enum SeekMode : uint32_t {
		SM_SET = 0,
		SM_CUR = 1,
		SM_END = 2,
	};

	// A default-constructed BSSystemFile holds no stream: hFile is null, so the destructor, which closes hFile unless
	// it is null, closes nothing.
	BSSystemFile() : eLastError(EC_NONE), hFile(nullptr) {}
	// Stores the modes and opens apName, keeping the open's error.
	BSSystemFile(const char* apName, uint32_t aeAccessMode, uint32_t aeOpenMode, bool abAllowAsync);
	~BSSystemFile();

	ErrorCode DoGetSize(uint64_t& auiSize);
	ErrorCode GetSize(uint64_t& auiSize);
	ErrorCode DoWrite(const void* apvBuffer, uint64_t auiBytes, uint64_t& auiWrite);
	ErrorCode DoRead(void* apvBuffer, uint64_t auiBytes, uint64_t& auiRead);
	// Reads through DoRead and keeps the result as the last error.
	ErrorCode Read(void* apvBuffer, uint64_t auiBytes, uint64_t& auiRead);
	// Writes auiBytes of apvBuffer; the count written goes to apuiWritten.
	ErrorCode Write(const void* apvBuffer, uint64_t auiBytes, uint64_t& auiWrite);
	// Seeks to auiOffset from aeWhence; the new position goes to aruiPos.
	ErrorCode DoSeek(uint64_t auiOffset, SeekMode aeWhence, uint64_t& auiPos);
	// Seeks through DoSeek and keeps the result as the last error.
	ErrorCode Seek(uint64_t auiOffset, SeekMode aeWhence, uint64_t& auiPos);
	void DoClose();
	// Returns eLastError.
	ErrorCode QLastError() const;
	BSSystemFile::ErrorCode DoFlush();
	BSSystemFile::ErrorCode DoOpen(const char* apcPath);

	// Deletes the file at apcPath: EC_NONE, or EC_FAILED when the OS refuses.
	static ErrorCode DeleteFileA(const char* apcPath);
	static BSSystemFile::ErrorCode GetSizeOfFile(const char* apcPath, uint64_t& auiSize);
	// The ErrorCode for the errno of a failed stream call, following the game's GetLastError mapping: 0 is EC_NONE,
	// ERROR_FILE_NOT_FOUND (ENOENT) EC_NOT_EXIST, ERROR_PATH_NOT_FOUND (ENOTDIR) EC_INVALID_PATH, ERROR_WRITE_PROTECT
	// (EROFS) EC_NO_PERMISSION, anything else EC_FAILED. Shared by DoOpen, DoRead, DoWrite and DoSeek; DoOpen tells a
	// missing folder (ENOENT, ERROR_PATH_NOT_FOUND) from a missing file itself.
	static ErrorCode ErrorCodeOf(int32_t aiErrno);
	static int32_t CopyFileA(const int8_t* lpExistingFileName, const int8_t* lpNewFileName, bool a3);

	ErrorCode eLastError;
	AccessMode eAccessMode;
	OpenMode eOpenMode;
	bool bAllowAsync;
	std::FILE* hFile;
};

using spErrorCallback_t = int32_t (*)(uint32_t, uint32_t, uint32_t);

// Sets spErrorCallback and returns its argument.
spErrorCallback_t SetFileCallback(spErrorCallback_t a1);
// Calls spErrorCallback with the three arguments under a lock; true
// when no callback is set.
char InvokeFileCallback(uint32_t a1, uint32_t a2, uint32_t a3);

inline spErrorCallback_t spErrorCallback{};
