// steam_api.dll's exports the program imports (src/SDK/Steam/*.h, src/BSSystemUtilities/SteamAPICallbacks.h declare
// them, C linkage), for the 64-bit program without the exe, which has no steam_api.dll to link: it never starts
// Steam (FNVD_NO_STEAM), so these do what the DLL does without SteamAPI_Init. Steam is not running, the interface
// getters return null, and callbacks are neither registered nor run.

#include <cstdint>

extern "C" {

bool SteamAPI_Init() {
    return false;
}

void SteamAPI_Shutdown() {}

bool SteamAPI_IsSteamRunning() {
    return false;
}

void SteamAPI_RunCallbacks() {}

void SteamAPI_RegisterCallback(void*, int32_t) {}

void SteamAPI_UnregisterCallback(void*) {}

void* SteamApps() {
    return nullptr;
}

void* SteamFriends() {
    return nullptr;
}

void* SteamUser() {
    return nullptr;
}

void* SteamUserStats() {
    return nullptr;
}

void* SteamUtils() {
    return nullptr;
}
}
