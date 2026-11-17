#pragma once

#include <cstdint>

// Packs float arrays into 16-bit integers spread over [fOffset - fHalfRange, fOffset + fHalfRange].
class NiCompUtility {
public:
	// Expands uiDestCount packed values into pfDestArray as fOffset + s / 32767 * fHalfRange; uiSrcCount is ignored.
	static void DecompressFloatArray(const int16_t* psSrcArray, uint32_t uiSrcCount, float fOffset, float fHalfRange,
		float* pfDestArray, uint32_t uiDestCount);
};
