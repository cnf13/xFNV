#include "Havok/Source/Animation/Animation/Compression/Wavelet/hkaWavelet.h"

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// The SDK's topRow and bottomRow of hkInverseWaveletTransform (`static const hkQuadReal ...[8]`, .bss in the .obj).
using hkInverseWaveletRow = const hkQuadReal[8];

// The function-local constants of hkInverseWaveletTransform, c11 to c28, held at file scope: the coefficients of topRow
// (c11..c18) and bottomRow (c21..c28). Identical ones are shared: c11 and c21 are hkQuadReal1111_0, c18 is c25 (zero)
// and c28 is c16.
static const hkQuadReal c27{-0.25f, 0.75f, -0.25f, -0.25f};
static const hkQuadReal c26{-0.25f, -0.125f, 0.0f, 0.0f};
static const hkQuadReal c25{0.0f, 0.0f, 0.0f, 0.0f};
static const hkQuadReal c24{-0.25f, 0.25f, 0.75f, 0.75f};
static const hkQuadReal c23{-0.25f, -0.25f, -0.25f, -0.25f};
static const hkQuadReal c22{0.5f, 0.5f, 0.5f, 0.5f};
static const hkQuadReal c17{0.0f, 0.0f, 0.0f, -0.125f};
static const hkQuadReal c16{0.0f, -0.125f, -0.25f, 0.75f};
static const hkQuadReal c15{-0.5f, 0.625f, -0.25f, -0.125f};
static const hkQuadReal c14{0.0f, -0.0625f, -0.125f, -0.1875f};
static const hkQuadReal c13{-0.5f, 0.0625f, 0.625f, 0.1875f};
static const hkQuadReal c12{-0.5f, -0.25f, 0.0f, 0.25f};

// detail[i] += liftParam * (scale[i] + scale[i + 1]), the last detail reflected over the end of scale.
void hkPredictReflect(float liftParam, const float* scale, float* detail, int32_t numCoefs) {
    for (int32_t i = 0; i < numCoefs - 1; i++) {
        detail[i] = (scale[i] + scale[i + 1]) * liftParam + detail[i];
    }
    detail[numCoefs - 1] = scale[numCoefs - 1] * (liftParam + liftParam) + detail[numCoefs - 1];
}

// scale[i] += liftParam * (detail[i - 1] + detail[i]), the first scale reflected over the start of detail.
void hkUpdateReflect(float liftParam, float* scale, const float* detail, int32_t numCoefs) {
    for (int32_t i = 1; i < numCoefs; i++) {
        scale[i] = (detail[i - 1] + detail[i]) * liftParam + scale[i];
    }
    *scale = (liftParam + liftParam) * *detail + *scale;
}

// The even samples of in to the first half of out, the odd ones to the second half, through a copy on the thread's
// stack.
void hkSplitOddEven(float* in, float* out, int32_t nValues) {
    float* pTemp = hkAllocateStack<float>(nValues, nullptr);
    hkString::memCpy4(pTemp, in, nValues);
    const int32_t iHalf = nValues >> 1;
    const float* pIn = pTemp;
    for (int32_t i = 0; i < iHalf; i++) {
        out[i] = pIn[2 * i];
        out[i + iHalf] = pIn[2 * i + 1];
    }
    hkDeallocateStack(pTemp);
}

// Interleaves the two halves of the nValues floats at in into out (even elements from the first half), through a copy
// on the thread's stack allocator, so that in and out may be the same.
void hkMergeOddEven(float* in, float* out, int32_t nValues) {
    float* pTemp = hkAllocateStack<float>(nValues, nullptr);
    hkString::memCpy4(pTemp, in, nValues);
    int32_t iHalf = nValues >> 1;
    for (int32_t i = 0; i < iHalf; i++) {
        out[2 * i] = pTemp[i];
        out[2 * i + 1] = pTemp[i + iHalf];
    }
    hkDeallocateStack(pTemp);
}

// Each level from the finest: split into even and odd samples, predict the odd ones and update the even ones.
void hkWaveletTransform(float* data, uint32_t nValues) {
    for (uint32_t i = nValues; i > 1;) {
        hkSplitOddEven(data, data, i);
        i >>= 1;
        hkPredictReflect(-0.5f, data, &data[i], i);
        hkUpdateReflect(0.25f, data, &data[i], i);
    }
}

// The magnitudes are rounded to floats where they are stored.
void hkZeroSmallWaveletCoefficients(
    float* waveletData,
    uint32_t nValues,
    float tolerance,
    hkBool useTruncationFraction
) {
    if (useTruncationFraction.m_bool) {
        float fN = static_cast<float>(nValues);
        if (1.0 / fN > tolerance) {
            return;
        }
        hkLocalArray<float> sortedCoefs(static_cast<int32_t>(nValues));
        sortedCoefs.setSize(static_cast<int32_t>(nValues));
        for (uint32_t i = 0; i < nValues; i++) {
            sortedCoefs.m_data[i] = std::fabs(waveletData[i]);
        }
        int32_t index = static_cast<int32_t>(fN * tolerance - 1.1920928955078125e-07) - 1;
        if (index >= static_cast<int32_t>(nValues)) {
            index = static_cast<int32_t>(nValues) - 1;
        }
        if (index < 0) {
            index = 0;
        }
        if (static_cast<int32_t>(nValues) > 1) {
            hkAlgorithm::quickSortRecursive(
                sortedCoefs.m_data,
                0,
                static_cast<int32_t>(nValues) - 1,
                static_cast<uint8_t>(0)
            );
        }
        const float fLimit = sortedCoefs.m_data[index];
        for (uint32_t i = 0; i < nValues; i++) {
            const float fMagnitude = std::fabs(waveletData[i]);
            if (fMagnitude <= fLimit) {
                waveletData[i] = 0.0f;
            }
        }
    }
    else {
        float fThreshold = tolerance;
        float fMaxZeroed = 0.0f;
        uint32_t uiLevel = nValues >> 1;
        for (uint32_t i = nValues; i != 0;) {
            i--;
            const float fMagnitude = std::fabs(waveletData[i]);
            if (fMagnitude < fThreshold) {
                waveletData[i] = 0.0f;
                if (fMaxZeroed < fMagnitude) {
                    fMaxZeroed = fMagnitude;
                }
            }
            if (i == uiLevel) {
                fThreshold = fThreshold - fMaxZeroed;
                uiLevel >>= 1;
                fMaxZeroed = 0.0f;
            }
        }
    }
}

void hkInverseWaveletTransform(float* data, uint32_t nValues) {
    __m128 v10;
    // The SDK's guarded function-local rows (release hkaInternal.lib: `?topRow@?2??hkInverseWaveletTransform@@9@4...`
    // and `?bottomRow@...`, guard $S1: bit 0 topRow, bit 1 bottomRow), each built from its row's coefficients on the
    // first call.
    static hkInverseWaveletRow topRow(hkQuadReal1111_0, c12, c13, c14, c15, c16, c17, c25);
    static hkInverseWaveletRow bottomRow(hkQuadReal1111_0, c22, c23, c24, c25, c26, c27, c16);
    __m128 v3 = _mm_shuffle_ps(
        _mm_set_ss(std::bit_cast<float>(*reinterpret_cast<uint32_t*>(data))),
        _mm_set_ss(std::bit_cast<float>(*reinterpret_cast<uint32_t*>(data))),
        0
    );
    __m128 v4 = v3;
    float* v5 = data + 1;
    int32_t n7 = 0;
    do {
        __m128 v7 = _mm_set_ss(std::bit_cast<float>(*reinterpret_cast<uint32_t*>(v5)));
        v3 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v7, v7, 0), topRow[n7 + 1]), v3);
        __m128 v8 = _mm_shuffle_ps(v7, v7, 0);
        __m128 v9 = bottomRow[++n7];
        ++v5;
        v10 = _mm_add_ps(_mm_mul_ps(v8, v9), v4);
        v4 = v10;
    }
    while (n7 < 7);
    // The sums are copied out float by float because data need not be aligned.
    data[0] = XFNV_M128_F32(v3, 0);
    data[1] = XFNV_M128_F32(v3, 1);
    data[2] = XFNV_M128_F32(v3, 2);
    data[3] = XFNV_M128_F32(v3, 3);
    uint32_t n8 = 8;
    data[4] = XFNV_M128_F32(v10, 0);
    data[5] = XFNV_M128_F32(v10, 1);
    data[6] = XFNV_M128_F32(v10, 2);
    data[7] = XFNV_M128_F32(v10, 3);
    while (n8 < nValues) {
        hkUpdateReflect(-0.25f, data, &data[n8], n8);
        hkPredictReflect(0.5f, data, &data[n8], n8);
        n8 *= 2;
        hkMergeOddEven(data, data, n8);
    }
}

// Hoare partition around the middle element, recursing on the left part and looping on the right one.
void hkAlgorithm::quickSortRecursive(float* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess) {
    hkAlgorithm::less<float> cmpLess;
    for (;;) {
        int32_t i = aiLow;
        int32_t j = aiHigh;
        const float fPivot = apArr[(aiLow + aiHigh) >> 1];
        do {
            while (cmpLess(apArr[i], fPivot)) {
                ++i;
            }
            while (cmpLess(fPivot, apArr[j])) {
                --j;
            }
            if (i > j) {
                break;
            }
            if (i != j) {
                float* pArr = apArr;
                const float fTmp = pArr[j];
                pArr[j] = pArr[i];
                pArr[i] = fTmp;
            }
            ++i;
            --j;
        }
        while (i <= j);
        if (aiLow < j) {
            hkAlgorithm::quickSortRecursive(apArr, aiLow, j, aiLess);
        }
        if (i >= aiHigh) {
            return;
        }
        aiLow = i;
    }
}
