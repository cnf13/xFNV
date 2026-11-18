#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppRayBundleVirtualMachine.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbCastUtils.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayBundleCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverCheck.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"

#include <xmmintrin.h>

static __m128* calcSlabIntervals(const __m128* a1, const __m128* a2, __m128* a3, __m128* r_eax);
static int32_t isMajorityActive(__m128* a1, __m128* a2, __m128* r_eax);

// Divides both ends by their difference: *a3 = a2 / (a2 - a1), *r_eax = -(a1 / (a2 - a1)), with the approximate
// reciprocal refined by one Newton step; returns r_eax.
// KEEP: the exe body writes only xmm0-xmm3 (and edx), and its caller hkpMoppRayBundleVirtualMachine::queryRayOnTree
// keeps xmm6/xmm7 (call at 0xDD3F71) and xmm5 (call at 0xDD40B1) live across the call; clang does not guarantee
// that.
static __m128* calcSlabIntervals(const __m128* a1, const __m128* a2, __m128* a3, __m128* r_eax) {
    __m128 v4 = _mm_sub_ps(*a2, *a1);
    __m128 v5 = _mm_rcp_ps(v4);
    __m128 v6 = _mm_sub_ps(_mm_add_ps(v5, v5), _mm_mul_ps(_mm_mul_ps(v5, v4), v5));
    *a3 = _mm_mul_ps(*a2, v6);
    __m128 v7 = _mm_mul_ps(*a1, v6);
    *r_eax = v7;
    *r_eax = _mm_xor_ps(v7, hkQuadRealNegateMask);
    return r_eax;
}

static int32_t isMajorityActive(__m128* a1, __m128* a2, __m128* r_eax) {
    __m128 v3 = _mm_mul_ps(
        _mm_or_ps(_mm_andnot_ps(*r_eax, _mm_setzero_ps()), _mm_and_ps(*r_eax, hkQuadReal1111)), hkQuadReal1111
    );
    __m128 v4 = _mm_add_ps(_mm_shuffle_ps(v3, v3, 78), v3);
    __m128 v5 = _mm_and_ps(_mm_cmplt_ps(*a1, *a2), *r_eax);
    __m128 v6 =
        _mm_mul_ps(_mm_or_ps(_mm_andnot_ps(v5, _mm_setzero_ps()), _mm_and_ps(v5, hkQuadReal1111)), hkQuadReal1111);
    __m128 v7 = _mm_add_ps(_mm_shuffle_ps(v4, v4, 177), v4);
    __m128 v8 = _mm_add_ps(_mm_shuffle_ps(v6, v6, 78), v6);
    __m128 v9 = _mm_add_ps(_mm_shuffle_ps(v8, v8, 177), v8);
    __m128 v10 = _mm_shuffle_ps(v9, v9, 0);
    return _mm_movemask_ps(_mm_cmple_ps(_mm_shuffle_ps(v7, v7, 0), _mm_add_ps(v10, v10)));
}

// True when every lane of *a1 or of the complement of *r_eax is set.
int32_t allActiveSet(__m128* a1, const __m128* r_eax) {
    return _mm_movemask_ps(_mm_or_ps(_mm_andnot_ps(*r_eax, rdata_10CE1C0), *a1)) == 15;
}

// Converts four bytes at r_eax to the four floats at a1, zero-extending each byte; returns the fourth byte.
int32_t convertBytesToFloats(float* a1, const uint8_t* r_eax) {
    *a1 = static_cast<float>(static_cast<int32_t>(r_eax[0]));
    a1[1] = static_cast<float>(static_cast<int32_t>(r_eax[1]));
    a1[2] = static_cast<float>(static_cast<int32_t>(r_eax[2]));
    int32_t result = r_eax[3];
    a1[3] = static_cast<float>(result);
    return result;
}

// Blends the three rows a2 * a6 + a3 * a5 into a4 under the mask at r_esi and into r_eax under the mask a1; returns
// r_eax.
__m128* selectInterpolatedPoints(
    const __m128* a1,
    const __m128* a2,
    const __m128* a3,
    __m128* a4,
    const __m128* a5,
    const __m128* a6,
    __m128* r_eax,
    const __m128* r_esi
) {
    __m128 v8 = _mm_add_ps(_mm_mul_ps(*a3, *a5), _mm_mul_ps(*a6, *a2));
    *a4 = _mm_or_ps(_mm_andnot_ps(*r_esi, *a4), _mm_and_ps(*r_esi, v8));
    *r_eax = _mm_or_ps(_mm_andnot_ps(*a1, *r_eax), _mm_and_ps(*a1, v8));
    __m128 v9 = _mm_add_ps(_mm_mul_ps(a2[1], *a6), _mm_mul_ps(a3[1], *a5));
    a4[1] = _mm_or_ps(_mm_and_ps(*r_esi, v9), _mm_andnot_ps(*r_esi, a4[1]));
    r_eax[1] = _mm_or_ps(_mm_andnot_ps(*a1, r_eax[1]), _mm_and_ps(*a1, v9));
    __m128 v10 = _mm_add_ps(_mm_mul_ps(a2[2], *a6), _mm_mul_ps(a3[2], *a5));
    a4[2] = _mm_or_ps(_mm_andnot_ps(*r_esi, a4[2]), _mm_and_ps(*r_esi, v10));
    r_eax[2] = _mm_or_ps(_mm_and_ps(*a1, v10), _mm_andnot_ps(*a1, r_eax[2]));
    return r_eax;
}

// Writes the lanes where *a1 * *a2 is negative as a mask to r_eax and returns r_eax.
__m128* calcSignDiffersMask(const __m128* a1, const __m128* a2, __m128* r_eax) {
    *r_eax = _mm_cmplt_ps(_mm_mul_ps(*a1, *a2), _mm_setzero_ps());
    return r_eax;
}

hkpMoppRayBundleVirtualMachine::QueryInt* hkpMoppRayBundleVirtualMachine::QueryInt::Assign(
    hkpMoppRayBundleVirtualMachine::QueryInt* apThis,
    const hkpMoppRayBundleVirtualMachine::QueryInt& arOther
) {
    *apThis = arOther;
    return apThis;
}

hkpMoppRayBundleVirtualMachine::QueryBundle* hkpMoppRayBundleVirtualMachine::QueryBundle::Assign(
    hkpMoppRayBundleVirtualMachine::QueryBundle* apThis,
    const hkpMoppRayBundleVirtualMachine::QueryBundle& arOther
) {
    hkVector4* v3 = &apThis->m_rayEnds[0].m_vec[2];
    const hkVector4* v4 = &arOther.m_rayEnds[0].m_vec[2];
    int32_t v5 = 2;
    do {
        v3[-2].m_quad = v4[-2].m_quad;
        v3[-1].m_quad = v4[-1].m_quad;
        (*v3).m_quad = v4->m_quad;
        v3 += 3;
        v4 += 3;
        --v5;
    }
    while (v5);
    apThis->m_activeMask = arOther.m_activeMask;
    return apThis;
}

// Stores the collection, the result array, the input rays and the code, converts the rays into the code's integer space
// as the root bundle under the caller's mask, walks the tree from the code's start and returns the lanes hit through
// apResult.
hkVector4Comparison hkpMoppRayBundleVirtualMachine::queryRayBundle(
    const hkpShapeContainer* collection,
    const hkpMoppCode* code,
    const hkpShapeRayBundleCastInput& input,
    hkpShapeRayBundleCastOutput& rayResults,
    const hkVector4Comparison& mask
) {
    QueryBundle kQueryBundle;
    QueryInt kScaledQuery;
    m_earlyOutHitFraction.m_quad = _mm_set1_ps(1.0f);
    m_collection = collection;
    m_rayResultPtr = &rayResults;
    m_from.m_vec[0] = input.m_from[0];
    m_from.m_vec[1] = input.m_from[1];
    m_from.m_vec[2] = input.m_from[2];
    m_to.m_vec[0] = input.m_to[0];
    m_to.m_vec[1] = input.m_to[1];
    m_to.m_vec[2] = input.m_to[2];
    m_code = code;
    const uint8_t* pCommands = code->m_data.m_data;
    kScaledQuery.m_FtoBoffset.m_quad = _mm_setzero_ps();
    const float fItoFScale = 1.0f / code->m_info.m_offset(3);
    m_ItoFScale.m_quad = _mm_set1_ps(fItoFScale);
    // Multiplies by the double 1/65536 and stores the product as a float.
    const float fFtoBScale = code->m_info.m_offset(3) * 1.52587890625e-05;
    const hkQuadReal kFtoBScale = _mm_set1_ps(fFtoBScale);
    const hkQuadReal kOffsetX = _mm_shuffle_ps(code->m_info.m_offset.m_quad, code->m_info.m_offset.m_quad, 0x00);
    const hkQuadReal kOffsetY = _mm_shuffle_ps(code->m_info.m_offset.m_quad, code->m_info.m_offset.m_quad, 0x55);
    const hkQuadReal kOffsetZ = _mm_shuffle_ps(code->m_info.m_offset.m_quad, code->m_info.m_offset.m_quad, 0xAA);
    kQueryBundle.m_rayEnds[0].m_vec[0].m_quad = _mm_mul_ps(_mm_sub_ps(m_from.m_vec[0].m_quad, kOffsetX), kFtoBScale);
    kQueryBundle.m_rayEnds[1].m_vec[0].m_quad = _mm_mul_ps(kFtoBScale, _mm_sub_ps(m_to.m_vec[0].m_quad, kOffsetX));
    kQueryBundle.m_rayEnds[0].m_vec[1].m_quad = _mm_mul_ps(kFtoBScale, _mm_sub_ps(m_from.m_vec[1].m_quad, kOffsetY));
    kQueryBundle.m_rayEnds[1].m_vec[1].m_quad = _mm_mul_ps(kFtoBScale, _mm_sub_ps(m_to.m_vec[1].m_quad, kOffsetY));
    kQueryBundle.m_rayEnds[0].m_vec[2].m_quad = _mm_mul_ps(kFtoBScale, _mm_sub_ps(m_from.m_vec[2].m_quad, kOffsetZ));
    kQueryBundle.m_rayEnds[1].m_vec[2].m_quad = _mm_mul_ps(_mm_sub_ps(m_to.m_vec[2].m_quad, kOffsetZ), kFtoBScale);
    kQueryBundle.m_activeMask = mask;
    kScaledQuery.m_FtoBScale.m_quad = kFtoBScale;
    m_rays = &input;
    kScaledQuery.m_primitiveOffset = 0;
    kScaledQuery.m_shift = 0;
    kScaledQuery.m_properties[0] = 0;
    m_hitsFound.m_mask = hkVector4Comparison_s_maskFromBits[hkVector4Comparison::MASK_NONE];
    // A code that starts with DATA_OFFSET puts the chunk id into its shape keys.
    m_reindexingMask = (*pCommands == HK_MOPP_DATA_OFFSET) ? -1 : 0;
    queryRayOnTree(&kScaledQuery, pCommands, &kQueryBundle, 0);
    return m_hitsFound;
}

// Interprets the MOPP code for the bundle, rescaling the query on the scale commands, narrowing the bundle's lanes at
// the cuts, walking both children of a split (the nearer one by recursion, the other in the loop with its far ends
// clamped to the early-out fractions) and casting the active lanes against the child shape at a terminal. Nothing
// happens while the physics key code is invalid.
// KEEP: a property command for properties 1-3 that takes the copy reads kQuery.m_properties[0] before anything wrote
// it (0xDD44E5, before the Assign call 0xDD4502) and stores it back after the copy; the recursive call (0xDD3B99) is
// then given kQuery, and its copy (Assign) reads the uninitialised value.
void hkpMoppRayBundleVirtualMachine::queryRayOnTree(
    const QueryInt* query,
    const uint8_t* PC,
    QueryBundle* fQuery,
    int32_t chunkId
) {
    hkVector4* const pStart = fQuery->m_rayEnds[0].m_vec;
    hkVector4* const pEnd = fQuery->m_rayEnds[1].m_vec;
    // The query this level rescales, reoffsets or sets properties on, copied from aScaledQuery on first change.
    QueryInt kQuery;
    // The DOUBLE_CUT24 offset: setBroadcast leaves it as it was for an axis outside 0..3, deliberately, as the game
    // does.
    hkVector4 kCutOffset;
    if (!hkpMoppRayBundleVirtualMachine_HK_flyingcolors_mopp) {
        hkpProcessFlyingColors(&hkpMoppRayBundleVirtualMachine_HK_flyingcolors_mopp);
        if (!hkpMoppRayBundleVirtualMachine_HK_flyingcolors_mopp) {
            return;
        }
    }
    while (true) {
        const int32_t iCommand = PC[0];
        const int32_t iByte1 = PC[1];
        const int32_t iByte2 = PC[2];
        const int32_t iByte3 = PC[3];
        // The command's first four bytes as floats, converted before every dispatch.
        const hkQuadReal kBytes = _mm_setr_ps(
            static_cast<float>(iCommand), static_cast<float>(iByte1), static_cast<float>(iByte2),
            static_cast<float>(iByte3)
        );
        int32_t iSplitAxis = 999;
        hkQuadReal kA;
        hkQuadReal kB;
        hkQuadReal kPlanes;
        hkQuadReal kLo;
        hkQuadReal kHi;
        int32_t iLeftOffset;
        int32_t iRightOffset;
        int32_t iPrimitive;
        int32_t iProperty;
        uint32_t uiPropertyValue;
        switch (iCommand) {
            case HK_MOPP_RETURN: return;
            case HK_MOPP_SCALE1:
            case HK_MOPP_SCALE2:
            case HK_MOPP_SCALE3:
            case HK_MOPP_SCALE4: {
                const hkQuadReal akScales[4] = {
                    _mm_set1_ps(2.0f), _mm_set1_ps(4.0f), _mm_set1_ps(8.0f), _mm_set1_ps(16.0f)
                };
                const hkQuadReal kScale = akScales[iCommand - HK_MOPP_SCALE1];
                const hkQuadReal kOffset = _mm_setr_ps(
                    static_cast<float>(iByte1), static_cast<float>(iByte2), static_cast<float>(iByte3), 0.0f
                );
                const hkQuadReal kOffsetX = _mm_shuffle_ps(kOffset, kOffset, 0x00);
                const hkQuadReal kOffsetY = _mm_shuffle_ps(kOffset, kOffset, 0x55);
                const hkQuadReal kOffsetZ = _mm_shuffle_ps(kOffset, kOffset, 0xAA);
                pStart[0].m_quad = _mm_mul_ps(_mm_sub_ps(pStart[0].m_quad, kOffsetX), kScale);
                pStart[1].m_quad = _mm_sub_ps(pStart[1].m_quad, kOffsetY);
                pStart[1].m_quad = _mm_mul_ps(kScale, pStart[1].m_quad);
                pStart[2].m_quad = _mm_sub_ps(pStart[2].m_quad, kOffsetZ);
                pStart[2].m_quad = _mm_mul_ps(kScale, pStart[2].m_quad);
                pEnd[0].m_quad = _mm_sub_ps(pEnd[0].m_quad, kOffsetX);
                pEnd[0].m_quad = _mm_mul_ps(kScale, pEnd[0].m_quad);
                pEnd[1].m_quad = _mm_sub_ps(pEnd[1].m_quad, kOffsetY);
                pEnd[1].m_quad = _mm_mul_ps(kScale, pEnd[1].m_quad);
                pEnd[2].m_quad = _mm_sub_ps(pEnd[2].m_quad, kOffsetZ);
                pEnd[2].m_quad = _mm_mul_ps(kScale, pEnd[2].m_quad);
                kQuery.m_FtoBoffset.m_quad = _mm_mul_ps(_mm_add_ps(query->m_FtoBoffset.m_quad, kOffset), kScale);
                kQuery.m_shift = query->m_shift + iCommand;
                kQuery.m_FtoBScale.m_quad = _mm_mul_ps(query->m_FtoBScale.m_quad, kScale);
                kQuery.m_properties[0] = query->m_properties[0];
                kQuery.m_primitiveOffset = query->m_primitiveOffset;
                query = &kQuery;
                PC += 4;
                continue;
            }
            case HK_MOPP_JUMP8: PC += iByte1 + 2; continue;
            case HK_MOPP_JUMP16: PC += (iByte1 << 8) + iByte2 + 3; continue;
            case HK_MOPP_JUMP24: PC += (((iByte1 << 8) + iByte2) << 8) + iByte3 + 4; continue;
            case HK_MOPP_TERM_REOFFSET8:
                if (query != &kQuery) {
                    QueryInt::Assign(&kQuery, *query);
                    query = &kQuery;
                }
                kQuery.m_primitiveOffset += iByte1;
                PC += 2;
                continue;
            case HK_MOPP_TERM_REOFFSET16: {
                const int32_t iOffset = (iByte1 << 8) + iByte2;
                if (query != &kQuery) {
                    QueryInt::Assign(&kQuery, *query);
                    query = &kQuery;
                }
                kQuery.m_primitiveOffset += iOffset;
                PC += 3;
                continue;
            }
            case HK_MOPP_TERM_REOFFSET32: {
                const uint32_t uiOffset = (((((iByte1 << 8) + iByte2) << 8) + iByte3) << 8) + PC[4];
                if (query != &kQuery) {
                    QueryInt::Assign(&kQuery, *query);
                    query = &kQuery;
                }
                kQuery.m_primitiveOffset = uiOffset;
                PC += 5;
                continue;
            }
            case HK_MOPP_JUMP_CHUNK:
                // The chunk id goes into the shape keys of the terminals below; chunks are 512 bytes.
                chunkId = (iByte1 << 8) | iByte2;
                PC = m_code->m_data.m_data + (chunkId << 9);
                continue;
            case HK_MOPP_DATA_OFFSET: PC += 5; continue;
            case HK_MOPP_SPLIT_X:
            case HK_MOPP_SPLIT_Y:
            case HK_MOPP_SPLIT_Z:
                iSplitAxis = iCommand - HK_MOPP_SPLIT_X;
                kA = pStart[iSplitAxis].m_quad;
                kB = pEnd[iSplitAxis].m_quad;
                kPlanes = kBytes;
                goto splitPlanes;
            case HK_MOPP_SPLIT_YZ:
                kPlanes = _mm_sub_ps(_mm_add_ps(kBytes, kBytes), _mm_set1_ps(0.0f));
                kA = _mm_add_ps(pStart[2].m_quad, pStart[1].m_quad);
                kB = _mm_add_ps(pEnd[2].m_quad, pEnd[1].m_quad);
                goto splitPlanes;
            case HK_MOPP_SPLIT_YMZ:
                kPlanes = _mm_sub_ps(_mm_add_ps(kBytes, kBytes), _mm_set1_ps(255.0f));
                kA = _mm_sub_ps(pStart[1].m_quad, pStart[2].m_quad);
                kB = _mm_sub_ps(pEnd[1].m_quad, pEnd[2].m_quad);
                goto splitPlanes;
            case HK_MOPP_SPLIT_XZ:
                kPlanes = _mm_sub_ps(_mm_add_ps(kBytes, kBytes), _mm_set1_ps(0.0f));
                kA = _mm_add_ps(pStart[2].m_quad, pStart[0].m_quad);
                kB = _mm_add_ps(pEnd[2].m_quad, pEnd[0].m_quad);
                goto splitPlanes;
            case HK_MOPP_SPLIT_XMZ:
                kPlanes = _mm_sub_ps(_mm_add_ps(kBytes, kBytes), _mm_set1_ps(255.0f));
                kA = _mm_sub_ps(pStart[0].m_quad, pStart[2].m_quad);
                kB = _mm_sub_ps(pEnd[0].m_quad, pEnd[2].m_quad);
                goto splitPlanes;
            case HK_MOPP_SPLIT_XY:
                kPlanes = _mm_sub_ps(_mm_add_ps(kBytes, kBytes), _mm_set1_ps(0.0f));
                kA = _mm_add_ps(pStart[1].m_quad, pStart[0].m_quad);
                kB = _mm_add_ps(pEnd[1].m_quad, pEnd[0].m_quad);
                goto splitPlanes;
            case HK_MOPP_SPLIT_XMY:
                kPlanes = _mm_sub_ps(_mm_add_ps(kBytes, kBytes), _mm_set1_ps(255.0f));
                kA = _mm_sub_ps(pStart[0].m_quad, pStart[1].m_quad);
                kB = _mm_sub_ps(pEnd[0].m_quad, pEnd[1].m_quad);
                goto splitPlanes;
            case HK_MOPP_SPLIT_XYZ:
                kPlanes = _mm_sub_ps(_mm_add_ps(_mm_add_ps(kBytes, kBytes), kBytes), _mm_set1_ps(0.0f));
                kA = _mm_add_ps(_mm_add_ps(pStart[1].m_quad, pStart[0].m_quad), pStart[2].m_quad);
                kB = _mm_add_ps(_mm_add_ps(pEnd[1].m_quad, pEnd[0].m_quad), pEnd[2].m_quad);
                goto splitPlanes;
            case HK_MOPP_SPLIT_XYMZ:
                kPlanes = _mm_sub_ps(_mm_add_ps(_mm_add_ps(kBytes, kBytes), kBytes), _mm_set1_ps(255.0f));
                kA = _mm_sub_ps(_mm_add_ps(pStart[1].m_quad, pStart[0].m_quad), pStart[2].m_quad);
                kB = _mm_sub_ps(_mm_add_ps(pEnd[1].m_quad, pEnd[0].m_quad), pEnd[2].m_quad);
                goto splitPlanes;
            case HK_MOPP_SPLIT_XMYZ:
                kPlanes = _mm_sub_ps(_mm_add_ps(_mm_add_ps(kBytes, kBytes), kBytes), _mm_set1_ps(255.0f));
                kA = _mm_add_ps(_mm_sub_ps(pStart[0].m_quad, pStart[1].m_quad), pStart[2].m_quad);
                kB = _mm_add_ps(_mm_sub_ps(pEnd[0].m_quad, pEnd[1].m_quad), pEnd[2].m_quad);
                goto splitPlanes;
            case HK_MOPP_SPLIT_XMYMZ:
                kPlanes = _mm_sub_ps(_mm_add_ps(_mm_add_ps(kBytes, kBytes), kBytes), _mm_set1_ps(510.0f));
                kA = _mm_sub_ps(_mm_sub_ps(pStart[0].m_quad, pStart[1].m_quad), pStart[2].m_quad);
                kB = _mm_sub_ps(_mm_sub_ps(pEnd[0].m_quad, pEnd[1].m_quad), pEnd[2].m_quad);
            splitPlanes:
                // The planes are bytes 1 (the left child's maximum) and 2 (the right child's minimum); the right child
                // is byte 3 bytes on, the left one follows the command.
                iRightOffset = PC[3];
                PC += 4;
                kHi = _mm_shuffle_ps(kPlanes, kPlanes, 0x55);
                kLo = _mm_shuffle_ps(kPlanes, kPlanes, 0xAA);
                iLeftOffset = 0;
                goto split;
            case HK_MOPP_SINGLE_SPLIT_X:
            case HK_MOPP_SINGLE_SPLIT_Y:
            case HK_MOPP_SINGLE_SPLIT_Z:
                iSplitAxis = iCommand - HK_MOPP_SINGLE_SPLIT_X;
                kA = pStart[iSplitAxis].m_quad;
                kB = pEnd[iSplitAxis].m_quad;
                PC += 3;
                kLo = _mm_shuffle_ps(kBytes, kBytes, 0x55);
                kHi = _mm_add_ps(kLo, hkQuadReal1111);
                iLeftOffset = 0;
                iRightOffset = iByte2;
                goto split;
            case HK_MOPP_SPLIT_JUMP_X:
            case HK_MOPP_SPLIT_JUMP_Y:
            case HK_MOPP_SPLIT_JUMP_Z:
                iSplitAxis = iCommand - HK_MOPP_SPLIT_JUMP_X;
                kB = pEnd[iSplitAxis].m_quad;
                iLeftOffset = (iByte3 << 8) + PC[4];
                iRightOffset = (PC[5] << 8) + PC[6];
                kA = pStart[iSplitAxis].m_quad;
                PC += 7;
                kHi = _mm_shuffle_ps(kBytes, kBytes, 0x55);
                kLo = _mm_shuffle_ps(kBytes, kBytes, 0xAA);
            split:
                {
                    const hkQuadReal kMask = fQuery->m_activeMask.m_mask;
                    const hkQuadReal kInactive = _mm_andnot_ps(kMask, rdata_10CE1C0);
                    // The active lanes wholly below the right child's minimum, and wholly above the left child's
                    // maximum.
                    const hkQuadReal kBelow =
                        _mm_and_ps(_mm_and_ps(_mm_cmplt_ps(kB, kLo), _mm_cmplt_ps(kA, kLo)), kMask);
                    const hkQuadReal kAbove =
                        _mm_and_ps(_mm_and_ps(_mm_cmplt_ps(kHi, kB), _mm_cmplt_ps(kHi, kA)), kMask);
                    if (_mm_movemask_ps(_mm_or_ps(kInactive, kBelow)) == 15) {
                        PC += iLeftOffset;
                        continue;
                    }
                    if (_mm_movemask_ps(_mm_or_ps(kInactive, kAbove)) == 15) {
                        PC += iRightOffset;
                        continue;
                    }
                    const hkQuadReal kALo = _mm_sub_ps(kA, kLo);
                    const hkQuadReal kBLo = _mm_sub_ps(kB, kLo);
                    const hkQuadReal kStart0 = pStart[0].m_quad;
                    const hkQuadReal kStart1 = pStart[1].m_quad;
                    const hkQuadReal kStart2 = pStart[2].m_quad;
                    const hkQuadReal kEnd0 = pEnd[0].m_quad;
                    const hkQuadReal kEnd1 = pEnd[1].m_quad;
                    const hkQuadReal kEnd2 = pEnd[2].m_quad;
                    // The left bundle: the lanes not wholly above, clipped at the left child's maximum.
                    QueryBundle kLeft;
                    QueryBundle kRight;
                    kLeft.m_activeMask.m_mask = _mm_and_ps(_mm_andnot_ps(kAbove, rdata_10CE1C0), kMask);
                    kRight.m_activeMask.m_mask = _mm_and_ps(_mm_andnot_ps(kBelow, rdata_10CE1C0), kMask);
                    hkQuadReal kBHi = _mm_sub_ps(kB, kHi);
                    hkQuadReal kAHi = _mm_sub_ps(kA, kHi);
                    const hkQuadReal kCrossHi = _mm_cmplt_ps(_mm_mul_ps(kBHi, kAHi), _mm_setzero_ps());
                    const hkQuadReal kDHi = _mm_sub_ps(kAHi, kBHi);
                    const hkQuadReal kRcpHi = _mm_rcp_ps(kDHi);
                    const hkQuadReal kInvHi =
                        _mm_sub_ps(_mm_add_ps(kRcpHi, kRcpHi), _mm_mul_ps(_mm_mul_ps(kRcpHi, kDHi), kRcpHi));
                    const hkQuadReal kTStartHi = _mm_xor_ps(_mm_mul_ps(kInvHi, kBHi), hkQuadRealNegateMask);
                    const hkQuadReal kAFirstHi = _mm_cmplt_ps(kAHi, kBHi);
                    const hkQuadReal kClipStartHi = _mm_and_ps(_mm_andnot_ps(kAFirstHi, rdata_10CE1C0), kCrossHi);
                    const hkQuadReal kClipEndHi = _mm_and_ps(kAFirstHi, kCrossHi);
                    const hkQuadReal kTEndHi = _mm_mul_ps(kInvHi, kAHi);
                    hkVector4* const pLeftStart = kLeft.m_rayEnds[0].m_vec;
                    hkVector4* const pLeftEnd = kLeft.m_rayEnds[1].m_vec;
                    const hkQuadReal kP0 = _mm_add_ps(_mm_mul_ps(kEnd0, kTEndHi), _mm_mul_ps(kTStartHi, kStart0));
                    pLeftStart[0].m_quad =
                        _mm_or_ps(_mm_and_ps(kP0, kClipStartHi), _mm_andnot_ps(kClipStartHi, kStart0));
                    pLeftEnd[0].m_quad = _mm_or_ps(_mm_andnot_ps(kClipEndHi, kEnd0), _mm_and_ps(kP0, kClipEndHi));
                    const hkQuadReal kP1 = _mm_add_ps(_mm_mul_ps(kStart1, kTStartHi), _mm_mul_ps(kEnd1, kTEndHi));
                    pLeftStart[1].m_quad =
                        _mm_or_ps(_mm_andnot_ps(kClipStartHi, kStart1), _mm_and_ps(kP1, kClipStartHi));
                    pLeftEnd[1].m_quad = _mm_or_ps(_mm_andnot_ps(kClipEndHi, kEnd1), _mm_and_ps(kP1, kClipEndHi));
                    const hkQuadReal kP2 = _mm_add_ps(_mm_mul_ps(kStart2, kTStartHi), _mm_mul_ps(kEnd2, kTEndHi));
                    pLeftStart[2].m_quad =
                        _mm_or_ps(_mm_andnot_ps(kClipStartHi, kStart2), _mm_and_ps(kP2, kClipStartHi));
                    pLeftEnd[2].m_quad = _mm_or_ps(_mm_andnot_ps(kClipEndHi, kEnd2), _mm_and_ps(kP2, kClipEndHi));
                    // The right bundle: the lanes not wholly below, clipped at the right child's minimum.
                    const hkQuadReal kBFirstHi = _mm_cmplt_ps(kBHi, kAHi);
                    const hkQuadReal kDLo = _mm_sub_ps(kALo, kBLo);
                    const hkQuadReal kRcpLo = _mm_rcp_ps(kDLo);
                    const hkQuadReal kCrossLo = _mm_cmplt_ps(_mm_mul_ps(kBLo, kALo), _mm_setzero_ps());
                    const hkQuadReal kInvLo =
                        _mm_sub_ps(_mm_add_ps(kRcpLo, kRcpLo), _mm_mul_ps(_mm_mul_ps(kRcpLo, kDLo), kRcpLo));
                    const hkQuadReal kTStartLo = _mm_xor_ps(_mm_mul_ps(kInvLo, kBLo), hkQuadRealNegateMask);
                    const hkQuadReal kTEndLo = _mm_mul_ps(kInvLo, kALo);
                    const hkQuadReal kClipStartLo = _mm_and_ps(_mm_andnot_ps(kBFirstHi, rdata_10CE1C0), kCrossLo);
                    const hkQuadReal kClipEndLo = _mm_and_ps(kBFirstHi, kCrossLo);
                    hkVector4* const pRightStart = kRight.m_rayEnds[0].m_vec;
                    hkVector4* const pRightEnd = kRight.m_rayEnds[1].m_vec;
                    const hkQuadReal kQ0 = _mm_add_ps(_mm_mul_ps(kTStartLo, kStart0), _mm_mul_ps(kEnd0, kTEndLo));
                    pRightStart[0].m_quad =
                        _mm_or_ps(_mm_and_ps(kQ0, kClipStartLo), _mm_andnot_ps(kClipStartLo, kStart0));
                    pRightEnd[0].m_quad = _mm_or_ps(_mm_andnot_ps(kClipEndLo, kEnd0), _mm_and_ps(kQ0, kClipEndLo));
                    const hkQuadReal kQ1 = _mm_add_ps(_mm_mul_ps(kStart1, kTStartLo), _mm_mul_ps(kEnd1, kTEndLo));
                    pRightStart[1].m_quad =
                        _mm_or_ps(_mm_andnot_ps(kClipStartLo, kStart1), _mm_and_ps(kQ1, kClipStartLo));
                    pRightEnd[1].m_quad = _mm_or_ps(_mm_andnot_ps(kClipEndLo, kEnd1), _mm_and_ps(kQ1, kClipEndLo));
                    const hkQuadReal kQ2 = _mm_add_ps(_mm_mul_ps(kEnd2, kTEndLo), _mm_mul_ps(kStart2, kTStartLo));
                    pRightStart[2].m_quad =
                        _mm_or_ps(_mm_andnot_ps(kClipStartLo, kStart2), _mm_and_ps(kQ2, kClipStartLo));
                    pRightEnd[2].m_quad = _mm_or_ps(_mm_andnot_ps(kClipEndLo, kEnd2), _mm_and_ps(kQ2, kClipEndLo));
                    // The nearer child first, by recursion; the other one goes on in this loop.
                    QueryBundle* pFirst;
                    QueryBundle* pSecond;
                    int32_t iFirstOffset;
                    int32_t iSecondOffset;
                    if (isMajorityActive(&kAHi, &kBHi, &fQuery->m_activeMask.m_mask)) {
                        pFirst = &kLeft;
                        iFirstOffset = iLeftOffset;
                        pSecond = &kRight;
                        iSecondOffset = iRightOffset;
                    }
                    else {
                        pFirst = &kRight;
                        iFirstOffset = iRightOffset;
                        pSecond = &kLeft;
                        iSecondOffset = iLeftOffset;
                    }
                    queryRayOnTree(query, PC + iFirstOffset, pFirst, chunkId);
                    // The lanes the recursion hit end at their early-out fraction now: their far ends are recomputed
                    // from the input rays in the query's space.
                    const hkQuadReal kFraction = m_earlyOutHitFraction.m_quad;
                    const hkQuadReal kOneMinusFraction = _mm_sub_ps(hkQuadReal1111, kFraction);
                    const hkQuadReal kOpen = _mm_cmplt_ps(kFraction, hkQuadReal1111);
                    const hkQuadReal kFtoBoffset = query->m_FtoBoffset.m_quad;
                    hkVector4* const pSecondEnd = pSecond->m_rayEnds[1].m_vec;
                    const hkQuadReal kEndX = _mm_sub_ps(
                        _mm_mul_ps(
                            _mm_sub_ps(
                                _mm_add_ps(
                                    _mm_mul_ps(m_from.m_vec[0].m_quad, kOneMinusFraction),
                                    _mm_mul_ps(m_to.m_vec[0].m_quad, kFraction)
                                ),
                                _mm_shuffle_ps(m_code->m_info.m_offset.m_quad, m_code->m_info.m_offset.m_quad, 0x00)
                            ),
                            _mm_shuffle_ps(query->m_FtoBScale.m_quad, query->m_FtoBScale.m_quad, 0x00)
                        ),
                        _mm_shuffle_ps(kFtoBoffset, kFtoBoffset, 0x00)
                    );
                    pSecondEnd[0].m_quad =
                        _mm_or_ps(_mm_and_ps(kEndX, kOpen), _mm_andnot_ps(kOpen, pSecondEnd[0].m_quad));
                    const hkQuadReal kEndY = _mm_sub_ps(
                        _mm_mul_ps(
                            _mm_sub_ps(
                                _mm_add_ps(
                                    _mm_mul_ps(m_from.m_vec[1].m_quad, kOneMinusFraction),
                                    _mm_mul_ps(m_to.m_vec[1].m_quad, kFraction)
                                ),
                                _mm_shuffle_ps(m_code->m_info.m_offset.m_quad, m_code->m_info.m_offset.m_quad, 0x55)
                            ),
                            _mm_shuffle_ps(query->m_FtoBScale.m_quad, query->m_FtoBScale.m_quad, 0x55)
                        ),
                        _mm_shuffle_ps(kFtoBoffset, kFtoBoffset, 0x55)
                    );
                    pSecondEnd[1].m_quad =
                        _mm_or_ps(_mm_and_ps(kEndY, kOpen), _mm_andnot_ps(kOpen, pSecondEnd[1].m_quad));
                    const hkQuadReal kEndZ = _mm_sub_ps(
                        _mm_mul_ps(
                            _mm_sub_ps(
                                _mm_add_ps(
                                    _mm_mul_ps(m_from.m_vec[2].m_quad, kOneMinusFraction),
                                    _mm_mul_ps(m_to.m_vec[2].m_quad, kFraction)
                                ),
                                _mm_shuffle_ps(m_code->m_info.m_offset.m_quad, m_code->m_info.m_offset.m_quad, 0xAA)
                            ),
                            _mm_shuffle_ps(query->m_FtoBScale.m_quad, query->m_FtoBScale.m_quad, 0xAA)
                        ),
                        _mm_shuffle_ps(kFtoBoffset, kFtoBoffset, 0xAA)
                    );
                    pSecondEnd[2].m_quad =
                        _mm_or_ps(_mm_and_ps(kEndZ, kOpen), _mm_andnot_ps(kOpen, pSecondEnd[2].m_quad));
                    if (iSplitAxis < 3) {
                        // A single-axis split also drops the shortened lanes that no longer reach the second child.
                        const hkQuadReal kAFirst = _mm_cmplt_ps(kAHi, kBHi);
                        const hkQuadReal kValue = pSecondEnd[iSplitAxis].m_quad;
                        hkQuadReal kMissed = _mm_or_ps(
                            _mm_andnot_ps(kAFirst, _mm_cmplt_ps(kHi, kValue)),
                            _mm_and_ps(_mm_cmplt_ps(kValue, kLo), kAFirst)
                        );
                        kMissed = _mm_and_ps(
                            kMissed,
                            _mm_and_ps(_mm_andnot_ps(kAbove, rdata_10CE1C0), _mm_andnot_ps(kBelow, rdata_10CE1C0))
                        );
                        kMissed = _mm_and_ps(kMissed, kOpen);
                        pSecond->m_activeMask.m_mask =
                            _mm_and_ps(_mm_andnot_ps(kMissed, rdata_10CE1C0), pSecond->m_activeMask.m_mask);
                    }
                    if (!_mm_movemask_ps(pSecond->m_activeMask.m_mask)) {
                        return;
                    }
                    PC += iSecondOffset;
                    QueryBundle::Assign(fQuery, *pSecond);
                    continue;
                }
            case HK_MOPP_DOUBLE_CUT_X:
            case HK_MOPP_DOUBLE_CUT_Y:
            case HK_MOPP_DOUBLE_CUT_Z:
                iSplitAxis = iCommand - HK_MOPP_DOUBLE_CUT_X;
                kLo = _mm_shuffle_ps(kBytes, kBytes, 0x55);
                kHi = _mm_shuffle_ps(kBytes, kBytes, 0xAA);
                PC += 3;
                goto doubleCut;
            case HK_MOPP_DOUBLE_CUT24_X:
            case HK_MOPP_DOUBLE_CUT24_Y:
            case HK_MOPP_DOUBLE_CUT24_Z: {
                // Two 24-bit cuts in the code's integer space, converted to the query's space.
                const int32_t iLoCut = (((iByte1 << 8) + iByte2) << 8) + iByte3;
                const int32_t iHiCut = (((PC[4] << 8) + PC[5]) << 8) + PC[6];
                const hkQuadReal kFtoBScale = query->m_FtoBScale.m_quad;
                const hkQuadReal kLoCut =
                    _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(static_cast<float>(iLoCut)), m_ItoFScale.m_quad), kFtoBScale);
                const hkQuadReal kHiCut =
                    _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(static_cast<float>(iHiCut)), m_ItoFScale.m_quad), kFtoBScale);
                iSplitAxis = iCommand - HK_MOPP_DOUBLE_CUT24_X;
                kCutOffset.setBroadcast(query->m_FtoBoffset, iSplitAxis);
                kLo = _mm_sub_ps(kLoCut, kCutOffset.m_quad);
                kHi = _mm_sub_ps(kHiCut, kCutOffset.m_quad);
                PC += 7;
                goto doubleCut;
            }
            doubleCut:
                {
                    // Drops the lanes wholly outside [kLo, kHi] and clips the others' ends to it.
                    kB = pEnd[iSplitAxis].m_quad;
                    kA = pStart[iSplitAxis].m_quad;
                    const hkQuadReal kBNotAfterA = _mm_cmple_ps(kB, kA);
                    const hkQuadReal kABeforeB = _mm_cmplt_ps(kA, kB);
                    const hkQuadReal kOutForward =
                        _mm_andnot_ps(kABeforeB, _mm_or_ps(_mm_cmplt_ps(kHi, kB), _mm_cmplt_ps(kA, kLo)));
                    const hkQuadReal kOutBackward =
                        _mm_and_ps(_mm_or_ps(_mm_cmplt_ps(kB, kLo), _mm_cmplt_ps(kHi, kA)), kABeforeB);
                    const hkQuadReal kActive = _mm_and_ps(
                        _mm_andnot_ps(_mm_or_ps(kOutForward, kOutBackward), rdata_10CE1C0), fQuery->m_activeMask.m_mask
                    );
                    fQuery->m_activeMask.m_mask = kActive;
                    if (!_mm_movemask_ps(kActive)) {
                        return;
                    }
                    const RayPointBundle kStart = fQuery->m_rayEnds[0];
                    const RayPointBundle kEnd = fQuery->m_rayEnds[1];
                    hkQuadReal kBHi = _mm_sub_ps(kB, kHi);
                    hkQuadReal kAHi = _mm_sub_ps(kA, kHi);
                    const hkQuadReal kCrossHi = _mm_cmplt_ps(_mm_mul_ps(kBHi, kAHi), _mm_setzero_ps());
                    hkQuadReal kTEndHi;
                    hkQuadReal kTStartHi;
                    calcSlabIntervals(&kBHi, &kAHi, &kTEndHi, &kTStartHi);
                    const hkQuadReal kP0 = _mm_add_ps(
                        _mm_mul_ps(kTEndHi, kEnd.m_vec[0].m_quad), _mm_mul_ps(kTStartHi, kStart.m_vec[0].m_quad)
                    );
                    const hkQuadReal kClipStartHi = _mm_and_ps(kCrossHi, kBNotAfterA);
                    const hkQuadReal kClipEndHi = _mm_and_ps(kCrossHi, kABeforeB);
                    pStart[0].m_quad =
                        _mm_or_ps(_mm_and_ps(kP0, kClipStartHi), _mm_andnot_ps(kClipStartHi, pStart[0].m_quad));
                    pEnd[0].m_quad = _mm_or_ps(_mm_and_ps(kP0, kClipEndHi), _mm_andnot_ps(kClipEndHi, pEnd[0].m_quad));
                    const hkQuadReal kP1 = _mm_add_ps(
                        _mm_mul_ps(kEnd.m_vec[1].m_quad, kTEndHi), _mm_mul_ps(kStart.m_vec[1].m_quad, kTStartHi)
                    );
                    pStart[1].m_quad =
                        _mm_or_ps(_mm_and_ps(kP1, kClipStartHi), _mm_andnot_ps(kClipStartHi, pStart[1].m_quad));
                    pEnd[1].m_quad = _mm_or_ps(_mm_and_ps(kP1, kClipEndHi), _mm_andnot_ps(kClipEndHi, pEnd[1].m_quad));
                    const hkQuadReal kP2 = _mm_add_ps(
                        _mm_mul_ps(kEnd.m_vec[2].m_quad, kTEndHi), _mm_mul_ps(kStart.m_vec[2].m_quad, kTStartHi)
                    );
                    pStart[2].m_quad =
                        _mm_or_ps(_mm_and_ps(kP2, kClipStartHi), _mm_andnot_ps(kClipStartHi, pStart[2].m_quad));
                    pEnd[2].m_quad = _mm_or_ps(_mm_and_ps(kP2, kClipEndHi), _mm_andnot_ps(kClipEndHi, pEnd[2].m_quad));
                    hkQuadReal kBLo = _mm_sub_ps(kB, kLo);
                    hkQuadReal kALo = _mm_sub_ps(kA, kLo);
                    const hkQuadReal kCrossLo = _mm_cmplt_ps(_mm_mul_ps(kBLo, kALo), _mm_setzero_ps());
                    hkQuadReal kTEndLo;
                    hkQuadReal kTStartLo;
                    calcSlabIntervals(&kBLo, &kALo, &kTEndLo, &kTStartLo);
                    const hkQuadReal kQ0 = _mm_add_ps(
                        _mm_mul_ps(kTEndLo, kEnd.m_vec[0].m_quad), _mm_mul_ps(kTStartLo, kStart.m_vec[0].m_quad)
                    );
                    const hkQuadReal kClipStartLo = _mm_and_ps(kCrossLo, kABeforeB);
                    const hkQuadReal kClipEndLo = _mm_and_ps(kCrossLo, kBNotAfterA);
                    pStart[0].m_quad =
                        _mm_or_ps(_mm_and_ps(kQ0, kClipStartLo), _mm_andnot_ps(kClipStartLo, pStart[0].m_quad));
                    pEnd[0].m_quad = _mm_or_ps(_mm_and_ps(kQ0, kClipEndLo), _mm_andnot_ps(kClipEndLo, pEnd[0].m_quad));
                    const hkQuadReal kQ1 = _mm_add_ps(
                        _mm_mul_ps(kEnd.m_vec[1].m_quad, kTEndLo), _mm_mul_ps(kStart.m_vec[1].m_quad, kTStartLo)
                    );
                    pStart[1].m_quad =
                        _mm_or_ps(_mm_and_ps(kQ1, kClipStartLo), _mm_andnot_ps(kClipStartLo, pStart[1].m_quad));
                    pEnd[1].m_quad = _mm_or_ps(_mm_and_ps(kQ1, kClipEndLo), _mm_andnot_ps(kClipEndLo, pEnd[1].m_quad));
                    const hkQuadReal kQ2 = _mm_add_ps(
                        _mm_mul_ps(kEnd.m_vec[2].m_quad, kTEndLo), _mm_mul_ps(kStart.m_vec[2].m_quad, kTStartLo)
                    );
                    pStart[2].m_quad =
                        _mm_or_ps(_mm_and_ps(kQ2, kClipStartLo), _mm_andnot_ps(kClipStartLo, pStart[2].m_quad));
                    pEnd[2].m_quad = _mm_or_ps(_mm_and_ps(kQ2, kClipEndLo), _mm_andnot_ps(kClipEndLo, pEnd[2].m_quad));
                    continue;
                }
            case HK_MOPP_TERM4_0:
            case HK_MOPP_TERM4_1:
            case HK_MOPP_TERM4_2:
            case HK_MOPP_TERM4_3:
            case HK_MOPP_TERM4_4:
            case HK_MOPP_TERM4_5:
            case HK_MOPP_TERM4_6:
            case HK_MOPP_TERM4_7:
            case HK_MOPP_TERM4_8:
            case HK_MOPP_TERM4_9:
            case HK_MOPP_TERM4_A:
            case HK_MOPP_TERM4_B:
            case HK_MOPP_TERM4_C:
            case HK_MOPP_TERM4_D:
            case HK_MOPP_TERM4_E:
            case HK_MOPP_TERM4_F:
            case HK_MOPP_TERM4_10:
            case HK_MOPP_TERM4_11:
            case HK_MOPP_TERM4_12:
            case HK_MOPP_TERM4_13:
            case HK_MOPP_TERM4_14:
            case HK_MOPP_TERM4_15:
            case HK_MOPP_TERM4_16:
            case HK_MOPP_TERM4_17:
            case HK_MOPP_TERM4_18:
            case HK_MOPP_TERM4_19:
            case HK_MOPP_TERM4_1A:
            case HK_MOPP_TERM4_1B:
            case HK_MOPP_TERM4_1C:
            case HK_MOPP_TERM4_1D:
            case HK_MOPP_TERM4_1E:
            case HK_MOPP_TERM4_1F: iPrimitive = iCommand - HK_MOPP_TERM4_0; goto terminal;
            case HK_MOPP_TERM8: iPrimitive = PC[1]; goto terminal;
            case HK_MOPP_TERM16: iPrimitive = (PC[1] << 8) + PC[2]; goto terminal;
            case HK_MOPP_TERM24: iPrimitive = (((PC[1] << 8) + PC[2]) << 8) + PC[3]; goto terminal;
            case HK_MOPP_TERM32:
                iPrimitive = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
            terminal:
                {
                    // The child's key: the primitive past the query's offset, with the chunk id when the code
                    // reindexes.
                    const uint32_t uiKey =
                        (query->m_primitiveOffset + iPrimitive) | (m_reindexingMask & (chunkId << 8));
                    alignas(16) char acShapeBuffer[512];
                    const hkpShape* pShape = m_collection->getChildShape(uiKey, acShapeBuffer);
                    if (m_rays->m_rayShapeCollectionFilter) {
                        // Only the filter info is set: the input's ends are left unwritten, as the game does.
                        XFNV_INLINED_LOCAL(hkpShapeRayCastInput, kInput, std::nullopt, std::nullopt);
                        // EXE-INLINED 0x632CA0
                        kInput.m_filterInfo = 0;
                        kInput.m_rayShapeCollectionFilter = nullptr;
                        kInput.m_filterInfo = m_rays->m_filterInfo;
                        if (!m_rays->m_rayShapeCollectionFilter->isCollisionEnabled(
                                kInput, *pShape, *m_collection, uiKey
                            )) {
                            return;
                        }
                    }
                    // Each lane's output gets a level for the child's key while the child casts.
                    m_rayResultPtr->m_outputs[0].m_shapeKeyIndex++;
                    m_rayResultPtr->m_outputs[1].m_shapeKeyIndex++;
                    m_rayResultPtr->m_outputs[2].m_shapeKeyIndex++;
                    m_rayResultPtr->m_outputs[3].m_shapeKeyIndex++;
                    hkVector4Comparison kHits =
                        pShape->castRayBundleImpl(*m_rays, *m_rayResultPtr, fQuery->m_activeMask);
                    for (int32_t i = 0; i < 4; ++i) {
                        m_rayResultPtr->m_outputs[i].m_shapeKeyIndex--;
                        if (_mm_movemask_ps(kHits.m_mask) & axisMaskBits_10C92E0[i]) {
                            hkpShapeRayCastOutput& rOutput = m_rayResultPtr->m_outputs[i];
                            m_earlyOutHitFraction(i) = rOutput.m_hitFraction;
                            rOutput.m_shapeKeys[rOutput.m_shapeKeyIndex] = uiKey;
                        }
                    }
                    m_hitsFound.m_mask = _mm_or_ps(kHits.m_mask, m_hitsFound.m_mask);
                    return;
                }
            case HK_MOPP_PROPERTY8_0:
            case HK_MOPP_PROPERTY8_1:
            case HK_MOPP_PROPERTY8_2:
            case HK_MOPP_PROPERTY8_3:
                iProperty = iCommand - HK_MOPP_PROPERTY8_0;
                uiPropertyValue = iByte1;
                PC += 2;
                goto setProperty;
            case HK_MOPP_PROPERTY16_0:
            case HK_MOPP_PROPERTY16_1:
            case HK_MOPP_PROPERTY16_2:
            case HK_MOPP_PROPERTY16_3:
                iProperty = iCommand - HK_MOPP_PROPERTY16_0;
                uiPropertyValue = (iByte1 << 8) + iByte2;
                PC += 3;
                goto setProperty;
            case HK_MOPP_PROPERTY32_0:
            case HK_MOPP_PROPERTY32_1:
            case HK_MOPP_PROPERTY32_2:
            case HK_MOPP_PROPERTY32_3:
                iProperty = iCommand - HK_MOPP_PROPERTY32_0;
                uiPropertyValue = (((((iByte1 << 8) + iByte2) << 8) + iByte3) << 8) + PC[4];
                PC += 5;
            setProperty:
                // QueryInt has one property slot (m_properties[1]). Nothing reads the properties (only QueryInt::Assign
                // copies them), so only property 0 is stored.
                if (query != &kQuery) {
                    QueryInt::Assign(&kQuery, *query);
                    query = &kQuery;
                }
                if (iProperty == 0) {
                    kQuery.m_properties[0] = uiPropertyValue;
                }
                continue;
            default: {
                // This is HK_ERROR; execution goes on with the same command after it.
                char acBuffer[512];
                hkErrStream kStream(acBuffer, 512);
                kStream.operator<<(
                    "Unknown command - This mopp data has been corrupted (check for memory trashing), or an "
                    "hkpMoppBvTreeShape has been pointed at invalid mopp data.\n"
                );
                if (hkError::s_instance->message(
                        hkError::MESSAGE_ERROR, 0x1298FEDD, acBuffer,
                        ".\\Collide\\Mopp\\Machine\\hkpMoppRayBundleVirtualMachine.cpp", 838
                    )) {
                    __builtin_debugtrap();
                }
                continue;
            }
        }
    }
}
