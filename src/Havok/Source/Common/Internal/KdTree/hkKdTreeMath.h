#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkIntVector.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeUtils.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"

#include <emmintrin.h>
#include <xmmintrin.h>

// 1 / 65535 in every lane: a projected entry's 16-bit bound times the tree's extent times this is its offset from the
// tree's minimum.
inline hkQuadReal hkGetOneOver65K() {
    static hkQuadReal x{1.5259022e-05f, 1.5259022e-05f, 1.5259022e-05f, 1.5259022e-05f};
    return x;
}

// The kd-tree's vector helpers: projected entry to vector and box conversions and the ray bundle box test. The class
// has no data.
class hkKdTreeMath {
public:
    // The entry's minimum and maximum words as floats in v1 and v2, the id words in w. The SDK and the exe read the
    // 16-aligned entry's raw bytes as four integers (an hkIntVector). Where the id halves are 32 bits the entry is not
    // those eight words: they are gathered from its fields. Each 32-bit lane's high half is scaled by 1e16, as the game
    // has it; the halves are 0 after the unpack, so the product is 0.
    static void convertProjectedEntryToHkVector4(const hkKdTreeProjectedEntry* splits, hkVector4& v1, hkVector4& v2) {
        const hkQuadReal highWordScale = _mm_set1_ps(1e16f);
        hkIntVector b;
        if constexpr (sizeof(hkHalfPointer) == sizeof(uint16_t)) {
            b = *reinterpret_cast<const hkIntVector*>(splits);
        }
        else {
            b.m_quad = _mm_set_epi16(
                static_cast<int16_t>(splits->m_primIdLow),
                static_cast<int16_t>(splits->m_max[2]),
                static_cast<int16_t>(splits->m_max[1]),
                static_cast<int16_t>(splits->m_max[0]),
                static_cast<int16_t>(splits->m_primIdHigh),
                static_cast<int16_t>(splits->m_min[2]),
                static_cast<int16_t>(splits->m_min[1]),
                static_cast<int16_t>(splits->m_min[0])
            );
        }
        const hkQuadUint zeroInt = _mm_setzero_si128();
        const hkQuadUint minUint = _mm_unpacklo_epi16(b.m_quad, zeroInt);
        const hkQuadUint maxUint = _mm_unpackhi_epi16(b.m_quad, zeroInt);
        v1.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(minUint, 16)), highWordScale),
            _mm_cvtepi32_ps(_mm_srli_epi32(_mm_slli_epi32(minUint, 16), 16))
        );
        v2.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(maxUint, 16)), highWordScale),
            _mm_cvtepi32_ps(_mm_srli_epi32(_mm_slli_epi32(maxUint, 16), 16))
        );
    }

    // The entry's box in the tree's space (offset plus its words times scaleIn / 65535), each axis's minimum and
    // maximum broadcast: vecsOut holds x min, x max, y min, y max, z min, z max.
    static void getAabbVecsFromProjectedEntry(
        const hkKdTreeProjectedEntry* aabbData,
        hkVector4* vecsOut,
        const hkVector4& offset,
        const hkVector4& scaleIn
    ) {
        const hkQuadReal scale = _mm_mul_ps(scaleIn.m_quad, hkGetOneOver65K());
        hkVector4 vMin;
        hkVector4 vMax;
        convertProjectedEntryToHkVector4(aabbData, vMin, vMax);
        const hkQuadReal aabbMin = _mm_add_ps(_mm_mul_ps(vMin.m_quad, scale), offset.m_quad);
        const hkQuadReal aabbMax = _mm_add_ps(_mm_mul_ps(vMax.m_quad, scale), offset.m_quad);
        vecsOut[0].m_quad = _mm_shuffle_ps(aabbMin, aabbMin, 0x00);
        vecsOut[1].m_quad = _mm_shuffle_ps(aabbMax, aabbMax, 0x00);
        vecsOut[2].m_quad = _mm_shuffle_ps(aabbMin, aabbMin, 0x55);
        vecsOut[3].m_quad = _mm_shuffle_ps(aabbMax, aabbMax, 0x55);
        vecsOut[4].m_quad = _mm_shuffle_ps(aabbMin, aabbMin, 0xAA);
        vecsOut[5].m_quad = _mm_shuffle_ps(aabbMax, aabbMax, 0xAA);
    }

    // The rays of the bundle whose [tNear, tFar] still overlaps the box of aabbData (getAabbVecsFromProjectedEntry's
    // six vectors) after clipping by each axis's slab. A ray parallel to an axis keeps its range there and passes
    // only when its origin is within the slab.
    static hkVector4Comparison rayBundleAabbCheck(
        const hkKdTreeUtils::RayBundle& orig,
        const hkKdTreeUtils::RayBundle& invDir,
        const hkVector4& tNearIn,
        const hkVector4& tFarIn,
        const hkVector4* aabbData
    ) {
        hkQuadReal tNear = tNearIn.m_quad;
        hkQuadReal tFar = tFarIn.m_quad;
        hkQuadReal flatOrigOK = hkVector4Comparison::s_maskFromBits[hkVector4Comparison::MASK_XYZW];
        for (int32_t i = 0; i < 3; ++i) {
            const hkQuadReal boxMin = aabbData[2 * i].m_quad;
            const hkQuadReal boxMax = aabbData[2 * i + 1].m_quad;
            const hkQuadReal origin = orig.m_vec[i].m_quad;
            const hkQuadReal inv = invDir.m_vec[i].m_quad;
            const hkQuadReal dNearTemp = _mm_mul_ps(_mm_sub_ps(boxMin, origin), inv);
            const hkQuadReal dFarTemp = _mm_mul_ps(_mm_sub_ps(boxMax, origin), inv);
            const hkQuadReal zero = _mm_setzero_ps();
            const hkQuadReal dirGeZero = _mm_cmple_ps(zero, inv);
            const hkQuadReal dirEqZero = _mm_cmpeq_ps(zero, inv);
            const hkQuadReal inRange = _mm_and_ps(_mm_cmple_ps(origin, boxMax), _mm_cmple_ps(boxMin, origin));
            flatOrigOK = _mm_and_ps(
                _mm_or_ps(_mm_and_ps(inRange, dirEqZero), _mm_andnot_ps(dirEqZero, flatOrigOK)),
                flatOrigOK
            );
            const hkQuadReal dNear = _mm_or_ps(_mm_and_ps(dirGeZero, dNearTemp), _mm_andnot_ps(dirGeZero, dFarTemp));
            const hkQuadReal dFar = _mm_or_ps(_mm_and_ps(dirGeZero, dFarTemp), _mm_andnot_ps(dirGeZero, dNearTemp));
            tNear = _mm_max_ps(_mm_or_ps(_mm_andnot_ps(dirEqZero, dNear), _mm_and_ps(tNear, dirEqZero)), tNear);
            tFar = _mm_min_ps(_mm_or_ps(_mm_andnot_ps(dirEqZero, dFar), _mm_and_ps(tFar, dirEqZero)), tFar);
        }
        hkVector4Comparison tOk;
        tOk.m_mask = _mm_and_ps(_mm_cmplt_ps(tNear, tFar), flatOrigOK);
        return tOk;
    }
};
