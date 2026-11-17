#pragma once

#include "Reconstruction.hpp"

#include <cstdint>

// The Steam SDK's user-stats callback results the exe registers for (steam_api's isteamuserstats.h; the PDB has no
// Steam types). Field names are the SDK's; the sizes are the exe's (each CCallback instance's GetCallbackSizeBytes
// returns them) and the callback ids the ones its Register bodies (0x6FFB20, 0x6FFB90, 0x6FFC20) pass.

// The current user's stats and achievements arrived (or failed to).
struct UserStatsReceived_t {
    static constexpr int32_t k_iCallback = 1101;

    uint64_t m_nGameID;
    // The SDK's EResult (1 is k_EResultOK).
    int32_t m_eResult;
    // The SDK's CSteamID: the user the stats belong to.
    uint64_t m_steamIDUser;
};

// A StoreStats call finished.
struct UserStatsStored_t {
    static constexpr int32_t k_iCallback = 1102;

    uint64_t m_nGameID;
    // The SDK's EResult.
    int32_t m_eResult;
};

// An achievement was stored, or its progress shown.
struct UserAchievementStored_t {
    static constexpr int32_t k_iCallback = 1103;

    uint64_t m_nGameID;
    bool m_bGroupAchievement;
    char m_rgchAchievementName[128];
    uint32_t m_nCurProgress;
    uint32_t m_nMaxProgress;
};

// The Steam SDK's user stats interface (isteamuserstats.h), the slots up to StoreStats. The exe calls
// SetAchievement through slot 7 (`call [eax+0x1C]`, 0xAF256A) and StoreStats through slot 10 (`call [eax+0x28]`,
// 0xAF2598). Names and parameters are the SDK's.
class ISteamUserStats {
public:
    virtual bool RequestCurrentStats() = 0;
    virtual bool GetStat(const char* pchName, int32_t* pData) = 0;
    virtual bool GetStat(const char* pchName, float* pData) = 0;
    virtual bool SetStat(const char* pchName, int32_t nData) = 0;
    virtual bool SetStat(const char* pchName, float fData) = 0;
    virtual bool UpdateAvgRateStat(const char* pchName, float flCountThisSession, double dSessionLength) = 0;
    virtual bool GetAchievement(const char* pchName, bool* pbAchieved) = 0;
    virtual bool SetAchievement(const char* pchName) = 0;
    virtual bool ClearAchievement(const char* pchName) = 0;
    virtual bool GetAchievementAndUnlockTime(const char* pchName, bool* pbAchieved, uint32_t* punUnlockTime) = 0;
    virtual bool StoreStats() = 0;
};

// steam_api.dll's SteamUserStats (cdecl), through the exe's import slot: the user stats interface, or null when
// Steam is not running.
using SteamUserStats_t = ISteamUserStats* (*)();
FNVD_IMPORT(SteamUserStats_t, SteamUserStats, 0xFDF490);
