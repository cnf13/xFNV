#pragma once

#include "Reconstruction.hpp"

#include <cstdint>

// The Steam SDK's apps interface (isteamapps.h), the slots up to BIsDlcInstalled. The exe calls BIsDlcInstalled
// through slot 7 (`call [edx+0x1C]`, one argument, 0xAF29C9). Names and parameters are the SDK's; the PDB has no
// Steam types.
class ISteamApps {
public:
    virtual bool BIsSubscribed() = 0;
    virtual bool BIsLowViolence() = 0;
    virtual bool BIsCybercafe() = 0;
    virtual bool BIsVACBanned() = 0;
    virtual const char* GetCurrentGameLanguage() = 0;
    virtual const char* GetAvailableGameLanguages() = 0;
    // SDK: AppId_t.
    virtual bool BIsSubscribedApp(uint32_t appID) = 0;
    // SDK: AppId_t.
    virtual bool BIsDlcInstalled(uint32_t appID) = 0;
};

// steam_api.dll's SteamApps (cdecl), through the exe's import slot: the apps interface, or null when Steam is not
// running.
using SteamApps_t = ISteamApps* (*)();
XFNV_IMPORT(SteamApps_t, SteamApps, 0xFDF494);
