#pragma once

#include <stdint.h>

// The state of the game's data.
enum GAMEDATA_STATUS : int32_t {
	GAMEDATA_STATUS_NOT_READY = 0,
	GAMEDATA_STATUS_OK = 1,
	GAMEDATA_STATUS_ERROR = 2,
	GAMEDATA_STATUS_FULL = 3,
};

// The platform's game data service: checks that there is room for the game's data and creates it.
class BSGameDataSystemUtility {
public:
	BSGameDataSystemUtility();
	virtual ~BSGameDataSystemUtility();

	virtual void Initialize();
	virtual void Destroy();
	virtual void CheckSpace(int32_t aiRequestSize) {}
	virtual void DataInit(int32_t aiRequestSize) {}
	virtual void DoMessage(const char* apcMsgString) {}

	static void operator delete(void* apMemory);
	// The utility, built in a guarded static on first use.
	static BSGameDataSystemUtility* GetSingleton();

	bool bIsActive;
	bool bNewGameData;
	uint8_t padding06[2];
	GAMEDATA_STATUS eGameDataStatus_;
	// The game data and content info directories; 2 bytes of tail padding follow.
	char cGameDataDir[1055];
	char cContentInfoDir[1055];
	uint8_t padding84A[2];
	int32_t iFreeSpaceKB_;
};
