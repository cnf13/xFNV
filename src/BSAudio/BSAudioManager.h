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
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class AudioLoadTask;
class BSAudioManagerThread;
class BSSoundInfo;
class BSSoundMessage;
class TESSound;

// A sound ID with its debug description, listed and sorted by BSAudioManager::DebugPrintSounds (8 bytes: its
// allocations there; the sort's compare 0xAE5FB0 reads iSoundID unsigned).
class AudioDebugString {
public:
	uint32_t iSoundID;
	char* pString;
};

// DebugPrintSounds' BSSimpleList<AudioDebugString*>::Sort compare: orders the strings by iSoundID, unsigned.
int32_t Compare_AE5FB0(AudioDebugString* apA, AudioDebugString* apB);
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
	// A sound type that is muted and the state of its mute. IDA-only name; not in the PDB.
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
	// Makes apHandle refer to the sound of form auiFormID.
	BSSoundHandle* GetSoundHandleByNumericID(BSSoundHandle* apHandle, uint32_t auiFormID, uint32_t aiTypeFlags);
	// Makes apResult refer to the sound of the file apFilename.
	BSSoundHandle* GetSoundHandleByFilePath(BSSoundHandle* apResult, const char* apFilename, uint32_t aeTypeFlags,
		TESSound* apSound);
	// Returns an empty handle through apResult (thiscall, ret 0x1C, 0xAE5F70; no caller in the exe).
	BSSoundHandle* SpawnSoundReference(BSSoundHandle* apResult, int32_t a2, int32_t a3, int32_t a4, int32_t a5,
		int32_t a6, int32_t a7);
	// Loads the sound of form auiFormID ahead of use.
	void PrecacheSoundFromID(uint32_t auiFormID, int32_t aiFlags);
	// Moves the end of the beam sound aiSoundID.
	bool SetBeamEnd(uint32_t aiSoundID, float afX, float afY, float afZ);
	// Runs apfnCallback with apSoundContext when the sound aiSoundID finishes; false when it
	// is unknown.
	bool SetCompletionCallback(uint32_t aiSoundID, CompletionCallback apfnCallback, void* apSoundContext);
	// Runs apfnCallback with apSoundContext when the sound aiSoundID starts; false when it is
	// unknown.
	bool SetPlayCallback(uint32_t aiSoundID, PlayCallback apfnCallback, void* apSoundContext);
	// IDA name FadeTo; the PDB name is used.
	// Fades the sound auiSound out over auiMilliseconds and then runs auiFollowUpAction.
	void FadeOut(uint32_t aiID, uint32_t aiMilliseconds, uint32_t aiFollowUpAction);
	// The sound type of aiSound.
	uint32_t GetSoundType(uint32_t aiSoundID);
	void StartAudioThread();
	void SetIgnoreGlobalTimeMultiplier(bool bIgnore);
	BSSoundHandle* GetSoundHandleByName(BSSoundHandle* apHandle, const char* apEditorID, uint32_t aiTypeFlags);
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
	void SetAttenuationCurve(uint32_t aiSoundID, uint16_t aiPt1, uint16_t aiPt2, uint16_t aiPt3, uint16_t aiPt4,
		uint16_t aiPt5);
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

	// Queues a command for the audio thread; the original implementation uses thread-local storage and the
	// message critical section, so it is not reconstructed. aspSmartPointerData is handed to the audio thread,
	// which releases its reference: callers only pass a null pointer or an object they already hold.
	void ComposeMessage(SOUND_MSG aMsg, uint32_t aiID, intptr_t aiData, void* apPointerData,
		NiPointer<NiAVObject> aspSmartPointerData, NiPoint3 aVector);
	void StopMovingSounds(NiAVObject* apObj, float afFadeSeconds, uint32_t aiFlags);
	bool SetBalance(uint32_t aiSoundID, int32_t aiNewBalance);
	void SetMinMax(uint32_t aiSoundID, float afMin, float afMax);
	bool SetOrientation(uint32_t aiSoundID, float afX, float afY, float afZ);
	BSSoundMessage* GetFreeMessage();
	BSSoundMessage* GetFreeQueueMessage();
	bool IsPausedType(uint32_t aeFlags);
	bool ClearPausedType(uint32_t auiTypes, bool abExact);
	// Inferred type: a1: char* from 0xadd79c (passed to strcpy_s 2)
	void PrecacheSound(char* a1, uint32_t a2, TESSound* a3);
	// Whether sounds are cached.
	bool IsEnableAudioCache();
	void SetCacheEnabled(bool abEnableCache);
	// Inferred type: auiNewSize: uint32_t: unsigned uses from 0xae6c10
	void SetAudioCacheSizeInK(uint32_t auiNewSize);
	bool IsValid(uint32_t aiSoundID);
	uint32_t AudioCacheSize();
	float GetCorrectedVolume(float afBaseVolume, uint32_t aiTypeFlags);
	bool IsPlaying(uint32_t aiSoundID);
	bool IsInCache(uint32_t aiSoundHash, bool abFuzzy);
	void RemoveLoadTask(AudioLoadTask* a1);

protected:
	void Process();

public:
	// IDA name; not in the PDB. The cache size AudioCacheSize reads, returned in eax (thiscall, ret 0, 0xAD8270).
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
	// IDA name; not in the PDB. Returns the list of the page's sound descriptions it builds
	// (0xAE5FF0 returns the list pointer in eax; BSAudio::GetDebugString returns it as BSSimpleList<AudioDebugString*>*).
	BSSimpleList<AudioDebugString*>* GetDebugString(uint32_t auiPage);
	bool SetPausedType(BSGameSound::SoundFlags auiTypes, bool abExact);

	// A singleton owned by BSAudio. IDA names the accessor GetSingleton; it is not reconstructed here because it
	// installs an exit handler, so this forwards to the original.
	static BSAudioManager& QInstance();
	static void operator delete(void* apMemory);

	int32_t iMessageListIndex;
	int32_t iMessageProcessingListIndex;
	// PDB name (IDA kMessages): the two message lists Update swaps between (0xAD90E0 constructs both).
	SoundMessageList Messages[2];
	SoundMessageStack FreeMessages;
	SoundMessageStack FreeQueueMessages;
	// The messages the audio thread has finished with (~BSAudioManager clears all three stacks).
	SoundMessageStack GarbageMessages;
	// PDB name (IDA kPlayingSounds): the sounds that are playing, by sound ID.
	NiTPointerMap<int32_t, BSGameSound*> SoundList;
	// PDB name (IDA kPlayingSoundInfos): the playing sounds' info by sound ID (0xAD90E0 constructs a
	// NiTPointerMap<int,BSSoundInfo*> here; IsValid 0xADC7D0 looks sounds up in it).
	NiTPointerMap<int32_t, BSSoundInfo*> SoundStateMap;
	NiTPointerMap<int32_t, BSSoundInfo*> StateUpdateMap;
	NiTPointerMap<int32_t, NiPointer<NiAVObject>> MovingRefrList;
	// PDB name (IDA kCachedSounds): the sounds the cache holds.
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
	// PDB name (IDA kTaskList): the audio thread's tasks.
	NiTPointerList<AudioLoadTask*> CacheLoadTaskList;
	uint32_t iDelayTimerDelta;
	bool bPrecacheCompleted;
	bool bListenerMoved;
	uint8_t padding136[2];
	uint32_t dwAudioThreadID;
	// PDB name (IDA the same): the audio thread StartAudioThread creates.
	BSAudioManagerThread* pUpdateThread;
	float fVolumes[12];
	uint32_t uiToFadeFlags;
	bool bDialogueFadeOut;
	bool bListenerUnderwater;
	uint8_t padding176[2];
	// IDA names; not in the PDB.
	MutedSoundType* pMutedSoundTypes;
	uint32_t uiMutedTypesCount;
	uint32_t uiSoundCacheIndex;
	bool bIgnoreGlobalTimeMultiplier;
	uint8_t padding185[0x3];
};

// A sound's ID, frequency, volume, type, duration and playing flag, with its debug info and a data block of path,
// attenuation and position.
class BSSoundInfo {
public:
	// The sound's path, attenuation and position data.
	struct Data {
		Data();

		void Free();
		// Copies apSource's paths, distances and position over this record and returns this. IDA and the PDB record
		// it as BSSoundInfo::SetDebugInfo, but the body's `this` is the debug-info record itself (its offsets are
		// this struct's, the PDB's BSDebugSoundInfo), not the sound (thiscall, ret 4, 0xAE88C0).
		Data* SetDebugInfo(Data* apSource);

		// IDA name; not in the PDB. The name buffer SetDebugInfo copies into (the PDB's BSDebugSoundInfo::pName).
		char* dword0;
		char cPath04[260];
		// The file name buffer SetDebugInfo copies into (BSDebugSoundInfo::pFileName).
		char* pFilePath;
		char cFilePath[260];
		float fMinAttenuationDist;
		float fMaxAttenuationDist;
		NiPoint3 kPosition;
		uint32_t dword224;
		float float228;
		float float22C;
		uint32_t uiStaticAttenuation;
		uint32_t uiDistanceAttenuation;
		uint32_t uiFaderAttenuation;
	};

	BSSoundInfo();
	BSSoundInfo(BSSoundInfo* si);
	~BSSoundInfo();

	BSSoundInfo* Copy(BSSoundInfo* a2);
	BSSoundInfo::Data* CopyData(BSSoundInfo::Data* a1);

	uint32_t iID;
	float fFrequency;
	float fVolume;
	uint32_t uiSoundType;
	uint32_t uiDuration;
	bool bIsPlaying;
	uint8_t padding15[3];
	Data kDebugInfo;
};

// One queued audio-thread command: the command, a sound ID, up to three kinds of payload, and a vector or curve.
class BSSoundMessage {
public:
	BSSoundMessage();
	~BSSoundMessage();

	// Copies the message's object into aspAVObject. The exe has no copy of its own: it inlines it, leaving NiPointer's
	// assignment (0x6E5CC0) on aspAVObject and &spData where the debug build calls SPData (FreeMessage 0xADAD80,
	// FreeQueueMessage 0xADAFA0).
	void SPData(NiPointer<NiAVObject>& aspAVObject) { aspAVObject = spData; }

	// The PDB's compiler-generated operator=(__that), out of line: the four dwords, spData
	// through operator= 0x6E5CC0, then both views of the union (three floats, five words); returns apThis
	// (thiscall, ret 4, 0xAE7990). SetAttenuationCurve 0xADBB30 and ProcessMessage 0xADF080 call it. Not operator=:
	// a user-declared one would change every copy of a BSSoundMessage in src.
	// Inferred type: apThis: BSSoundMessage* from 0xae0102 (result of BSAudioManager::GetFreeQueueMessage), its members
	// used at 0xae7990; arOther: BSSoundMessage* from 0xae0102 (parameter 0 of BSAudioManager::ProcessMessage), its
	// members used at 0xae79a0
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
inline uint32_t data_11C4B50;

// The manager's own vtable (~BSAudioManager stores it first; the engine's table holds only the deleting destructor).
inline constexpr fnvd::Vtable BSAudioManager_vftable{};

// INIPrefSettingCollection: 0xfb7199 calls 0x4de4a0.
extern SettingT<INIPrefSettingCollection> gs_fDefaultMasterVolume_Audio;

// INIPrefSettingCollection: 0xfb723d calls 0x4de4a0.
extern SettingT<INIPrefSettingCollection> gs_fDefaultFootVolume_Audio;

// INIPrefSettingCollection: 0xfb72dd calls 0x4de4a0.
extern SettingT<INIPrefSettingCollection> gs_fDefaultVoiceVolume_Audio;

// INIPrefSettingCollection: 0xfb7379 calls 0x4de4a0.
extern SettingT<INIPrefSettingCollection> gs_fDefaultEffectsVolume_Audio;

// INIPrefSettingCollection: 0xfb741d calls 0x4de4a0.
extern SettingT<INIPrefSettingCollection> gs_fDefaultMusicVolume_Audio;

// INIPrefSettingCollection: 0xfb74bd calls 0x4de4a0.
extern SettingT<INIPrefSettingCollection> gs_fDefaultRadioVolume_Audio;

// INISettingCollection: 0xfb7558 calls 0x44f570.
extern SettingT<INISettingCollection> g_bUseAudioDebugInformation_Audio;

// INISettingCollection: 0xfb75f9 calls 0x44f570.
extern SettingT<INISettingCollection> gs_fAudioDebugDelay_Audio;

// INISettingCollection: its initializer 0xFB7650 stores that collection's vtable and adds it there.
extern SettingT<INISettingCollection> gs_fMasterVolumeMult;

// The PDB's file-local settings of BSAudioManager.cpp; IDA's gs_fDialogueFade*:Audio.
// INISettingCollection: their initializers 0xFB76F0, 0xFB7790 and 0xFB7830 store its vtable.
extern SettingT<INISettingCollection> fDialogueFadeDecibels;
extern SettingT<INISettingCollection> fDialogueFadeSecondsIn;
extern SettingT<INISettingCollection> fDialogueFadeSecondsOut;

// INISettingCollection: 0xfb7a58 calls 0x44f570.
extern SettingT<INISettingCollection> gs_bEnableAudioCache_Audio;

// INISettingCollection: 0xfb7afb calls 0x44f570.
extern SettingT<INISettingCollection> gs_iAudioCacheSize_Audio;

// INISettingCollection: 0xfb7b9b calls 0x44f570.
extern SettingT<INISettingCollection> gs_iMaxSizeForCachedSound;

// INISettingCollection: 0xfb7c3b calls 0x44f570.
extern SettingT<INISettingCollection> gs_iAudioHWThread_Audio;
