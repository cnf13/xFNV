#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

class BSDLCSystemUtility;
class BSMsgDialogSystemUtility;
class BSNetworkSystemUtility;
class BSSaveDataSystemUtility;

void DestroySteamAwardsUtility();
char InitDLCUtility();
void DestroyDLCUtility();

// The console's video region (PDB values).
enum SYSUTIL_REGION : int32_t {
	SYSUTIL_REGION_UNKNOWN = 0,
	SYSUTIL_REGION_NTSC = 1,
	SYSUTIL_REGION_PAL = 2,
};

// The platform services hub: owns the save, DLC, network and other system utilities.
class BSSystemUtility {
public:
	// The platform events the system utility callback reports.
	enum EVENT_NOTIFICATION : int32_t {
		EVENT_NOTIFICATION_UTILITY_VISIBLE_BEGIN = 0,
		EVENT_NOTIFICATION_UTILITY_VISIBLE_END = 1,
		EVENT_NOTIFICATION_USER_REQUEST_EXIT = 2,
		EVENT_NOTIFICATION_UNKNOWN = 3,
	};

	BSSystemUtility();
	// Not reconstructed yet (slot 0).
	virtual ~BSSystemUtility();

	// slot 1, thiscall, ret 0xC.
	virtual void Initialize(const char* apcTitle, const char* apcDefaultTitleID, uint32_t auiDefaultParentalLevel);
	// slot 2.
	virtual void Destroy();
	// Runs the per-frame update of the awards, network and DLC utilities (slot 3).
	virtual void Update();
	virtual bool QHardwareDipSwitch(uint32_t auiDipSwitch);

	// The DLC utility singleton (thiscall, 0x5DFA10; the body ignores this).
	BSDLCSystemUtility& QDLCUtility();

	// The network utility singleton (thiscall, 0x7D6720; the body ignores this).
	BSNetworkSystemUtility& QNetworkUtility();
	// Inferred type: return: BSMsgDialogSystemUtility* from 0x8526cc (result of BSMsgDialogSystemUtility::QInstance),
	// its members used at 0x852345
	BSMsgDialogSystemUtility* GetMsgDialogUtility();
	BSSaveDataSystemUtility* GetSaveDataUtility();
	void SetLostFocus(bool abLostFocus);

	// The singleton, built in a guarded static on first use.
	static BSSystemUtility& QInstance();
	static void operator delete(void* apMemory);

	char cTitle[128];
	// The notification callbacks (the PDB's types); SetLostFocus 0xAF3370 calls the focus one with the new state.
	void (*pEventNotificationCallback)(EVENT_NOTIFICATION);
	void (*pFocusNotificationCallback)(bool);
	bool bVisible;
	uint8_t padding8D[3];
	SYSUTIL_REGION eRegion;
	bool bLostFocus;
	bool bExitApp;
	uint8_t padding96[2];

	static inline int32_t IsCreated;
};
inline constexpr fnvd::Vtable BSSystemUtility_vftable{};
