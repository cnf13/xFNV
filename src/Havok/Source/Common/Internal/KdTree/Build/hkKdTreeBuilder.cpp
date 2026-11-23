#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuilder.h"

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"
#include "Havok/Source/Physics/Internal/Collide/BroadPhase/3AxisSweep/hkp3AxisSweep.h"
#include "xfnv/Compiler.h"

#include <cmath>
#include <emmintrin.h>
#include <xmmintrin.h>

static int32_t hkMinU16(uint16_t a, uint16_t b);

static int32_t hkMinU16(uint16_t a, uint16_t b) {
    int32_t result = a;
    if (a >= b) {
        return b;
    }
    return result;
}

using hkKdTreeBuilder_lconst_shkKdTreeProjectedEntry_p_chkKdTreeProjectedEntry_p_c0 = hkKdTreeBuilder<
    const hkKdTreeProjectedEntry*,
    hkKdTreeProjectedEntry*,
    0>;

using hkKdTreeBuilder_lconst_shkKdTreeProjectedEntry_p_chkKdTreeProjectedEntry_p_c0 = hkKdTreeBuilder<
    const hkKdTreeProjectedEntry*,
    hkKdTreeProjectedEntry*,
    0>;

using hkKdTreeBuilder_lconst_shkKdTreeProjectedEntry_p_chkKdTreeProjectedEntry_p_c0 = hkKdTreeBuilder<
    const hkKdTreeProjectedEntry*,
    hkKdTreeProjectedEntry*,
    0>;

using hkKdTreeBuilder_lconst_shkKdTreeProjectedEntry_p_chkKdTreeProjectedEntry_p_c0 = hkKdTreeBuilder<
    const hkKdTreeProjectedEntry*,
    hkKdTreeProjectedEntry*,
    0>;

// The bounds' extent per lane as floats: the max words minus the min words (the w lane: the id's low word minus its
// high word). The entry is read as raw memory, its eight words as one unaligned integer vector, and unpacked to 32-bit
// lanes; each lane's high half (zero after the unpack) is scaled by 1e16 and added to its low half. Where the id halves
// are 32 bits (8-byte pointers, hkKdTree.h's HK_POINTER_SIZE case) the entry is not those eight words: they are
// gathered from its fields, as hkKdTreeMath::convertProjectedEntryToHkVector4 does.
void hkGetExtentFromBounds(const hkKdTreeProjectedEntry& bounds, hkVector4& vout) {
    const __m128 kScale = _mm_set1_ps(1e+16f);
    __m128i kWords;
    if constexpr (sizeof(hkHalfPointer) == sizeof(uint16_t)) {
        kWords = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&bounds));
    }
    else {
        kWords = _mm_set_epi16(
            static_cast<int16_t>(bounds.m_primIdLow),
            static_cast<int16_t>(bounds.m_max[2]),
            static_cast<int16_t>(bounds.m_max[1]),
            static_cast<int16_t>(bounds.m_max[0]),
            static_cast<int16_t>(bounds.m_primIdHigh),
            static_cast<int16_t>(bounds.m_min[2]),
            static_cast<int16_t>(bounds.m_min[1]),
            static_cast<int16_t>(bounds.m_min[0])
        );
    }
    const __m128i kZero = _mm_setzero_si128();
    const __m128i kMax = _mm_unpackhi_epi16(kWords, kZero);
    const __m128i kMin = _mm_unpacklo_epi16(kWords, kZero);
    const __m128 kMaxF = _mm_add_ps(
        _mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(kMax, 16)), kScale),
        _mm_cvtepi32_ps(_mm_srli_epi32(_mm_slli_epi32(kMax, 16), 16))
    );
    const __m128 kMinF = _mm_add_ps(
        _mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(kMin, 16)), kScale),
        _mm_cvtepi32_ps(_mm_srli_epi32(_mm_slli_epi32(kMin, 16), 16))
    );
    vout.m_quad = _mm_sub_ps(kMaxF, kMinF);
}

// Zeroes the 128 bytes at dst, eight aligned 16-byte stores, first to last.
void hkString_memClear128Exactly(void* dst) {
    __m128* pQuads = static_cast<__m128*>(dst);
    const __m128 kZero = _mm_setzero_ps();
    for (int32_t i = 0; i < 8; ++i) {
        pQuads[i] = kZero;
    }
}

int32_t hkGetNumLeadingZero(uint32_t x) {
    uint32_t v1 = x << ((static_cast<uint32_t>(static_cast<uint16_t>(x)) - 256) >> 16 & 8);
    int32_t v2 = (static_cast<uint16_t>(v1) - 0x1000) >> 16 & 4;
    uint32_t v3 = v1 << v2;
    return ((static_cast<uint16_t>(v3) - 0x4000) >> 16 & 2) + (v2 + ((static_cast<uint16_t>(x) - 256) >> 16 & 8)) -
        (static_cast<uint16_t>(v3 << ((static_cast<uint32_t>(static_cast<uint16_t>(v3)) - 0x4000) >> 16 & 2)) >> 14 &
         ~(static_cast<uint16_t>(v3 << ((static_cast<uint32_t>(static_cast<uint16_t>(v3)) - 0x4000) >> 16 & 2)) >>
           15)) +
        2;
}

void WRITE_NODE_WITH_MANAGER(hkKdTreeNode* base, const hkKdTreeNode& nodeToWrite, int32_t nodeIdx) {
    base[nodeIdx] = nodeToWrite;
}

void WRITE_PROJ_ENT_WITH_MANAGER(hkKdTreeProjectedEntry* base, const hkKdTreeProjectedEntry& peToWrite, int32_t peIdx) {
    base[peIdx] = peToWrite;
}

// The constructor leaves the write buffer uninitialised.
hkKdTreeBuilderSparseWriter::hkKdTreeBuilderSparseWriter() : m_index(0) {}

void hkKdTreeBuilderSparseWriter::putProjEntToMainMemory(
    hkKdTreeProjectedEntry* base,
    int32_t peIdx,
    const hkKdTreeProjectedEntry& peOut
) {
    base[peIdx] = peOut;
}

void hkKdTreeBuilderSparseWriter::putNodeToMainMemory(
    hkKdTreeNode* root,
    int32_t currentNodeIdx,
    hkKdTreeNode& thisNode
) {
    root[currentNodeIdx] = thisNode;
}

void hkKdTreeBuilderSparseWriter::flush() {}

// Two primitives are ordered by their lower bound on the bounds' longest axis; fewer than eight go to
// splitPrimitivesExactly. Otherwise every step-th primitive's lower and upper bound per axis is counted into eight
// buckets (bucketsIn, bucketsOut), and the seven planes between them are priced per axis by the surface area heuristic:
// a float cost (counts times float areas), the cheapest strictly lower one kept where both sides get primitives or an
// empty side is allowed on that axis. The primitives are then partitioned by their lower bound (the best left count
// short of all), else by their upper bound (the best right count short of all), else halved in order. The extents'
// 16-bit differences, the lane masks and the plane test are compared as unsigned 16-bit values, then as signed ints.
hkSplitPrimitivesStatus hkKdTreeProjectedEntryBuilder::splitPrimitives(
    const hkKdTreeBuildInput::WorkingSet& buffersIn,
    int32_t startPrimIdx,
    int32_t endPrimIdx,
    SplitPrimitivesResults& results,
    int32_t& numAvailableEmptyNodes,
    hkKdTreeBuildInput::WorkingSet& buffersOut,
    const hkKdTreeProjectedEntry& bounds,
    const hkVector4Comparison& allowedEmptySplits
) {
    const int32_t numPrimitives = endPrimIdx - startPrimIdx;
    if (numPrimitives == 2) {
        const hkKdTreeProjectedEntry* pIn = buffersIn.m_entriesIn + startPrimIdx;
        hkKdTreeProjectedEntry* pOut = buffersIn.m_entriesOut + startPrimIdx;
        const hkKdTreeProjectedEntry kA = pIn[0];
        const hkKdTreeProjectedEntry kB = pIn[1];
        const uint16_t uiExtentZ = static_cast<uint16_t>(bounds.m_max[2] - bounds.m_min[2]);
        const uint16_t uiExtentX = static_cast<uint16_t>(bounds.m_max[0] - bounds.m_min[0]);
        const uint16_t uiExtentY = static_cast<uint16_t>(bounds.m_max[1] - bounds.m_min[1]);
        int32_t axis;
        if (uiExtentX > uiExtentY) {
            axis = (uiExtentZ < uiExtentX) ? 0 : 2;
        }
        else {
            axis = (uiExtentZ < uiExtentY) ? 1 : 2;
        }
        const uint16_t uiMinB = kB.m_min[axis];
        const uint16_t uiMinA = kA.m_min[axis];
        if (uiMinA > uiMinB) {
            pOut[0] = kB;
            pOut[1] = kA;
            results.m_splitMax = kB.m_max[axis];
            results.m_splitMin = uiMinA;
        }
        else {
            pOut[0] = kA;
            pOut[1] = kB;
            results.m_splitMax = kA.m_max[axis];
            results.m_splitMin = uiMinB;
        }
        results.m_splitAxis = axis;
        results.m_splitIndex = startPrimIdx + 1;
    }
    else if (numPrimitives < 8) {
        return splitPrimitivesExactly(buffersIn, startPrimIdx, endPrimIdx, results, buffersOut, bounds);
    }
    else {
        const hkKdTreeProjectedEntry* entriesIn = buffersIn.m_entriesIn;
        // The extent and both bucket arrays are on 128-byte boundaries.
        alignas(128) hkVector4 extent;
        hkGetExtentFromBounds(bounds, extent);

        // The extent as (z, x, y, 0) and (y, z, x, 0): their product is each axis's split face area, their sum times an
        // eighth of the extent the area one bucket adds to a side.
        const __m128 kZxy = _mm_set_ps(
            0.0f,
            XFNV_M128_F32(extent.m_quad, 1),
            XFNV_M128_F32(extent.m_quad, 0),
            XFNV_M128_F32(extent.m_quad, 2)
        );
        const __m128 kYzx = _mm_set_ps(
            0.0f,
            XFNV_M128_F32(extent.m_quad, 0),
            XFNV_M128_F32(extent.m_quad, 2),
            XFNV_M128_F32(extent.m_quad, 1)
        );
        alignas(16) float initialSA[4];
        _mm_store_ps(initialSA, _mm_mul_ps(kZxy, kYzx));
        alignas(16) float deltaSurfaceArea[4];
        _mm_store_ps(
            deltaSurfaceArea,
            _mm_mul_ps(
                _mm_mul_ps(_mm_set1_ps(0.125f), _mm_load_ps(&reinterpret_cast<float*>(&extent.m_quad)[0])),
                _mm_add_ps(kZxy, kYzx)
            )
        );
        int32_t bestBucket = 0;
        int32_t bestAxis = 0;

        alignas(128) int32_t bucketsIn[8][4];
        alignas(128) int32_t bucketsOut[8][4];
        hkString_memClear128Exactly(bucketsIn);
        hkString_memClear128Exactly(bucketsOut);

        int32_t numBitsToShift[3];
        for (int32_t k = 0; k < 3; ++k) {
            const int32_t iBits = 13 - hkGetNumLeadingZero(static_cast<uint16_t>(bounds.m_max[k] - bounds.m_min[k]));
            numBitsToShift[k] = (iBits <= 0) ? 0 : iBits;
        }

        const hkKdTreeProjectedEntry* pStart = entriesIn + startPrimIdx;
        int32_t d = 1;
        if (numPrimitives > 10) {
            d = 2;
        }
        if (numPrimitives > 100) {
            d = 3;
        }
        if (numPrimitives > 1000) {
            d = 4;
        }
        if (numPrimitives > 10000) {
            d = 5;
        }
        const hkKdTreeProjectedEntry* pEntry = pStart;
        for (int32_t idx = startPrimIdx; idx < endPrimIdx; idx += d) {
            const int32_t iMinX = bounds.m_min[0];
            const int32_t iMinY = bounds.m_min[1];
            const int32_t iMinZ = bounds.m_min[2];
            ++bucketsIn[(pEntry->m_min[0] - iMinX) >> numBitsToShift[0]][0];
            ++bucketsOut[(pEntry->m_max[0] - iMinX) >> numBitsToShift[0]][0];
            ++bucketsIn[(pEntry->m_min[1] - iMinY) >> numBitsToShift[1]][1];
            ++bucketsOut[(pEntry->m_max[1] - iMinY) >> numBitsToShift[1]][1];
            ++bucketsIn[(pEntry->m_min[2] - iMinZ) >> numBitsToShift[2]][2];
            ++bucketsOut[(pEntry->m_max[2] - iMinZ) >> numBitsToShift[2]][2];
            pEntry += d;
        }

        const int32_t numPrimsBucketted = (d + numPrimitives - 1) / d;
        float bestCost = HK_REAL_MAX;
        const bool allowEmptySplit = numAvailableEmptyNodes > 1;
        const int32_t axisMasks[3] =
            {hkVector4Comparison::MASK_X, hkVector4Comparison::MASK_Y, hkVector4Comparison::MASK_Z};
        int32_t bestLeft = numPrimsBucketted;
        int32_t bestRight = numPrimsBucketted;
        for (int32_t axis = 0; axis < 3; ++axis) {
            const float deltaSA = deltaSurfaceArea[axis];
            int32_t numRight = numPrimsBucketted;
            float rightSA = deltaSA * 8.0 + initialSA[axis];
            float leftSA = initialSA[axis];
            int32_t numLeft = 0;
            for (int32_t i = 0; i < 7; ++i) {
                numRight -= bucketsOut[i][axis];
                numLeft += bucketsIn[i][axis];
                leftSA = leftSA + deltaSA;
                rightSA = rightSA - deltaSA;
                const float cost = static_cast<float>(
                    static_cast<double>(numRight) * rightSA + static_cast<double>(numLeft) * leftSA
                );
                bool bValid;
                if (allowEmptySplit &&
                    (_mm_movemask_ps(_mm_load_ps(&reinterpret_cast<const float*>(&allowedEmptySplits.m_mask)[0])) &
                     axisMasks[axis]) != 0) {
                    bValid = true;
                }
                else {
                    bValid = numLeft > 0 && numRight > 0;
                }
                // Kept only when the best so far is greater (never for a NaN).
                if (bestCost > cost && bValid) {
                    bestCost = cost;
                    bestAxis = axis;
                    bestBucket = i;
                    bestLeft = numLeft;
                    bestRight = numRight;
                }
            }
        }

        // The axis is read back as a signed 16-bit value.
        const int32_t axis = static_cast<int16_t>(bestAxis);
        const int32_t splitPlane = bounds.m_min[axis] + (1 << numBitsToShift[axis]) * (bestBucket + 1);
        // The right half's lower bound and the left half's upper bound, from the far ends of the bounds.
        uint16_t splitMin = bounds.m_max[axis];
        uint16_t splitMax = bounds.m_min[axis];
        hkKdTreeProjectedEntry* pA = buffersIn.m_entriesOut + startPrimIdx;
        hkKdTreeProjectedEntry* pB = buffersIn.m_entriesOut + endPrimIdx - 1;
        // Both partition loops end on an unsigned compare with the end address computed from entriesIn and endPrimIdx,
        // not on a count.
        const uintptr_t uEndEntry = static_cast<uintptr_t>(endPrimIdx) * sizeof(hkKdTreeProjectedEntry) +
            reinterpret_cast<uintptr_t>(entriesIn);
        const hkKdTreeProjectedEntry* p = pStart;
        int32_t splitIndex;
        if (bestLeft < numPrimsBucketted) {
            for (; reinterpret_cast<uintptr_t>(p) < uEndEntry; ++p) {
                const uint16_t minE = p->m_min[axis];
                const hkKdTreeProjectedEntry kEntry = *p;
                if (static_cast<int32_t>(minE) < splitPlane) {
                    const uint16_t maxE = p->m_max[axis];
                    *pA++ = kEntry;
                    if (splitMax <= maxE) {
                        splitMax = maxE;
                    }
                }
                else {
                    *pB-- = kEntry;
                    if (splitMin >= minE) {
                        splitMin = minE;
                    }
                }
            }
            splitIndex = static_cast<int32_t>(pA - buffersIn.m_entriesOut);
        }
        else if (bestRight < numPrimsBucketted) {
            for (; reinterpret_cast<uintptr_t>(p) < uEndEntry; ++p) {
                const uint16_t maxE = p->m_max[axis];
                const hkKdTreeProjectedEntry kEntry = *p;
                if (static_cast<int32_t>(maxE) < splitPlane) {
                    *pA++ = kEntry;
                    if (splitMax <= maxE) {
                        splitMax = maxE;
                    }
                }
                else {
                    *pB-- = kEntry;
                    const uint16_t minE = p->m_min[axis];
                    if (splitMin >= minE) {
                        splitMin = minE;
                    }
                }
            }
            splitIndex = static_cast<int32_t>(pA - buffersIn.m_entriesOut);
        }
        else {
            splitIndex = startPrimIdx + (numPrimitives >> 1);
            int32_t i = startPrimIdx;
            for (; i < splitIndex; ++i, ++p) {
                const uint16_t maxE = p->m_max[axis];
                *pA++ = *p;
                if (splitMax <= maxE) {
                    splitMax = maxE;
                }
            }
            for (; i < endPrimIdx; ++i, ++p) {
                const uint16_t minE = p->m_min[axis];
                *pA++ = *p;
                if (splitMin >= minE) {
                    splitMin = minE;
                }
            }
        }

        if (splitIndex == startPrimIdx || splitIndex == endPrimIdx) {
            numAvailableEmptyNodes -= 2;
        }
        results.m_splitMin = splitMin;
        results.m_splitMax = splitMax;
        results.m_splitIndex = splitIndex;
        results.m_splitAxis = bestAxis;
    }

    hkKdTreeProjectedEntry* pIn = buffersIn.m_entriesIn;
    buffersOut.m_entriesOut = pIn;
    buffersOut.m_entriesIn = buffersIn.m_entriesOut;
    return SPLIT_PRIMITIVES_OK;
}

// No primitive makes an empty leaf, one a single primitive leaf and up to four a multiple primitive leaf, as does a set
// splitPrimitives cannot tell apart. Otherwise the node becomes an inner node over the next two node indices and both
// halves are built; a split that leaves one half empty takes its axis out of the halves' allowed empty splits
// (hkVector4Comparison::s_maskFromBits). The halves' inputs leave their padding unwritten.
int32_t hkKdTreeProjectedEntryBuilder::buildTreeRecursive(
    const hkKdTreeBuildInput::BuildRecursiveInput& input,
    hkKdTreeBuildInput::TreeStream* treeOut,
    hkKdTreeProjectedEntry* projEntOut,
    int32_t& numNodes,
    int32_t& numAvailableEmptyNodes
) {
    const int32_t numPrimitives = input.m_endPrimIdx - input.m_startPrimIdx;
    if (numPrimitives == 0) {
        writeEmptyLeaf(treeOut->m_root, input.m_currentNodeIdx, input.m_writer);
        return 1;
    }
    if (numPrimitives == 1) {
        writeSinglePrimitiveToLeaf(
            treeOut->m_root,
            input.m_buffers->m_entriesIn,
            projEntOut,
            input.m_currentNodeIdx,
            input.m_startPrimIdx,
            input.m_writer
        );
        return 1;
    }
    if (numPrimitives <= 4) {
        numAvailableEmptyNodes += writeMultiplePrimitivesToLeaf(
            treeOut->m_root,
            input.m_buffers->m_entriesIn,
            projEntOut,
            input.m_currentNodeIdx,
            input.m_startPrimIdx,
            numPrimitives,
            input.m_writer
        );
        return 1;
    }

    SplitPrimitivesResults results;
    hkKdTreeBuildInput::WorkingSet splitBuffers;
    if (splitPrimitives(
            *input.m_buffers,
            input.m_startPrimIdx,
            input.m_endPrimIdx,
            results,
            numAvailableEmptyNodes,
            splitBuffers,
            input.m_bounds,
            input.m_allowedEmptySplits
        ) == SPLIT_PRIMITIVES_INDISTINGUISHABLE) {
        numAvailableEmptyNodes += writeMultiplePrimitivesToLeaf(
            treeOut->m_root,
            input.m_buffers->m_entriesIn,
            projEntOut,
            input.m_currentNodeIdx,
            input.m_startPrimIdx,
            numPrimitives,
            input.m_writer
        );
        return 1;
    }

    const int32_t axis = results.m_splitAxis;
    const int32_t iNumNodes = numNodes;
    const int32_t leftIdx = iNumNodes + 1;
    numNodes = iNumNodes + 2;
    // The node's left child offset, axis and node flag in one word, then its split planes.
    hkKdTreeNode& thisNode = treeOut->m_root[input.m_currentNodeIdx];
    thisNode.m_data.m_node.m_type_left_axis = static_cast<int32_t>(
        (static_cast<uint32_t>(leftIdx - input.m_currentNodeIdx) << 3) | static_cast<uint32_t>(axis & 3) | 0x80000000u
    );
    thisNode.m_data.m_node.m_splitMin = results.m_splitMin;
    thisNode.m_data.m_node.m_splitMax = results.m_splitMax;

    const __m128 kAllAxes = _mm_load_ps(
        &reinterpret_cast<float*>(&hkVector4Comparison::s_maskFromBits[hkVector4Comparison::MASK_XYZ])[0]
    );
    hkKdTreeBuildInput::WorkingSet leftBuffers = splitBuffers;
    hkKdTreeBuildInput::WorkingSet rightBuffers = splitBuffers;

    hkKdTreeBuildInput::BuildRecursiveInput leftInput;
    leftInput.m_buffers = &leftBuffers;
    leftInput.m_currentNodeIdx = leftIdx;
    leftInput.m_maxNodeIdx = input.m_maxNodeIdx;
    leftInput.m_startPrimIdx = input.m_startPrimIdx;
    leftInput.m_endPrimIdx = results.m_splitIndex;
    leftInput.m_bounds = input.m_bounds;
    leftInput.m_bounds.m_max[axis] = results.m_splitMax;
    leftInput.m_writer = input.m_writer;
    _mm_store_ps(&reinterpret_cast<float*>(&leftInput.m_allowedEmptySplits.m_mask)[0], kAllAxes);

    hkKdTreeBuildInput::BuildRecursiveInput rightInput;
    rightInput.m_buffers = &rightBuffers;
    rightInput.m_currentNodeIdx = leftIdx + 1;
    rightInput.m_maxNodeIdx = input.m_maxNodeIdx;
    rightInput.m_startPrimIdx = results.m_splitIndex;
    rightInput.m_endPrimIdx = input.m_endPrimIdx;
    rightInput.m_bounds = input.m_bounds;
    rightInput.m_bounds.m_min[axis] = results.m_splitMin;
    rightInput.m_writer = input.m_writer;
    _mm_store_ps(&reinterpret_cast<float*>(&rightInput.m_allowedEmptySplits.m_mask)[0], kAllAxes);

    if (results.m_splitIndex == input.m_endPrimIdx || results.m_splitIndex == input.m_startPrimIdx) {
        const int32_t negateAxisMasks[3] =
            {hkVector4Comparison::MASK_YZ, hkVector4Comparison::MASK_XZ, hkVector4Comparison::MASK_XY};
        const __m128 kAllowed = _mm_and_ps(
            _mm_load_ps(&reinterpret_cast<const float*>(&input.m_allowedEmptySplits.m_mask)[0]),
            _mm_load_ps(&reinterpret_cast<float*>(&hkVector4Comparison::s_maskFromBits[negateAxisMasks[axis]])[0])
        );
        _mm_store_ps(&reinterpret_cast<float*>(&leftInput.m_allowedEmptySplits.m_mask)[0], kAllowed);
        _mm_store_ps(&reinterpret_cast<float*>(&rightInput.m_allowedEmptySplits.m_mask)[0], kAllowed);
    }

    const int32_t leftDepth = buildTreeRecursive(leftInput, treeOut, projEntOut, numNodes, numAvailableEmptyNodes);
    const int32_t rightDepth = buildTreeRecursive(rightInput, treeOut, projEntOut, numNodes, numAvailableEmptyNodes);
    return ((leftDepth > rightDepth) ? leftDepth : rightDepth) + 1;
}

// The split by the mean of the primitives' centres on the bounds' longest axis. The in and out buffers are distinct, so
// the degenerate split's halves are copied and scanned in one pass.
hkSplitPrimitivesStatus
hkKdTreeBuilder<const hkKdTreeProjectedEntry*, hkKdTreeProjectedEntry*, 0>::splitPrimitivesExactly(
    const hkKdTreeBuildInput::WorkingSet& buffersIn,
    int32_t startPrimIdx,
    int32_t endPrimIdx,
    hkKdTreeBuilder<const hkKdTreeProjectedEntry*, hkKdTreeProjectedEntry*, 0>::SplitPrimitivesResults& results,
    hkKdTreeBuildInput::WorkingSet& buffersOut,
    const hkKdTreeProjectedEntry& bounds
) {
    const hkKdTreeProjectedEntry* pIn = &buffersIn.m_entriesIn[startPrimIdx];
    const int32_t iNumPrims = endPrimIdx - startPrimIdx;
    const uint16_t uiExtentX = static_cast<uint16_t>(bounds.m_max[0] - bounds.m_min[0]);
    const uint16_t uiExtentY = static_cast<uint16_t>(bounds.m_max[1] - bounds.m_min[1]);
    const uint16_t uiExtentZ = static_cast<uint16_t>(bounds.m_max[2] - bounds.m_min[2]);
    int32_t iSplitAxis;
    if (uiExtentX <= uiExtentY) {
        iSplitAxis = uiExtentZ < uiExtentY ? 1 : 2;
    }
    else {
        iSplitAxis = uiExtentZ < uiExtentX ? 0 : 2;
    }

    int32_t iCentreSum = 0;
    for (int32_t i = 0; i < iNumPrims; ++i) {
        iCentreSum += (pIn[i].m_min[iSplitAxis] + pIn[i].m_max[iSplitAxis]) >> 1;
    }
    const int32_t iSplitValue = iCentreSum / iNumPrims;

    int32_t iNumLeft = 0;
    int32_t iNumRight = 0;
    for (int32_t i = 0; i < iNumPrims; ++i) {
        iNumLeft += pIn[i].m_min[iSplitAxis] <= iSplitValue;
        iNumRight += pIn[i].m_max[iSplitAxis] >= iSplitValue;
    }
    const bool bDegenerate = !iNumLeft || !iNumRight;
    if (iNumLeft == iNumRight && iNumLeft == iNumPrims) {
        return SPLIT_PRIMITIVES_INDISTINGUISHABLE;
    }

    hkKdTreeProjectedEntry* pEntriesOut = buffersIn.m_entriesOut;
    uint16_t uiSplitMin = bounds.m_max[iSplitAxis];
    uint16_t uiSplitMax = bounds.m_min[iSplitAxis];
    hkKdTreeProjectedEntry* pLeft = &pEntriesOut[startPrimIdx];
    int32_t iSplitIndex;
    if (bDegenerate) {
        // Neither side is empty only by the mean: split the primitives in two halves as they lie.
        const int32_t iHalf = iNumPrims >> 1;
        iSplitIndex = startPrimIdx + iHalf;
        for (int32_t i = 0; i < iHalf; ++i) {
            pLeft[i] = pIn[i];
            if (uiSplitMax <= pIn[i].m_max[iSplitAxis]) {
                uiSplitMax = pIn[i].m_max[iSplitAxis];
            }
        }
        for (int32_t i = iHalf; i < iNumPrims; ++i) {
            pLeft[i] = pIn[i];
            if (uiSplitMin >= pIn[i].m_min[iSplitAxis]) {
                uiSplitMin = pIn[i].m_min[iSplitAxis];
            }
        }
    }
    else if (iNumLeft >= iNumRight) {
        // The right side takes the primitives reaching the split value.
        iSplitIndex = endPrimIdx - iNumRight;
        hkKdTreeProjectedEntry* pRight = &pEntriesOut[iSplitIndex];
        for (int32_t i = 0; i < iNumPrims; ++i) {
            const uint16_t uiMin = pIn[i].m_min[iSplitAxis];
            const uint16_t uiMax = pIn[i].m_max[iSplitAxis];
            if (uiMax < iSplitValue) {
                *pLeft++ = pIn[i];
                if (uiSplitMax <= uiMax) {
                    uiSplitMax = uiMax;
                }
            }
            else {
                *pRight++ = pIn[i];
                if (uiSplitMin >= uiMin) {
                    uiSplitMin = uiMin;
                }
            }
        }
    }
    else {
        // The left side takes the primitives starting at or below the split value.
        iSplitIndex = startPrimIdx + iNumLeft;
        hkKdTreeProjectedEntry* pRight = &pEntriesOut[iSplitIndex];
        for (int32_t i = 0; i < iNumPrims; ++i) {
            const uint16_t uiMin = pIn[i].m_min[iSplitAxis];
            const uint16_t uiMax = pIn[i].m_max[iSplitAxis];
            if (uiMin <= iSplitValue) {
                *pLeft++ = pIn[i];
                if (uiSplitMax <= uiMax) {
                    uiSplitMax = uiMax;
                }
            }
            else {
                *pRight++ = pIn[i];
                if (uiSplitMin >= uiMin) {
                    uiSplitMin = uiMin;
                }
            }
        }
    }
    results.m_splitMin = uiSplitMin;
    results.m_splitMax = uiSplitMax;
    results.m_splitAxis = iSplitAxis;
    results.m_splitIndex = iSplitIndex;
    buffersOut.m_entriesOut = buffersIn.m_entriesIn;
    buffersOut.m_entriesIn = buffersIn.m_entriesOut;
    return SPLIT_PRIMITIVES_OK;
}

int32_t hkKdTreeProjectedEntryBuilder::writeEmptyLeaf(
    hkKdTreeNode* treeRoot,
    int32_t nodeIdx,
    hkKdTreeBuilderSparseWriter* writer
) {
    treeRoot[nodeIdx].setEmpty();
    treeRoot[nodeIdx].setNumPrimitivesInLeaf(0);
    return 0;
}

void hkKdTreeBuilder<const hkKdTreeProjectedEntry*, hkKdTreeProjectedEntry*, 0>::writeSinglePrimitiveToLeaf(
    hkKdTreeNode* treeRoot,
    hkKdTreeProjectedEntry* entriesIn,
    hkKdTreeProjectedEntry* projEntOut,
    int32_t nodeIdx,
    int32_t primIdx,
    hkKdTreeBuilderSparseWriter* writer
) {
    hkKdTreeProjectedEntry v6 = entriesIn[primIdx];
    treeRoot[nodeIdx].m_data.m_leaf.m_type_primId = primIdx & 0x7FFFFFFF;
    treeRoot[nodeIdx].m_data.m_leaf.m_numPrimitives = 1;
    projEntOut[primIdx] = v6;
}

int32_t hkKdTreeBuilder<const hkKdTreeProjectedEntry*, hkKdTreeProjectedEntry*, 0>::writeMultiplePrimitivesToLeaf(
    hkKdTreeNode* treeRoot,
    const hkKdTreeProjectedEntry* entriesIn,
    hkKdTreeProjectedEntry* projEntOut,
    int32_t nodeIdx,
    int32_t primIdx,
    int32_t numPrims,
    hkKdTreeBuilderSparseWriter* writer
) {
    const hkKdTreeProjectedEntry* v7 = &entriesIn[primIdx];
    treeRoot[nodeIdx].m_data.m_leaf.m_type_primId = primIdx & 0x7FFFFFFF;
    treeRoot[nodeIdx].m_data.m_leaf.m_numPrimitives = numPrims;
    if (numPrims > 0) {
        hkKdTreeProjectedEntry* v8 = &projEntOut[primIdx];
        int32_t v9 = numPrims;
        do {
            *v8++ = *v7++;
            --v9;
        }
        while (v9);
    }
    return 2 * numPrims - 2;
}

// Splits the primitives into four children under one node and its two child nodes. Every stride-th primitive (the
// stride grows with their count) is counted into 64 buckets per axis by the top six bits of its lower and of its upper
// bound. Per axis, the bucket plane that best balances the samples starting below it against those ending above it is
// found, then the same on each side of it; the axis whose four children's counts differ least wins (float sums, the
// first strictly lower one kept). The primitives are partitioned by their lower bound against the three planes; when a
// child stays empty or a side had no sample, they are cut into quarters in order. The children's bounds on the split
// axis are kept as 16-bit minima and maxima.
void hkKdTreeBuilder<const hkKdTreeProjectedEntry*, hkKdTreeProjectedEntry*, 0>::splitTreeBuilding(
    const hkKdTreeBuildInput::WorkingSet& buffersIn,
    hkKdTreeNode* treeOut,
    int32_t treeSize,
    int32_t numPrimitives,
    hkKdTreeBuildInput::BuildRecursiveInput* subJobs,
    int32_t* subTreeSizes,
    const hkKdTreeProjectedEntry& bounds
) {
    alignas(128) int32_t bucketsMin[64][4];
    alignas(128) int32_t bucketsMax[64][4];
    hkString::memClear16(bucketsMin, 64);
    hkString::memClear16(bucketsMax, 64);

    int32_t stride = 1;
    if (numPrimitives > 10) {
        stride = 2;
    }
    if (numPrimitives > 100) {
        stride = 3;
    }
    if (numPrimitives > 1000) {
        stride = 4;
    }
    if (numPrimitives > 10000) {
        stride = 5;
    }
    const hkKdTreeProjectedEntry* pSample = buffersIn.m_entriesIn;
    for (int32_t i = 0; i < numPrimitives; i += stride) {
        ++bucketsMin[pSample->m_min[0] >> 10][0];
        ++bucketsMin[pSample->m_min[1] >> 10][1];
        ++bucketsMin[pSample->m_min[2] >> 10][2];
        ++bucketsMax[pSample->m_max[0] >> 10][0];
        ++bucketsMax[pSample->m_max[1] >> 10][1];
        ++bucketsMax[pSample->m_max[2] >> 10][2];
        pSample += stride;
    }
    const int32_t numSamples = numPrimitives / stride;

    // Deliberately 3.40282e+38f rather than FLT_MAX.
    float bestCost = HK_REAL_MAX;
    int32_t bestCounts[4] = {0, 0, 0, 0};
    int32_t bestAxis = -1;
    int32_t bestSplitLo = -1;
    int32_t bestSplitMid = -1;
    int32_t bestSplitHi = -1;
    for (int32_t axis = 0; axis < 3; ++axis) {
        // The middle plane, between buckets 1 and 61.
        float bestDiff = HK_REAL_MAX;
        int32_t numBelow = bucketsMin[0][axis];
        int32_t numAbove = numSamples - bucketsMax[0][axis];
        int32_t splitMid = 0;
        int32_t numLeft = 0;
        int32_t numRight = 0;
        for (int32_t j = 1; j < 62; ++j) {
            numBelow += bucketsMin[j][axis];
            numAbove -= bucketsMax[j][axis];
            const float diff = std::fabs(numBelow - numAbove);
            if (bestDiff > diff) {
                bestDiff = diff;
                numLeft = numBelow;
                numRight = numAbove;
                splitMid = j;
            }
        }

        // The lower plane, below the middle one, over the left side's samples.
        float bestDiffLo = HK_REAL_MAX;
        int32_t splitLo = 0;
        int32_t counts[4] = {0, 0, 0, 0};
        numBelow = 0;
        numAbove = numLeft;
        for (int32_t k = 0; k < splitMid; ++k) {
            numBelow += bucketsMin[k][axis];
            numAbove -= bucketsMax[k][axis];
            const float diff = std::fabs(numBelow - numAbove);
            if (bestDiffLo > diff) {
                bestDiffLo = diff;
                counts[0] = numBelow;
                counts[1] = numAbove;
                splitLo = k;
            }
        }

        // The upper plane, above the middle one, over the right side's samples.
        float bestDiffHi = HK_REAL_MAX;
        int32_t splitHi = 0;
        numBelow = 0;
        numAbove = numRight;
        for (int32_t k = splitMid + 1; k < 63; ++k) {
            numBelow += bucketsMin[k][axis];
            numAbove -= bucketsMax[k][axis];
            const float diff = std::fabs(numBelow - numAbove);
            if (bestDiffHi > diff) {
                bestDiffHi = diff;
                counts[2] = numBelow;
                counts[3] = numAbove;
                splitHi = k;
            }
        }

        // The cost: the children's count differences summed, each pair once (a child with itself included).
        float cost = 0.0f;
        for (int32_t a = 0; a < 3; ++a) {
            for (int32_t b = a; b < 4; ++b) {
                const float diff = std::fabs(counts[a] - counts[b]);
                cost += diff;
            }
        }
        if (bestCost > cost) {
            bestCost = cost;
            bestSplitLo = splitLo;
            bestSplitMid = splitMid;
            bestSplitHi = splitHi;
            bestAxis = axis;
            bestCounts[0] = counts[0];
            bestCounts[1] = counts[1];
            bestCounts[2] = counts[2];
            bestCounts[3] = counts[3];
        }
    }

    int32_t numInChild[4] = {0, 0, 0, 0};
    uint16_t childMin[4];
    uint16_t childMax[4];
    childMin[0] = childMin[1] = childMin[2] = childMin[3] = bounds.m_max[bestAxis];
    childMax[0] = childMax[1] = childMax[2] = childMax[3] = bounds.m_min[bestAxis];
    const int32_t planeMid = (bestSplitMid + 1) << 10;
    const int32_t planeHi = (bestSplitHi + 1) << 10;
    const int32_t planeLo = (bestSplitLo + 1) << 10;
    const hkKdTreeProjectedEntry* pEnd = buffersIn.m_entriesIn + numPrimitives;
    const hkKdTreeProjectedEntry* pCur = buffersIn.m_entriesIn;
    bool bFallback = !bestCounts[0] || !bestCounts[1] || !bestCounts[2] || !bestCounts[3];
    if (!bFallback) {
        for (; pCur < pEnd; ++pCur) {
            const uint16_t uiMin = pCur->m_min[bestAxis];
            const uint16_t uiMax = pCur->m_max[bestAxis];
            int32_t child;
            if (uiMin < planeLo) {
                child = 0;
            }
            else if (uiMin < planeMid) {
                child = 1;
            }
            else if (uiMin < planeHi) {
                child = 2;
            }
            else {
                child = 3;
            }
            ++numInChild[child];
            if (childMin[child] >= uiMin) {
                childMin[child] = uiMin;
            }
            if (childMax[child] <= uiMax) {
                childMax[child] = uiMax;
            }
        }
        if (!numInChild[0] || !numInChild[1] || !numInChild[2] || !numInChild[3]) {
            childMin[0] = childMin[1] = childMin[2] = childMin[3] = bounds.m_max[bestAxis];
            numInChild[0] = numInChild[1] = numInChild[2] = numInChild[3] = 0;
            pCur = buffersIn.m_entriesIn;
            bFallback = true;
            childMax[0] = childMax[1] = childMax[2] = childMax[3] = bounds.m_min[bestAxis];
        }
    }

    if (bFallback) {
        const int32_t quarter = numPrimitives / 4;
        if (treeSize < 8 * quarter + 8) {
            char acBuffer[0x200];
            hkErrStream errStream(acBuffer, 0x200);
            errStream << "Not enough memory allocated for balanced fallback";
            if (hkError::s_instance->message(
                    hkError::MESSAGE_ERROR,
                    0x5010D60C,
                    acBuffer,
                    ".\\KdTree\\Build\\hkKdTreeBuilder.cpp",
                    408
                )) {
                xfnv::DebugTrap();
            }
        }
        const hkKdTreeProjectedEntry* const quarterEnds[4] = {
            buffersIn.m_entriesIn + quarter,
            buffersIn.m_entriesIn + 2 * quarter,
            buffersIn.m_entriesIn + 3 * quarter,
            pEnd
        };
        for (int32_t c = 0; c < 4; ++c) {
            for (; pCur < quarterEnds[c]; ++pCur) {
                ++numInChild[c];
                childMin[c] = static_cast<uint16_t>(hkMinU16(childMin[c], pCur->m_min[bestAxis]));
                childMax[c] = static_cast<uint16_t>(max2Uint16(childMax[c], pCur->m_max[bestAxis]));
            }
        }
    }

    hkKdTreeProjectedEntry* pOut0 = buffersIn.m_entriesOut;
    hkKdTreeProjectedEntry* pOut1 = buffersIn.m_entriesOut + numInChild[0];
    hkKdTreeProjectedEntry* pOut2 = buffersIn.m_entriesOut + (numInChild[0] + numInChild[1]);
    hkKdTreeProjectedEntry* pOut3 = buffersIn.m_entriesOut + (numInChild[1] + numInChild[2] + numInChild[0]);
    pCur = buffersIn.m_entriesIn;
    if (bFallback) {
        // The quarters in order.
        const int32_t quarter = numPrimitives / 4;
        const hkKdTreeProjectedEntry* pIn = pCur;
        for (; pCur < pIn + quarter; ++pCur) {
            *pOut0++ = *pCur;
        }
        for (; pCur < pIn + 2 * quarter; ++pCur) {
            *pOut1++ = *pCur;
        }
        for (; pCur < pIn + 3 * quarter; ++pCur) {
            *pOut2++ = *pCur;
        }
        for (; pCur < pEnd; ++pCur) {
            *pOut3++ = *pCur;
        }
    }
    else {
        for (; pCur < pEnd; ++pCur) {
            const int32_t iMin = pCur->m_min[bestAxis];
            if (iMin < planeLo) {
                *pOut0++ = *pCur;
            }
            else if (iMin < planeMid) {
                *pOut1++ = *pCur;
            }
            else if (iMin < planeHi) {
                *pOut2++ = *pCur;
            }
            else {
                *pOut3++ = *pCur;
            }
        }
    }

    // The node and its two children: each an inner node splitting on bestAxis, its children 0x10 bytes (two nodes)
    // on from node 0 and node 2, 0x18 bytes on from node 3; node 1 is not written. The split words are the right
    // child's lower and the left child's upper bound.
    const uint16_t uiMax01 = hkMath::max2(childMax[1], childMax[0]);
    const uint16_t uiMin23 = hkMath::min2(childMin[2], childMin[3]);
    treeOut[0].m_data.m_node.m_splitMin = uiMin23;
    treeOut[0].m_data.m_node.m_splitMax = uiMax01;
    treeOut[0].m_data.m_node.m_type_left_axis = static_cast<int32_t>(0x80000010u | (bestAxis & 3));
    treeOut[2].m_data.m_node.m_type_left_axis = static_cast<int32_t>(0x80000010u | (bestAxis & 3));
    treeOut[2].m_data.m_node.m_splitMin = childMin[1];
    treeOut[2].m_data.m_node.m_splitMax = childMax[0];
    treeOut[3].m_data.m_node.m_type_left_axis = static_cast<int32_t>(0x80000018u | (bestAxis & 3));
    treeOut[3].m_data.m_node.m_splitMin = childMin[3];
    treeOut[3].m_data.m_node.m_splitMax = childMax[2];

    // The four sub jobs, reading their children's entries from this split's output.
    hkKdTreeProjectedEntry* pEntriesIn = buffersIn.m_entriesIn;
    hkKdTreeProjectedEntry* pEntriesOut = buffersIn.m_entriesOut;
    int32_t startPrimIdx = 0;
    for (int32_t c = 0; c < 4; ++c) {
        hkKdTreeBuildInput::BuildRecursiveInput& job = subJobs[c];
        job.m_buffers->m_entriesIn = pEntriesOut;
        job.m_buffers->m_entriesOut = pEntriesIn;
        job.m_startPrimIdx = startPrimIdx;
        job.m_currentNodeIdx = 0;
        startPrimIdx += numInChild[c];
        job.m_endPrimIdx = startPrimIdx;
        job.m_bounds = bounds;
        job.m_bounds.m_min[bestAxis] = childMin[c];
        job.m_bounds.m_max[bestAxis] = childMax[c];
        job.m_maxNodeIdx = treeSize;
        subTreeSizes[c] = 2 * numInChild[c];
    }
}
