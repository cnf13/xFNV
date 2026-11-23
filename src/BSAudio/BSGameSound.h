#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

class BSSoundMessage;

// A sound the audio engine plays. The audio manager selects the sounds a message applies to by its flags.
class BSGameSound {
public:
    enum SoundFlags : uint32_t {
        // uiTypeFlags is tested against 0.
        SOUND_FLAG_NONE = 0x0,
        SOUND_FLAG_IS_2D = 0x1,
        SOUND_FLAG_IS_3D = 0x2,
        SOUND_FLAG_VOICE = 0x4,
        SOUND_FLAG_FOOTSTEPS = 0x8,
        SOUND_FLAG_LOOP = 0x10,
        SOUND_FLAG_SYSTEM_SOUND = 0x20,
        SOUND_FLAG_VARIABLE_FREQUENCY = 0x40,
        SOUND_FLAG_BATTLE_SOUND = 0x80,
        SOUND_FLAG_ONE_SHOT = 0x100,
        SOUND_FLAG_UNKBIT9 = 0x200,
        SOUND_FLAG_UNKBIT10 = 0x400,
        SOUND_FLAG_MUSIC = 0x800,
        SOUND_FLAG_REGION = 0x1000,
        SOUND_FLAG_MAYBE_UNDERWATER = 0x2000,
        SOUND_FLAG_IMPACT = 0x4000,
        SOUND_FLAG_CACHED = 0x8000,
        SOUND_FLAG_DONT_CACHE = 0x10000,
        SOUND_FLAG_IS_2D_GUNFIRE = 0x20000,
        SOUND_FLAG_FIRST_PERSON = 0x40000,
        SOUND_FLAG_MODULATED = 0x80000,
        SOUND_FLAG_RADIO = 0x100000,
        SOUND_FLAG_IGNORE_TIMESCALE = 0x200000,
        SOUND_FLAG_UNKBIT22 = 0x400000,
        SOUND_FLAG_MUSIC_B23 = 0x800000,
        SOUND_FLAG_EXTERNAL_SYNC = 0x1000000,
        SOUND_FLAG_ENVELOPE_FAST = 0x2000000,
        SOUND_FLAG_ENVELOPE_SLOW = 0x4000000,
        SOUND_FLAG_IS_2D_RADIUS = 0x8000000,
        SOUND_FLAG_BEAM_EMITTER = 0x10000000,
        SOUND_FLAG_UNKBIT29 = 0x20000000,
        SOUND_FLAG_ANIMATION_DRIVEN = 0x40000000,
        SOUND_FLAG_UNKBIT31 = 0x80000000,
    };

    // The state bits of uiStateFlags.
    enum StateFlags : uint32_t {
        STATE_OPENED = 0x1,
        STATE_UNK_1 = 0x2,
        STATE_UNK_2 = 0x4,
        STATE_UNK_3 = 0x8,
        STATE_STOPPED = 0x10,
        STATE_PLAYING = 0x20,
        STATE_PAUSED = 0x40,
        STATE_UNK_7 = 0x80,
        STATE_VALID = 0x100,
        STATE_FADING = 0x200,
        STATE_MUTED = 0x400,
        STATE_READY = 0x800,
        STATE_OPENING = 0x1000,
        STATE_PLAYED = 0x2000,
        STATE_UNK_14 = 0x4000,
        STATE_IS_VALID_BEAM = 0x8000,
        STATE_PLAY_LATER = 0x10000,
        STATE_INAUDIBLE = 0x20000,
        STATE_UNK_18 = 0x40000,
        STATE_IN_DECAY_19 = 0x80000,
        STATE_EXPLICIT_PAUSE = 0x100000,
    };

    enum ModifierFlags : uint32_t {
        MODIFIER_MUTE_WHEN_SUBMERGED = 0x1,
        MODIFIER_START_AT_RANDOM_POSITION = 0x2,
    };

    BSGameSound();
    BSGameSound(const char* apFilename);
    virtual ~BSGameSound();

    virtual bool IsPlaying();
    virtual bool IsInDecay();
    virtual bool IsPaused();
    virtual float GetVolume();
    // Tests the whole byte; bool would assume 0/1.
    virtual void SetInaudible(bool abIn);
    virtual void SetEnvironmentType(uint32_t aiEnvType);
    virtual bool PriorityTest(BSGameSound* apToCompare);
    // true when the sound's priority is below aiToCompare
    virtual bool PriorityTest(uint32_t aiToCompare);
    virtual bool Open() = 0;
    virtual bool Prepare();
    virtual void Copy(BSGameSound* apToCopy, bool abPlayable);
    virtual bool Play(bool abLoopPlayback) = 0;
    virtual bool Pause() = 0;
    virtual bool Stop() = 0;
    virtual bool SetVolume(float afNewVolume) = 0;
    virtual void UpdateEmitterPosition() = 0;
    virtual bool Update(uint32_t adwTimeDelta);
    virtual bool ProcessDataRequest(int32_t aiRequestNum) = 0;
    virtual bool SetEmitterPosition(float afX, float afY, float afZ) = 0;
    virtual bool SetEmitterPosition(NiPoint3 vin) = 0;
    virtual NiPoint3 GetEmitterPosition() = 0;
    virtual void SetEmitterOrientation(float afX, float afY, float afZ) = 0;
    virtual void SetMinMax(float afMin, float afMax) = 0;
    virtual void
    SetAttenuationCurve(uint16_t aiPt1, uint16_t aiPt2, uint16_t aiPt3, uint16_t aiPt4, uint16_t aiPt5) = 0;
    virtual bool SetFrequency(float afNewFrequency) = 0;
    virtual float GetFrequency() = 0;
    virtual void Seek(uint32_t uiMilliseconds) = 0;

    bool CanPlayNow(uint32_t aiConditions, float afGamehour);
    bool OpenResult(bool abResult);
    bool Is3DSound() const;
    bool Is2DRadiusSound() const;
    bool IsLoopingSound() const {
        return (iSoundTypeFlags & SOUND_FLAG_LOOP) ||
            (iSoundTypeFlags & SOUND_FLAG_ENVELOPE_SLOW) ||
            (iSoundTypeFlags & SOUND_FLAG_ENVELOPE_FAST);
    }
    bool SetupPath(char* apSrc, bool abSetID);
    // SetupPath on the sound's own pFilename.
    bool SetupPath(bool abSetID);
    void SetIsMuted(char abMuted);
    void Init();
    void HandlePlayCallback();

    static void operator delete(void* apMemory);
    static uint32_t GetNameHash(const char* pString);

    uint32_t iSoundID;
    SoundFlags iSoundTypeFlags;
    uint32_t iModifierFlags;
    // StateFlags bits.
    uint32_t iStateFlags;
    uint32_t iDurationInMS;
    uint16_t iStaticAttenuation;
    uint16_t iReverbAttenuation;
    uint16_t iSystemAttenuation;
    uint16_t iDistanceAttenuation;
    uint16_t iFaderAttenuation;
    float fVolume;
    float fBeginTime;
    float fEndTime;
    uint32_t iWeatherFlags;
    uint16_t iSamplesPerSecond;
    // strcpy_s is bounded by 260 in SetupPath.
    char pFilename[260];
    float fMaxDist;
    float fMinDist;
    uint32_t iSoundHash;
    uint32_t iDirectoryHash;
    uint32_t iSoundSize;
    float fDistanceToListener;
    uint32_t iEnvironmentType;
    uint8_t iFreqVariance;
    uint16_t iModSamplesPerSecond;
    uint32_t iSynchStartTime;
    uint32_t iCleanupDelayInMS;
    // Run with pSoundCompletionContext when the sound finishes (whether it played), and with pSoundPlayContext and
    // the sound's duration when it starts.
    void (*fnCompletionCallback)(void* apContext, bool abSucceeded);
    void (*fnPlayedCallback)(void* apContext, int32_t aiDuration);
    void* pSoundCompletionContext;
    void* pSoundPlayContext;
    uint32_t iLoopStart;
    uint32_t iLoopEnd;
    NiPoint3 vBeamEnd;
    uint32_t iPriority;
    // The sound's queued messages. It is emptied with NiTListBase::RemoveAll in ~BSGameSound and appended to with
    // AddTail.
    NiTPointerList<BSSoundMessage*> QueuedMessages;
};
