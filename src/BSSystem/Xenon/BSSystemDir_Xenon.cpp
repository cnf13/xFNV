#include "BSSystem/BSSystemDir.h"
#include "BSSystem/BSSystemFile.h"
#include "platform/Paths.h"

#include <filesystem>
#include <system_error>
#include <vector>

// The first call searches DirPath_ once and keeps the entries; each later call moves to the next one, setting abOutDone
// once none is left. A search that found nothing fails on every call. FindData_ holds the current entry.
BSSystemFile::ErrorCode BSSystemDir::NextEntry(bool& abIsComplete) {
	bool bFound = false;
	if (Handle_) {
		if (Handle_->empty()) {
			eLastError = BSSystemFile::EC_FAILED;
		} else {
			// FindNextFileA's failure is not an error: FindData_ keeps the last entry.
			abIsComplete = uiEntryPos + 1 >= Handle_->size();
			if (!abIsComplete) {
				++uiEntryPos;
				bFound = true;
			}
		}
	} else {
		Handle_ = new std::vector<platform::FileEntry>(platform::FindFiles(DirPath_));
		if (Handle_->empty()) {
			eLastError = BSSystemFile::EC_FAILED;
		} else {
			uiEntryPos = 0;
			bFound = true;
			eLastError = BSSystemFile::EC_NONE;
		}
	}
	if (bFound) {
		// Every field FindFirstFileA gives; the reserved words and the bytes after each name's NUL are zero.
		const platform::FileEntry& rEntry = (*Handle_)[uiEntryPos];
		FindData_ = {};
		FindData_.dwFileAttributes = rEntry.attributes;
		FindData_.ftCreationTime.dwLowDateTime = static_cast<uint32_t>(rEntry.creationTime);
		FindData_.ftCreationTime.dwHighDateTime = static_cast<uint32_t>(rEntry.creationTime >> 32);
		FindData_.ftLastAccessTime.dwLowDateTime = static_cast<uint32_t>(rEntry.accessTime);
		FindData_.ftLastAccessTime.dwHighDateTime = static_cast<uint32_t>(rEntry.accessTime >> 32);
		FindData_.ftLastWriteTime.dwLowDateTime = static_cast<uint32_t>(rEntry.writeTime);
		FindData_.ftLastWriteTime.dwHighDateTime = static_cast<uint32_t>(rEntry.writeTime >> 32);
		FindData_.nFileSizeHigh = static_cast<uint32_t>(rEntry.size >> 32);
		FindData_.nFileSizeLow = static_cast<uint32_t>(rEntry.size);
		strcpy_s(FindData_.cFileName, 260, rEntry.name.c_str());
		strcpy_s(FindData_.cAlternateFileName, 14, rEntry.shortName.c_str());
	}
	return eLastError;
}

BSSystemFile::ErrorCode BSSystemDir::DoOpen(const char* apcPath) {
	if (!apcPath) {
		return BSSystemFile::EC_INVALID_PATH;
	}
	strcpy_s(DirPath_, 260, apcPath);
	strcat_s(DirPath_, 260, "\\*");
	Handle_ = nullptr;
	eLastError = BSSystemFile::EC_NONE;
	return eLastError;
}

// The entries are freed and Handle_ cleared, so a second close (the destructor after an explicit one) frees nothing.
BSSystemFile::ErrorCode BSSystemDir::DoClose() {
	delete Handle_;
	Handle_ = nullptr;
	return eLastError;
}

BSSystemDir* BSSystemDir::ClearFindAttributeByte() {
	FindData_.dwFileAttributes =
		static_cast<uint32_t>((static_cast<uint32_t>(FindData_.dwFileAttributes) & 0xFFFFFF00u) |
			(static_cast<uint32_t>(static_cast<uint8_t>(0))));
	return this;
}

// CreateDirectoryA: EC_FAILED when the folder exists already or cannot be made.
BSSystemFile::ErrorCode BSSystemDir::MakeDir(const char* apcPath) {
	std::error_code kError;
	return std::filesystem::create_directory(platform::NativePath(apcPath), kError) ? BSSystemFile::EC_NONE
																					: BSSystemFile::EC_FAILED;
}
