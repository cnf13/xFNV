#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

struct FileData;

void InitXGamerProfile();
// Compares the two find-data records' creation times.
int32_t CompareFindDataCreationTime(_WIN32_FIND_DATAA* apFirst, _WIN32_FIND_DATAA* apSecond);
char return_0_6();
char return_1_3();

// The signed-in player's profile: presence, downloaded content and the default settings.
class XGamerProfile {
public:
    // A presence entry: the player index, then the entry pointer. AddSignedInEntry fills the next free one.
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
    void Initialize();
    void Shutdown(int32_t aiUnused);
    // The bit of the content code cCode (Classic, Tribal, Caravan, Mercenary), 0 for an unknown code.
    uint32_t GetContentBit(const char* cCode);
    // Returns m_bSignedIn[m_PlayerIndex]; the argument is never read.
    bool GetPlayerSignedIn(int32_t aiPlayerIndex);
    // When return_1_3 holds and apEntry is set, stores apEntry and m_PlayerIndex into the next presence entry and
    // increments m_uiNumPresenceEntries. No caller.
    void AddSignedInEntry(void* apEntry);
    // Tests m_bSignedIn[m_PlayerIndex], unk4, return_1_3 and bForcePresenceUpdate and reads
    // gs_uXenonPresenceUpdateInterval_Online, with no effect left in the PC build. No caller.
    void UpdatePresence();

    // The profile, allocated and constructed on first use.
    static XGamerProfile* Instance();
    // Copies the value at *appBuffer, advances *appBuffer and *apuiSize and returns the new *appBuffer.
    static char* WriteSettingToBuffer(void** appBuffer, uint32_t* apuiSize, int32_t pvSrc);
    static char* WriteSettingToBuffer(void** appBuffer, uint32_t* apuiSize, char acValue);
    static char* WriteSettingToBuffer(void** appBuffer, uint32_t* apuiSize, int16_t pvSrc);
    static char* WriteSettingToBuffer(void** appBuffer, uint32_t* apuiSize, void* pvSrc, uint32_t stCount);

    bool m_bSignedIn[4];
    uint8_t unk4;
    // The constructor memsets all of m_EventData.
    _XOVERLAPPED m_EventData;
    bool m_bProcessing;
    PresenceEntry m_PresenceEntries[5];
    // The count of presence entries filled; the constructor sets it to 0.
    uint32_t m_uiNumPresenceEntries;
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
    // One 4-aligned block, which the constructor memsets.
    alignas(4) uint8_t unk8C[0x320];
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
