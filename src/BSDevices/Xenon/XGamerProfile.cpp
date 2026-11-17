#include "BSDevices/Xenon/XGamerProfile.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/XPrefSettings.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

char _11F3630;
SettingT<INISettingCollection> gs_uXenonPresenceUpdateInterval_Online("uXenonPresenceUpdateInterval:Online", 600000u);
static SettingT<INISettingCollection> gs_iLanguage_Controls("iLanguage:Controls", 0);
static SettingT<INISettingCollection> gs_bXenonLoadDownloadedContent_Online("bXenonLoadDownloadedContent:Online", true);

char sub_A25870() {
    return (int8_t)_11F3630;
}

void InitXGamerProfile() {
    if (!XGamerProfile::pInstance) {
        XGamerProfile* v1;
        XGamerProfile* apMemory = static_cast<XGamerProfile*>(MemoryManager_Singleton.Allocate(sizeof(XGamerProfile)));
        if (apMemory) {
            v1 = ::new (apMemory) XGamerProfile;
        }
        else {
            v1 = nullptr;
        }
        XGamerProfile::pInstance = v1;
    }
    XGamerProfile::pInstance->sub_A25DE0();
}

void sub_A25BE0() {
    ;
}

int32_t sub_A25D70() {
    return 0;
}

char sub_A25D80() {
    return 0;
}

void sub_A25D90() {
    ;
}

char sub_A25DA0() {
    return 0;
}

void sub_A25DB0() {
    ;
}

void sub_A25DC0(int32_t a1) {
    ;
}

void sub_A25DD0(int32_t a1) {
    ;
}

void sub_A25DF0() {
    ;
}

// CompareFileTime on the creation times: the three-way compare (-1, 0, 1) of the two 64-bit FILETIME values.
int32_t CompareFindDataCreationTime(_WIN32_FIND_DATAA* a1, _WIN32_FIND_DATAA* a2) {
    const uint64_t uiTime1 =
        (static_cast<uint64_t>(a1->ftCreationTime.dwHighDateTime) << 32) | a1->ftCreationTime.dwLowDateTime;
    const uint64_t uiTime2 =
        (static_cast<uint64_t>(a2->ftCreationTime.dwHighDateTime) << 32) | a2->ftCreationTime.dwLowDateTime;
    return (uiTime1 > uiTime2) - (uiTime1 < uiTime2);
}

char return_0_6() {
    return 0;
}

char return_1_3() {
    return 1;
}

XGamerProfile::XGamerProfile()
  : unk4(0), m_bProcessing(false), unk50(0), unk64(0), m_PlayerIndex(0), m_pSaveData(nullptr), m_iNumCorruptContent(0),
    iCurrentPlayerLevel(1), iCurrentPlayerAlignment(1), iDifficultyDefault(-1), iControllerSensitivityDefault(-1),
    bUsesDifficultyDefault(true), bRumbleDefault(1), bInvertYDefault(false), bForcePresenceUpdate(1), bIsPlayingGame(0),
    m_DownloadedContent(), unk3B4(0), unk14278(0) {
    for (int32_t igamePad = 0; igamePad < 4; ++igamePad) {
        m_bSignedIn[igamePad] = false;
    }
    std::memset(&m_EventData, 0, sizeof(m_EventData));
    std::memset(unk3B8, 0, sizeof(unk3B8));
    std::memset(unk8C, 0, sizeof(unk8C));
}

// m_DownloadedContent's destructor then empties the list.
XGamerProfile::~XGamerProfile() {
    if (XGamerProfile::pInstance) {
        XGamerProfile* Block = XGamerProfile::pInstance;
        XGamerProfile::pInstance->~XGamerProfile();
        MemoryManager_Singleton.Deallocate(Block);
        sub_A25DD0(0);
    }
}

uint8_t XGamerProfile::GetRumbleDefault() {
    return bRumbleDefault;
}

void XGamerProfile::ForcePresenceUpdate() {
    bForcePresenceUpdate = (uint8_t)1;
}

void XGamerProfile::SetPlayingFlag(bool abPlaying) {
    bIsPlayingGame = (uint8_t)abPlaying;
}

bool XGamerProfile::IsCodeRedeemed(const char* cCode) {
    SettingT<INIPrefSettingCollection>::InitCollection();
    INIPrefSettingCollection* pCollection = SettingT_INIPrefSettingCollection_pCollection;
    Setting* pSetting = nullptr;
    pCollection->Get("uiRedeemedContent:GamePlay", pSetting);
    Setting* pRedeemed = pSetting;
    if (!pRedeemed) {
        return false;
    }
    uint32_t uiContentBit = GetContentBit(cCode);
    return (uiContentBit & pRedeemed->uValue.uint) != 0;
}

bool XGamerProfile::RedeemCode(const char* cCode) {
    Setting::Info* p_uValue;
    SettingT<INIPrefSettingCollection>::InitCollection();
    INIPrefSettingCollection* pCollection = SettingT_INIPrefSettingCollection_pCollection;
    Setting* pSetting = nullptr;
    pCollection->Get("uiRedeemedContent:GamePlay", pSetting);
    Setting* v8 = pSetting;
    uint32_t bit = GetContentBit(cCode);
    if (v8) {
        p_uValue = &v8->uValue;
    }
    else {
        Setting::DefaultValue.uint = 0;
        p_uValue = &Setting::DefaultValue;
    }
    p_uValue->uint |= bit;
    return bit != 0;
}

void XGamerProfile::RevokeCode(const char* cCode) {
    Setting::Info* p_uValue;
    SettingT<INIPrefSettingCollection>::InitCollection();
    INIPrefSettingCollection* pCollection = SettingT_INIPrefSettingCollection_pCollection;
    Setting* pSetting = nullptr;
    pCollection->Get("uiRedeemedContent:GamePlay", pSetting);
    Setting* v5 = pSetting;
    if (v5) {
        p_uValue = &v5->uValue;
    }
    else {
        Setting::DefaultValue.uint = 0;
        p_uValue = &Setting::DefaultValue;
    }
    p_uValue->uint &= ~GetContentBit(cCode);
}

void XGamerProfile::LoadSettings() {
    ;
}

void XGamerProfile::BeginGameplay() {
    ;
}

void XGamerProfile::EndGameplay() {
    ;
}

void XGamerProfile::Update() {
    ;
}

void XGamerProfile::UpdateSignedInPlayers() {
    ;
}

void XGamerProfile::ReadSettings(uint8_t* apData, uint16_t awSize) {
    ;
}

void XGamerProfile::sub_A25DE0() {
    ;
}

uint32_t XGamerProfile::GetContentBit(const char* cCode) {
    const char* apContentCodes[4] = {"Classic", "Tribal", "Caravan", "Mercenary"};
    uint32_t i;
    for (i = 0; i < 4 && _stricmp(apContentCodes[i], cCode); ++i) {
        ;
    }
    if (i == 4) {
        return 0;
    }
    else {
        return 1 << i;
    }
}

bool XGamerProfile::GetPlayerSignedIn(int32_t a1) {
    return m_bSignedIn[m_PlayerIndex];
}

void XGamerProfile::AddSignedInEntry(void* apEntry) {
    if (return_1_3()) {
        if (apEntry) {
            unk28[unk50].pEntry = apEntry;
            unk28[unk50].uiIndex = m_PlayerIndex;
            ++unk50;
        }
    }
}

void XGamerProfile::UpdatePresence() {
    if (m_bSignedIn[m_PlayerIndex]) {
        if (unk4) {
            if (return_1_3()) {
                if (!bForcePresenceUpdate) {
                    Setting::Info* p_uValue;
                    p_uValue = &gs_uXenonPresenceUpdateInterval_Online.uValue;
                    (void)p_uValue->uint;
                }
            }
        }
    }
}

uint32_t XGamerProfile::sub_A25D50() {
    return unk14278;
}

bool XGamerProfile::sub_A25FD0() {
    for (FileData* m_item : m_DownloadedContent.ItemsUntilNull()) {}
    return true;
}

XGamerProfile* XGamerProfile::Instance() {
    XGamerProfile* v1;
    void* v2;
    if (!XGamerProfile::pInstance) {
        v2 = (&MemoryManager_Singleton)->Allocate(sizeof(XGamerProfile));
        if (v2) {
            v1 = ::new (v2) XGamerProfile;
        }
        else {
            v1 = nullptr;
        }
        XGamerProfile::pInstance = v1;
    }
    return XGamerProfile::pInstance;
}

char* XGamerProfile::WriteSettingToBuffer(void** a1, uint32_t* a2, int32_t pvSrc) {
    rt::memcpy(*a1, &pvSrc, 4u);
    *a2 += 4;
    char* result = static_cast<char*>(*a1) + 4;
    *a1 = result;
    return result;
}

char* XGamerProfile::WriteSettingToBuffer(void** a1, uint32_t* a2, char a3) {
    rt::memcpy(*a1, &a3, 1u);
    ++*a2;
    char* result = static_cast<char*>(*a1) + 1;
    *a1 = result;
    return result;
}

char* XGamerProfile::WriteSettingToBuffer(void** a1, uint32_t* a2, int16_t pvSrc) {
    rt::memcpy(*a1, &pvSrc, 2u);
    *a2 += 2;
    char* result = static_cast<char*>(*a1) + 2;
    *a1 = result;
    return result;
}

char* XGamerProfile::WriteSettingToBuffer(void** a1, uint32_t* a2, void* pvSrc, uint32_t stCount) {
    rt::memcpy(*a1, pvSrc, stCount);
    *a2 += stCount;
    char* result = static_cast<char*>(*a1) + stCount;
    *a1 = result;
    return result;
}
