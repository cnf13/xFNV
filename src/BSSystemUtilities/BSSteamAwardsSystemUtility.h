#pragma once

#include "BSCore/BSSimpleList.h"
#include "Reconstruction.hpp"

char return_0_1();

// The Steam achievements system utility. IDA-only; the PDB has no such class.
class BSSteamAwardsSystemUtility {
public:
	BSSteamAwardsSystemUtility();
	~BSSteamAwardsSystemUtility();

	// Unlocks achievement aiAchievement (thiscall, ret 4, 0xAF2420; the PDB's BSAwardsSystemUtility::Unlock).
	bool Unlock(int32_t aiAchievement);
	// Runs the per-frame update.
	void Idle();

	// The utility, built in a guarded static on first use (cdecl, ret 0, 0xAF22D0; the PDB's
	// BSAwardsSystemUtility::QInstance).
	static BSSteamAwardsSystemUtility* QInstance();
	// The PDB's BSAwardsSystemUtility::Initialize. Builds the utility; returns QInstance's result, which the exe
	// leaves in eax (cdecl, ret 0, 0xAF2360).
	static BSSteamAwardsSystemUtility* Initialize();
	static void operator delete(void* apMemory);

	// The achievement IDs to unlock, queued by Unlock and awarded by Idle (the PDB's BSAwardsSystemUtility::m_list,
	// a BSSimpleList<unsigned char*> there; IDA's BSSimpleList_void kAchievements). The PC exe stores a pointer to
	// each 4-byte ID: Unlock allocates it (`push 4; call MemoryManager::Allocate`, 0xAF2429) and appends it through
	// the folded pointer-list AddTail (0xAF25B0, call at 0xAF2462); Idle dereferences the item and compares it as a
	// signed int (`mov ecx,[eax]; cmp [ebp-8],0x64; jle`, 0xAF24FE, 0xAF2517) before freeing it.
	BSSimpleList<int32_t*> m_list;

	static inline BSSteamAwardsSystemUtility* m_pInstance;
};
