#include "BSSystemUtilities/SteamAPICallbacks.h"

#include "BSCore/NewOverloads.h"
#include "SDK/Steam/CCallback.h"
#include "SDK/Steam/isteamuser.h"
#include "SDK/Steam/isteamuserstats.h"
#include "SDK/Steam/isteamutils.h"

#include <new>

// Assumes CCallback's native constructor CCallback(T* apObj, func_t aFunc).
SteamAPICallbacks::SteamAPICallbacks()
  : kUserStatsReceived(
        this,
        [](SteamAPICallbacks* apObj, UserStatsReceived_t* apParam) { apObj->OnUserStatsReceived(apParam); }
    ),
    kUserStatsStored(
        this,
        [](SteamAPICallbacks* apObj, UserStatsStored_t* apParam) { apObj->OnUserStatsStored(apParam); }
    ),
    kUserAchievementStored(
        this,
        [](SteamAPICallbacks* apObj, UserAchievementStored_t* apParam) {
            apObj->OnUserStatsStored_Achievement(apParam);
        }
    ),
    bSteamInitialized(false), bStatsValid(0), bUseCloud(true), uiAppID(0) {
#if XFNV_NO_STEAM
    // With Steam switched off, SteamAPI_Init is not called, so the game runs as it does when Steam is not there (no
    // stats, achievements or callbacks).
#else
    bSteamInitialized = SteamAPI_Init();
    if (bSteamInitialized) {
        bool bLoggedOn = OnUserAuthenticated();
        if (SteamUtils()) {
            uiAppID = SteamUtils()->GetAppID();
        }
        if (bLoggedOn) {
            if (SteamUserStats()) {
                SteamUserStats()->RequestCurrentStats();
            }
        }
    }
#endif
}

void InitSteamAPI() {
    SteamAPICallbacks::GetSingleton();
}

// Deletes the Steam state and clears the singleton; returns the deleted object's address, or the null singleton.
void* DestroySteamAPICallbacks() {
    SteamAPICallbacks* pCallbacks = g_steamAPICallbacks;
    void* pResult = pCallbacks;
    if (pCallbacks) {
        delete pCallbacks;
    }
    g_steamAPICallbacks = nullptr;
    return pResult;
}

// The three callbacks are destroyed by CCallback's own destructor.
SteamAPICallbacks::~SteamAPICallbacks() {
    SteamAPI_Shutdown();
}

void SteamAPICallbacks::SetNoCloud(bool abUseCloud) {
    bUseCloud = abUseCloud;
}

void SteamAPICallbacks::RunCallbacks() {
    SteamAPI_RunCallbacks();
}

bool SteamAPICallbacks::OnUserAuthenticated() {
    if (!bSteamInitialized || !SteamUser()) {
        return false;
    }
    return SteamUser()->BLoggedOn();
}

// The CGameID of the app id (uiAppID in the low 32 bits, 0 above), compared with the result's game id.
void SteamAPICallbacks::OnUserStatsReceived(UserStatsReceived_t* apCallback) {
    uint64_t ulGameID = uiAppID;
    if (ulGameID == apCallback->m_nGameID) {
        bStatsValid = apCallback->m_eResult == 1;
    }
}

void SteamAPICallbacks::OnUserStatsStored([[maybe_unused]] UserStatsStored_t* apCallback) {}

void SteamAPICallbacks::OnUserStatsStored_Achievement([[maybe_unused]] UserAchievementStored_t* apCallback) {}

SteamAPICallbacks* SteamAPICallbacks::GetSingleton() {
    if (!g_steamAPICallbacks) {
        SteamAPICallbacks* pCallbacks;
        pCallbacks = new SteamAPICallbacks;
        g_steamAPICallbacks = pCallbacks;
    }
    return g_steamAPICallbacks;
}

// delete frees with ourFree.
void SteamAPICallbacks::operator delete(void* apMemory) {
    ourFree(apMemory);
}
