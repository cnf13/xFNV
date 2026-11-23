#pragma once

#include "BSAudio/BSAudio.h"
#include "BSAudio/DirectSound.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

#include <stdint.h>

int32_t SeekFileCallback(std::FILE* Stream, int32_t Offset, int32_t OffsetHigh, int32_t Origin);

// The PC audio engine: BSAudio implemented over Win32/XAudio. The engine reaches it through
// BSAudio::QInstance; the volume sliders are cached in the audio manager.
class BSWin32Audio : public BSAudio {
public:
    BSWin32Audio();
    ~BSWin32Audio() override;

    void Init(void* pHandle) override;
    void Shutdown() override;
    BSGameSound* CreateNewGameSound(const char* apcFileName) override;
    void FixSoundPath(char* FileName) override;

    float GetMasterVolume();
    float GetFootVolume();
    float GetVoiceVolume();
    float GetEffectsVolume();
    float GetMusicVolume();
    float GetRadioVolume();

    // Fades or stops every moving sound attached to apObj, per the fade flags.
    void StopMovingSounds(void* apObj, float afFadeSeconds, uint32_t aiFlags);

    void SetOrientation(NiPoint3 aFront, NiPoint3 aTop);
    void PrecacheSoundFromID(int32_t auiFormID, int32_t aiFlags);
    // Whether the audio manager caches sounds, asked under the audio memory context.
    bool IsEnableAudioCache();
    // True for a non-negative (successful) aiResult; a failure goes to the release build's empty error report
    // (InitializationFlow) and returns false. apMessage, the caller's message, is not read.
    bool Error(int32_t aiResult, const char* apMessage);
    void SetAudioCacheSize(uint32_t auiNewSize);
    void SetCacheEnabled(bool abEnableCache);

    static void SetFootstepVolumeMult(float afMult);
    static void SetVoiceVolumeMult(float afMult);
    static void SetEffectsVolumeMult(float afMult);
    static void SetMusicVolumeMult(float afMult);
    static void SetRadioVolumeMult(float afMult);
    static void Create();

    IDirectSound8* pDirectSound8;
    IDirectSoundBuffer* pPrimarySoundBuffer;
    // The DirectSound device caps.
    DSCAPS kBufferCaps;
    HWND__* hWindow;
};
inline GUID CLSID_IDirectSound3DListener{0x279AFA84, 18817, 4558, {165, 33, 0, 32, 175, 11, 229, 96}};
