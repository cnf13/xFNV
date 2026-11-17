#pragma once

#include <cstdint>

// The SDK's EUniverse and EAccountType, the values the header's constants use.
enum EUniverse : int32_t {
	k_EUniverseInvalid = 0,
	k_EUniversePublic = 1,
};
enum EAccountType : int32_t {
	k_EAccountTypeInvalid = 0,
};

// The Steam SDK's CSteamID (steam_api's steamclientpublic.h; the PDB has no Steam types): a 64-bit Steam account id,
// the account number in the low dword and the instance, account type and universe as bit fields of the high one.
// The field names are the SDK's; the layout is the exe's default constructor's (0x6FFC90: +0 cleared, then the high
// dword's bits 20-23, 24-31 and 0-19).
class CSteamID {
public:
	CSteamID();
	CSteamID(uint32_t unAccountID, uint32_t unAccountInstance, EUniverse eUniverse, EAccountType eAccountType);
	// The SDK's Set: the account id, then the universe, account type and instance bit fields, in the exe's order
	// (thiscall, ret 0x10, 0x6FFD20).
	void Set(uint32_t unAccountID, uint32_t unAccountInstance, EUniverse eUniverse, EAccountType eAccountType);

	uint32_t m_unAccountID;
	uint32_t m_unAccountInstance : 20;
	uint32_t m_EAccountType : 4;
	int32_t m_EUniverse : 8;
};

// The SDK's header constants: statics of every file that includes this header, as in the original (the exe's
// initialisers 0xF8BE50 to 0xF8BEC0, one set per file).
static const CSteamID k_steamIDNil{};
static const CSteamID k_steamIDOutofDateGS{0, 0, k_EUniverseInvalid, k_EAccountTypeInvalid};
static const CSteamID k_steamIDLanModeGS{0, 0, k_EUniversePublic, k_EAccountTypeInvalid};
static const CSteamID k_steamIDNotInitYetGS{1, 0, k_EUniverseInvalid, k_EAccountTypeInvalid};
static const CSteamID k_steamIDNonSteamGS{2, 0, k_EUniverseInvalid, k_EAccountTypeInvalid};
