#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class Tile;

// One body part of a VATS target the player can pick: where it is on screen and in the world, its chance to hit and the
// meter tile that shows it.
class VATSBodyPart {
public:
	VATSBodyPart();

	NiPoint2 kScreenPos;
	NiPoint3 kRelativePos;
	NiPoint3 kPos;
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
