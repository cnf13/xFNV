#pragma once

#include "BSMain/Setting.h"
#include "BSMovie/MoviePlayer.h"

// The game's movie player. Only some of its members are declared.
class BGSMoviePlayer : public MoviePlayer {
public:
    BGSMoviePlayer();
    ~BGSMoviePlayer() override;

    void MuteAudio() override;
    void UnmuteAudio() override;
    void PauseMusic() override;
    void UnPauseMusic() override;
    void SuspendAudioThread() override {}
    void ResumeAudioThread() override {}
    IDirect3DDevice9* GetDevice() override;
    void LockRenderer() override;
    void UnlockRenderer() override;
    uint32_t GetScreenWidth() override;
    uint32_t GetScreenHeight() override;
    bool IsRendererInsideFrame() override;
    uint32_t GetScreenOffsetX() override;
    uint32_t GetScreenOffsetY() override {
        return 0;
    }
    uint32_t GetMainThreadID() override;
    bool CheckContinuePlaying(bool abAllowInterruption) override;

    bool IsPlaying();
    // Copies the path BinkPaths holds for apMovieName into pcDest (MAX_PATH bytes); false when it holds none.
    // Movie::OpenBink passes its source name.
    char GetMoviePath(const char* apMovieName, char* pcDest);

    static void operator delete(void* apMemory);
    static void Create();
    static void Destroy();

    static inline bool bAudioThreading;
    // The movie a new game opens with (sIntroMovie:General).
    static SettingT<INISettingCollection> sIntroMovie;
    // The intro movie of the main menu (sMainMenuMovieIntro:General).
    static SettingT<INISettingCollection> sMainMenuMovieIntro;
};
