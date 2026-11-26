#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

class hkStreamReader;
class hkStreamWriter;

// Havok's way to open files for reading and writing.
class hkFileSystem : public hkReferencedObject, public hkSingleton<hkFileSystem> {
public:
    // The standard streams an hkStdioStreamWriter can write to instead of a file.
    enum StdStream : int32_t {
        STDOUT = 1,
        STDERR = 2,
    };

    // The file/directory bit values an Entry carries.
    enum FlagValues { F_ISFILE = 1, F_ISDIR = 2 };

    // One directory entry, as a listDirectory call appends it (12 bytes).
    struct Entry {
        // The SDK's: its arrays treat it as plain data (hkTypeTraits.h).
        HK_DECLARE_POD_TYPE();

        const char* name;
        uint32_t timeModified;
        hkFlags<FlagValues, uint32_t> flags;
    };

    // The directory contents a listDirectory call fills. The entries come first and the allocator they come from sits
    // behind them, unlike the Havok SDK header, which has the allocator first.
    struct DirectoryListing {
        hkArray<Entry> m_entries;
        hkMemoryAllocator* m_alloc;
    };

    hkFileSystem();
    ~hkFileSystem() override;

    // Opens a file for reading; null if it cannot be opened.
    virtual hkStreamReader* openReader(const char* apName) = 0;
    // Opens a file for writing; null if it cannot be opened.
    virtual hkStreamWriter* openWriter(const char* apName) = 0;
    // The base fails (returns HK_FAILURE).
    virtual hkResult listDirectory(const char* basePath, hkFileSystem::DirectoryListing& listingOut);

    // hkSingleton<hkFileSystem>::s_instance.
    static inline hkFileSystem* s_instance;
};

inline hkFileSystem::hkFileSystem() {}

inline hkResult hkFileSystem::listDirectory(const char* basePath, hkFileSystem::DirectoryListing& listingOut) {
    return HK_FAILURE;
}
