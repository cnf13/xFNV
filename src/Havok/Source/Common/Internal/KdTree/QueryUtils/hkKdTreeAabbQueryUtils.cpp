#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbQueryUtils.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbCastUtils.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeUtils.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"

// hkGetOneOver65K's function-local static x (hkKdTreeMath.inl; the release hkInternal.lib's hkKdTreeAabbQueryUtils.obj
// has it as ?x@?1??hkGetOneOver65K@@YA?AUhkQuadReal@@XZ@4U2@A): this translation unit's copy, 1 / 65535 in every lane.
static hkQuadReal hkGetOneOver65K_x{1.5259022e-05f, 1.5259022e-05f, 1.5259022e-05f, 1.5259022e-05f};

// Gives queryAabbInternal the hit array, between the timer macros.
void hkKdTreeAabbQueryUtils::queryAabbIterative(
    const hkKdTree* tree,
    const hkAabb& aabbIn,
    hkArray<hkUlong, hkContainerTempAllocator>& hits
) {
    HK_TIMER_BEGIN("queryAabb", nullptr);
    const hkKdTreeNode* nodes = tree->m_nodes.m_data;
    const hkKdTreeProjectedEntry* projectedEntries = tree->m_projectedEntries.m_data;
    hkVector4 offset = tree->m_aabb.m_min;
    int32_t treeDepth = tree->m_maxDepth;
    hkVector4 scale;
    scale.m_quad = _mm_sub_ps(tree->m_aabb.m_max.m_quad, tree->m_aabb.m_min.m_quad);
    if (treeDepth <= 0) {
        treeDepth = DEFAULT_STACK_SIZE;
    }
    queryAabbInternal(nodes, projectedEntries, scale, offset, aabbIn, treeDepth, &hits, nullptr, nullptr, 0);
    HK_TIMER_END();
}

// Writes the ids of the primitives of root whose boxes overlap aabb to *hitsOut, at most maxHits; returns how many.
uint32_t hkKdTreeAabbQueryUtils::queryAabbIterative(
    const hkKdTree* root,
    const hkAabb& aabb,
    hkUlong** hitsOut,
    int32_t maxHits
) {
    const hkKdTreeNode* nodes = root->m_nodes.m_data;
    const hkKdTreeProjectedEntry* projectedEntries = root->m_projectedEntries.m_data;
    hkVector4 offset = root->m_aabb.m_min;
    int32_t treeDepth = root->m_maxDepth;
    hkVector4 scale;
    scale.m_quad = _mm_sub_ps(root->m_aabb.m_max.m_quad, root->m_aabb.m_min.m_quad);
    if (treeDepth <= 0) {
        treeDepth = DEFAULT_STACK_SIZE;
    }
    return queryAabbInternal(
        nodes, projectedEntries, scale, offset, aabb, treeDepth, nullptr, nullptr, hitsOut, maxHits
    );
}

// Hands collector every primitive of root whose box overlaps aabb, with neither hit array nor hit iterator, between the
// timer macros.
void hkKdTreeAabbQueryUtils::queryAabbIterative(
    const hkKdTree* root,
    const hkAabb& aabb,
    hkKdTreeCastCollector* collector
) {
    HK_TIMER_BEGIN("queryAabb", nullptr);
    const hkKdTreeNode* nodes = root->m_nodes.m_data;
    const hkKdTreeProjectedEntry* projectedEntries = root->m_projectedEntries.m_data;
    hkVector4 offset = root->m_aabb.m_min;
    int32_t treeDepth = root->m_maxDepth;
    hkVector4 scale;
    scale.m_quad = _mm_sub_ps(root->m_aabb.m_max.m_quad, root->m_aabb.m_min.m_quad);
    if (treeDepth <= 0) {
        treeDepth = DEFAULT_STACK_SIZE;
    }
    queryAabbInternal(nodes, projectedEntries, scale, offset, aabb, treeDepth, nullptr, collector, nullptr, 0);
    HK_TIMER_END();
}

// Hands _queryAabbRecursive the tree's root (as both its root and its first node) and the tree's dequantisation offset
// and scale; no timer macros.
void hkKdTreeAabbQueryUtils::queryAabbRecursive(
    const hkKdTree* tree,
    const hkAabb& aabb,
    hkArray<hkUlong, hkContainerTempAllocator>& hits
) {
    const hkKdTreeNode* root = tree->m_nodes.m_data;
    hkVector4 offset = tree->m_aabb.m_min;
    const hkKdTreeProjectedEntry* projectedEntries = tree->m_projectedEntries.m_data;
    hkVector4 scale;
    scale.m_quad = _mm_sub_ps(tree->m_aabb.m_max.m_quad, tree->m_aabb.m_min.m_quad);
    _queryAabbRecursive(root, root, projectedEntries, scale, offset, aabb, hits);
}

// An inner node's planes are its 16-bit splits / 65535.0 times scale plus offset, each stored as a float, tested
// against aabb: the first child alone when the low plane lies above the box (NaN goes on to the next test), the second
// alone when the high plane lies below it (NaN recurses into the first), else the first is walked by a recursive call
// and the loop goes on with the second. A leaf's entries are tested four lanes at a time against aabb (x, y and z),
// with each 16-bit word converted to a float. Each hit's id (the entry's words at +6 and +0xE) is pushed onto hits.
void hkKdTreeAabbQueryUtils::_queryAabbRecursive(
    const hkKdTreeNode* root,
    const hkKdTreeNode* ppuNode,
    const hkKdTreeProjectedEntry* projectedEntries,
    const hkVector4& scale,
    const hkVector4& offset,
    const hkAabb& aabb,
    hkArray<hkUlong, hkContainerTempAllocator>& hits
) {
    // A node's two words are loaded once, when the node is picked, and the code works from those copies: the header
    // (sign, left offset and axis; a leaf's primitive index) and the second word (an inner node's two splits, a leaf's
    // primitive count).
    int32_t iHeader = ppuNode->m_data.m_node.m_type_left_axis;
    uint32_t uiWord1 = ppuNode->m_data.m_leaf.m_numPrimitives;
    while (iHeader < 0) {
        // The node stores its left child as a byte offset from itself: the header is read again for it and the offset
        // is added to the node's address as a raw byte count.
        const hkKdTreeNode* pLeft = reinterpret_cast<const hkKdTreeNode*>(
            reinterpret_cast<const char*>(ppuNode) + (ppuNode->m_data.m_node.m_type_left_axis & 0x7FFFFFFC)
        );
        const hkKdTreeNode* pRight = pLeft + 1;
        const int32_t iAxis = iHeader & 3;
        // Each plane is rounded to a float before its compare. The second word's low half is m_splitMin, its high half
        // m_splitMax.
        const float fLow = static_cast<int32_t>(uiWord1 & 0xFFFF) / 65535.0 * scale(iAxis) + offset(iAxis);
        if (fLow > aabb.m_max(iAxis)) {
            ppuNode = pLeft;
        }
        else {
            const float fHigh = static_cast<int32_t>(uiWord1 >> 16) / 65535.0 * scale(iAxis) + offset(iAxis);
            if (!(fHigh < aabb.m_min(iAxis))) {
                _queryAabbRecursive(root, pLeft, projectedEntries, scale, offset, aabb, hits);
            }
            ppuNode = pRight;
        }
        iHeader = ppuNode->m_data.m_node.m_type_left_axis;
        uiWord1 = ppuNode->m_data.m_leaf.m_numPrimitives;
    }

    const int32_t iNumPrimitives = static_cast<int32_t>(uiWord1);
    if ((iHeader ^ 0x7FFFFFFF) == 0 || iNumPrimitives <= 0) {
        return;
    }
    const hkKdTreeProjectedEntry* pEntry = projectedEntries + (iHeader & 0x7FFFFFFF);
    for (int32_t i = 0; i < iNumPrimitives; ++i, ++pEntry) {
        const __m128 kScale = _mm_mul_ps(scale.m_quad, hkGetOneOver65K_D1B280());
        const __m128 kMinWords = _mm_set_ps(
            static_cast<float>(pEntry->m_primIdHigh), static_cast<float>(pEntry->m_min[2]),
            static_cast<float>(pEntry->m_min[1]), static_cast<float>(pEntry->m_min[0])
        );
        const __m128 kMaxWords = _mm_set_ps(
            static_cast<float>(pEntry->m_primIdLow), static_cast<float>(pEntry->m_max[2]),
            static_cast<float>(pEntry->m_max[1]), static_cast<float>(pEntry->m_max[0])
        );
        const __m128 kBoxMin = _mm_add_ps(_mm_mul_ps(kMinWords, kScale), offset.m_quad);
        const __m128 kBoxMax = _mm_add_ps(_mm_mul_ps(kMaxWords, kScale), offset.m_quad);
        const __m128 kOverlap =
            _mm_and_ps(_mm_cmple_ps(kBoxMin, aabb.m_max.m_quad), _mm_cmple_ps(aabb.m_min.m_quad, kBoxMax));
        if ((_mm_movemask_ps(kOverlap) & 7) != 7) {
            continue;
        }
        const hkPrimitiveId uiPrimitiveId = pEntry->getPrimitiveId();
        hits.pushBack(uiPrimitiveId);
    }
}

// The node stack takes (treeDepth pointers + 0x7F) & ~0x7F bytes of the router's stack memory, given back before the
// return. An inner node's planes are its 16-bit splits / 65535.0 times scale plus offset, each stored as a float,
// tested against a copy of aabb: the first child alone when the low plane lies above the box (NaN goes on to the next
// test), the second alone when the high plane lies below it (NaN pushes the second and visits the first), else the
// second is pushed and the first visited. A leaf's entries are tested four lanes at a time against aabb itself (x, y
// and z), with the words converted directly to floats. A hit goes to collector->addMidPhaseHit (its id the entry's
// words; a true result ends the query), else to hitArray, else to hitsOut, where the count reaching maxHits ends the
// query.
uint32_t hkKdTreeAabbQueryUtils::queryAabbInternal(
    const hkKdTreeNode* root,
    const hkKdTreeProjectedEntry* projectedEntries,
    const hkVector4& scale,
    const hkVector4& offset,
    const hkAabb& aabbIn,
    int32_t stackSize,
    hkArray<hkUlong, hkContainerTempAllocator>* hitArray,
    hkKdTreeCastCollector* collector,
    hkUlong** hitsOut,
    int32_t maxHits
) {
    const hkKdTreeNode* pNode = root;
    // A node's two words are loaded once, when the node is picked (the root's before the stack allocation), and the
    // code works from those copies: the header (sign, left offset and axis; a leaf's primitive index) and the second
    // word (an inner node's two splits, a leaf's primitive count).
    int32_t iHeader = pNode->m_data.m_node.m_type_left_axis;
    uint32_t uiWord1 = pNode->m_data.m_leaf.m_numPrimitives;
    const hkKdTreeNode** const pStackBase = hkAllocateStack<const hkKdTreeNode*>(stackSize, nullptr);
    const hkKdTreeNode** pTop = pStackBase;
    const hkAabb kAabb = aabbIn;
    int32_t iNumHits = 0;

    for (;;) {
        // Descend through the inner nodes.
        while (iHeader < 0) {
            // The node stores its left child as a byte offset from itself; it is read from the header again and added
            // to the node's address.
            const hkKdTreeNode* pLeft = reinterpret_cast<const hkKdTreeNode*>(
                reinterpret_cast<const char*>(pNode) + (pNode->m_data.m_node.m_type_left_axis & 0x7FFFFFFC)
            );
            const hkKdTreeNode* pRight = pLeft + 1;
            const int32_t iAxis = iHeader & 3;
            // Each plane is rounded to a float before its compare. The second word's low half is m_splitMin, its high
            // half m_splitMax.
            const float fLow = static_cast<int32_t>(uiWord1 & 0xFFFF) / 65535.0 * scale(iAxis) + offset(iAxis);
            if (fLow > kAabb.m_max(iAxis)) {
                pNode = pLeft;
            }
            else {
                const float fHigh = static_cast<int32_t>(uiWord1 >> 16) / 65535.0 * scale(iAxis) + offset(iAxis);
                if (fHigh < kAabb.m_min(iAxis)) {
                    pNode = pRight;
                }
                else {
                    *pTop++ = pRight;
                    pNode = pLeft;
                }
            }
            iHeader = pNode->m_data.m_node.m_type_left_axis;
            uiWord1 = pNode->m_data.m_leaf.m_numPrimitives;
        }

        const int32_t iNumPrimitives = static_cast<int32_t>(uiWord1);
        if ((iHeader ^ 0x7FFFFFFF) != 0 && iNumPrimitives > 0) {
            const hkKdTreeProjectedEntry* pEntry = projectedEntries + (iHeader & 0x7FFFFFFF);
            for (int32_t i = 0; i < iNumPrimitives; ++i, ++pEntry) {
                const __m128 kScale = _mm_mul_ps(hkGetOneOver65K_D1B280(), scale.m_quad);
                const __m128 kMinWords = _mm_set_ps(
                    static_cast<float>(pEntry->m_primIdHigh), static_cast<float>(pEntry->m_min[2]),
                    static_cast<float>(pEntry->m_min[1]), static_cast<float>(pEntry->m_min[0])
                );
                const __m128 kMaxWords = _mm_set_ps(
                    static_cast<float>(pEntry->m_primIdLow), static_cast<float>(pEntry->m_max[2]),
                    static_cast<float>(pEntry->m_max[1]), static_cast<float>(pEntry->m_max[0])
                );
                const __m128 kBoxMin = _mm_add_ps(_mm_mul_ps(kMinWords, kScale), offset.m_quad);
                const __m128 kBoxMax = _mm_add_ps(_mm_mul_ps(kMaxWords, kScale), offset.m_quad);
                const __m128 kOverlap =
                    _mm_and_ps(_mm_cmple_ps(aabbIn.m_min.m_quad, kBoxMax), _mm_cmple_ps(kBoxMin, aabbIn.m_max.m_quad));
                if ((_mm_movemask_ps(kOverlap) & 7) != 7) {
                    continue;
                }
                const hkPrimitiveId uiPrimitiveId = pEntry->getPrimitiveId();
                if (collector) {
                    if (collector->addMidPhaseHit(uiPrimitiveId).m_bool) {
                        goto done;
                    }
                }
                else if (hitArray) {
                    hitArray->pushBack(uiPrimitiveId);
                }
                else if (hitsOut) {
                    // Reads *hitsOut again for the increment, after the store.
                    **hitsOut = uiPrimitiveId;
                    ++*hitsOut;
                    if (++iNumHits >= maxHits) {
                        goto done;
                    }
                }
            }
        }

        // Pop the next subtree.
        if (pTop == pStackBase) {
            break;
        }
        pNode = *--pTop;
        iHeader = pNode->m_data.m_node.m_type_left_axis;
        uiWord1 = pNode->m_data.m_leaf.m_numPrimitives;
    }

done:
    hkDeallocateStack(pStackBase);
    return iNumHits;
}

// hkGetOneOver65K, the dequantisation factor of a projected entry's 16-bit bounds: the same value in all four lanes.
hkQuadReal hkGetOneOver65K_D1B280() {
    return hkGetOneOver65K_x;
}

// The SDK's recursive kd tree ray cast: a4 and a5 are the tree's extent and minimum (a node's 16-bit split planes and
// an entry's 16-bit bounds dequantised against them), a6 and a7 the ray's start and direction, a8 to a9 the part of
// the ray still to walk and a10 the factor on the collector's hit fraction that ends it. An inner node whose split
// plane the ray runs along walks each child the start lies in (a quarter unit of slack); any other walks the near
// child, the far one, or both (the near one first, the far one only while it can still beat the collector's hit). A
// leaf hands each entry whose box the ray passes through to the collector.
void castRayRecursive(
    const hkKdTreeNode* a1,
    const hkKdTreeNode* a2,
    const hkKdTreeProjectedEntry* a3,
    __m128* a4,
    __m128* a5,
    __m128* a6,
    __m128* a7,
    float a8,
    float a9,
    float a10,
    hkKdTreeCastCollector* a11
) {
    const hkKdTreeNode* pNode = a2;
    while (pNode->m_data.m_node.m_type_left_axis < 0) {
        int32_t iWord = pNode->m_data.m_node.m_type_left_axis;
        // The left child sits the word's byte offset past this node (a byte offset over the node array); the right one
        // follows it.
        const hkKdTreeNode* pLeft =
            reinterpret_cast<const hkKdTreeNode*>(reinterpret_cast<const char*>(pNode) + (iWord & 0x7FFFFFFC));
        const hkKdTreeNode* pRight = pLeft + 1;
        int32_t iAxis = iWord & 3;
        float fSplitMin = static_cast<double>(pNode->m_data.m_node.m_splitMin) / 65535.0 * (*a4)[iAxis] + (*a5)[iAxis];
        float fSplitMax = static_cast<double>(pNode->m_data.m_node.m_splitMax) / 65535.0 * (*a4)[iAxis] + (*a5)[iAxis];
        uint32_t uiAxisBit = axisMaskBits_10C92E0[iAxis];
        if ((_mm_movemask_ps(_mm_cmpeq_ps(_mm_setzero_ps(), *a7)) & uiAxisBit) == uiAxisBit) {
            bool bLeft = fSplitMax + 0.25 >= (*a6)[iAxis];
            bool bRight = (*a6)[iAxis] >= fSplitMin - 0.25;
            if (bLeft) {
                castRayRecursive(a1, pLeft, a3, a4, a5, a6, a7, a8, a9, a10, a11);
            }
            if (!bRight) {
                return;
            }
            pNode = pRight;
            continue;
        }
        float fT1 = (fSplitMin - (*a6)[iAxis]) / (*a7)[iAxis];
        float fT2 = (fSplitMax - (*a6)[iAxis]) / (*a7)[iAxis];
        const hkKdTreeNode* pFirst = pLeft;
        const hkKdTreeNode* pSecond = pRight;
        if ((_mm_movemask_ps(_mm_cmplt_ps(_mm_setzero_ps(), *a7)) & uiAxisBit) != uiAxisBit) {
            pFirst = pRight;
            pSecond = pLeft;
            float fSwap = fT1;
            fT1 = fT2;
            fT2 = fSwap;
        }
        float fHit = a11->getHitFraction() * a10;
        float fLimit = a9 > fHit ? fHit : a9;
        if (a8 > fT2) {
            pNode = pSecond;
            continue;
        }
        if (fLimit < fT1) {
            pNode = pFirst;
            continue;
        }
        // Asks the collector whether it has a hit and leaves the answer unread, as the game does.
        a11->hasHit();
        float fSecondStart = hkMath::max2<float>(fT1, a8);
        castRayRecursive(a1, pFirst, a3, a4, a5, a6, a7, a8, hkMath::min2(a9, fT2), a10, a11);
        float fHitAfter = a11->getHitFraction() * a10;
        if (fSecondStart < hkMath::min2(fHitAfter, a9)) {
            castRayRecursive(a1, pSecond, a3, a4, a5, a6, a7, fSecondStart, a9, a10, a11);
        }
        return;
    }
    int32_t iFirst = pNode->m_data.m_leaf.m_type_primId;
    if (iFirst == 0x7FFFFFFF) {
        return;
    }
    int32_t iCount = static_cast<int32_t>(pNode->m_data.m_leaf.m_numPrimitives);
    if (iCount <= 0) {
        return;
    }
    const __m128 vZero = _mm_setzero_ps();
    const __m128 vStart = _mm_set1_ps(a8);
    const __m128 vEnd = _mm_set1_ps(a9);
    const __m128 vNoEnter = _mm_set1_ps(rdata_104E148);
    const __m128 vNoExit = _mm_set1_ps(rdata_104DA48);
    const hkKdTreeProjectedEntry* pEntry = &a3[iFirst & 0x7FFFFFFF];
    do {
        // The id's low half is the entry's m_primIdHigh, its high half m_primIdLow.
        hkPrimitiveId primId = pEntry->getPrimitiveId();
        __m128 vQuantScale = _mm_mul_ps(_mm_set1_ps(1.5259022e-05f), *a4);
        __m128 vOffset = *a5;
        // Each of the entry's eight words is converted to a float holding the word's exact value (+0 for a zero word),
        // so these conversions give the same result bit for bit without SSE2.
        __m128 vMinQ = _mm_set_ps(
            static_cast<float>(pEntry->m_primIdHigh), static_cast<float>(pEntry->m_min[2]),
            static_cast<float>(pEntry->m_min[1]), static_cast<float>(pEntry->m_min[0])
        );
        __m128 vMaxQ = _mm_set_ps(
            static_cast<float>(pEntry->m_primIdLow), static_cast<float>(pEntry->m_max[2]),
            static_cast<float>(pEntry->m_max[1]), static_cast<float>(pEntry->m_max[0])
        );
        __m128 vBoxMin = _mm_add_ps(_mm_mul_ps(vMinQ, vQuantScale), vOffset);
        __m128 vBoxMax = _mm_add_ps(_mm_mul_ps(vMaxQ, vQuantScale), vOffset);
        __m128 vDir = *a7;
        __m128 vDirW1 = _mm_shuffle_ps(vDir, _mm_unpackhi_ps(vDir, hkQuadReal1111), 0xC4);
        __m128 vRcp = _mm_rcp_ps(vDirW1);
        __m128 vInv = _mm_sub_ps(_mm_add_ps(vRcp, vRcp), _mm_mul_ps(_mm_mul_ps(vRcp, vDirW1), vRcp));
        __m128 vDirZero = _mm_cmpeq_ps(vZero, vDir);
        vInv = _mm_or_ps(_mm_andnot_ps(vDirZero, vInv), _mm_and_ps(vDirZero, vZero));
        __m128 vFrom = *a6;
        __m128 vTMin = _mm_mul_ps(_mm_sub_ps(vBoxMin, vFrom), vInv);
        __m128 vTMax = _mm_mul_ps(_mm_sub_ps(vBoxMax, vFrom), vInv);
        __m128 vPositive = _mm_cmple_ps(vZero, vInv);
        __m128 vInvZero = _mm_cmpeq_ps(vZero, vInv);
        __m128 vEnter = _mm_or_ps(
            _mm_andnot_ps(vInvZero, _mm_or_ps(_mm_andnot_ps(vPositive, vTMax), _mm_and_ps(vPositive, vTMin))),
            _mm_and_ps(vInvZero, vNoEnter)
        );
        __m128 vExit = _mm_or_ps(
            _mm_andnot_ps(vInvZero, _mm_or_ps(_mm_and_ps(vPositive, vTMax), _mm_andnot_ps(vPositive, vTMin))),
            _mm_and_ps(vInvZero, vNoExit)
        );
        __m128 vExitMin = _mm_min_ps(
            vEnd,
            _mm_min_ps(
                _mm_min_ps(_mm_shuffle_ps(vExit, vExit, 0x00), _mm_shuffle_ps(vExit, vExit, 0x55)),
                _mm_shuffle_ps(vExit, vExit, 0xAA)
            )
        );
        __m128 vEnterMax = _mm_max_ps(
            vStart,
            _mm_max_ps(
                _mm_max_ps(_mm_shuffle_ps(vEnter, vEnter, 0x00), _mm_shuffle_ps(vEnter, vEnter, 0x55)),
                _mm_shuffle_ps(vEnter, vEnter, 0xAA)
            )
        );
        __m128 vHit = _mm_or_ps(
            _mm_andnot_ps(vInvZero, _mm_cmplt_ps(vEnterMax, vExitMin)),
            _mm_and_ps(_mm_and_ps(_mm_cmple_ps(vBoxMin, vFrom), _mm_cmple_ps(vFrom, vBoxMax)), vInvZero)
        );
        if ((_mm_movemask_ps(vHit) & 7) == 7) {
            a11->addMidPhaseHit(primId);
        }
        ++pEntry;
    }
    while (--iCount);
}
