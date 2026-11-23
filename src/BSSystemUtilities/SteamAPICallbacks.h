#pragma once

#include "SDK/Steam/CCallback.h"
#include "SDK/Steam/CCallbackBase.h"
#include "SDK/Steam/isteamuserstats.h"
#include "SDK/Steam/steamclientpublic.h"

#include <stdint.h>

class SteamAPICallbacks;

void InitSteamAPI();
void* DestroySteamAPICallbacks();

// steam_api.dll's SteamAPI_RunCallbacks.
using SteamAPI_RunCallbacks_t = void (*)();
S_API void SteamAPI_RunCallbacks();

using SteamAPI_Shutdown_t = void (*)();
S_API void SteamAPI_Shutdown();

using SteamAPI_IsSteamRunning_t = bool (*)();
S_API bool SteamAPI_IsSteamRunning();

// steam_api.dll's SteamAPI_Init (steam_api.h).
using SteamAPI_Init_t = bool (*)();
S_API bool SteamAPI_Init();

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
    CCallback<SteamAPICallbacks, UserStatsReceived_t, false> kUserStatsReceived;
    CCallback<SteamAPICallbacks, UserStatsStored_t, false> kUserStatsStored;
    CCallback<SteamAPICallbacks, UserAchievementStored_t, false> kUserAchievementStored;
    // SteamAPI_Init's result.
    bool bSteamInitialized;
    // Whether the last stats received for this app came back with k_EResultOK.
    uint8_t bStatsValid;
    // Cleared by "-nocloud"; the constructor sets it.
    bool bUseCloud;
    // The app id SteamUtils reports.
    uint32_t uiAppID;
};

using Steam = SteamAPICallbacks;

using CCallback_SteamAPICallbacks_UserStatsReceived_t_false = CCallback<SteamAPICallbacks, UserStatsReceived_t, false>;
using CCallback_SteamAPICallbacks_UserStatsStored_t_false = CCallback<SteamAPICallbacks, UserStatsStored_t, false>;
using CCallback_SteamAPICallbacks_UserAchievementStored_t_false = CCallback<
    SteamAPICallbacks,
    UserAchievementStored_t,
    false>;

// GetSingleton stores the constructed object here.
inline SteamAPICallbacks* g_steamAPICallbacks;
