#include "BSDevices/Xenon/XGamerProfile.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/XPrefSettings.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

SettingT<INISettingCollection> gs_uXenonPresenceUpdateInterval_Online("uXenonPresenceUpdateInterval:Online", 600000u);
static SettingT<INISettingCollection> gs_iLanguage_Controls("iLanguage:Controls", 0);
static SettingT<INISettingCollection> gs_bXenonLoadDownloadedContent_Online("bXenonLoadDownloadedContent:Online", true);

void InitXGamerProfile() {
    if (!XGamerProfile::pInstance) {
        XGamerProfile* pProfile;
        pProfile = new XGamerProfile;
        XGamerProfile::pInstance = pProfile;
    }
    XGamerProfile::pInstance->Initialize();
}

// CompareFileTime on the creation times: the three-way compare (-1, 0, 1) of the two 64-bit FILETIME values.
int32_t CompareFindDataCreationTime(_WIN32_FIND_DATAA* apFirst, _WIN32_FIND_DATAA* apSecond) {
    const uint64_t uiTime1 = (static_cast<uint64_t>(apFirst->ftCreationTime.dwHighDateTime) << 32) |
        apFirst->ftCreationTime.dwLowDateTime;
    const uint64_t uiTime2 = (static_cast<uint64_t>(apSecond->ftCreationTime.dwHighDateTime) << 32) |
        apSecond->ftCreationTime.dwLowDateTime;
    return (uiTime1 > uiTime2) - (uiTime1 < uiTime2);
}

char return_0_6() {
    return 0;
}

char return_1_3() {
    return 1;
}

XGamerProfile::XGamerProfile()
  : unk4(0), m_bProcessing(false), m_uiNumPresenceEntries(0), unk64(0), m_PlayerIndex(0), m_pSaveData(nullptr),
    m_iNumCorruptContent(0), iCurrentPlayerLevel(1), iCurrentPlayerAlignment(1), iDifficultyDefault(-1),
    iControllerSensitivityDefault(-1), bUsesDifficultyDefault(true), bRumbleDefault(1), bInvertYDefault(false),
    bForcePresenceUpdate(1), bIsPlayingGame(0), m_DownloadedContent(), unk3B4(0), unk14278(0) {
    for (bool& rbSignedIn : m_bSignedIn) {
        rbSignedIn = false;
    }
    std::memset(&m_EventData, 0, sizeof(m_EventData));
    std::memset(unk3B8, 0, sizeof(unk3B8));
    std::memset(unk8C, 0, sizeof(unk8C));
}

// m_DownloadedContent's destructor then empties the list.
XGamerProfile::~XGamerProfile() {
    if (XGamerProfile::pInstance) {
        delete XGamerProfile::pInstance;
        Shutdown(0);
    }
}

uint8_t XGamerProfile::GetRumbleDefault() {
    return bRumbleDefault;
}

void XGamerProfile::ForcePresenceUpdate() {
    bForcePresenceUpdate = 1;
}

void XGamerProfile::SetPlayingFlag(bool abPlaying) {
    bIsPlayingGame = abPlaying;
}

bool XGamerProfile::IsCodeRedeemed(const char* cCode) {
    SettingT<INIPrefSettingCollection>::InitCollection();
    INIPrefSettingCollection* pCollection = SettingT<INIPrefSettingCollection>::pCollection;
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
    Setting::Info* pValue;
    SettingT<INIPrefSettingCollection>::InitCollection();
    INIPrefSettingCollection* pCollection = SettingT<INIPrefSettingCollection>::pCollection;
    Setting* pSetting = nullptr;
    pCollection->Get("uiRedeemedContent:GamePlay", pSetting);
    Setting* pRedeemed = pSetting;
    uint32_t bit = GetContentBit(cCode);
    if (pRedeemed) {
        pValue = &pRedeemed->uValue;
    }
    else {
        Setting::DefaultValue.uint = 0;
        pValue = &Setting::DefaultValue;
    }
    pValue->uint |= bit;
    return bit != 0;
}

void XGamerProfile::RevokeCode(const char* cCode) {
    Setting::Info* pValue;
    SettingT<INIPrefSettingCollection>::InitCollection();
    INIPrefSettingCollection* pCollection = SettingT<INIPrefSettingCollection>::pCollection;
    Setting* pSetting = nullptr;
    pCollection->Get("uiRedeemedContent:GamePlay", pSetting);
    Setting* pRedeemed = pSetting;
    if (pRedeemed) {
        pValue = &pRedeemed->uValue;
    }
    else {
        Setting::DefaultValue.uint = 0;
        pValue = &Setting::DefaultValue;
    }
    pValue->uint &= ~GetContentBit(cCode);
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

void XGamerProfile::Initialize() {
    ;
}

void XGamerProfile::Shutdown(int32_t aiUnused) {
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

    return 1 << i;
}

bool XGamerProfile::GetPlayerSignedIn(int32_t aiPlayerIndex) {
    return m_bSignedIn[m_PlayerIndex];
}

void XGamerProfile::AddSignedInEntry(void* apEntry) {
    if (return_1_3() && apEntry) {
        m_PresenceEntries[m_uiNumPresenceEntries].pEntry = apEntry;
        m_PresenceEntries[m_uiNumPresenceEntries].uiIndex = m_PlayerIndex;
        ++m_uiNumPresenceEntries;
    }
}

void XGamerProfile::UpdatePresence() {
    if (m_bSignedIn[m_PlayerIndex] && unk4 && return_1_3() && (!bForcePresenceUpdate)) {
        Setting::Info* p_uValue;
        p_uValue = &gs_uXenonPresenceUpdateInterval_Online.uValue;
        static_cast<void>(p_uValue->uint);
    }
}

XGamerProfile* XGamerProfile::Instance() {
    XGamerProfile* pProfile;
    if (!XGamerProfile::pInstance) {
        pProfile = new XGamerProfile;
        XGamerProfile::pInstance = pProfile;
    }
    return XGamerProfile::pInstance;
}

char* XGamerProfile::WriteSettingToBuffer(void** appBuffer, uint32_t* apuiSize, int32_t pvSrc) {
    rt::memcpy(*appBuffer, &pvSrc, 4u);
    *apuiSize += 4;
    char* result = static_cast<char*>(*appBuffer) + 4;
    *appBuffer = result;
    return result;
}

char* XGamerProfile::WriteSettingToBuffer(void** appBuffer, uint32_t* apuiSize, char acValue) {
    rt::memcpy(*appBuffer, &acValue, 1u);
    ++*apuiSize;
    char* result = static_cast<char*>(*appBuffer) + 1;
    *appBuffer = result;
    return result;
}

char* XGamerProfile::WriteSettingToBuffer(void** appBuffer, uint32_t* apuiSize, int16_t pvSrc) {
    rt::memcpy(*appBuffer, &pvSrc, 2u);
    *apuiSize += 2;
    char* result = static_cast<char*>(*appBuffer) + 2;
    *appBuffer = result;
    return result;
}

char* XGamerProfile::WriteSettingToBuffer(void** appBuffer, uint32_t* apuiSize, void* pvSrc, uint32_t stCount) {
    rt::memcpy(*appBuffer, pvSrc, stCount);
    *apuiSize += stCount;
    char* result = static_cast<char*>(*appBuffer) + stCount;
    *appBuffer = result;
    return result;
}
