#include "BSSystem/BSSystemFile.h"

#include "BSCore/BSCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/Paths.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/Windows/winnt.h"

#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <stdint.h>
#include <system_error>

BSSystemFile::BSSystemFile(const char* apName, uint32_t aeAccessMode, uint32_t aeOpenMode, bool abAllowAsync)
	: eLastError(EC_NONE), eAccessMode(static_cast<AccessMode>(aeAccessMode)),
	  eOpenMode(static_cast<OpenMode>(aeOpenMode)), bAllowAsync(abAllowAsync) {
	eLastError = DoOpen(apName);
}

BSSystemFile::~BSSystemFile() {
	DoClose();
}

// The size is the length from the end of the stream, with the position put back; 0 and EC_FAILED when there is no
// stream or the stream cannot seek. std::fseek and std::ftell take a long, 32 bits on Win32: the game's files are below
// 2 GB.
BSSystemFile::ErrorCode BSSystemFile::DoGetSize(uint64_t& auiSize) {
	if (hFile) {
		const long iPos = std::ftell(hFile);
		if (iPos >= 0 && !std::fseek(hFile, 0, SEEK_END)) {
			const long iSize = std::ftell(hFile);
			if (!std::fseek(hFile, iPos, SEEK_SET) && iSize >= 0) {
				auiSize = static_cast<uint64_t>(iSize);
				return BSSystemFile::EC_NONE;
			}
		}
	}
	auiSize = 0;
	return BSSystemFile::EC_FAILED;
}

BSSystemFile::ErrorCode BSSystemFile::GetSize(uint64_t& auiSize) {
	eLastError = static_cast<BSSystemFile::ErrorCode>((int32_t)DoGetSize(auiSize));
	return static_cast<BSSystemFile::ErrorCode>((int32_t)eLastError);
}

// Writes the low 32 bits of auiBytes; the count written goes to auiWrite. Without a stream, or on a file opened without
// GENERIC_WRITE (AM_RDONLY), the write fails: EC_FAILED. The seek to the current position lets a write follow a read,
// which a Win32 handle allows and C requires a seek for.
BSSystemFile::ErrorCode BSSystemFile::DoWrite(const void* apvBuffer, uint64_t auiBytes, uint64_t& auiWrite) {
	if (bAllowAsync) {
		return BSSystemFile::EC_FAILED;
	}
	if (!hFile || (eAccessMode != BSSystemFile::AM_RDWR && eAccessMode != BSSystemFile::AM_WRONLY)) {
		return BSSystemFile::EC_FAILED;
	}
	std::fseek(hFile, 0, SEEK_CUR);
	errno = 0;
	const size_t uiWritten = std::fwrite(apvBuffer, 1, static_cast<uint32_t>(auiBytes), hFile);
	if (uiWritten < static_cast<uint32_t>(auiBytes)) {
		return ErrorCodeOf(errno);
	}
	auiWrite = static_cast<uint64_t>(uiWritten);
	return BSSystemFile::EC_NONE;
}

// ReadFile of the low 32 bits of auiBytes; the count read goes to auiRead, short at the end of the file (ReadFile
// succeeds there). Without a stream, or on a file opened without GENERIC_READ (AM_WRONLY), ReadFile fails with
// ERROR_INVALID_HANDLE or ERROR_ACCESS_DENIED: EC_FAILED. The seek to the current position lets a read follow a write
// and clears the stream's end-of-file flag, which a Win32 handle does not have.
BSSystemFile::ErrorCode BSSystemFile::DoRead(void* apvBuffer, uint64_t auiBytes, uint64_t& auiRead) {
	if (bAllowAsync) {
		return BSSystemFile::EC_FAILED;
	}
	if (!hFile || (eAccessMode != BSSystemFile::AM_RDONLY && eAccessMode != BSSystemFile::AM_RDWR)) {
		return BSSystemFile::EC_FAILED;
	}
	std::fseek(hFile, 0, SEEK_CUR);
	errno = 0;
	const size_t uiRead = std::fread(apvBuffer, 1, static_cast<uint32_t>(auiBytes), hFile);
	if (uiRead < static_cast<uint32_t>(auiBytes) && std::ferror(hFile)) {
		return ErrorCodeOf(errno);
	}
	auiRead = static_cast<uint64_t>(uiRead);
	return BSSystemFile::EC_NONE;
}

BSSystemFile::ErrorCode BSSystemFile::Write(const void* apvBuffer, uint64_t auiBytes, uint64_t& auiWrite) {
	eLastError = static_cast<BSSystemFile::ErrorCode>((int32_t)DoWrite(apvBuffer, auiBytes, auiWrite));
	return static_cast<BSSystemFile::ErrorCode>((int32_t)eLastError);
}

// CloseHandle; the stream is cleared after it, since a second std::fclose (the destructor after an explicit DoClose)
// is undefined where a second CloseHandle only fails.
void BSSystemFile::DoClose() {
	if (hFile) {
		std::fclose(hFile);
		hFile = nullptr;
	}
}

// DeleteFileA: EC_FAILED when the file is missing, is a folder or is read-only (all of which DeleteFileA refuses).
BSSystemFile::ErrorCode BSSystemFile::DeleteFileA(const char* apcPath) {
	std::error_code kError;
	const std::filesystem::path kPath = platform::NativePath(apcPath);
	const std::filesystem::file_status kStatus = std::filesystem::status(kPath, kError);
	if (std::filesystem::is_directory(kStatus) ||
		(kStatus.permissions() & std::filesystem::perms::owner_write) == std::filesystem::perms::none) {
		return BSSystemFile::EC_FAILED;
	}
	return std::filesystem::remove(kPath, kError) ? BSSystemFile::EC_NONE : BSSystemFile::EC_FAILED;
}

BSSystemFile::ErrorCode BSSystemFile::ErrorCodeOf(int32_t aiErrno) {
	switch (aiErrno) {
	case 0:
		return BSSystemFile::EC_NONE;
	case ENOENT:
		return BSSystemFile::EC_NOT_EXIST;
	case ENOTDIR:
		return BSSystemFile::EC_INVALID_PATH;
	case EROFS:
		return BSSystemFile::EC_NO_PERMISSION;
	default:
		return BSSystemFile::EC_FAILED;
	}
}

spErrorCallback_t SetFileCallback(spErrorCallback_t a1) {
	spErrorCallback = a1;
	return a1;
}

// Calls spErrorCallback under a lock built on first use; true when no callback is set. The lock is a function-local
// BSCriticalSection, like the same-shaped locks of MemoryPool::FreeUnusedPagesForAllPools and CheckPools.
char InvokeFileCallback(uint32_t a1, uint32_t a2, uint32_t a3) {
	char cResult;
	static BSCriticalSection kLock;
	EnterCriticalSection(&kLock.CriticalSection);
	if (spErrorCallback) {
		// Only the low byte of the callback's result is kept.
		cResult = static_cast<char>(spErrorCallback(a1, a2, a3));
	} else {
		cResult = 1;
	}
	LeaveCriticalSection(&kLock.CriticalSection);
	return cResult;
}
