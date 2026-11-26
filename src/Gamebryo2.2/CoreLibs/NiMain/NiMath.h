#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "xfnv/Compiler.h"

#include <cfloat>
#include <cmath>
#include <limits>

// Fills s_fSinTableA and s_fCosTableA: entry i is the sine and cosine of i * 2pi / 512.
void CreateSinTable();
float NiCos(float fRadians);
float NiSin(float fRadians);
// Quick cosine of fRadians: a table entry, loaded as a float.
float NiCosQ(float afRadians);
// Quick sine of afRadians: a table entry, loaded as a float.
float NiSinQ(float afRadians);
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

inline void NiSinCosQ(float afAngle, float* apSin, float* apCos) {
    int32_t iIndex = static_cast<int32_t>(512.0 / NI_TWO_PI * afAngle);
    *apSin = s_fSinTableA[iIndex & 511];
    *apCos = s_fCosTableA[iIndex & 511];
}

// Each file that includes the header keeps its own copy of NI_INFINITY.
static const float NI_INFINITY = std::numeric_limits<float>::max();

// A random value in [0, 1], rounded to float.
inline float NiUnitRandom() {
    return static_cast<float>(rt::rand() / static_cast<double>(rt::kRandMax));
}

// A random value in [-1, 1], rounded to float.
inline float NiSymmetricRandom() {
    return static_cast<float>(2.0 * rt::rand() / static_cast<double>(rt::kRandMax) - 1.0);
}

// The remainder in double precision, rounded to float.
inline float NiFmod(float fNumerator, float fDenominator) {
    return static_cast<float>(std::fmod(static_cast<double>(fNumerator), static_cast<double>(fDenominator)));
}

// Rounds in the FPU's current rounding mode, not by truncation.
inline int32_t NiFastFloatToInt(float afValue) {
    int32_t iValue;
    xfnv::RoundToInt(iValue, afValue);
    return iValue;
}

inline float NiAbs(float fValue) {
    return std::fabs(fValue);
}

inline float NiSqrt(float fValue) {
    return std::sqrt(fValue);
}

inline float NiCos(float fRadians) {
    return std::cos(fRadians);
}

inline float NiSin(float fRadians) {
    return std::sin(fRadians);
}

inline float NiATan2(float fNumerator, float fDenominator) {
    return std::atan2(fNumerator, fDenominator);
}

inline float NiASin(float fValue) {
    if (fValue > -1.0) {
        if (fValue < 1.0) {
            return std::asin(fValue);
        }
        return NI_HALF_PI;
    }
    return -NI_HALF_PI;
}

inline float NiSign(float fValue) {
    if (fValue > 0.0) {
        return 1.0f;
    }

    if (!(fValue < 0.0)) {
        return 0.0f;
    }

    return -1.0f;
}

inline float NiSinQ(float afRadians) {
    return s_fSinTableA[static_cast<int32_t>(512.0 / NI_TWO_PI * afRadians) & 511];
}

inline float NiCosQ(float afRadians) {
    return s_fCosTableA[static_cast<int32_t>(512.0 / NI_TWO_PI * afRadians) & 511];
}

inline double NiATan(float afX) {
    return std::atan(afX);
}
