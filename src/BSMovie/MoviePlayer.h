#pragma once

#include "BSCore/BSSemaphore.h"
#include "BSMain/BSTCaseInsensitiveStringMap.h"
#include "BSMain/Setting.h"
#include "SDK/Bink/bink.h"

struct BINKTEXTURESET;

void BinkFree(void* apMemory);
void* BinkAlloc(void* apThis, [[maybe_unused]] void* apEdx, uint32_t Size);

// What a movie is doing (ClearData stores 0, Play 1, PlayBackground 2).
enum PLAYING_STATE : int32_t {
    MOVIE_NOT_PLAYING = 0,
    MOVIE_PLAYING = 1,
    MOVIE_PLAYING_BACKGROUND = 2,
};

// One Bink movie being played: its Bink handle, the texture set it is drawn through and where on the screen.
class Movie {
public:
    Movie();
    ~Movie();

    bool OpenBink(const char* apFileName, bool abSkipWarning, bool abPreloadBink, bool abForcePreload);
    // Loads the Bink movie apFileName.
    bool LoadBink(const char* apFileName, bool abSkipWarning, bool abPreloadBink, bool abLetterBoxed);
    // Pauses the movie.
    void Pause();
    // Resumes the movie.
    void Resume();
    // Resets the movie to its unloaded state.
    void ClearData();
    bool PlayBackground(const char* apFileName, bool abPreloadBink);
    // Runs the movie's next frame; false when it finished.
    bool Update(bool abAllowInterruption, bool abLoop, bool abLetterBoxed);
    void Stop();
    // Plays the movie through Bink; false when it could not be opened or it finished early.
    bool Play(
        const char* apFileName,
        bool abAllowInterruption,
        bool abSkipWarning,
        bool abLoop,
        bool abPreloadBink,
        bool abLetterBoxed,
        bool abMuteGameAudio,
        bool abPauseGameMusic,
        float afTimeBeforeMovieSkippable
    );

    BINK* pBink;
    IDirect3DDevice9* dxDevice;
    BINKTEXTURESET* pTextureSet;
    uint32_t iFrameCount;
    uint32_t iNumFramesSkipped;
    float fScale;
    float fXCoordOffset;
    float fYCoordOffset;
    PLAYING_STATE ePlayingState;
    bool bLoading;
};

// Plays the Bink movies and movie sequences. Only some of the members are declared.
class MoviePlayer {
public:
    MoviePlayer();
    virtual ~MoviePlayer();

    // The platform hooks are pure virtual here; BGSMoviePlayer implements them.
    virtual void MuteAudio() = 0;
    virtual void UnmuteAudio() = 0;
    virtual void PauseMusic() = 0;
    virtual void UnPauseMusic() = 0;
    virtual void SuspendAudioThread() = 0;
    virtual void ResumeAudioThread() = 0;
    virtual IDirect3DDevice9* GetDevice() = 0;
    virtual void LockRenderer() = 0;
    virtual void UnlockRenderer() = 0;
    virtual uint32_t GetScreenWidth() = 0;
    virtual uint32_t GetScreenHeight() = 0;
    virtual bool IsRendererInsideFrame() = 0;
    virtual uint32_t GetScreenOffsetX() = 0;
    virtual uint32_t GetScreenOffsetY() = 0;
    virtual uint32_t GetMainThreadID() = 0;
    // Pumps the window messages; false when the movie should stop.
    virtual bool CheckContinuePlaying(bool abAllowInterruption);

    // Plays the movie sequence apString.
    void PlaySequence(char* apString);
    // Stores the movie a background thread loaded.
    void SetThreadLoadedMovie(Movie* apMovie);
    void SetSequenceThreadPriority(int32_t aiPriority);
    // Whether a movie sequence is playing.
    bool GetPlayingSequence();
    // Blocks until the playing movie sequence ends.
    void WaitForSequenceToEnd();
    // Stops the playing movie.
    void Stop();
    // Opens apFileName as the current movie; stops the player when Bink fails.
    bool Open(const char* apFileName, bool abSkipWarning, bool abPreloadBink, bool abForcePreload);
    // Plays the movie apFileName; false when it could not be opened.
    bool Play(
        const char* apFileName,
        bool abAllowInterruption,
        bool abSkipWarning,
        bool abLoop,
        bool abPreloadBink,
        bool abLetterBoxed,
        bool abAlreadyOpened,
        bool abMuteGameAudio,
        bool abPauseGameMusic
    );
    void StopSequence();
    void IncreaseSequenceThreadPriority();
    void Resume();
    // Runs the current movie's next frame; false when no movie is playing or it finished.
    bool Update(bool abAllowInterruption, bool abLoop);
    // Pauses the current movie and keeps it for Resume; only one movie can be paused.
    void Pause();
    float CalculateScale(uint32_t afWidth, uint32_t afHeight, bool abLetterBoxed);
    float CalculateXOffset(float afScale, uint32_t afWidth, bool abLetterBoxed);
    float CalculateYOffset(float afScale, uint32_t afHeight, bool abLetterBoxed);
    // Points Bink at DirectSound when audio is enabled.
    void Initialize(bool abAudioEnabled);
    void WaitForSequenceToBegin();
    bool PlayBackground(const char* apFileName, bool abPreloadBink);
    // Starts the sequence thread and waits for Bink to open.
    void StartSequence(
        const char* apString,
        bool abLetterBoxed,
        int32_t aiSequencePriority,
        bool abMuteGameAudio,
        bool abPauseGameAudio,
        float afTimeBeforeMovieSkippable
    );
    void SignalBinkOpen();
    void DecreaseSequenceThreadPriority();
    void DeleteBinkPaths();
    void ThreadLoadMovie(const char* apFileName, void (*afpCallback)());
    void WaitForMovieLoadingThreadToFinish();
    void AddBinkPath(const char* apFileName, const char* apPath);

    // The player is freed through MemoryManager::Deallocate.
    static void operator delete(void* apMemory);
    static uint32_t PlaySequenceThread(void* apString);
    static uint32_t MovieLoadingThread(void* apFileName);

    BSSystemThread* hMovieLoadingThreadHandle;
    BSSystemThread* hSequenceThreadHandle;
    Movie* pCurrentMovie;
    Movie* pPausedMovie;
    Movie* pThreadLoadedMovie;
    bool bUseThreadLoadedMovie;
    bool bPAL50;
    void (*fpThreadLoadCallback)();
    bool bWaitingForSequenceToEnd;
    bool bStopSequence;
    // StartSequence stores its letterbox, mute and pause arguments here.
    bool bLetterBoxedSequence;
    bool bSequenceMutesGameAudio;
    bool bSequencePausesGameAudio;
    float m_fTimeBeforeMovieSkippable;
    int32_t iCurrentSequenceThreadPriority;
    // StartSequence waits on it; SignalBinkOpen releases it and sets bBinkOpenSignaled.
    BSSemaphore OpenBinkSema;
    bool bBinkOpenSignaled;
    // The movie paths by name.
    BSTCaseInsensitiveStringMap<char*> BinkPaths;
};

// The movie player singleton.
inline MoviePlayer* pMoviePlayer;

extern SettingT<INISettingCollection> gs_bPreloadIntroSequence_General;

extern SettingT<INISettingCollection> gs_iIntroSequencePriority_General;
