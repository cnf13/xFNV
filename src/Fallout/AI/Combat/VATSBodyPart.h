#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

class Tile;

// One body part of a VATS target the player can pick: where it is on screen and in the world, its chance to hit and
// the meter tile that shows it. The PDB does not know this class; every name here is IDA's.
class VATSBodyPart {
public:
	VATSBodyPart();

	NiPoint2 kScreenPos;
	NiPoint3 kRelativePos;
	NiPoint3 kPos;
	// IDA name; not in the PDB. IDA's enum BODY_PART_TYPE (src has no such enum).
	uint32_t eBodyPart;
	float fPercentVisible;
	float fChance;
	bool bOnScreen;
	bool bChanceCalculated;
	bool bFirstTimeShown;
	bool bNeedsRecalc;
	Tile* pMeter;
	float unk34;
	bool byte38;
};
