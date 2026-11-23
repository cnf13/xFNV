#pragma once

#include "Havok/Source/Animation/Animation/Compression/Spline/hkaNURBS.h"

// Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
void hkPredictReflect(float liftParam, const float* scale, float* detail, int32_t numCoefs);
// Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
void hkUpdateReflect(float liftParam, float* scale, const float* detail, int32_t numCoefs);
// Havok SDK signature (hkaInternal.lib, tied by bytes,reloc).
void hkSplitOddEven(float* in, float* out, int32_t nValues);
// Havok SDK signature (hkaInternal.lib, tied by bytes).
void hkMergeOddEven(float* in, float* out, int32_t nValues);
// The lifting wavelet transform of the n floats of data, in place.
void hkWaveletTransform(float* data, uint32_t nValues);
// Zeroes the n coefficients of data whose magnitude is below the threshold: with sorted, those at or below the
// magnitude at the threshold's fraction of n; else, per level from the finest, those below a threshold lowered by each
// level's largest zeroed magnitude.
void hkZeroSmallWaveletCoefficients(
    float* waveletData,
    uint32_t nValues,
    float tolerance,
    hkBool useTruncationFraction
);
// Inverts hkWaveletTransform on the nValues floats of data.
void hkInverseWaveletTransform(float* data, uint32_t nValues);
