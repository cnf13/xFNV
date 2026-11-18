#pragma once

#include "Reconstruction.hpp"
#include "SDK/Steam/CCallback.h"
#include "SDK/Steam/CCallbackBase.h"
#include "SDK/Steam/isteamuserstats.h"

#include <stdint.h>

class SteamAPICallbacks;

void InitSteamAPI();
void* DestroySteamAPICallbacks();

// steam_api.dll's SteamAPI_RunCallbacks.
using SteamAPI_RunCallbacks_t = void (*)();
XFNV_IMPORT(SteamAPI_RunCallbacks_t, SteamAPI_RunCallbacks, 0xFDF488);

using SteamAPI_Shutdown_t = void (*)();
XFNV_IMPORT(SteamAPI_Shutdown_t, SteamAPI_Shutdown, 0xFDF480);

using SteamAPI_IsSteamRunning_t = bool (*)();
XFNV_IMPORT(SteamAPI_IsSteamRunning_t, SteamAPI_IsSteamRunning, 0xFDF498);

// steam_api.dll's SteamAPI_Init (steam_api.h).
using SteamAPI_Init_t = bool (*)();
XFNV_IMPORT(SteamAPI_Init_t, SteamAPI_Init, 0xFDF478);

// The Steam user-stats callbacks and the Steam state WinMain configures, allocated by GetSingleton.
class SteamAPICallbacks {
public:
    SteamAPICallbacks();
    ~SteamAPICallbacks();

    // WinMain passes false for "-nocloud".
    void SetNoCloud(bool abUseCloud);
    // TESMain::OnIdle's per-frame Steam callback dispatch.
    void RunCallbacks();
    // Returns SteamUser's BLoggedOn.
    bool OnUserAuthenticated();
    // The UserStatsReceived_t handler; returns void, as the callback's func_t does.
    void OnUserStatsReceived(UserStatsReceived_t* apCallback);
    // The UserStatsStored_t handler; returns void, as the callback's func_t requires.
    void OnUserStatsStored(UserStatsStored_t* apCallback);
    void OnUserStatsStored_Achievement(UserAchievementStored_t* apCallback);

    // The Steam state, created on first use.
    static SteamAPICallbacks* GetSingleton();
    static void operator delete(void* apMemory);

    // The three CCallback members (0x14 bytes each); the constructor builds each with this as m_pObj.
    CCallback<SteamAPICallbacks, UserStatsReceived_t, false> unk00;
    CCallback<SteamAPICallbacks, UserStatsStored_t, false> unk14;
    CCallback<SteamAPICallbacks, UserAchievementStored_t, false> unk28;
    // SteamAPI_Init's result.
    bool bSteamInitialized;
    uint8_t unk3D;
    // Cleared by "-nocloud"; the constructor sets it.
    bool bUseCloud;
    uint8_t padding3F;
    // The app id SteamUtils reports.
    uint32_t uiAppID;
};

using Steam = SteamAPICallbacks;

using CCallback_SteamAPICallbacks_UserStatsReceived_t_false = CCallback<SteamAPICallbacks, UserStatsReceived_t, false>;
using CCallback_SteamAPICallbacks_UserStatsStored_t_false = CCallback<SteamAPICallbacks, UserStatsStored_t, false>;
using CCallback_SteamAPICallbacks_UserAchievementStored_t_false =
    CCallback<SteamAPICallbacks, UserAchievementStored_t, false>;

// GetSingleton stores the constructed object here.
inline SteamAPICallbacks* g_steamAPICallbacks;
