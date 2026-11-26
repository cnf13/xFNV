#include "BSSystemUtilities/Xenon/BSDLCSystemUtility_Xenon.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/BSContentPackage.h"
#include "BSSystemUtilities/BSDLCSystemUtility.h"
#include "BSSystemUtilities/BSSystemUtility.h"
#include "SDK/Steam/isteamapps.h"
#include "SDK/Steam/isteamfriends.h"
#include "SDK/Steam/isteamutils.h"

#include <cstdint>

void nullsub(BSDLCSystemUtility* apThis, [[maybe_unused]] void* apEdx) {
    ;
}

char return_1_1() {
    return 1;
}

// The lists are built and emptied by BSSimpleList's own constructor and destructor.
BSDLCSystemUtility::BSDLCSystemUtility() : m_state(0), m_pOfferCountCtx(nullptr), m_pOfferListCtx(nullptr) {}

BSDLCSystemUtility::~BSDLCSystemUtility() {
    UnloadDLC(false);
}

// The PC opens the Steam overlay's store page of the running app.
void BSDLCSystemUtility::ShowMarketplace(uint32_t anPlayerIndex) {
    if (SteamFriends() && SteamUtils()) {
        ISteamUtils* pUtils = SteamUtils();
        ISteamFriends* pFriends = SteamFriends();
        pFriends->ActivateGameOverlayToStore(pUtils->GetAppID());
    }
}

// The PC asks Steam which entries of the DLC table are installed and adds a package for each to m_validDLC; returns how
// many it added.
uint32_t BSDLCSystemUtility::LoadDLC(uint32_t anPlayerIndex, uint32_t anDevice) {
    UnloadDLC(true);
    uint32_t uiCount = 0;
    if (SteamApps()) {
        for (int32_t i = 0; DLCAppTable[i].iAppID != -1; ++i) {
            ISteamApps* pApps = SteamApps();
            if (pApps->BIsDlcInstalled(DLCAppTable[i].iAppID)) {
                ++uiCount;
                BSContentPackage* pPackage = new BSContentPackage(DLCAppTable[i].pDisplayName);
                BSContentPackage* pItem = pPackage;
                m_validDLC.AddTail(pItem);
            }
        }
    }
    return uiCount;
}

void BSDLCSystemUtility::UnloadDLC(bool unmount) {
    if (unmount) {
        nullsub(this, nullptr);
    }
    for (BSSimpleList<BSContentPackage*>* i = &m_validDLC; i; i = i->GetNext()) {
        if (!i->GetNext() && !i->m_item) {
            break;
        }
        delete i->m_item;
    }
    m_validDLC.RemoveAll();
}

bool BSDLCSystemUtility::GetOfferCounts(
    uint32_t playerIndex,
    void (*callback)(uint32_t, uint32_t, void*),
    void* param
) {
    callback(0, 0, param);
    return true;
}

void BSDLCSystemUtility::CheckForCorruptContentAndDisplayMessage() {}

bool BSDLCSystemUtility::IsContentCorrupt(uint32_t anPlayerIndex, const _XCONTENT_DATA& aData) {
    return false;
}

void BSDLCSystemUtility::doIdleOfferCounts() {}

void BSDLCSystemUtility::doIdleMount() {}

BSDLCSystemUtility* BSDLCSystemUtility::GetSingleton() {
    if (!BSDLCSystemUtility::pSingleton) {
        BSDLCSystemUtility::pSingleton = new BSDLCSystemUtility;
    }
    return BSDLCSystemUtility::pSingleton;
}

// The utility is freed with MemoryManager::Deallocate after its destructor (DestroyDLCUtility).
void BSDLCSystemUtility::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
