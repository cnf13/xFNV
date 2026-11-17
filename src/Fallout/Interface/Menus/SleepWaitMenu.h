#pragma once

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "Reconstruction.hpp"

class SleepWaitMenu : public Menu {
public:
	SleepWaitMenu();
	~SleepWaitMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	void DoIdle() override;
	int32_t GetClass() override;
	bool DoGamepad(int32_t, float) override;

	bool UpdateSleep();
	bool UpdateSleeping();
	void SetCurrentTime();
	// The exe's Create calls QuantityMenu's check of the tiles on the SleepWaitMenu.
	bool IsWellFormedMenu();

	// Updates the actors' 3D after the rest, drops the bed fader and fades the menu out.
	static void Close();
	static int32_t GetClassStatic();
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static Tile* Create(bool abSleep);

	Tile* pTiles[6];
	float fElapsedSecs;
	bool bRestingActive;
	bool bSleepingInBed;
	bool bButtonReleased;
	uint8_t padding47[1];
	float fPrevQuantity;

	static inline SleepWaitMenu* pMe;
	static inline int32_t iUT_CurrentValue;
};
inline constexpr fnvd::Vtable SleepWaitMenu_vftable{};

// INIPrefSettingCollection: 0xf8c80a calls 0x709f00.
extern SettingT<INIPrefSettingCollection> gs_bSaveOnWait_GamePlay;

// INIPrefSettingCollection: 0xf8c7da calls 0x709f00.
extern SettingT<INIPrefSettingCollection> gs_bSaveOnRest_GamePlay;
