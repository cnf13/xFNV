#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

#include <cfloat>

// Fills s_fSinTableA and s_fCosTableA: entry i is the sine and cosine of i * 2pi / 512.
void CreateSinTable();
// The cosine of fRadians through the float cos (cdecl, ST0 float, 0x5B9E80).
float NiCos(float fRadians);
// The sine of fRadians through the float sin (cdecl, ST0 float, 0x5C53D0).
float NiSin(float fRadians);
// NiCosQ, the PDB's quick cosine of fRadians: a table entry, loaded as a float (cdecl, ST0
// float, 0x57E960).
float NiCosQ(float afRadians);
// NiSinQ, the PDB's quick sine of fRadians: a table entry, loaded as a float (cdecl, ST0
// float, 0x63C600).
float NiSinQ(float afRadians);
// A copy of NiSymmetricRandom the exe calls out of line, through the rand wrapper 0x6397B0 where the inline form
// calls rand: * 2.0 / 32767.0 + -1.0, rounded to float (cdecl, ST0, 0x6397C0; called by
// Precipitation::CreateShaderGeometry and NiPSysGravityModifier::Update).
// The PDB's NiSymmetricRandom (Fallout Shared\sky\Precipitation.cpp); the suffix tells this copy from the inline one.
float NiSymmetricRandom_6397C0();
float NiSqrt(float fValue);
void NiSinCos(float fRadians, float& fSin, float& fCos);
// The arc cosine of fValue clamped to [-1, 1], as a float (cdecl, 0xA58510, returned in ST(0)).
float NiACos(float fValue);
// The arc sine of fValue clamped to [-1, 1]: -pi/2 at or below -1, pi/2 above 1 (both from
// halved_pi), else asin(float) (cdecl, ST0 float, 0x4B54C0).
float NiASin(float fValue);
float NiFastATan2(float fY, float fX);

// Gamebryo's NiMath.h constants. NI_HALF_PI and NI_TWO_PI are statics the exe computes at startup in every file that
// includes the header: one copy per file.
const float NI_PI = 3.1415927f;
static const float NI_HALF_PI = 0.5f * NI_PI;
static const float NI_TWO_PI = 2.0f * NI_PI;

// The 512-entry lookup tables covering one turn, filled by CreateSinTable.
using SinCosTable = float[512];
inline SinCosTable s_fSinTableA{};
inline SinCosTable s_fCosTableA{};
inline float NiFastATan2_NI_HALF_PI = 1.5707964f;

// Gamebryo's `const float NI_INFINITY = FLT_MAX;`: the exe keeps a .rdata copy in every file that includes the header
// (the PDB lists it per module; 413 FLT_MAX dwords in the exe's .rdata, 352 of them unreferenced).
static const float NI_INFINITY = FLT_MAX;

// A random value in [0, 1], rounded to float.
inline float NiUnitRandom() {
	return static_cast<float>(rt::rand() / 32767.0);
}

// A random value in [-1, 1], rounded to float.
inline float NiSymmetricRandom() {
	return static_cast<float>(2.0 * rt::rand() / 32767.0 - 1.0);
}

// fld/fistp: rounds in the FPU's current rounding mode, not by truncation.
inline int32_t NiFastFloatToInt(float afValue) {
	int32_t iValue;
	__asm__("flds %1\n\tfistpl %0" : "=m"(iValue) : "m"(afValue));
	return iValue;
}
