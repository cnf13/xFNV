#pragma once

#include "Reconstruction.hpp"
#include "SDK/Steam/CCallback.h"
#include "SDK/Steam/CCallbackBase.h"
#include "SDK/Steam/isteamuserstats.h"

#include <stdint.h>

class SteamAPICallbacks;

void InitSteamAPI();
void* DestroySteamAPICallbacks();

// steam_api.dll's SteamAPI_RunCallbacks (cdecl), through the exe's import slot.
using SteamAPI_RunCallbacks_t = void (*)();
FNVD_IMPORT(SteamAPI_RunCallbacks_t, SteamAPI_RunCallbacks, 0xFDF488);

// steam_api.dll's SteamAPI_Shutdown (cdecl), through the exe's import slot.
using SteamAPI_Shutdown_t = void (*)();
FNVD_IMPORT(SteamAPI_Shutdown_t, SteamAPI_Shutdown, 0xFDF480);

// steam_api.dll's SteamAPI_IsSteamRunning (cdecl; steam_api.h, bool: BSNetworkSystemUtility::Idle tests al,
// 0xAF2DC7), through the exe's import slot.
using SteamAPI_IsSteamRunning_t = bool (*)();
FNVD_IMPORT(SteamAPI_IsSteamRunning_t, SteamAPI_IsSteamRunning, 0xFDF498);

// steam_api.dll's SteamAPI_Init (cdecl; steam_api.h, bool: the constructor 0x6FF690 stores al), through the exe's
// import slot.
using SteamAPI_Init_t = bool (*)();
FNVD_IMPORT(SteamAPI_Init_t, SteamAPI_Init, 0xFDF478);

// IDA name; not in the PDB (the PDB has no Steam classes).
// The Steam user-stats callbacks and the Steam state WinMain configures (0x44 bytes, allocated by GetSingleton
// 0x6FF580).
class SteamAPICallbacks {
public:
	SteamAPICallbacks();
	~SteamAPICallbacks();

	// IDA name (SteamAPI::SetNoCloud); not in the PDB. WinMain passes false for "-nocloud".
	void SetNoCloud(bool abUseCloud);
	// IDA name SteamAPI_RunCallbacks_; not in the PDB. TESMain::OnIdle's per-frame Steam callback dispatch.
	void RunCallbacks();
	// IDA name; not in the PDB. SteamUser's BLoggedOn (bool: the exe returns its al).
	bool OnUserAuthenticated();
	// Not named in the PDB or IDA; name chosen here. The UserStatsReceived_t handler (thiscall, ret 4; void, as the
	// callback's func_t: the exe leaves eax undefined).
	void OnUserStatsReceived(UserStatsReceived_t* apCallback);
	// Not named in the PDB or IDA; name chosen here. The UserStatsStored_t handler (thiscall, ret 4; void, as the
	// callback's func_t: the exe leaves the argument in eax, which CCallback::Run drops).
	void OnUserStatsStored(UserStatsStored_t* apCallback);
	// Not named in the PDB or IDA; name chosen here. The UserAchievementStored_t handler: the exe folds it into
	// OnUserStatsStored (0x6FF8F0, one body for both callbacks).
	void OnUserStatsStored_Achievement(UserAchievementStored_t* apCallback);

	// The Steam state, created on first use.
	static SteamAPICallbacks* GetSingleton();
	static void operator delete(void* apMemory);

	// The three CCallback members (0x14 bytes each); the constructor builds each with this as m_pObj.
	CCallback<SteamAPICallbacks, UserStatsReceived_t, false> unk00;
	CCallback<SteamAPICallbacks, UserStatsStored_t, false> unk14;
	CCallback<SteamAPICallbacks, UserAchievementStored_t, false> unk28;
	// IDA has no names for the fields below.
	// SteamAPI_Init's result.
	bool bSteamInitialized;
	uint8_t unk3D;
	// Cleared by "-nocloud"; the constructor sets it.
	bool bUseCloud;
	uint8_t padding3F;
	// The app id SteamUtils reports.
	uint32_t uiAppID;
};

// The class's name in the exe's RTTI (CCallback<Steam,UserStatsReceived_t,0> and the other two instances).
using Steam = SteamAPICallbacks;

using CCallback_SteamAPICallbacks_UserStatsReceived_t_false = CCallback<SteamAPICallbacks, UserStatsReceived_t, false>;
using CCallback_SteamAPICallbacks_UserStatsStored_t_false = CCallback<SteamAPICallbacks, UserStatsStored_t, false>;
using CCallback_SteamAPICallbacks_UserAchievementStored_t_false =
	CCallback<SteamAPICallbacks, UserAchievementStored_t, false>;

// GetSingleton stores the constructed object here.
inline SteamAPICallbacks* g_steamAPICallbacks;
