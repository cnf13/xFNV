#pragma once

#include "BSCore/BSSimpleList.h"

class TESObjectREFR;
class VATSBodyPart;

// One target VATS can shoot at: the reference, how it is classed, its targetable body parts and the angle and distance
// from the player.
class VATSTarget {
public:
	VATSTarget();
	~VATSTarget();

	static void operator delete(void* apMemory);

	TESObjectREFR* pTargetRef;
	uint32_t eType;
	BSSimpleList<VATSBodyPart*> kBodyParts;
	bool bChancesCalculated;
	float unk14;
	float fAngle;
	float fDistance;
	bool bHostile;
};
