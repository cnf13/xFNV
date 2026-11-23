#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"
#include "Havok/Source/Common/Base/System/Io/Reader/Buffered/hkBufferedStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Reader/Stdio/hkStdioStreamReader.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

// The function the file system uses to turn a Havok path into the platform's form, called with the path and the buffer
// it fills. Its callee is installed at startup.
using havokToPlatformConvert_t = const char* (*)(const char* apPath, hkStringBuf& arBuffer);
extern havokToPlatformConvert_t s_havokToPlatformConvert;

// The function listDirectory hands the path and the listing to (the SDK's ListDirectoryFunType).
using listDirectoryFunc_t = hkResult (*)(const char* apBasePath, hkFileSystem::DirectoryListing& arListingOut);

// Implements Havok file access through the host filesystem.
class hkNativeFileSystem : public hkFileSystem {
public:
    ~hkNativeFileSystem() override;

    // Opens the file through the platform's path, behind a buffered reader when the file reader cannot mark.
    hkStreamReader* openReader(const char* name) override {
        hkStringBuf buffer;
        hkStreamReader* s = new hkStdioStreamReader(s_havokToPlatformConvert(name, buffer));
        hkStreamReader* pResult = s;
        if (!s->markSupported().m_bool) {
            pResult = new hkBufferedStreamReader(s, 0x1000);
            s->removeReference();
        }
        return pResult;
    }

    hkStreamWriter* openWriter(const char* name) override;

    hkResult listDirectory(const char* basePath, hkFileSystem::DirectoryListing& listingOut) override {
        return s_listDirectory(basePath, listingOut);
    }

    // Stores the path in the buffer, rewrites its separators as backslashes and returns the buffer's characters.
    static const char* nativeHavokToPlatformConvertPath(const char* pathIn, hkStringBuf& buffer);
    // Copies the path into the buffer, makes its separators slashes, collapses "/./" and drops a leading "./", then
    // returns the buffer's characters.
    static const char* nativePlatformToHavokConvertPath(const char* pathIn, hkStringBuf& buffer);

    // The platform's directory listing, installed at startup.
    static listDirectoryFunc_t s_listDirectory;
};
