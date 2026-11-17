#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Io/Directory/Win32/hkWin32ListDirectory.h"
#include "Havok/Source/Common/Base/System/Io/FileSystem/hkNativeFileSystem.h"
#include "platform/Paths.h"

#include <filesystem>
#include <system_error>
#include <vector>

// Lists a directory, appending an entry per file and subdirectory; 0 when the path is a directory, 1 when it is not
// (cdecl, ret 0, 0xDA25A0). The exe asks GetFileAttributesA and the find API; here std::filesystem and
// platform::FindFiles, in the same order. Not named in the PDB; the Havok lib's symbol is
// ?hkWin32ListDirectory@@YA?AW4hkResult@@PBDAAUDirectoryListing@hkFileSystem@@@Z.
hkResult hkWin32ListDirectory(const char* pathIn, hkFileSystem::DirectoryListing& directoryListingOut) {
	FNVD_INLINED_LOCAL(hkStringBuf, pathBuffer, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(hkStringBuf, nameBuffer, std::nullopt, std::nullopt);

	::new (&pathBuffer) hkStringBuf;
	::new (&nameBuffer) hkStringBuf(pathIn);
	if (nameBuffer.m_string.getSize() - 1 > 0) {
		nameBuffer.replace('\\', '/', hkStringBuf::REPLACE_ALL);
		if (!nameBuffer.endsWith("/"))
			nameBuffer += "/";
		s_havokToPlatformConvert(nameBuffer.m_string.begin(), pathBuffer);
	}
	// hkStringBuf's destructor, which the exe inlines (0xCBBD40).
	hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
	nameBuffer.m_string.m_size = 0;
	if (nameBuffer.m_string.getCapacityAndFlags() >= 0)
		pTemp->bufFree(nameBuffer.m_string.begin(), nameBuffer.m_string.getCapacityAndFlags() & 0x3FFFFFFF);
	// GetFileAttributesA: missing (INVALID_FILE_ATTRIBUTES) or no FILE_ATTRIBUTE_DIRECTORY.
	std::error_code kError;
	if (!std::filesystem::is_directory(platform::NativePath(pathBuffer.m_string.begin()), kError)) {
		// Not a directory: drop the buffer and report the failure as 1.
		pTemp = hkMemoryRouter::getInstance().m_temp;
		pathBuffer.m_string.m_size = 0;
		if (pathBuffer.m_string.getCapacityAndFlags() >= 0)
			pTemp->bufFree(pathBuffer.m_string.begin(), pathBuffer.m_string.getCapacityAndFlags() & 0x3FFFFFFF);
		return HK_FAILURE;
	}
	if (pathBuffer.m_string.getSize() != 1)
		pathBuffer += "*";
	// The exe's FindFirstFileA and FindNextFileA, in their order.
	const std::vector<platform::FileEntry> kFound = platform::FindFiles(pathBuffer.m_string.begin());
	for (const platform::FileEntry& rEntry : kFound) {
		if (hkString::strCmp(rEntry.name.c_str(), ".") == 0)
			continue;
		if (hkString::strCmp(rEntry.name.c_str(), "..") == 0)
			continue;
		if ((rEntry.attributes >> 4) & 1) {
			if (directoryListingOut.m_entries.getSize() ==
				(directoryListingOut.m_entries.getCapacityAndFlags() & 0x3FFFFFFF))
				hkArrayUtil::_reserveMore(*directoryListingOut.m_alloc, &directoryListingOut.m_entries,
					sizeof(hkFileSystem::Entry));
			int32_t iIndex = directoryListingOut.m_entries.getSize();
			hkFileSystem::Entry* pEntry = directoryListingOut.m_entries.begin() + iIndex;
			directoryListingOut.m_entries.m_size = static_cast<uint32_t>(iIndex + 1);
			pEntry->name = hkString::strDup(rEntry.name.c_str(), *directoryListingOut.m_alloc);
			pEntry->flags.m_storage = hkFileSystem::F_ISDIR;
			pEntry->timeModified = 0;
		} else {
			// The write time in seconds since 1970: the FILETIME's ticks less the 1601-to-1970 offset, divided by
			// 10,000,000 (the exe divides with __alldiv and keeps the low word).
			int64_t iTicks = static_cast<int64_t>(rEntry.writeTime);
			uint32_t uiModified = static_cast<uint32_t>((iTicks - 116444736000000000LL) / 10000000);
			if (directoryListingOut.m_entries.getSize() ==
				(directoryListingOut.m_entries.getCapacityAndFlags() & 0x3FFFFFFF))
				hkArrayUtil::_reserveMore(*directoryListingOut.m_alloc, &directoryListingOut.m_entries,
					sizeof(hkFileSystem::Entry));
			int32_t iIndex = directoryListingOut.m_entries.getSize();
			hkFileSystem::Entry* pEntry = directoryListingOut.m_entries.begin() + iIndex;
			directoryListingOut.m_entries.m_size = static_cast<uint32_t>(iIndex + 1);
			pEntry->name = hkString::strDup(rEntry.name.c_str(), *directoryListingOut.m_alloc);
			pEntry->flags.m_storage = hkFileSystem::F_ISFILE;
			pEntry->timeModified = uiModified;
		}
	}
	pTemp = hkMemoryRouter::getInstance().m_temp;
	pathBuffer.m_string.m_size = 0;
	if (pathBuffer.m_string.getCapacityAndFlags() >= 0)
		pTemp->bufFree(pathBuffer.m_string.begin(), pathBuffer.m_string.getCapacityAndFlags() & 0x3FFFFFFF);
	return HK_SUCCESS;
}
