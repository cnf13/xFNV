#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

struct FileData;

char sub_A25870();
// Not named in the PDB or IDA; name chosen here. Returns nothing: the exe leaves in eax whatever its last step left
// (XGamerProfile::sub_A25DE0 is empty), and it has no callers that read it.
void InitXGamerProfile();
void sub_A25BE0();
int32_t sub_A25D70();
char sub_A25D80();
void sub_A25D90();
char sub_A25DA0();
void sub_A25DB0();
void sub_A25DC0(int32_t a1);
void sub_A25DD0(int32_t a1);
void sub_A25DF0();
// Compares the two find-data records' creation times.
int32_t CompareFindDataCreationTime(_WIN32_FIND_DATAA* a1, _WIN32_FIND_DATAA* a2);
char return_0_6();
char return_1_3();

// The signed-in player's profile: presence, downloaded content and the default settings.
class XGamerProfile {
public:
	// The 8-byte presence entries at +0x28: the player index, then the entry pointer. IDA splits them into gap25's
	// bytes; AddSignedInEntry stores them at +0x28 and +0x2C of the unk50-th entry.
	struct PresenceEntry {
		uint32_t uiIndex;
		void* pEntry;
	};

	XGamerProfile();
	~XGamerProfile();

	// The profile's default for the gamepad rumble (thiscall, 0x7D3170). The exe returns the field's raw byte in al.
	uint8_t GetRumbleDefault();
	void ForcePresenceUpdate();
	void SetPlayingFlag(bool abPlaying);
	// True when the code apCode has been redeemed on this profile.
	bool IsCodeRedeemed(const char* cCode);
	// Redeems the code apCode on this profile; the exe's result in eax (thiscall, ret 4,
	// 0xA26090).
	bool RedeemCode(const char* cCode);
	void RevokeCode(const char* cCode);
	// Empty on PC.
	void LoadSettings();
	void BeginGameplay();
	void EndGameplay();
	void Update();
	void UpdateSignedInPlayers();
	void ReadSettings(uint8_t* apData, uint16_t awSize);
	void sub_A25DE0();
	// The bit of the content code cCode (Classic, Tribal, Caravan, Mercenary), 0 for an unknown code.
	uint32_t GetContentBit(const char* cCode);
	// m_bSignedIn[m_PlayerIndex]; the stack argument is popped and never read (thiscall,
	// ret 4, 0xA25BF0). No caller; the offsets (+0x0 indexed by +0x68) and the address inside XGamerProfile.cpp's
	// functions give the class. The PDB's GetPlayerSignedIn takes no argument, so it is not that.
	bool GetPlayerSignedIn(int32_t a1);
	// When return_1_3 holds and apEntry is set, stores apEntry and m_PlayerIndex into the unk50-th presence entry and
	// increments unk50. No caller.
	void AddSignedInEntry(void* apEntry);
	// Tests m_bSignedIn[m_PlayerIndex], unk4, return_1_3 and bForcePresenceUpdate and reads
	// gs_uXenonPresenceUpdateInterval_Online, with no effect left in the PC build. No caller.
	void UpdatePresence();
	// Returns unk14278. No caller.
	uint32_t sub_A25D50();
	// Walks m_DownloadedContent (+0x3AC) to its first empty node and returns true; the loop
	// body was compiled out. No caller.
	bool sub_A25FD0();

	// The profile, allocated and constructed on first use.
	static XGamerProfile* Instance();
	// Copies the value at *a1, advances *a1 and *a2 and returns the new *a1.
	static char* WriteSettingToBuffer(void** a1, uint32_t* a2, int32_t pvSrc);
	static char* WriteSettingToBuffer(void** a1, uint32_t* a2, char a3);
	static char* WriteSettingToBuffer(void** a1, uint32_t* a2, int16_t pvSrc);
	static char* WriteSettingToBuffer(void** a1, uint32_t* a2, void* pvSrc, uint32_t stCount);

	// IDA: bool m_bSignedIn[4]; the PDB's m_bSignedIn is bool[4] as well. The constructor clears the four bytes one
	// by one (0xA259EF).
	bool m_bSignedIn[4];
	uint8_t unk4;
	// The PDB's _XOVERLAPPED (0x1C bytes, NiSystem.h); the PC build keeps the same struct, and the constructor memsets
	// all of it (0xA25A0B, 0xA25A16). IDA names the member m_EventData, 4-aligned.
	_XOVERLAPPED m_EventData;
	// The PDB's m_bProcessing, which follows m_EventData there as here (IDA names it so); the constructor clears it.
	bool m_bProcessing;
	// IDA: gap25[43]'s head. The constructor does not touch it.
	uint8_t unk25[3];
	PresenceEntry unk28[5];
	// IDA: dword50; the constructor stores 0 (0xA25A28); the count of presence entries filled.
	uint32_t unk50;
	// IDA: gap54[16].
	uint8_t unk54[0x10];
	// IDA: dword64; the constructor stores 0 (0xA25A46).
	uint32_t unk64;
	// IDA: m_PlayerIndex; the constructor stores 0 (0xA259FD).
	uint32_t m_PlayerIndex;
	// The PDB's m_pSaveData and m_iNumCorruptContent, which sit between m_PlayerIndex and iCurrentPlayerLevel there as
	// here (IDA: dword6C and dword70); the constructor stores 0 in both (0xA25A32, 0xA25A3C).
	uint8_t* m_pSaveData;
	uint32_t m_iNumCorruptContent;
	// IDA takes these four names from the PDB, whose layout is another build's; the constructor sets the first two to
	// 1 and the last two to -1 (0xA25A50, 0xA25A57, 0xA25A71, 0xA25A64).
	int32_t iCurrentPlayerLevel;
	int32_t iCurrentPlayerAlignment;
	int32_t iDifficultyDefault;
	int32_t iControllerSensitivityDefault;
	// The PDB's bools between iControllerSensitivityDefault and bForcePresenceUpdate, in the same order; the
	// constructor sets bUsesDifficultyDefault to 1 and bInvertYDefault to 0.
	bool bUsesDifficultyDefault;
	uint8_t bRumbleDefault;
	bool bInvertYDefault;
	uint8_t bForcePresenceUpdate;
	uint8_t bIsPlayingGame;
	// IDA: char char8C is 4-aligned, so the three bytes before it are the alignment's.
	uint8_t unk89[3];
	// IDA: char char8C followed by gap8D[799]: one 0x320-byte block, which the constructor memsets (0xA25AC3).
	uint8_t unk8C[0x320];
	// The PDB's BSSimpleList<FileData*> m_DownloadedContent (IDA: BSSimpleList_void). The constructor clears the
	// head's two words (0xA259C7), and its destructor empties it with RemoveAll (0xA26240) when ~XGamerProfile runs.
	BSSimpleList<FileData*> m_DownloadedContent;
	// IDA: dword3B4; the constructor stores 0 (0xA25AEC).
	uint32_t unk3B4;
	// IDA: char char3B8 followed by gap3B9[81599]: one 0x13EC0-byte block, which the constructor memsets (0xA25AAA).
	uint8_t unk3B8[0x13EC0];
	// IDA: dword14278; the constructor stores 0 (0xA25ADF).
	uint32_t unk14278;
	// The four bytes the class's 0x14280 size leaves after IDA's last field; the constructor does not write them.
	uint32_t unk1427C;

	static inline XGamerProfile* pInstance;
};

extern SettingT<INISettingCollection> gs_uXenonPresenceUpdateInterval_Online;
