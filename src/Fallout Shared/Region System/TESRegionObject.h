#pragma once

#include "Fallout Shared/Region System/TESRegionObjectBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

class TESForm;
class TESRegionObjectList;

// A region's object entry (trees, rocks and the like). The PC exe has no vtable, constructor or accessor of it of
// its own: ConvertNormalToAngle (0x4F6E00), which TESRegionGrassObject::GetGenerateChance calls, IsFormLandTexture
// (0x4F6F70), which reads pForm at +4, and TESRegionObjectList::IsInListTOL (0x4F6E60), which reads the items'
// pChildren at +0x34 (through the one-field getter 0x5F36F0). Data and pParent follow the PDB; no PC function is
// known to touch them.
class TESRegionObject : public TESRegionObjectBase {
public:
	// The object's placement settings (the PDB's TESRegionObject::REGION_OBJECT_DATA, sizeof 0x2C).
	struct REGION_OBJECT_DATA {
		float fDensity;
		uint8_t cCluster;
		uint8_t cMinSlopeDegrees;
		uint8_t cMaxSlopeDegrees;
		uint8_t cObjectFlags;
		uint16_t sParentRadius;
		uint16_t sGenerateNearRadius;
		float fMinimumZ;
		float fMaximumZ;
		float fSinkBase;
		float fSinkVariance;
		float fScaleVariance;
		int16_t sXAngleVariance;
		int16_t sYAngleVariance;
		int16_t sZAngleVariance;
		uint32_t dVertexColor;
	};

	// Whether the object's form (pForm) is a TESLandTexture.
	bool IsFormLandTexture();

	// The slope angle of aNormal: acosf of z over the length.
	static float ConvertNormalToAngle(NiPoint3 aNormal);

	TESForm* pForm;
	REGION_OBJECT_DATA Data;
	TESRegionObjectList* pChildren;
	TESRegionObject* pParent;
};
