#pragma once

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"

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
    // Create calls QuantityMenu's check of the tiles on the SleepWaitMenu.
    bool IsWellFormedMenu();

    // Updates the actors' 3D after the rest, drops the bed fader and fades the menu out.
    static void Close();
    static int32_t GetClassStatic();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static Tile* Create(bool abSleep);

    Tile* pTiles[6];
    float fElapsedSecs;
    bool bRestingActive;
    bool bSleepingInBed;
    bool bButtonReleased;
    float fPrevQuantity;

    static inline SleepWaitMenu* pMe;
    static inline int32_t iUT_CurrentValue;
};

extern SettingT<INIPrefSettingCollection> gs_bSaveOnWait_GamePlay;

extern SettingT<INIPrefSettingCollection> gs_bSaveOnRest_GamePlay;
