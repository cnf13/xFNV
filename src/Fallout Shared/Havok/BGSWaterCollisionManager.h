#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// IDA name; not in the PDB. The game's water splash effects. Only the members the reconstructed code calls are
// declared.
class BGSWaterCollisionManager {
public:
	// The PDB's static TESWaterListener::PlaySplashEffects(afSplashingBodySize, aSplashLocation), in
	// TESWaterListener.cpp; src keeps IDA's class until its callers move with it. Plays the splash sound sized for
	// afSplashingBodySize at arSplashLocation (cdecl, ret 0, 0x62EB50; the point is passed by value).
	static void PlaySplashEffects(float afSplashingBodySize, NiPoint3 arSplashLocation);
};
