#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

struct FileData;

char sub_A25870();
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
    // A presence entry: the player index, then the entry pointer. AddSignedInEntry stores them in the unk50-th entry.
    struct PresenceEntry {
        uint32_t uiIndex;
        void* pEntry;
    };

    XGamerProfile();
    ~XGamerProfile();

    // The profile's default for the gamepad rumble. Returns the field's raw byte.
    uint8_t GetRumbleDefault();
    void ForcePresenceUpdate();
    void SetPlayingFlag(bool abPlaying);
    // True when the code apCode has been redeemed on this profile.
    bool IsCodeRedeemed(const char* cCode);
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
    // Returns m_bSignedIn[m_PlayerIndex]; the argument is never read.
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

    bool m_bSignedIn[4];
    uint8_t unk4;
    // The constructor memsets all of m_EventData.
    _XOVERLAPPED m_EventData;
    bool m_bProcessing;
    // The constructor does not touch it.
    uint8_t unk25[3];
    PresenceEntry unk28[5];
    // The count of presence entries filled; the constructor sets it to 0.
    uint32_t unk50;
    uint8_t unk54[0x10];
    uint32_t unk64;
    // The constructor stores 0.
    uint32_t m_PlayerIndex;
    // The constructor stores 0 in both m_pSaveData and m_iNumCorruptContent.
    uint8_t* m_pSaveData;
    uint32_t m_iNumCorruptContent;
    // The constructor sets the first two to 1 and the last two to -1.
    int32_t iCurrentPlayerLevel;
    int32_t iCurrentPlayerAlignment;
    int32_t iDifficultyDefault;
    int32_t iControllerSensitivityDefault;
    // The constructor sets bUsesDifficultyDefault to 1 and bInvertYDefault to 0.
    bool bUsesDifficultyDefault;
    uint8_t bRumbleDefault;
    bool bInvertYDefault;
    uint8_t bForcePresenceUpdate;
    uint8_t bIsPlayingGame;
    // unk8C is 4-aligned, so the three bytes before it are alignment padding.
    uint8_t unk89[3];
    // One block, which the constructor memsets.
    uint8_t unk8C[0x320];
    // The constructor clears the list head, and the list is emptied with RemoveAll when ~XGamerProfile runs.
    BSSimpleList<FileData*> m_DownloadedContent;
    // The constructor stores 0.
    uint32_t unk3B4;
    // One block, which the constructor memsets.
    uint8_t unk3B8[0x13EC0];
    // The constructor stores 0.
    uint32_t unk14278;
    // The constructor does not write this member.
    uint32_t unk1427C;

    static inline XGamerProfile* pInstance;
};

extern SettingT<INISettingCollection> gs_uXenonPresenceUpdateInterval_Online;
