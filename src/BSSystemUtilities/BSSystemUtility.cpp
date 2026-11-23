#include "BSSystemUtilities/BSSystemUtility.h"

#include "BSCore/MemoryManager.h"
#include "BSSystemUtilities/BSCacheDriveSystemUtility.h"
#include "BSSystemUtilities/BSDLCSystemUtility.h"
#include "BSSystemUtilities/BSDiscBootSystemUtility.h"
#include "BSSystemUtilities/BSGameDataSystemUtility.h"
#include "BSSystemUtilities/BSMsgDialogSystemUtility.h"
#include "BSSystemUtilities/BSNetworkSystemUtility.h"
#include "BSSystemUtilities/BSSaveDataSystemUtility.h"
#include "BSSystemUtilities/BSSteamAwardsSystemUtility.h"
#include "BSSystemUtilities/BSSysInfoSystemUtility.h"
#include "BSSystemUtilities/BSSystemUtilitiesStrings.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

void DestroySteamAwardsUtility() {
    if (BSSteamAwardsSystemUtility::m_pInstance) {
        delete BSSteamAwardsSystemUtility::m_pInstance;
        BSSteamAwardsSystemUtility::m_pInstance = nullptr;
    }
}

char InitDLCUtility() {
    BSDLCSystemUtility::GetSingleton();
    return 1;
}

void DestroyDLCUtility() {
    if (BSDLCSystemUtility::pSingleton) {
        delete BSDLCSystemUtility::pSingleton;
        BSDLCSystemUtility::pSingleton = nullptr;
    }
}

BSSystemUtility::BSSystemUtility()
  : pEventNotificationCallback(nullptr), pFocusNotificationCallback(nullptr), bVisible(false),
    eRegion(SYSUTIL_REGION_UNKNOWN), bLostFocus(false), bExitApp(false) {
    std::memset(cTitle, 0, sizeof(cTitle));
}

BSSystemUtility::~BSSystemUtility() {}

void BSSystemUtility::Initialize(
    const char* apcTitle,
    const char* apcDefaultTitleID,
    uint32_t auiDefaultParentalLevel
) {
    Destroy();
    if (apcTitle) {
        size_t stCount;
        size_t stLen = std::strlen(apcTitle);
        if (stLen <= 127) {
            stCount = stLen;
        }
        else {
            stCount = 127;
        }
        strncpy_s(cTitle, stCount + 1, apcTitle, stCount);
    }
    BSSysInfoSystemUtility::GetSingleton()->Initialize();
    BSDiscBootSystemUtility::GetSingleton()->Initialize(apcDefaultTitleID, auiDefaultParentalLevel);
    BSSaveDataSystemUtility::GetSingleton()->Initialize();
    BSSystemUtilitiesStrings::QInstance()->Initialize();
    BSGameDataSystemUtility::GetSingleton()->Initialize();
    BSCacheDriveSystemUtility::GetSingleton()->Initialize();
    BSSteamAwardsSystemUtility::QInstance();
    BSSteamAwardsSystemUtility::Initialize();
    BSNetworkSystemUtility::QInstance();
    BSNetworkSystemUtility::Initialize();
    BSDLCSystemUtility::GetSingleton();
    InitDLCUtility();
}

void BSSystemUtility::Destroy() {
    BSDLCSystemUtility::GetSingleton();
    DestroyDLCUtility();
    BSNetworkSystemUtility::QInstance();
    BSNetworkSystemUtility::Destroy();
    BSSteamAwardsSystemUtility::QInstance();
    DestroySteamAwardsUtility();
    BSSysInfoSystemUtility::GetSingleton()->Destroy();
    BSDiscBootSystemUtility::GetSingleton()->Destroy();
    BSSaveDataSystemUtility::GetSingleton()->Destroy();
    BSSystemUtilitiesStrings::QInstance()->Destroy();
    BSGameDataSystemUtility::GetSingleton()->Destroy();
    BSCacheDriveSystemUtility::GetSingleton()->Destroy();
    std::memset(cTitle, 0, sizeof(cTitle));
    bLostFocus = false;
    eRegion = SYSUTIL_REGION_UNKNOWN;
    pEventNotificationCallback = nullptr;
    pFocusNotificationCallback = nullptr;
}

void BSSystemUtility::Update() {
    BSSteamAwardsSystemUtility::QInstance()->Idle();
    BSNetworkSystemUtility::QInstance().Idle();
    BSDLCSystemUtility::GetSingleton()->Idle();
}

// The PC build has no hardware dip switches: none is set.
bool BSSystemUtility::QHardwareDipSwitch(uint32_t auiDipSwitch) {
    return false;
}

BSDLCSystemUtility& BSSystemUtility::QDLCUtility() {
    return *BSDLCSystemUtility::GetSingleton();
}

BSNetworkSystemUtility& BSSystemUtility::QNetworkUtility() {
    return BSNetworkSystemUtility::QInstance();
}

BSMsgDialogSystemUtility* BSSystemUtility::GetMsgDialogUtility() {
    return &BSMsgDialogSystemUtility::QInstance();
}

BSSaveDataSystemUtility* BSSystemUtility::GetSaveDataUtility() {
    return BSSaveDataSystemUtility::GetSingleton();
}

void BSSystemUtility::SetLostFocus(bool abLostFocus) {
    if ((abLostFocus != bLostFocus) && pFocusNotificationCallback) {
        pFocusNotificationCallback(abLostFocus);
    }

    bLostFocus = abLostFocus;
}

BSSystemUtility& BSSystemUtility::QInstance() {
    static BSSystemUtility stutility;
    return stutility;
}

void BSSystemUtility::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
