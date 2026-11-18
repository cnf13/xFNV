#pragma once

#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class BSAudioListener;
class TESSound;

class AudioTick {
public:
    AudioTick();
    ~AudioTick();

    // Whether the audio tick is paused.
    int32_t IsPaused();
    // Adds the ticks since the last update to the timer unless it is paused.
    void Update();
    // Returns GetTickCount's value.
    uint32_t Restart();

    int32_t iMilliseconds;
    int32_t iLastUpdateTime;
    uint32_t iFlags;
};

// The game's callback that gives a sound's loop points (FalloutAudio_GetLoopPoints).
using BSAudioLoopPointCallback = bool (*)(void*, uint32_t*, uint32_t*);

// The audio subsystem: owns the listener, the sound-file callbacks and the audio thread.
class BSAudio {
public:
    BSAudio();
    virtual ~BSAudio();

    // the xbox build has GetAudioObject before Init; the PC vtable does not
    virtual void Init(void* pHandle);
    virtual void Shutdown();
    virtual void SetLoopPointCallback(BSAudioLoopPointCallback apfnLoopPointCallback) {}
    virtual BSAudioLoopPointCallback GetLoopPointCallback() {
        return nullptr;
    }
    virtual BSGameSound* CreateNewGameSound(const char* apcFileName) = 0;
    virtual void FixSoundPath(char* FileName) = 0;
    virtual void PrintDebugInfo() {}

    // Whether the synch timer is paused.
    int32_t IsSynchTimerPaused();

    void PauseWorldVoices();
    void UnPauseWorldVoices();
    void PauseAnimationSounds();
    void UnPauseAnimationSounds();
    void UnMute(uint32_t aiFadeTimeMS);
    void UnMuteType(uint32_t aiType, uint32_t aiFadeTimeMS);
    void PauseAllOfType(uint32_t aiType, bool abExactMatch);
    void UnPauseAllOfType(uint32_t aiType, bool abExactMatch);
    void EndAllSoundsOfType(uint32_t aiType);
    // Empty: does nothing.
    void EndAllMovingSounds();
    bool IsListenerUnderwater();
    void SetEnvironment(uint32_t aiEnviroType);
    void SetListenerUnderwater(bool abIsUnderwater);
    void PositionListener(NiPoint3 aNewPosition);
    // A handle for the sound with this editor name and the given flags.
    BSSoundHandle GetSoundHandleByName(const char* apSoundName, uint32_t aiTypeFlags);
    // A handle for the sound file apFilename with the given flags, with apSettingsContext's settings.
    BSSoundHandle GetSoundHandleByFilename(const char* apFilename, uint32_t aiTypeFlags, void* apSettingsContext);
    // A handle for the sound form aiFormID with the given flags.
    BSSoundHandle GetSoundHandleByNumericID(uint32_t aiFormID, uint32_t aiTypeFlags);
    // A handle for the sound form auiFormID with the given flags, placed at akPosition and played looping, under the
    // audio memory context; aiUnused is ignored.
    BSSoundHandle
    PlaySound(uint32_t auiFormID, BSGameSound::SoundFlags aeAudioFlags, NiPoint3 akPosition, int32_t aiUnused);
    void SetConditions(float afCurrentGamehour, uint32_t aiWeatherConditions);
    // The per-frame audio update.
    void Update(bool abPlayGameWorldSounds);
    // Queues a sound for caching under the audio heap context.
    void Precache(const char* apSoundName, uint32_t aiFlags);
    // Caches the sound file apFilename with the given flags and apSettingsContext's settings. apFilename is the result
    // of TESSound::GetFilename.
    void PrecacheSound(const char* apFilename, uint32_t aiFlags, TESSound* apSettingsContext);
    bool IsAudioEnabled();
    bool IsMultiThreaded();
    // Runs the audio update on its own thread or not.
    void SetMultiThreaded(bool abUseMultipleThreads);
    // Fades the sounds of type aiType to aiMillibels over aiFadeTimeMS.
    void MuteType(uint32_t aiType, uint32_t aiFadeTimeMS, uint32_t aiMillibels);
    void Mute(uint32_t aiFadeTimeMS, uint32_t aiMillibels) {
        MuteType(-1, aiFadeTimeMS, aiMillibels);
    }
    float GetMasterSecondaryVolume();
    // Each setter stores afNewVol in the audio manager's volume array and has it reapply the volumes.
    void SetMasterVolume(float afNewVol);
    void SetFootVolume(float afNewVol);
    void SetVoiceVolume(float afNewVol);
    void SetEffectVolume(float afNewVol);
    void SetMusicVolume(float afNewVol);
    void SetRadioVolume(float afNewVol);
    void SetMasterSecondaryVolume(float afNewVol);
    void VolumeChanged();
    void UnPauseWorldSounds();
    void ClearSecondaryVolumes();
    void KillAllOfType(uint32_t aiType, uint32_t aiFadeTimeMS);
    void PauseWorldSounds();
    float GetFootSecondaryVolume();
    float GetVoiceSecondaryVolume();
    float GetEffectSecondaryVolume();
    float GetMusicSecondaryVolume();
    float GetRadioSecondaryVolume();
    void ExitDialogue();
    void UnPauseSynchTimer();
    NiPoint3* GetListenerPosition(NiPoint3* apResult);
    void EnterDialogue();
    void PauseSynchTimer();
    void PrintDebugInfoThunk();
    BSSimpleList<AudioDebugString*>* GetDebugString(uint32_t aiPage);
    uint32_t GetSynchTime();
    void StopMovingSounds(NiAVObject* apObj, float afFadeSeconds, uint32_t aiFlags);

    // The process-wide instance, created by Init.
    static BSAudio& QInstance();
    static float GetMinDialogDist();
    static float GetMaxDialogDist();
    static void operator delete(void* apMemory);

    bool bAudioEnabled;
    bool bInitialized;
    bool bMultiThread;
    uint8_t padding07;
    int32_t iMusicStartOffset;
    bool bSeekMusic;
    uint8_t padding0D[3];
    BSAudioListener* pListener;
    // The audio tick sound handles synchronize against.
    AudioTick SynchTimer;
    // The callbacks the game installs before Init (Main::Main_AudioInit: FalloutAudio::ResolveSoundID, ResolveSoundName
    // and ApplySettingsFromMasterFile, whose signatures these are). The settings context is the game's sound form,
    // opaque to the audio layer. pfnIDCallback gives the sound file, flags and settings context of the sound form
    // auiFormID; false when there is none.
    bool (*pfnIDCallback)(uint32_t auiFormID, char* apOutFilePath, uint32_t* apOutFlags, void** appSettingsContext);
    // The same for the sound with the editor name apName.
    bool (*pfnNameCallback)(const char* apName, char* apOutFilePath, uint32_t* apOutFlags, void** appSettingsContext);
    // Picks a random sound file from the folder in apFilePath.
    bool (*pfnRandomFilenameCallback)(char* apFilePath);
    // Applies the settings context's settings to the sound of apSoundHandle; the handle's address is passed.
    uint32_t (*pfnSettingsCallback)(BSSoundHandle* apSoundHandle, void* apSettingsContext, uint32_t* apOutFlags);
    // Called when the synch timer pauses and unpauses.
    void (*pfnSynchPausedCallback)();
    void (*pfnSynchUnPausedCallback)();

    static inline BSAudio* pAudioInstance;
    // The bEnableAudio:Audio INI setting; Init reads it. Its initialiser adds it to the INI collection.
    static SettingT<INISettingCollection> bEnableAudio;
    // The distances between which the volume of dialogue falls off (INI settings).
    static SettingT<INISettingCollection> fDialogMinDistance;
    static SettingT<INISettingCollection> fDialogMaxDistance;
    static BSSoundHandle kVatsLoopHandle;
};

extern SettingT<INISettingCollection> gs_bMultiThreadAudio_Audio;
