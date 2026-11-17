#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// The game's water splash effects. Only the members this code calls are declared.
class BGSWaterCollisionManager {
public:
    // Plays the splash sound sized for afSplashingBodySize at arSplashLocation; the point is passed by value.
    static void PlaySplashEffects(float afSplashingBodySize, NiPoint3 arSplashLocation);
};
