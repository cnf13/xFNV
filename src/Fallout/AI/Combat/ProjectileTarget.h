#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiAVObject;
class TESObjectREFR;

// What a projectile is aimed at: the reference, the part of its model and the offset from that part.
class ProjectileTarget {
public:
	ProjectileTarget();
	~ProjectileTarget();

	NiPoint3* GetWorldLocation(NiPoint3* apResult);

	static void operator delete(void* apMemory);

	TESObjectREFR* pReference;
	NiPointer<NiAVObject> spPart;
	NiPoint3 kOffset;
};
