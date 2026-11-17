#pragma once

#include "Fallout/AI/Combat/MissileProjectile.h"

// Creation info for the hinge-limits constraint; parameters are held by the base's constraint data.
struct hkHingeLimitsConstraintCinfo : hkConstraintCinfo {
	hkHingeLimitsConstraintCinfo();
	~hkHingeLimitsConstraintCinfo() override;

	void CloneData(hkpConstraintData*, NiCloningProcess&) override;
	void LoadBinary(NiStream&) override;
	void SaveBinary(NiStream&) override;
	uint32_t CreateData() override;
	void GetViewerStrings(NiTPrimitiveArray<char*>*) override;

	// Frees the info through MemoryManager::Deallocate.
	static void operator delete(void* apMemory);
};
