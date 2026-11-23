#include "BSSystem/Win32/BSWin32FileCache.h"

#include "BSCore/MemoryManager.h"
#include "BSSystem/BSFileCache.h"

// The PC build has no cache drive: these overrides do nothing.
BSWin32FileCache::BSWin32FileCache() : BSFileCache() {}

BSWin32FileCache::~BSWin32FileCache() {}

void BSWin32FileCache::Init(bool abCacheDriveAvailable) {
    bCacheDriveAvailable = abCacheDriveAvailable;
}

void BSWin32FileCache::Shutdown() {}

int32_t BSWin32FileCache::ParseCachedFileList(const char* apFileList, uint32_t aiCurrentPriority) {
    return 0;
}

void BSWin32FileCache::BeginFileCaching(const char* apcWarningMsg) {}

int32_t BSWin32FileCache::GetCachedFileIndex(char* apFileName) {
    return 0;
}

void BSWin32FileCache::GetCachedFileName(char* apInputFileName, char* apCachedFileName) {}

bool BSWin32FileCache::CacheFile(CachedFile* apCachedFile) {
    return false;
}

void BSWin32FileCache::SetPriority(int32_t aiPriority) {}

void BSWin32FileCache::CalculatePriority() {}

void BSWin32FileCache::OutputCacheDriveListing(char* apDirectory) {}

void BSWin32FileCache::WaitForFileCachingToFinish(
    uint32_t aiMaxWait,
    int32_t aiPriority,
    bool abUpdateLoadingBar,
    uint8_t cLoadingBarSection
) {}

void BSWin32FileCache::WaitDuringLoadingScreen() {}

bool BSWin32FileCache::RunConsistencyCheck(CachedFile* apCachedFile) {
    return false;
}

void BSWin32FileCache::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
