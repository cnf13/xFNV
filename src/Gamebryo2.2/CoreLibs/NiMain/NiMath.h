#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cfloat>

// Fills s_fSinTableA and s_fCosTableA: entry i is the sine and cosine of i * 2pi / 512.
void CreateSinTable();
float NiCos(float fRadians);
float NiSin(float fRadians);
// Quick cosine of fRadians: a table entry, loaded as a float.
float NiCosQ(float afRadians);
// Quick sine of afRadians: a table entry, loaded as a float.
float NiSinQ(float afRadians);
// An out-of-line copy of NiSymmetricRandom that calls through a rand wrapper where the inline form calls rand: rand *
// 2.0 / 32767.0 + -1.0, rounded to float. Called by Precipitation::CreateShaderGeometry and
// NiPSysGravityModifier::Update.
float NiSymmetricRandom_6397C0();
float NiSqrt(float fValue);
void NiSinCos(float fRadians, float& fSin, float& fCos);
// The arc cosine of fValue clamped to [-1, 1], as a float.
float NiACos(float fValue);
// The arc sine of fValue clamped to [-1, 1]: -pi/2 at or below -1, pi/2 above 1, else asin(float).
float NiASin(float fValue);
float NiFastATan2(float fY, float fX);

// NI_HALF_PI and NI_TWO_PI are statics computed at startup in every file that includes the header: one copy per file.
const float NI_PI = 3.1415927f;
static const float NI_HALF_PI = 0.5f * NI_PI;
static const float NI_TWO_PI = 2.0f * NI_PI;

// The 512-entry lookup tables covering one turn, filled by CreateSinTable.
using SinCosTable = float[512];
inline SinCosTable s_fSinTableA{};
inline SinCosTable s_fCosTableA{};
inline float NiFastATan2_NI_HALF_PI = 1.5707964f;

// Each file that includes the header keeps its own copy of NI_INFINITY, as the game does.
static const float NI_INFINITY = FLT_MAX;

// A random value in [0, 1], rounded to float.
inline float NiUnitRandom() {
    return static_cast<float>(rt::rand() / 32767.0);
}

// A random value in [-1, 1], rounded to float.
inline float NiSymmetricRandom() {
    return static_cast<float>(2.0 * rt::rand() / 32767.0 - 1.0);
}

// Rounds in the FPU's current rounding mode, not by truncation.
inline int32_t NiFastFloatToInt(float afValue) {
    int32_t iValue;
    __asm__("flds %1\n\tfistpl %0" : "=m"(iValue) : "m"(afValue));
    return iValue;
}
