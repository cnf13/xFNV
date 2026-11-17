#pragma once

#include "Fallout/AI/Combat/MissileProjectile.h"

// Creation info for a point-to-path constraint; the constructor initializes only the base's members.
struct hkPointToPathConstraintCinfo : hkConstraintCinfo {
	hkPointToPathConstraintCinfo();
	~hkPointToPathConstraintCinfo() override;

	void CloneData(hkpConstraintData*, NiCloningProcess&) override;
	void LoadBinary(NiStream&) override;
	void SaveBinary(NiStream&) override;
	uint32_t CreateData() override;
	void GetViewerStrings(NiTPrimitiveArray<char*>*) override;
};
