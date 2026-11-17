#pragma once

#include "Fallout/AI/Combat/MissileProjectile.h"

// Creation info for a point-to-path constraint; the exe constructor initializes only the base's five words.
struct hkPointToPathConstraintCinfo : hkConstraintCinfo {
	hkPointToPathConstraintCinfo();
	~hkPointToPathConstraintCinfo() override;

	void CloneData(hkpConstraintData*, NiCloningProcess&) override;
	void LoadBinary(NiStream&) override;
	void SaveBinary(NiStream&) override;
	uint32_t CreateData() override;
	void GetViewerStrings(NiTPrimitiveArray<char*>*) override;
};
inline constexpr fnvd::Vtable hkPointToPathConstraintCinfo_vftable{};
