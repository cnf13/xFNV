#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

class hkVector4;

// Fits a NURBS curve of the given degree within tolerance to the numSamples samples sampleData at sampleTimes;
// writes the knot and control point counts and copies the knots and control points out (the SDK's signature;
// hkaNURBS.cpp, cdecl, ret 0, 0xE58740).
void ApproximateNURBSCurve(int32_t numSamples, const float* sampleTimes, const hkVector4* sampleData, int32_t degree,
	float tolerance, int32_t* numKnotsOut, float* knotsOut, int32_t* numPointsOut, hkVector4* pointsOut);

// How a block of quantized animation data is encoded: the bit width of each value, the symbol that marks a run and
// the number of values kept unencoded at its start. hkBlockDecode reads m_bitWidth (+0) and m_preserved
// (+8).
struct hkaBlockDesc {
	uint32_t m_bitWidth;
	uint32_t m_runSymbol;
	uint32_t m_preserved;
};
