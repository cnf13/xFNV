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

// Lists a directory, appending an entry per file and subdirectory; 0 when the path is a directory, 1 when it is not.
// Uses std::filesystem and platform::FindFiles, in the same order as the game's attribute check and find calls.
hkResult hkWin32ListDirectory(const char* pathIn, hkFileSystem::DirectoryListing& directoryListingOut) {
    hkStringBuf pathBuffer;
    hkStringBuf nameBuffer(pathIn);
    if (nameBuffer.m_string.getSize() - 1 > 0) {
        nameBuffer.replace('\\', '/', hkStringBuf::REPLACE_ALL);
        if (!nameBuffer.endsWith("/")) {
            nameBuffer += "/";
        }
        s_havokToPlatformConvert(nameBuffer.m_string.begin(), pathBuffer);
    }
    // GetFileAttributesA: missing (INVALID_FILE_ATTRIBUTES) or no FILE_ATTRIBUTE_DIRECTORY.
    std::error_code kError;
    if (!std::filesystem::is_directory(platform::NativePath(pathBuffer.m_string.begin()), kError)) {
        // Not a directory: report the failure as 1.
        return HK_FAILURE;
    }
    if (pathBuffer.m_string.getSize() != 1) {
        pathBuffer += "*";
    }
    const std::vector<platform::FileEntry> kFound = platform::FindFiles(pathBuffer.m_string.begin());
    for (const platform::FileEntry& rEntry : kFound) {
        if (hkString::strCmp(rEntry.name.c_str(), ".") == 0) {
            continue;
        }
        if (hkString::strCmp(rEntry.name.c_str(), "..") == 0) {
            continue;
        }
        if ((rEntry.attributes >> 4) & 1) {
            hkFileSystem::Entry* pEntry = &directoryListingOut.m_entries._expandOne(*directoryListingOut.m_alloc);
            pEntry->name = hkString::strDup(rEntry.name.c_str(), *directoryListingOut.m_alloc);
            pEntry->flags.m_storage = hkFileSystem::F_ISDIR;
            pEntry->timeModified = 0;
        }
        else {
            // The write time in seconds since 1970: the FILETIME's ticks less the 1601-to-1970 offset, divided by
            // 10,000,000, keeping the low 32 bits.
            int64_t iTicks = static_cast<int64_t>(rEntry.writeTime);
            uint32_t uiModified = static_cast<uint32_t>((iTicks - 116444736000000000LL) / 10000000);
            hkFileSystem::Entry* pEntry = &directoryListingOut.m_entries._expandOne(*directoryListingOut.m_alloc);
            pEntry->name = hkString::strDup(rEntry.name.c_str(), *directoryListingOut.m_alloc);
            pEntry->flags.m_storage = hkFileSystem::F_ISFILE;
            pEntry->timeModified = uiModified;
        }
    }
    return HK_SUCCESS;
}
