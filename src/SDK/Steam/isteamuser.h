#pragma once

#include "Reconstruction.hpp"

#include <cstdint>

// The Steam SDK's user interface (isteamuser.h), the slots up to BLoggedOn. The exe calls BLoggedOn through slot 1
// (`call [eax+4]`, 0x6FF852). Names are the SDK's; the PDB has no Steam types.
class ISteamUser {
public:
    // SDK: HSteamUser.
    virtual int32_t GetHSteamUser() = 0;
    virtual bool BLoggedOn() = 0;
};

// steam_api.dll's SteamUser (cdecl), through the exe's import slot: the user interface, or null when Steam is not
// running.
using SteamUser_t = ISteamUser* (*)();
FNVD_IMPORT(SteamUser_t, SteamUser, 0xFDF484);
