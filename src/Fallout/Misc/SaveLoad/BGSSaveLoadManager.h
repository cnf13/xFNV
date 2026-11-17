#pragma once

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSSystemUtilities/BSSaveDataSystemUtility.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSSaveLoadFile;
class BGSSaveLoadFileEntry;
class BSSaveDataSystemUtilityFile;
template <class T> class BSSimpleList;

// Prints the formatted text through the CRT's vprintf (cdecl, variadic, 0x84CBD0).
int32_t BSprintf(const char* apFormat, ...);
// Not named in the PDB or IDA; name chosen here. BSprintf as Havok's error report function (hkErrorReportFunction):
// bhkWorld's InitSystem hands hkBaseSystem::init the exe's BSprintf itself, which prints the report's text as its
// format.
void BSprintf_ErrorReport(const char* apText, void* apErrorReportObject);
void LoadGameMissingContentCallBack();

// Lists, writes and loads the save games.
class BGSSaveLoadManager {
public:
	// What a save file is opened for: OpenSaveFile gives a load a 2 MB buffer, the others 64 KB.
	enum SAVEFILE_TYPE : int32_t {
		SAVEFILE_SCREENSHOT = 0x0,
		SAVEFILE_LOAD = 0x1,
		SAVEFILE_SAVE = 0x2,
	};

	BGSSaveLoadManager();
	~BGSSaveLoadManager();

	// Runs the queued saves.
	void UpdateQueuedSaves();
	// Loads the most recent save game when there is one.
	bool LoadMostRecentSaveGame();
	// Writes the autosave.
	void Autosave();
	// Writes the system save.
	int8_t SystemSave();
	// Cleans apFileName of characters a file name cannot hold; returns its length.
	void ScrubFileName(char* apFileName);
	// Whether apSaveFileName names a quick save.
	bool IsSaveFileNameQuicksave(const char* apSaveFileName);
	// Writes the player's play time into apPlayTimeString (260 bytes) as hours.minutes.seconds, the hours three
	// digits wide when abDisplayOnly.
	void GetPlayTimeString(char* apPlayTimeString, bool abDisplayOnly);
	// Whether apSaveFileName names a full save.
	// Inferred type: apSaveFileName: char* from 0x846723 (BGSSaveLoadFileEntry::pFileName (this of
	// BGSSaveLoadFileEntry::SaveGame))
	bool IsSaveFileNameGenerated(const char* apSaveFileName);
	// Opens the save file apFileName for reading, or for writing when abWrite, with a buffer sized for aeType.
	BGSSaveLoadFile* OpenSaveFile(const char* apFileName, bool abWrite, SAVEFILE_TYPE aeBufferType, int32_t aiDeviceID);
	// Closes and frees apFile; when written, rotates the .bak backups and renames the written file into place.
	void CloseSaveFile(BGSSaveLoadFile* apFile, bool abWrite);
	// Whether apSaveFileName names an autosave.
	bool IsSaveFileNameAutosave(const char* apSaveFileName);
	// Whether apSaveFileName names a system save.
	// Inferred type: apSaveFileName: char* from 0x851ae7 (parameter 0 of BGSSaveLoadManager::IsSaveFileNameCustom)
	bool IsSaveFileNameSystemSave(const char* apSaveFileName);
	// Whether a most recent save game is known.
	bool HasMostRecentSaveGame();
	// Writes a save game to apFileName (a new numbered save when null) on device aiDeviceID; true on success.
	bool SaveGame(const char* apFileName, int32_t aiDeviceID, bool abOutputStats);
	// Writes the save directory into apDirectoryPath.
	void GetSaveDirectoryPath(char* apDirectoryPath);
	// True when the game may be saved now.
	bool IsSavingAllowed(bool abAutosave);
	// Sets pSysUtilSaveCompleteCallback (a setter the exe shares).
	void SetSystemUtilitySaveCompleteCallback(void (*apCallback)(BSSaveDataSystemUtility::RESULT,
		BSSaveDataSystemUtilityFile*&));
	// Sets pSysUtilLoadCompleteCallback (a setter the exe shares with other classes' +0x2C fields).
	void SetSystemUtilityLoadCompleteCallback(void (*apCallback)(BSSaveDataSystemUtility::RESULT,
		BSSaveDataSystemUtilityFile*&));
	// The save format's version, 48: the exe calls a shared "return 0x30" (thiscall, ret 0).
	int32_t GetMajorVersion();
	int32_t GetMinorVersion();
	// Writes the full path of the save apFileName to apFullFileName.
	void GetFullFileName(const char* apFileName, char* apFullFileName);
	// Writes the game's version text, at most aiSize bytes, to apVersionInfo.
	void GetVersionInfo(char* apVersionInfo, uint32_t aiSize);
	// Readies the manager for a load (thiscall, ecx unused, ret 0, 0x850C20).
	void StartLoadGame();
	// Shows the quick save message and saves to "quicksave"; true on success.
	void Quicksave();
	// Shows the quick load message and loads "quicksave"; true on success.
	bool Quickload();
	// Loads the save game apFileName from device aiDeviceID; true on success.
	bool LoadGame(const char* apFileName, int32_t aiDeviceID, bool abOutputStats, bool abIgnoreMissingContent);
	bool IsLoadingGame();
	void SetAutoSaveTimerTo1();
	void SetForceSaveTimer();
	void SetShouldSystemSave();
	// Whether a save can go to device aiDeviceID; the exe's body is the shared "return true"
	// (thiscall, ret 4, 0x401290).
	bool CanSave(int32_t aiDeviceID);
	bool IsSaveFileNameCustom(const char* apSaveFileName);
	// Deletes the save file apFileName on device aiDeviceID; nonzero on success.
	void DeleteSaveFile(const char* apFileName, int32_t aiDeviceID);
	// Removes apFileEntry from the save game list and deletes it.
	void RemoveSaveGame(BGSSaveLoadFileEntry* apFileEntry);
	// Name not in the PDB (the field at +0xC); folded accessor, IDA: InterfaceManager::GetCurrentMode.
	bool GetSimulatedMode();
	// Deletes the save game list and every entry.
	void DeleteSaveGameList();
	void SetMostRecentSaveGame(const char* apSaveFileName, int32_t aiDeviceID);
	void GetFullPath(const char* apFileName, char* apFullPath, bool abWrite);
	bool LoadGameHeader(BGSSaveLoadFile* apFile);
	// IDA name; not in the PDB (the Xbox build's counterpart is BuildSaveGameList). Rebuilds the save game list
	// from the save directory (thiscall, 0x851330). Hex-Rays: char result; no caller reads eax after the call.
	void SetupSavesList();
	void AddSaveGame(const char* apFileName, int32_t aiDeviceID, uint32_t aiSaveGameNumber);
	void GenerateSaveFileName(char* apSaveFileName, bool abDisplayOnly, bool abAutosave);

protected:
	void StartSaveGame();
	void EndSaveGame();
	void SaveGameHeader(BGSSaveLoadFile* apFile);

public:
	void EndLoadGame();
	void CopySaveGames(bool bFromMemoryStick);
	void GetPlayerLocationName(char* apLocationName);

	// Deletes pSaveLoadManager and clears it.
	static void Destroy();
	// Writes the system save name to apDest.
	// Inferred type: apDest: char* from 0x850b61 (passed to BSsprintf 0)
	static void CreateSystemSaveName(char* apDest);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void Create();
	static void SysUtilSaveCompleteCallback(BSSaveDataSystemUtility::RESULT aeResult,
		BSSaveDataSystemUtilityFile*& apSaveDataFile);
	static void SysUtilDeleteCompleteCallback(BSSaveDataSystemUtility::RESULT aeResult);

	// The save game list: AddSaveGame adds an entry to it, DeleteSaveGameList deletes it and every entry.
	BSSimpleList<BGSSaveLoadFileEntry*>* pSaveGameList;
	uint32_t iSaveGameCount;
	uint32_t iCurrentSaveGameNumber;
	// Whether the game runs in simulated mode.
	bool bSimulatedMode;
	// Whether autosaves are turned off because there is no disk space for them.
	bool bAutosaveDisabledForDiskspace;
	bool bhandledCorrupt;
	uint8_t cQueuedAutosave;
	uint8_t cQueuedForceSave;
	uint8_t cSystemSaveTimer;
	uint8_t padding12[0x2];
	char* pMostRecentSaveGame;
	int32_t iMostRecentSaveGameDeviceID;
	bool bSysUtilHasSaveData;
	uint8_t padding1D[3];
	// The save file being written or read.
	BGSSaveLoadFile* pSaveLoadFile;
	bool bReturnedFromSysUtil;
	// Whether the system utility has finished handing the loaded save data over.
	bool bSysUtilLoadDataComplete;
	bool bStartMenuLoading;
	uint8_t padding27[1];
	// Called with the result and the save data file when the system utility finishes a save or a load
	// (SysUtilSaveCompleteCallback, 0x852740, calls the save one so).
	void (*pSysUtilSaveCompleteCallback)(BSSaveDataSystemUtility::RESULT, BSSaveDataSystemUtilityFile*&);
	void (*pSysUtilLoadCompleteCallback)(BSSaveDataSystemUtility::RESULT, BSSaveDataSystemUtilityFile*&);
	// IDA name; not in the PDB (the Xbox class ends at 0x30). endGame_MSGBox 0x75F750 matches its text against the
	// saves' player names; the destructor 0x84FE50 destroys it.
	BSStringT<char> pSaveName;
};

// The save/load manager singleton.
inline BGSSaveLoadManager* pSaveLoadManager;

// The game settings BGSSaveLoadManager.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bDisplayMissingContentDialogue;
extern SettingT<INISettingCollection> sSaveGameSafeMarkerID;
extern SettingT<INISettingCollection> bAllowProfileTransfer;
extern SettingT<INISettingCollection> bAllowScriptedAutosave;
extern SettingT<INISettingCollection> bAllowScriptedForceSave;
extern SettingT<INISettingCollection> iSaveGameBackupCount;
extern SettingT<INISettingCollection> bOutputSaveGameScreenshot;
extern SettingT<INISettingCollection> bCopySaveGameToHostOrMemStick;
inline int32_t aiDeviceID = -1;
inline int32_t IntListBox_NumUnfiltered_iUT_num_filtered;

// Hex-Rays: char; its one writer (LoadGame, 0x850760) stores LoadGame's bool abOutputStats.
inline bool bLoadGameMissingContentOutputStats;
using g_sMissingContentSaveName_t = char[260];
inline g_sMissingContentSaveName_t g_sMissingContentSaveName{};
