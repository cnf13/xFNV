#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/FileFinder.h"
#include "platform/Paths.h"
#include "SDK/MSVC/string.h"

#include <vector>

bool FileFinder::SearchCWD(int32_t aiFlags, int32_t archiveType) {
	return (aiFlags & 2) == 0;
}

bool FileFinder::SearchAlternateSubdirectories(int32_t aiFlags, int32_t archiveType) {
	return (aiFlags & 4) == 0;
}

// Adds a copy of apBaseFilename with its file name replaced by each match of apSearchName, in the order the matches are
// found.
void FileFinder::PlatformBuildFileList(const char* apSearchName, const char* apBaseFilename,
	BSSimpleList<const char*>* apFileList) {
	const std::vector<platform::FileEntry> kFound = platform::FindFiles(apSearchName);
	if (!kFound.empty()) {
		int32_t iPathSize = std::strlen(apBaseFilename);
		const char* pslash = std::strrchr(apBaseFilename, '\\');
		if (pslash) {
			iPathSize -= std::strlen(pslash + 1);
		}
		for (const platform::FileEntry& rEntry : kFound) {
			uint32_t isize = iPathSize + std::strlen(rEntry.name.c_str()) + 1;
			char* pfound = static_cast<char*>(MemoryManager_Singleton.Allocate(isize));
			rt::memcpy(pfound, apBaseFilename, iPathSize);
			pfound[iPathSize] = 0;
			char* pslash_2 = const_cast<char*>(std::strrchr(static_cast<const char*>(pfound), '\\'));
			if (pslash_2) {
				++pslash_2;
				strcpy_s(pslash_2, isize - (pslash_2 - pfound), rEntry.name.c_str());
				apFileList->AddHead(pfound);
			}
		}
	}
}

// FindFirstFileA: whether anything matches apSearchName.
bool FileFinder::PlatformFindSingleFile(const char* apSearchName) {
	return !platform::FindFiles(apSearchName).empty();
}

// FindFirstFileA: the first match's last write time; false when nothing matches.
bool FileFinder::PlatformGetFileTime(const char* apFileName, _FILETIME& arFileTime) {
	const std::vector<platform::FileEntry> kFound = platform::FindFiles(apFileName);
	if (kFound.empty()) {
        return false;
    }
	arFileTime.dwLowDateTime = static_cast<uint32_t>(kFound[0].writeTime);
	arFileTime.dwHighDateTime = static_cast<uint32_t>(kFound[0].writeTime >> 32);
    return true;
}
