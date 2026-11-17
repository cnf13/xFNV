#include "Havok/Source/Common/Base/System/Io/FileSystem/hkNativeFileSystem.h"

#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/System/Io/Directory/Win32/hkWin32ListDirectory.h"

listDirectoryFunc_t hkNativeFileSystem::s_listDirectory{&hkWin32ListDirectory};
havokToPlatformConvert_t s_havokToPlatformConvert{&hkNativeFileSystem::nativeHavokToPlatformConvertPath};

// The file system adds nothing to destroy.
hkNativeFileSystem::~hkNativeFileSystem() {}

const char* hkNativeFileSystem::nativeHavokToPlatformConvertPath(const char* pathIn, hkStringBuf& buffer) {
	buffer.operator=(pathIn);
	buffer.replace('/', '\\', hkStringBuf::REPLACE_ALL);
	return buffer.m_string.m_data;
}

// Stores the path in the buffer, rewrites its separators as slashes, collapses "/./" and drops a leading "./", then
// returns the buffer's characters; the replacement's results are deliberately ignored, as the game does.
const char* hkNativeFileSystem::nativePlatformToHavokConvertPath(const char* pathIn, hkStringBuf& buffer) {
	buffer.operator=(pathIn);
	buffer.replace('\\', '/', hkStringBuf::REPLACE_ALL);
	replace(&buffer, "/./", "/", 1);
	if (buffer.startsWith("./") != 0) {
		buffer.chompStart(2);
	}
	return buffer.m_string.m_data;
}
