#pragma once

#include "BSSystem/BSFileCache.h"

// The PC's file cache: its overrides are stubs (Init keeps the cache-drive flag, the rest do nothing or return 0).
class BSWin32FileCache : public BSFileCache {
public:
    BSWin32FileCache();
    ~BSWin32FileCache() override;

    void Init(bool abCacheDriveAvailable) override;
    void Shutdown() override;
    int32_t ParseCachedFileList(const char* apFileList, uint32_t aiCurrentPriority) override;
    void BeginFileCaching(const char* apcWarningMsg) override;
    int32_t GetCachedFileIndex(char* apFileName) override;
    void GetCachedFileName(char* apInputFileName, char* apCachedFileName) override;
    bool CacheFile(CachedFile* apCachedFile) override;
    void SetPriority(int32_t aiPriority) override;
    void CalculatePriority() override;
    void OutputCacheDriveListing(char* apDirectory) override;
    void WaitForFileCachingToFinish(
        uint32_t aiMaxWait,
        int32_t aiPriority,
        bool abUpdateLoadingBar,
        uint8_t cLoadingBarSection
    ) override;
    void WaitDuringLoadingScreen() override;
    bool RunConsistencyCheck(CachedFile* apCachedFile) override;

    // The deleting destructor frees through MemoryManager::Deallocate.
    static void operator delete(void* apMemory);
};
