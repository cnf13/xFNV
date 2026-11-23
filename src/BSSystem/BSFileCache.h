#pragma once

#include "BSCore/BSSimpleArray.h"

#include <stdint.h>

class CachedFile;

// The cache thread's state (BSFileCache::eFileCacheState).
enum FILE_CACHE_STATE : uint32_t {
    FILE_CACHING_DISABLED = 0,
    FILE_CACHING_IN_PROGRESS = 1,
    FILE_CACHING_PAUSED = 2,
    FILE_CACHING_FORCE_PAUSED = 3,
    FILE_CACHING_FINISHED = 4,
};

// The DVD layer the cache is reading from (BSFileCache::eCurrentLayer); on PC CalculateCurrentLayer is empty and the
// constructor leaves it at DVD_LAYER_FRONT.
enum DVD_LAYER : uint32_t {
    DVD_LAYER_FRONT = 0,
    DVD_LAYER_BACK = 1,
};

// How much of a file is on the cache drive (CachedFile::eCacheStatus).
enum CACHE_STATUS : uint32_t {
    CACHE_STATUS_NOT_CACHED = 0,
    CACHE_STATUS_PARTIALLY_CACHED = 1,
    CACHE_STATUS_FULLY_CACHED = 2,
    CACHE_STATUS_COUNT = 3,
};

// One file of the cache lists. Only some of its members are declared.
class CachedFile {
public:
    char* pFileName;
    char* pFileName2;
    uint32_t iFileSize;
    CACHE_STATUS eCacheStatus;
};

// The background file cache: copies the files in its cache lists to a cache drive on a thread, by priority.
class BSFileCache {
public:
    BSFileCache();
    virtual ~BSFileCache();

    virtual void Init(bool abCacheDriveAvailable) {}
    virtual void Shutdown() {}
    virtual int32_t ParseCachedFileList(const char* apFileList, uint32_t aiCurrentPriority);
    virtual void BeginFileCaching(const char* apcWarningMsg) {}
    virtual int32_t GetCachedFileIndex(char* apFileName);
    // Writes the cached copy's name of apInputFileName to apCachedFileName.
    virtual void GetCachedFileName(char* apInputFileName, char* apCachedFileName) {}
    virtual bool CacheFile(CachedFile* apCachedFile);
    virtual void SetPriority(int32_t aiPriority) {}
    virtual void CalculatePriority() {}
    virtual void OutputCacheDriveListing(char* apDirectory) {}
    virtual void WaitForFileCachingToFinish(
        uint32_t aiMaxWait,
        int32_t aiPriority,
        bool abUpdateLoadingBar,
        uint8_t cLoadingBarSection
    );
    // Waits while the loading screen is up.
    virtual void WaitDuringLoadingScreen() {}
    virtual bool RunConsistencyCheck(CachedFile* apCachedFile);
    virtual void ForceFileCaching() {}
    virtual void FinishThread();

    // Pause and resume caching.
    void PauseFileCaching();
    void ResumeFileCaching();
    // Stops a running or paused cache until ForceResumeFileCaching.
    void ForcePauseFileCaching();

    // Whether apFileName is held by the cache: its index from GetCachedFileIndex (vtable +0x14), then
    // IsFileCached(int).
    bool IsFileCached(char* apFileName);
    // Whether the cache list's file aiIndex is fully on the cache drive.
    bool IsFileCached(int32_t aiIndex);
    void CalculateCurrentLayer();
    void WaitForEssentialFileCachingToFinish();
    void WaitIfPaused();
    void PauseAndWaitForIdle();
    void ForcePauseAndWaitForIdle();
    void SetFileCacheList(const char* aEssentialFileCacheList, const char* aUnessentialFileCacheList);

    // The cache, created on first use.
    static BSFileCache& QInstance();
    static void operator delete(void* apMemory);

    CachedFile* pCurrentlyCachingFile;
    uint32_t iCurrentWritten;
    uint32_t iCurrentFileSize;
    uint32_t iTotalWritten;
    uint32_t iTotalDiskSpaceAvailable;
    uint32_t iFreeDiskSpaceAvailable;
    uint32_t iTotalAlreadyCached;
    uint32_t iTotalSizeToCache;
    uint32_t iNumCachedFiles;
    int32_t iCurrentThreadPriority;
    uint32_t iThreadTime;
    int32_t iCurrentPriority;
    int32_t iPrecachePriority;
    int32_t iEssentialPriority;
    int32_t iUnessentialPriority;
    DVD_LAYER eCurrentLayer;
    bool bArraySorted;
    bool bCacheDriveAvailable;
    bool bRunningFromDVD;
    bool bInPauseLoop;
    uint32_t iFileCacheLoadScreenTime;
    uint32_t iFileCacheLoadScreenEssentialTime;
    BSSimpleArray<CachedFile*, 1024>* pCachedFiles;
    void (*pFileCacheCompletionCallback)(const char*);
    void (*pForceFileCacheCallback)(bool);
    volatile FILE_CACHE_STATE eFileCacheState;
    void* hFileCachingThreadHandle;
};

// Pauses file caching while it lives: the constructor records whether caching was already paused and pauses a running
// cache; the destructor resumes it otherwise.
class BSFileCachePauser {
public:
    // Records whether caching was paused and pauses a running cache; the argument is unused.
    BSFileCachePauser(int32_t aiUnused);
    // Unless caching has finished or was paused before, resumes a cache this pauser paused.
    ~BSFileCachePauser();

    bool bWasPaused;
};

inline const char* sEssentialFileCacheList = "";
inline const char* sUnessentialFileCacheList = "";
