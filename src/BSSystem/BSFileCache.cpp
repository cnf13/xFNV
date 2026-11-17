#include "BSSystem/BSFileCache.h"

#include "BSCore/MemoryManager.h"
#include "BSSystem/Win32/BSWin32FileCache.h"
#include "SDK/MSVC/stdlib.h"

// iEssentialPriority, iUnessentialPriority and bRunningFromDVD are deliberately left unset, as the game does.
BSFileCache::BSFileCache()
  : pCurrentlyCachingFile(nullptr), iCurrentWritten(0), iCurrentFileSize(0), iTotalWritten(0),
    iTotalDiskSpaceAvailable(0), iFreeDiskSpaceAvailable(0), iTotalAlreadyCached(0), iTotalSizeToCache(0),
    iNumCachedFiles(0), iCurrentThreadPriority(-1), iThreadTime(1), iCurrentPriority(0), iPrecachePriority(0),
    eCurrentLayer(DVD_LAYER_FRONT), bArraySorted(false), bCacheDriveAvailable(false), bInPauseLoop(false),
    iFileCacheLoadScreenTime(0), iFileCacheLoadScreenEssentialTime(2), pCachedFiles(nullptr),
    pFileCacheCompletionCallback(nullptr), pForceFileCacheCallback(nullptr), eFileCacheState(FILE_CACHING_IN_PROGRESS),
    hFileCachingThreadHandle(nullptr) {}

BSFileCache::~BSFileCache() {}

int32_t BSFileCache::ParseCachedFileList(const char* apFileList, uint32_t aiCurrentPriority) {
    return 0;
}

// The PC build keeps no cache index: no file is found.
int32_t BSFileCache::GetCachedFileIndex(char* apFileName) {
    return 0;
}

bool BSFileCache::CacheFile(CachedFile* apCachedFile) {
    return (bool)0;
}

void BSFileCache::WaitForFileCachingToFinish(
    uint32_t aiMaxWait,
    int32_t aiPriority,
    bool abUpdateLoadingBar,
    uint8_t cLoadingBarSection
) {
    ;
}

// The PC build checks nothing.
bool BSFileCache::RunConsistencyCheck(CachedFile* apCachedFile) {
    return false;
}

void BSFileCache::FinishThread() {
    hFileCachingThreadHandle = nullptr;
}

void BSFileCache::PauseFileCaching() {
    if (eFileCacheState == 1) {
        eFileCacheState = FILE_CACHING_PAUSED;
    }
}

void BSFileCache::ResumeFileCaching() {
    if (eFileCacheState == 2) {
        eFileCacheState = FILE_CACHING_IN_PROGRESS;
    }
}

bool BSFileCache::IsFileCached(char* apFileName) {
    int32_t iIndex = GetCachedFileIndex(apFileName);
    return IsFileCached(iIndex);
}

// The compare of aiIndex with iNumCachedFiles is signed, so the count is read as int32_t.
bool BSFileCache::IsFileCached(int32_t aiIndex) {
    if (aiIndex < 0 || aiIndex >= (int32_t)iNumCachedFiles) {
        return false;
    }
    return pCachedFiles->pBuffer[aiIndex]->eCacheStatus == CACHE_STATUS_FULLY_CACHED;
}

void BSFileCache::CalculateCurrentLayer() {
    ;
}

void BSFileCache::WaitForEssentialFileCachingToFinish() {
    WaitForFileCachingToFinish((uint32_t)0, iEssentialPriority, (bool)1, (uint8_t)0);
}

void BSFileCache::WaitIfPaused() {
    while (eFileCacheState == 2 || eFileCacheState == 3) {
        bInPauseLoop = 1;
        Sleep(50u);
    }
    bInPauseLoop = 0;
}

void BSFileCache::PauseAndWaitForIdle() {
    if (eFileCacheState == 1) {
        eFileCacheState = FILE_CACHING_PAUSED;
    }
    if (eFileCacheState == 2 || eFileCacheState == 3) {
        while (!bInPauseLoop) {
            Sleep(10u);
        }
    }
}

void BSFileCache::ForcePauseAndWaitForIdle() {
    if (hFileCachingThreadHandle) {
        if (eFileCacheState == 1 || eFileCacheState == 2) {
            eFileCacheState = FILE_CACHING_FORCE_PAUSED;
        }
        if (eFileCacheState == 2 || eFileCacheState == 3) {
            while (!bInPauseLoop) {
                Sleep(10u);
            }
        }
    }
}

void BSFileCache::SetFileCacheList(const char* aEssentialFileCacheList, const char* aUnessentialFileCacheList) {
    sEssentialFileCacheList = aEssentialFileCacheList;
    sUnessentialFileCacheList = aUnessentialFileCacheList;
}

BSFileCache& BSFileCache::QInstance() {
    static BSWin32FileCache _sInstance;
    return _sInstance;
}

void BSFileCache::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

// bWasPaused is deliberately left unset while caching has finished, as the game does.
BSFileCachePauser::BSFileCachePauser(int32_t aiUnused) {
    BSFileCache& rCache = BSFileCache::QInstance();
    if (rCache.eFileCacheState != FILE_CACHING_FINISHED) {
        bWasPaused =
            rCache.eFileCacheState == FILE_CACHING_PAUSED || rCache.eFileCacheState == FILE_CACHING_FORCE_PAUSED;
        rCache.PauseFileCaching();
    }
}

BSFileCachePauser::~BSFileCachePauser() {
    BSFileCache& rCache = BSFileCache::QInstance();
    if (rCache.eFileCacheState != FILE_CACHING_FINISHED && !bWasPaused) {
        rCache.ResumeFileCaching();
    }
}
