#pragma once

#include "Reconstruction.hpp"
#include "SDK/Steam/steamclientpublic.h"

#include <cstdint>

// SDK: struct FriendGameInfo_t, held by pointer only.
struct FriendGameInfo_t;

// The Steam SDK's friends interface (isteamfriends.h, SteamFriends007), the slots up to ActivateGameOverlayToStore.
// The exe calls ActivateGameOverlayToStore through slot 22 (`call [edx+0x58]`, one argument, 0xAF2938). Names and
// parameters are the SDK's; the PDB has no Steam types.
class ISteamFriends {
public:
	virtual const char* GetPersonaName() = 0;
	virtual void SetPersonaName(const char* pchPersonaName) = 0;
	// SDK: EPersonaState.
	virtual int32_t GetPersonaState() = 0;
	virtual int32_t GetFriendCount(int32_t iFriendFlags) = 0;
	virtual CSteamID GetFriendByIndex(int32_t iFriend, int32_t iFriendFlags) = 0;
	// SDK: EFriendRelationship.
	virtual int32_t GetFriendRelationship(CSteamID steamIDFriend) = 0;
	// SDK: EPersonaState.
	virtual int32_t GetFriendPersonaState(CSteamID steamIDFriend) = 0;
	virtual const char* GetFriendPersonaName(CSteamID steamIDFriend) = 0;
	virtual bool GetFriendGamePlayed(CSteamID steamIDFriend, FriendGameInfo_t* pFriendGameInfo) = 0;
	virtual const char* GetFriendPersonaNameHistory(CSteamID steamIDFriend, int32_t iPersonaName) = 0;
	virtual bool HasFriend(CSteamID steamIDFriend, int32_t iFriendFlags) = 0;
	virtual int32_t GetClanCount() = 0;
	virtual CSteamID GetClanByIndex(int32_t iClan) = 0;
	virtual const char* GetClanName(CSteamID steamIDClan) = 0;
	virtual const char* GetClanTag(CSteamID steamIDClan) = 0;
	virtual int32_t GetFriendCountFromSource(CSteamID steamIDSource) = 0;
	virtual CSteamID GetFriendFromSourceByIndex(CSteamID steamIDSource, int32_t iFriend) = 0;
	virtual bool IsUserInSource(CSteamID steamIDUser, CSteamID steamIDSource) = 0;
	virtual void SetInGameVoiceSpeaking(CSteamID steamIDUser, bool bSpeaking) = 0;
	virtual void ActivateGameOverlay(const char* pchDialog) = 0;
	virtual void ActivateGameOverlayToUser(const char* pchDialog, CSteamID steamID) = 0;
	virtual void ActivateGameOverlayToWebPage(const char* pchURL) = 0;
	// SDK: AppId_t.
	virtual void ActivateGameOverlayToStore(uint32_t nAppID) = 0;
};

// steam_api.dll's SteamFriends (cdecl), through the exe's import slot: the friends interface, or null when Steam is
// not running.
using SteamFriends_t = ISteamFriends*(*)();
FNVD_IMPORT(SteamFriends_t, SteamFriends, 0xFDF474);
