#pragma once

#include "BSCore/BSSimpleList.h"

char return_0_1();

// The Steam achievements system utility.
class BSSteamAwardsSystemUtility {
public:
	BSSteamAwardsSystemUtility();
	~BSSteamAwardsSystemUtility();

	bool Unlock(int32_t aiAchievement);
	// Runs the per-frame update.
	void Idle();

	// The utility, built in a guarded static on first use.
	static BSSteamAwardsSystemUtility* QInstance();
	// Builds the utility and returns QInstance's result.
	static BSSteamAwardsSystemUtility* Initialize();
	static void operator delete(void* apMemory);

	// The achievement IDs to unlock, queued by Unlock and awarded by Idle. Each item is a pointer to a 4-byte ID:
	// Unlock allocates it and appends it; Idle dereferences the item and compares it as a signed int before freeing it.
	BSSimpleList<int32_t*> m_list;

	static inline BSSteamAwardsSystemUtility* m_pInstance;
};
