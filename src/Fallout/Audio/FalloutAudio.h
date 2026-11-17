#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSAcousticSpace;
class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class TESSound;

void PlayMysteriousStrangerMusic();
void PlaySound_MUSMysteriousStrangerA02();

// The game's audio glue: region sounds and the acoustic space the listener is in. Only the members the
// reconstructed functions use are declared.
class FalloutAudio {
public:
	using CurrentRegionSounds_t = NiTPointerMap<uint32_t, BSSoundHandle*>;

	static void ClearAcousticSpace(BGSAcousticSpace* apSpace);
	// Plays and stops the region sounds of the current cell.
	static void UpdateRegionSounds();
	static void DisableRegionSoundUpdates(bool abIn);
	static void SetNextAcousticSpace(BGSAcousticSpace* apSpace);
	// Makes apSpace the acoustic space of the listener.
	static void SetAcousticSpace(BGSAcousticSpace* apSpace);
	static void LockAcousticSpace(bool abIn);
	// Reverts the audio save data: an empty function the exe shares with
	// InitializationFlow (cdecl, 0x40FBE0).
	static void Revert(BGSLoadGameBuffer* apLoadGameBuffer);
	static void SetMainMenuMusic(bool abMainMenuMusic);
	static void MuteRadio();
	static void UnMuteRadio();
	static bool GetNewGameMusic();
	static bool IsRadioMute();
	static void SetNewGameMusic(bool abNewGameMusic);
	// The sound played when an activation is refused.
	static void SetNoActivationSound(TESSound* apNoActivateSnd);
	static void ClearNoActivationSound();
	// The no-activation sound, looked up by editor ID on first use (IDA names it GetNoActivateSound).
	static TESSound* GetNoActivationSound();
	// Saves and loads the no-activation sound.
	static void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	static void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	static bool ResolveSoundID(uint32_t aiFormID, char* apFilename, uint32_t* aiSoundTypeFlags,
		void** apSettingsContext);
	static bool ResolveSoundName(const char* apFormName, char* apFilename, uint32_t* aiSoundTypeFlags,
		void** apSettingsContext);
	static uint32_t ApplySettingsFromMasterFile(BSSoundHandle& aSoundHandle, void* apSettingsContext,
		uint32_t* aiModifierFlags);
	static bool GetLoopPoints(void* apSettingsContext, uint32_t* aiLoopStart, uint32_t* aiLoopEnd);
	static bool SoundDistanceTest(TESSound* apSound, NiPoint3 aLoc, float aMod);

	static inline bool bLockAcousticSpace;
	static inline bool bUpdateAcousticSpace;
	static inline BGSAcousticSpace* pCurrentSpace;
	// The looping ambient sound of the current acoustic space.
	static BSSoundHandle kAmbientLoop;
	// The exe constructs these with SettingT<INISettingCollection>'s float (0x44F440) and Info (0x45CFE0)
	// constructors.
	static SettingT<INISettingCollection> fMainMenuMusicVolume;
	static SettingT<INISettingCollection> iMusicSynchOverride;
	static inline TESSound* pNoActivationSound;
	static inline bool bRadioMute;
	static inline bool bNewGameMusic;
	static inline bool bRegionUpdateDisabled;
	static inline bool bMainMenuMusic;
	static inline BGSAcousticSpace* pCurrentCellSpace;
	static inline int32_t iCurrentTime;
	static inline bool bUseWalla;
	static inline int32_t uiLastRegionUpdate;
	static CurrentRegionSounds_t CurrentRegionSounds;
	// IDA: TESAudio::kUnderwaterAmbiance.
	static BSSoundHandle UnderwaterLoop;
	static SettingT<INISettingCollection> iMusicTransitionInMS;
};

// INISettingCollection: 0xf9dcae calls 0x44f440.
extern SettingT<INISettingCollection> gs_fRegionLoopFadeOutTime_Audio;

// INISettingCollection: 0xf9dc52 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fASFadeStartTime_Audio;

// INISettingCollection: 0xf9dc82 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fRegionLoopFadeInTime_Audio;

// INISettingCollection: 0xf9dbf2 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fASFadeInTime_Audio;

// INISettingCollection: 0xfb6e4d calls 0x44f570.
extern SettingT<INISettingCollection> gs_fDawnTime_Audio;

// INISettingCollection: 0xfb6eed calls 0x44f570.
extern SettingT<INISettingCollection> gs_fDayTime_Audio;

// INISettingCollection: 0xfb6f8d calls 0x44f570.
extern SettingT<INISettingCollection> gs_fDuskTime_Audio;

// INISettingCollection: 0xfb702d calls 0x44f570.
extern SettingT<INISettingCollection> gs_fNightTime_Audio;
