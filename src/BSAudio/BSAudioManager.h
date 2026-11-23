#pragma once

#include "BSAudio/BSGameSound.h"
#include "BSAudio/SoundMessageList.h"
#include "BSAudio/SoundMessageStack.h"
#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "BSMenu/Font.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class AudioLoadTask;
class BSAudioManagerThread;
class BSSoundInfo;
class BSSoundMessage;
class TESSound;

// A sound ID with its debug description, listed and sorted by BSAudioManager::DebugPrintSounds; the sort compares
// iSoundID unsigned.
class AudioDebugString {
public:
    uint32_t iSoundID;
    char* pString;
};

void au_re_ExitThread(uint32_t dwExitCode, int32_t x0);

// One command for the audio thread. ComposeMessage queues it and BSAudioManager::Update runs it.
enum SOUND_MSG : int32_t {
    MINVALID_MSG = 0x0,
    MPLAY = 0x1,
    MPLAYLOOPING = 0x2,
    MPAUSE = 0x3,
    MSTOP = 0x4,
    MPLAY_AFTER = 0x5,
    MSTOP_AFTER = 0x6,
    MMUTE = 0x7,
    MUNMUTE = 0x8,
    MFADE_OUT = 0x9,
    MFADE_TO = 0xA,
    MFADE_IN = 0xB,
    MFADE_FROM = 0xC,
    MDIALOGUE_FADE = 0xD,
    MVOLUME_LEVELS_CHANGED = 0xE,
    MFREE_IF_STOPPED = 0xF,
    MSET_VOLUME = 0x10,
    MINCREASE_VOLUME_TO = 0x11,
    MDECREASE_VOLUME_TO = 0x12,
    MSET_FREQUENCY = 0x13,
    MSET_FREQUENCY_VARIANCE = 0x14,
    MINCREASE_FREQUENCY_TO = 0x15,
    MDECREASE_FREQUENCY_TO = 0x16,
    MKILL_ALL_SOUNDS = 0x17,
    MMUTE_ALL_OFTYPE = 0x18,
    MUNMUTE_ALL_OFTYPE = 0x19,
    MPAUSE_ALL_OFTYPE = 0x1A,
    MUNPAUSE_ALL_OFTYPE = 0x1B,
    MPAUSE_ALL_OFTYPE_EXACT = 0x1C,
    MUNPAUSE_ALL_OFTYPE_EXACT = 0x1D,
    MSET_POS = 0x1E,
    MSET_BEAM_END = 0x1F,
    MSET_ORIENT = 0x20,
    MSET_STATIC_ATTENUATION = 0x21,
    MSET_REVERB_ATTENUATION = 0x22,
    MSET_MINMAX_DISTANCE = 0x23,
    MSET_ATTENUATION_CURVE = 0x24,
    MADD_SOUND_TO_LIST = 0x25,
    MDELETE_SOUND = 0x26,
    MREGISTER_MOVING_SOUND = 0x27,
    MPURGE_MOVING_SOUNDS = 0x28,
    MSTOP_MOVING_SOUNDS = 0x29,
    MKILL_ALL_OFTYPE = 0x2A,
    MMUTE_ALL_LOOPS = 0x2B,
    MUNMUTE_ALL_LOOPS = 0x2C,
    MFORCE_LOOP = 0x2D,
    MSET_SOUND_TYPE = 0x2E,
    MSET_WEATHER_TYPES = 0x2F,
    MPRECACHE_COMBAT_SOUNDS = 0x30,
    MENABLE_SOUND_CACHE = 0x31,
    MCACHE_SIZE_CHANGED = 0x32,
    MADD_SOUND_TO_CACHE = 0x33,
    MTERMINATE_THREAD = 0x34,
    MSUSPEND_THREAD = 0x35,
    MERROR_TOO_MANY_SOUNDS = 0x36,
    MSET_UNDERWATER = 0x37,
    MSET_LISTENER_POSITION = 0x38,
    MDATA_REQUEST = 0x39,
    MSET_COMPLETION_CALLBACK = 0x3A,
    MSET_PLAY_CALLBACK = 0x3B,
    MSET_SYNCH_START = 0x3C,
    MCLEAR_SYNCH_START = 0x3D,
    MSET_TIME_CONSTRAINTS = 0x3E,
    MSEEK = 0x3F,
    MSET_AT_LISTENER = 0x40,
    MSET_PRIORITY = 0x41,
    MSET_MOD_FREQUENCY = 0x42,
};

// The audio thread's message queue, the registry of playing sounds and the volume settings.
class BSAudioManager {
public:
    // A sound type that is muted and the state of its mute.
    struct MutedSoundType {
        uint32_t uiTypes;
        uint16_t usState;
    };

    // The callbacks the manager runs for a sound: when it finishes (apContext, whether it played) and when it
    // starts (apContext, its duration).
    using CompletionCallback = void (*)(void* apContext, bool abSucceeded);
    using PlayCallback = void (*)(void* apContext, int32_t aiDuration);
    // The audio thread runs the manager's loop: BSAudioManagerThread::ThreadProc calls Process.
    friend class BSAudioManagerThread;

    BSAudioManager();
    // The engine's vtable holds only the deleting destructor.
    virtual ~BSAudioManager();

    // Shuts the manager down.
    void Shutdown();
    // Stops the audio thread.
    void StopAudioThread();
    // A handle for the sound of form auiFormID.
    BSSoundHandle GetSoundHandleByNumericID(uint32_t auiFormID, uint32_t aiTypeFlags);
    // A handle for the sound of the file apFilename.
    BSSoundHandle GetSoundHandleByFilePath(const char* apFilename, uint32_t aeTypeFlags, TESSound* apSound);
    // An empty handle.
    BSSoundHandle SpawnSoundReference(
        int32_t aiFormID,
        int32_t aiTypeFlags,
        int32_t aiLocX,
        int32_t aiLocY,
        int32_t aiLocZ,
        int32_t aiID
    );
    // Loads the sound of form auiFormID ahead of use.
    void PrecacheSoundFromID(uint32_t auiFormID, int32_t aiFlags);
    // Queues the move of the sound aiID to (afX, afY, afZ); always false.
    bool SetPosition(uint32_t aiID, float afX, float afY, float afZ);
    // Moves the end of the beam sound aiSoundID.
    bool SetBeamEnd(uint32_t aiSoundID, float afX, float afY, float afZ);
    // Runs apfnCallback with apSoundContext when the sound aiSoundID finishes; false when it
    // is unknown.
    bool SetCompletionCallback(uint32_t aiSoundID, CompletionCallback apfnCallback, void* apSoundContext);
    // Runs apfnCallback with apSoundContext when the sound aiSoundID starts; false when it is
    // unknown.
    bool SetPlayCallback(uint32_t aiSoundID, PlayCallback apfnCallback, void* apSoundContext);
    // Fades the sound auiSound out over auiMilliseconds and then runs auiFollowUpAction.
    void FadeOut(uint32_t aiID, uint32_t aiMilliseconds, uint32_t aiFollowUpAction);
    // The sound type of aiSound.
    uint32_t GetSoundType(uint32_t aiSoundID);
    void StartAudioThread();
    void SetIgnoreGlobalTimeMultiplier(bool bIgnore);
    BSSoundHandle GetSoundHandleByName(const char* apEditorID, uint32_t aiTypeFlags);
    void Precache(const char* apToCache, uint32_t aiFlags);
    void Update(bool abPlayGameWorldSounds);
    void SynchVoiceStates();

    void SetConditions(float afCurrentGamehour, uint32_t aiWeatherConditions);
    void UnMuteAllOfType(uint32_t aiTargetType, uint32_t aiFadeTimeMS);
    void PauseAllOfType(uint32_t aiType, bool abExactMatch);
    void UnPauseAllOfType(uint32_t aiType, bool abExactMatch);
    void SetListenerUnderwater(bool abIsUnderwater);
    void PositionListener(NiPoint3 aNewPosition);
    void KillAllOfType(uint32_t aiTargetType, uint32_t aiFadeTimeMS);
    void MuteAllOfType(uint32_t aiTargetType, uint32_t aiFadeTimeMS, uint32_t aiMillibels);
    float GetVolume(uint32_t aiSoundID);
    int32_t GetDuration(uint32_t aiSoundID);
    void FadeInPlay(uint32_t aiID, uint32_t aiMilliseconds);
    void SetTimeConstraints(uint32_t aiSoundID, uint8_t aiStart, uint8_t aiStop);
    void SetPriority(uint32_t aiSoundID, uint32_t aiPriority);
    void ClearSynchStartTime(uint32_t aiSoundID);
    void SetSynchStartTime(uint32_t aiSoundID, uint32_t aiStartTime);
    void SetAtListener(uint32_t aiSoundID, bool abSetAtListener);
    void Seek(uint32_t aiSoundID, uint32_t aiMilliseconds);
    void VolumeChanged();
    bool SetVolume(uint32_t aiSoundID, float afNewVolume);
    void SetReverbAttenuation(uint32_t aiSoundID, uint16_t aiReverbAtt);
    void SetAttenuationCurve(
        uint32_t aiSoundID,
        uint16_t aiPt1,
        uint16_t aiPt2,
        uint16_t aiPt3,
        uint16_t aiPt4,
        uint16_t aiPt5
    );
    bool SetFrequencyVariance(uint32_t aiSoundID, uint8_t aiFreqVariance);
    bool SetModFrequency(uint32_t aiSoundID, float afNewFrequency);
    bool SetFrequency(uint32_t aiSoundID, float afNewFrequency);
    bool SetStaticAttenuation(uint32_t aiSoundID, uint16_t aiStaticAttenutation);
    bool Pause(uint32_t aiSoundID);
    bool PlayAfter(uint32_t aiSoundID, uint32_t aiDelay, int32_t aiPlayFlags);
    bool Play(uint32_t aiSoundID, bool abLoopPlayback);
    bool Stop(uint32_t aiSoundID);
    bool Release(uint32_t aiSoundID);
    void ClearSecondaryVolumes();

    // Queues a command for the audio thread. aspSmartPointerData is handed to the audio thread, which releases its
    // reference: callers only pass a null pointer or an object they already hold.
    void ComposeMessage(
        SOUND_MSG aMsg,
        uint32_t aiID,
        intptr_t aiData,
        void* apPointerData,
        NiPointer<NiAVObject> aspSmartPointerData,
        NiPoint3 aVector
    );
    void StopMovingSounds(NiAVObject* apObj, float afFadeSeconds, uint32_t aiFlags);
    bool SetBalance(uint32_t aiSoundID, int32_t aiNewBalance);
    void SetMinMax(uint32_t aiSoundID, float afMin, float afMax);
    bool SetOrientation(uint32_t aiSoundID, float afX, float afY, float afZ);
    BSSoundMessage* GetFreeMessage();
    BSSoundMessage* GetFreeQueueMessage();
    bool IsPausedType(uint32_t aeFlags);
    bool ClearPausedType(uint32_t auiTypes, bool abExact);
    void PrecacheSound(char* apFilenameToCache, uint32_t aiFlags, TESSound* apSettingsContext);
    // Whether sounds are cached.
    bool IsEnableAudioCache();
    void SetCacheEnabled(bool abEnableCache);
    void SetAudioCacheSizeInK(uint32_t auiNewSize);
    bool IsValid(uint32_t aiSoundID);
    uint32_t AudioCacheSize();
    float GetCorrectedVolume(float afBaseVolume, uint32_t aiTypeFlags);
    bool IsPlaying(uint32_t aiSoundID);
    bool IsInCache(uint32_t aiSoundHash, bool abFuzzy);
    void RemoveLoadTask(AudioLoadTask* apTask);

private:
    // GetDebugString's BSSimpleList<AudioDebugString*>::Sort compare: orders the strings by iSoundID, unsigned.
    static int32_t AudioStringCompareFn(AudioDebugString* apA, AudioDebugString* apB);

protected:
    void Process();

public:
    // Returns the cache size AudioCacheSize reads.
    uint32_t GetAudioCacheSize();
    void Initialize();
    void InitializeAudio();
    void SuspendAudioThread();
    void SetVoiceState(uint32_t aiID, BSGameSound* apSound);
    void FreeMessage(BSSoundMessage* apMsgToFree);
    void FreeQueueMessage(BSSoundMessage* apMsgToFree);
    void SetObjectToFollow(uint32_t aiID, NiAVObject* apObj);
    void FadeIn(uint32_t aiID, uint32_t aiMilliseconds, uint32_t aiFollowUpAction);
    void AddToCache(BSGameSound* apToCache);
    BSGameSound* GetFromCache(uint32_t aiSoundHash, uint32_t aiDirectoryHash, uint32_t aiFlagsToApply);
    void Precache(BSGameSound* apToCache);
    void ProcessMessages();
    void ProcessSoundUpdates();
    void UpdateMovingSounds();
    bool ProcessMessage(BSSoundMessage* apMsg);
    // Returns the list of the page's sound descriptions it builds.
    BSSimpleList<AudioDebugString*>* GetDebugString(uint32_t auiPage);
    bool SetPausedType(BSGameSound::SoundFlags auiTypes, bool abExact);

    // A singleton owned by BSAudio.
    static BSAudioManager& QInstance();
    static void operator delete(void* apMemory);

    int32_t iMessageListIndex;
    int32_t iMessageProcessingListIndex;
    // The two message lists Update swaps between.
    SoundMessageList Messages[2];
    SoundMessageStack FreeMessages;
    SoundMessageStack FreeQueueMessages;
    // The messages the audio thread has finished with (~BSAudioManager clears all three stacks).
    SoundMessageStack GarbageMessages;
    // The sounds that are playing, by sound ID.
    NiTPointerMap<int32_t, BSGameSound*> SoundList;
    // The playing sounds' info by sound ID; IsValid looks sounds up in it.
    NiTPointerMap<int32_t, BSSoundInfo*> SoundStateMap;
    NiTPointerMap<int32_t, BSSoundInfo*> StateUpdateMap;
    NiTPointerMap<int32_t, NiPointer<NiAVObject>> MovingRefrList;
    NiTPointerList<BSGameSound*> CachedSoundList;
    uint32_t iCurrentCachSize;
    uint32_t iWeatherFlags;
    float fCurrentGamehour;
    // The message, state, cache, processing and cache-load-task critical sections (0x18 bytes each on this ABI).
    _RTL_CRITICAL_SECTION MessageCritSection;
    _RTL_CRITICAL_SECTION StateCritSection;
    _RTL_CRITICAL_SECTION CacheCritSection;
    _RTL_CRITICAL_SECTION ProcessingCritSection;
    _RTL_CRITICAL_SECTION CacheLoadTaskCritSection;
    // The audio thread's tasks.
    NiTPointerList<AudioLoadTask*> CacheLoadTaskList;
    uint32_t iDelayTimerDelta;
    bool bPrecacheCompleted;
    bool bListenerMoved;
    uint32_t dwAudioThreadID;
    // The audio thread StartAudioThread creates.
    BSAudioManagerThread* pUpdateThread;
    float fVolumes[12];
    uint32_t uiToFadeFlags;
    bool bDialogueFadeOut;
    bool bListenerUnderwater;
    MutedSoundType* pMutedSoundTypes;
    uint32_t uiMutedTypesCount;
    uint32_t uiSoundCacheIndex;
    bool bIgnoreGlobalTimeMultiplier;
};

// A sound's ID, frequency, volume, type, duration and playing flag, with its debug info and a data block of path,
// attenuation and position.
class BSSoundInfo {
public:
    // The sound's path, attenuation and position data.
    struct Data {
        Data();

        void Free();
        // Copies apSource's paths, distances and position over this record and returns this. `this` is the debug-info
        // record itself, not the sound.
        Data* SetDebugInfo(Data* apSource);

        // The name buffer SetDebugInfo copies into.
        char* pName;
        char cName[260];
        // The file name buffer SetDebugInfo copies into.
        char* pFilePath;
        char cFilePath[260];
        float fMinAttenuationDist;
        float fMaxAttenuationDist;
        NiPoint3 kPosition;
        uint32_t uiStateFlags;
        float fBeginTime;
        float fEndTime;
        uint32_t uiStaticAttenuation;
        uint32_t uiDistanceAttenuation;
        uint32_t uiFaderAttenuation;
    };

    BSSoundInfo();
    // The members are assigned in the body, after kDebugInfo is constructed; the frequency and volume are left unset.
    explicit BSSoundInfo(uint32_t aiID) {
        iID = aiID;
        uiSoundType = 0;
        uiDuration = 0;
        bIsPlaying = false;
    }
    BSSoundInfo(BSSoundInfo* si);
    ~BSSoundInfo();

    BSSoundInfo* Copy(BSSoundInfo* si);
    BSSoundInfo::Data* CopyData(BSSoundInfo::Data* apResult);

    uint32_t iID;
    float fFrequency;
    float fVolume;
    uint32_t uiSoundType;
    uint32_t uiDuration;
    bool bIsPlaying;
    Data kDebugInfo;
};

// One queued audio-thread command: the command, a sound ID, up to three kinds of payload, and a vector or curve.
class BSSoundMessage {
public:
    BSSoundMessage();
    ~BSSoundMessage();

    // Copies the message's object into aspAVObject.
    void SPData(NiPointer<NiAVObject>& aspAVObject) {
        aspAVObject = spData;
    }

    // Copies arOther into apThis the way the compiler-generated operator= does: the leading members, spData through its
    // operator=, then both views of the union (three floats, five words); returns apThis. SetAttenuationCurve and
    // ProcessMessage call it. It is not an operator= because a user-declared one would change every copy of a
    // BSSoundMessage.
    static BSSoundMessage* Assign(BSSoundMessage* apThis, const BSSoundMessage& arOther);

    SOUND_MSG msg;
    int32_t iID;
    intptr_t iData;
    void* pData;
    NiPointer<NiAVObject> spData;

    union {
        float vector3[3];
        uint16_t curve[5];
    };
};

inline int32_t Singleton_guard;

extern SettingT<INIPrefSettingCollection> gs_fDefaultMasterVolume_Audio;

extern SettingT<INIPrefSettingCollection> gs_fDefaultFootVolume_Audio;

extern SettingT<INIPrefSettingCollection> gs_fDefaultVoiceVolume_Audio;

extern SettingT<INIPrefSettingCollection> gs_fDefaultEffectsVolume_Audio;

extern SettingT<INIPrefSettingCollection> gs_fDefaultMusicVolume_Audio;

extern SettingT<INIPrefSettingCollection> gs_fDefaultRadioVolume_Audio;

extern SettingT<INISettingCollection> g_bUseAudioDebugInformation_Audio;

extern SettingT<INISettingCollection> gs_fAudioDebugDelay_Audio;

extern SettingT<INISettingCollection> gs_fMasterVolumeMult;

// File-local settings of BSAudioManager.cpp, in the Audio section of the INI.
extern SettingT<INISettingCollection> fDialogueFadeDecibels;
extern SettingT<INISettingCollection> fDialogueFadeSecondsIn;
extern SettingT<INISettingCollection> fDialogueFadeSecondsOut;

extern SettingT<INISettingCollection> gs_bEnableAudioCache_Audio;

extern SettingT<INISettingCollection> gs_iAudioCacheSize_Audio;

extern SettingT<INISettingCollection> gs_iMaxSizeForCachedSound;

extern SettingT<INISettingCollection> gs_iAudioHWThread_Audio;
