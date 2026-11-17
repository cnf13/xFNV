#include "BSSystemUtilities/Xenon/BSDLCSystemUtility_Xenon.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/BSContentPackage.h"
#include "BSSystemUtilities/BSDLCSystemUtility.h"
#include "BSSystemUtilities/BSSystemUtility.h"
#include "Reconstruction.hpp"
#include "SDK/Steam/isteamapps.h"
#include "SDK/Steam/isteamfriends.h"
#include "SDK/Steam/isteamutils.h"

#include <stdint.h>

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
void nullsub(BSDLCSystemUtility* apThis, [[maybe_unused]] void* apEdx) {
	;
}

char return_1_1() {
	return 1;
}

// The CRT _stricmp of the two strings.
// KEEP, not registered (0x404DC0): an exe caller reads ecx after the call, relying on the body leaving it intact (the
// exe body never writes it), which clang does not guarantee.
int32_t BSstricmp(const char* apString1, const char* apString2) {
	return _stricmp(apString1, apString2);
}

BSContentPackage::~BSContentPackage() {}

// The lists are built and emptied by BSSimpleList's own constructor and destructor.
BSDLCSystemUtility::BSDLCSystemUtility() : m_state(0), m_pOfferCountCtx(nullptr), m_pOfferListCtx(nullptr) {}

BSDLCSystemUtility::~BSDLCSystemUtility() {
	UnloadDLC(false);
}

// The PC opens the Steam overlay's store page of the running app.
void BSDLCSystemUtility::ShowMarketplace(uint32_t anPlayerIndex) {
	if (SteamFriends()) {
		if (SteamUtils()) {
			ISteamUtils* pUtils = SteamUtils();
			ISteamFriends* pFriends = SteamFriends();
			pFriends->ActivateGameOverlayToStore(pUtils->GetAppID());
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
// The PC asks Steam which entries of the DLC table are installed and adds a package for each to m_validDLC; returns
// how many it added.
uint32_t BSDLCSystemUtility::LoadDLC(uint32_t anPlayerIndex, uint32_t anDevice) {
	UnloadDLC(true);
	uint32_t uiCount = 0;
	if (SteamApps()) {
		for (int32_t i = 0; DLCAppTable[i].iAppID != -1; ++i) {
			ISteamApps* pApps = SteamApps();
			if (pApps->BIsDlcInstalled(DLCAppTable[i].iAppID)) {
				++uiCount;
				BSContentPackage* pPackage;
				BSContentPackage* pMemory =
					static_cast<BSContentPackage*>(MemoryManager_Singleton.Allocate(sizeof(BSContentPackage)));
				if (pMemory) {
					pPackage = ::new (pMemory) BSContentPackage(DLCAppTable[i].pDisplayName);
				} else {
					pPackage = nullptr;
				}
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
		BSContentPackage* Block = i->m_item;
		if (i->m_item) {
			Block->~BSContentPackage();
			MemoryManager_Singleton.Deallocate(Block);
		}
	}
	m_validDLC.RemoveAll();
}

bool BSDLCSystemUtility::GetOfferCounts(uint32_t playerIndex, void (*callback)(uint32_t, uint32_t, void*),
	void* param) {
	callback(0, 0, param);
	return 1;
}

void BSDLCSystemUtility::CheckForCorruptContentAndDisplayMessage() {}

bool BSDLCSystemUtility::IsContentCorrupt(uint32_t anPlayerIndex, const _XCONTENT_DATA& aData) {
	return false;
}

void BSDLCSystemUtility::doIdleOfferCounts() {}

void BSDLCSystemUtility::doIdleMount() {}

// The exe's cleanup-only C++ EH frame is left out.
BSDLCSystemUtility* BSDLCSystemUtility::GetSingleton() {
	if (!BSDLCSystemUtility::pSingleton) {
		BSDLCSystemUtility* v1;
		BSDLCSystemUtility* apMemory =
			static_cast<BSDLCSystemUtility*>(MemoryManager_Singleton.Allocate(sizeof(BSDLCSystemUtility)));
		if (apMemory) {
			v1 = ::new (apMemory) BSDLCSystemUtility;
		} else {
			v1 = 0;
		}
		BSDLCSystemUtility::pSingleton = v1;
	}
	return BSDLCSystemUtility::pSingleton;
}

// The exe frees the utility with MemoryManager::Deallocate after its destructor (DestroyDLCUtility).
void BSDLCSystemUtility::operator delete(void* apMemory) {
	MemoryManager_Singleton.Deallocate(apMemory);
}
