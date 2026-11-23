#pragma once

#include <cstdint>

struct FalloutAudioManagerThreads;
class MediaLocationController;

// The music the game is playing: two tracks, each with its file path, volume, position and media ID, and the pipboy
// radio state.
class PlayingMusic {
public:
    struct PerTrack {
        uint32_t eMediaType;
        uint32_t offset;
        uint32_t uiCurrentPosition;
        uint32_t uiDuration;
        uint32_t uiSubDuration;
        uint32_t unk014;
        uint32_t unk018;
        uint32_t uITimePassed;
        uint32_t uiMediaID;
    };

    char cTrack0Path[260];
    FalloutAudioManagerThreads* pThreads;
    char cTrack1Path[260];
    uint8_t padding20C[4];
    float fMediaAVolume;
    float track0flt004;
    float track0CalculatedVolume;
    float track0Volume;
    // PlayingMusic's flags, as a byte; no enum is declared for them.
    uint8_t usTrackFlags[2];
    bool bMenuPaused;
    bool bIsPipboyRadioPlaying;
    PerTrack kTrackA;
    float fMediaBVolume;
    float track1flt004;
    float track1CalculatedVolume;
    float track1Volume;
    PerTrack kTrackB;
    int32_t eMediaNextTrack;
    uint32_t uiThreadCount;
    MediaLocationController* pCurrentMediaController;
};
