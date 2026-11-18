#include "BSSystemUtilities/SteamAPICallbacks.h"

#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"
#include "SDK/Steam/CCallback.h"
#include "SDK/Steam/isteamuser.h"
#include "SDK/Steam/isteamuserstats.h"
#include "SDK/Steam/isteamutils.h"

// Assumes CCallback's native constructor CCallback(T* apObj, func_t aFunc).
SteamAPICallbacks::SteamAPICallbacks()
  : unk00(this, &xfnv::callMember<&SteamAPICallbacks::OnUserStatsReceived>),
    unk14(this, &xfnv::callMember<&SteamAPICallbacks::OnUserStatsStored>),
    unk28(this, &xfnv::callMember<&SteamAPICallbacks::OnUserStatsStored_Achievement>), bSteamInitialized(false),
    unk3D(0), bUseCloud(true), uiAppID(0) {
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
    int8_t v0 = (int8_t)SteamAPICallbacks::GetSingleton()->bSteamInitialized;
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
        unk3D = apCallback->m_eResult == 1;
    }
}

// Builds the app's game id (uiAppID, 0 above) and keeps it in a local: the handler compares nothing.
void SteamAPICallbacks::OnUserStatsStored([[maybe_unused]] UserStatsStored_t* apCallback) {
    [[maybe_unused]] uint64_t ulGameID = uiAppID;
}

void SteamAPICallbacks::OnUserStatsStored_Achievement([[maybe_unused]] UserAchievementStored_t* apCallback) {
    [[maybe_unused]] uint64_t ulGameID = uiAppID;
}

SteamAPICallbacks* SteamAPICallbacks::GetSingleton() {
    if (!g_steamAPICallbacks) {
        SteamAPICallbacks* pCallbacks;
        SteamAPICallbacks* pMemory = static_cast<SteamAPICallbacks*>(ourMalloc(sizeof(SteamAPICallbacks)));
        if (pMemory) {
            pCallbacks = ::new (pMemory) SteamAPICallbacks;
        }
        else {
            pCallbacks = nullptr;
        }
        g_steamAPICallbacks = pCallbacks;
    }
    return g_steamAPICallbacks;
}

// delete frees with ourFree.
void SteamAPICallbacks::operator delete(void* apMemory) {
    ourFree(apMemory);
}
