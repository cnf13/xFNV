#pragma once

#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

// Lists a directory through the Win32 find API; hkNativeFileSystem::s_listDirectory's value.
hkResult hkWin32ListDirectory(const char* pathIn, hkFileSystem::DirectoryListing& directoryListingOut);
