#pragma once

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

class NiTexture;

// One save game file in the save/load list: its name, header data and screenshot.
class BGSSaveLoadFileEntry {
public:
	BGSSaveLoadFileEntry(char* apFileName, int32_t aiDeviceID);
	~BGSSaveLoadFileEntry();

	// Saves the current game over this entry: for an autosave or full save, a new numbered save on its device, then
	// deletes its file; otherwise saves to its file name, then the manager removes and deletes this entry. True on
	// success.
	bool SaveGame();
	// Deletes this entry's save file, then removes the entry from the manager's list.
	void DeleteGame();
	void LoadData();
	// The save file's last write time.
	void GetFileTime(_FILETIME& arFileTime);
	void SetLoaded();
	uint32_t GetPlayerLevel();
	bool LoadGame();
	bool IsCorrupt();
	uint32_t GetScreenshotWidth();
	uint32_t GetScreenshotHeight();
	uint32_t GetScreenshotOffset();
	// The save's screenshot as a texture, or null; loads the header first when it is not loaded yet.
	NiTexture* GetScreenshot();
	const char* GetPlayerName();
	const char* GetPlayTime();
	const char* GetLocation();
	const char* GetPlayerTitle();
	uint32_t GetSaveGameNumber();
	bool IsQuicksave();
	bool IsAutosave();
	bool IsSystemSave();
	// Records the save file's last write time in FileTime when it is not set yet.
	void CalculateFileTime();
	// Copies Src into pcLanguage; callers pass "ENGLISH".
	int32_t SetVersionInfo(const char* Src);

	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static int32_t Compare(BGSSaveLoadFileEntry* a1, BGSSaveLoadFileEntry* a2);

	char* pFileName;
	bool bLoaded;
	bool bCorrupt;
	uint8_t padding06[2];
	uint32_t iSaveGameNumber;
	char* pPlayerName;
	char* pPlayerTitle;
	char* pLocation;
	char* pPlayTime;
	uint32_t iPlayerLevel;
	uint32_t iScreenshotWidth;
	uint32_t iScreenshotHeight;
	uint32_t iScreenshotOffset;
	_FILETIME FileTime;
	int32_t iDeviceID;
	uint32_t iVersion;
	// The version text, stored inline.
	char pcLanguage[64];
};
