#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuWorldRaycastJob.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbCastUtils.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseCastCollector.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpFixedBufferRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Util/hkAabbTreeRaycastUtil.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobUtils.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"

#include <xmmintrin.h>

// hkAabbTreeQueryUtils<hkAabbTreeAabb16, hkUint16>::castRayBundle: forwards its six arguments to castRayBundleImpl.
static void castRayBundle(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* tree,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* nodePpu,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* escapeNode,
    const hkAabbTreeData::RayBundle& rayBundle,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector** collectors,
    float* earlyHitFractions
);
// The SDK's hkAabbTreeQueryUtils<hkAabbTreeAabb, hkUint16>::castRayBundle
// (hkAabbTreeQueryUtils.h): forwards its six arguments to castRayBundleInternal.
static void castRayBundle(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* tree,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* nodePpu,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* escapeNode,
    const hkAabbTreeData::RayBundle& rayBundle,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector** collectors,
    float* earlyHitFractions
);
// The castRayBundle body for hkAabbTreeAabb16: tree, start node, escape node, the four-ray bundle, four collectors and
// four early-out fractions (the caller castRayBundleAabbTree16 passes the bundle it built and its collector and
// fraction arrays).
static void castRayBundleImpl(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* tree,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* nodePpu,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* escapeNode,
    const hkAabbTreeData::RayBundle& rayBundle,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector** collectors,
    float* earlyHitFractions
);
// The SDK's
// hkTreeRayCastJobUtil::castRayBundleAabbTree<hkAabbTreeAabb> (hkpRayCastQueryJobUtils.h), as
// hkTreeRayCastJobUtil::castRayBundleKdTree is called, with the first four arguments.
static void castRayBundleAabbTreeFp(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* aabbTree,
    hkpWorldRayCastBundleCommand* command,
    const hkpCollisionFilter* filter,
    hkVector4* earlyHitFractionInOut
);
// The SDK's
// hkTreeRayCastJobUtil::castRayBundleAabbTree<hkAabbTreeAabb16> (hkpRayCastQueryJobUtils.h), as
// hkTreeRayCastJobUtil::castRayBundleKdTree is called, with the first four arguments.
static void castRayBundleAabbTree16(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* aabbTree,
    hkpWorldRayCastBundleCommand* command,
    const hkpCollisionFilter* filter,
    hkVector4* earlyHitFractionInOut
);
// Builds rayBundle from the four rays' from and to points and mask
// (the SDK's RayBundle constructor on the transposed points) and returns rayBundle. castRayBundleAabbTreeFp and
// castRayBundleAabbTree16 pass a local hkAabbTreeData::RayBundle, the bundle command's points and MASK_XYZW.
static hkAabbTreeData::RayBundle* setRayBundle(
    hkAabbTreeData::RayBundle* rayBundle,
    const hkQuadReal* from0,
    const hkQuadReal* to0,
    const hkQuadReal* from1,
    const hkQuadReal* to1,
    const hkQuadReal* from2,
    const hkQuadReal* to2,
    const hkQuadReal* from3,
    const hkQuadReal* to3,
    hkVector4Comparison::Mask mask
);
// The body of hkAabbTreeQueryUtils<hkAabbTreeAabb, hkUint16>::castRayBundle (hkAabbTreeQueryUtils.h); its Aabb16 twin
// is castRayBundleImpl. Returns void, and neither caller expects a result.
static void castRayBundleInternal(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* tree,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* nodePpu,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* escapeNode,
    const hkAabbTreeData::RayBundle& rayBundle,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector** collectors,
    float* earlyHitFractions
);

// hkTreeRayCastJobUtil::castRayBundleAabbTree<hkAabbTreeAabb> (hkpRayCastQueryJobUtils.h): one single-ray command and
// one collector per ray of the bundle, the bundle cast through the tree, then each ray's hit count and early-out
// fraction handed back. The collectors' destructors run over the array in reverse order.
static void castRayBundleAabbTreeFp(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* aabbTree,
    hkpWorldRayCastBundleCommand* command,
    const hkpCollisionFilter* filter,
    hkVector4* earlyHitFractionInOut
) {
    using Collector = hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector;
    if (!aabbTree) {
        return;
    }
    hkAabbTreeData::RayBundle rayBundle;
    setRayBundle(
        &rayBundle, &command->m_rayInput[0].m_from.m_quad, &command->m_rayInput[0].m_to.m_quad,
        &command->m_rayInput[1].m_from.m_quad, &command->m_rayInput[1].m_to.m_quad,
        &command->m_rayInput[2].m_from.m_quad, &command->m_rayInput[2].m_to.m_quad,
        &command->m_rayInput[3].m_from.m_quad, &command->m_rayInput[3].m_to.m_quad, hkVector4Comparison::MASK_XYZW
    );
    hkpWorldRayCastCommand commands[4];
    hkCpuWorldRayCastCollector collectors[4];
    Collector* collectorPtrs[4];
    for (int32_t i = 0; i < 4; ++i) {
        hkCpuWorldRayCastCollector& collector = collectors[i];

        hkpWorldRayCastCommand& single = commands[i];
        single.m_rayInput.m_from = command->m_rayInput[i].m_from;
        single.m_rayInput.m_to = command->m_rayInput[i].m_to;
        single.m_rayInput.m_enableShapeCollectionFilter = command->m_rayInput[i].m_enableShapeCollectionFilter;
        single.m_rayInput.m_filterInfo = command->m_rayInput[i].m_filterInfo;
        single.m_results = command->m_results[i];
        single.m_resultsCapacity = 1;
        single.m_numResultsOut = 0;
        single.m_useCollector = false;
        single.m_stopAfterFirstHit = command->m_stopAfterFirstHit;

        collector.m_filter = filter;
        collector.m_command = &single;
        collector.m_originalInput = &single.m_rayInput;
        collector.m_result = single.m_results;
        collector.m_nextFreeResult = single.m_results;
        collectorPtrs[i] = &collector;
    }
    float earlyHitFractions[4];
    earlyHitFractions[0] = (*earlyHitFractionInOut)(0);
    earlyHitFractions[1] = (*earlyHitFractionInOut)(1);
    earlyHitFractions[2] = (*earlyHitFractionInOut)(2);
    earlyHitFractions[3] = (*earlyHitFractionInOut)(3);
    rayBundle.m_mask = hkAabbTreeQueryUtilsData::s_numRaysToMask[command->m_numActiveRays];
    castRayBundle(aabbTree, aabbTree->m_rootNode, nullptr, rayBundle, collectorPtrs, earlyHitFractions);
    command->m_numResultsOut[0] = commands[0].m_numResultsOut;
    command->m_numResultsOut[1] = commands[1].m_numResultsOut;
    command->m_numResultsOut[2] = commands[2].m_numResultsOut;
    command->m_numResultsOut[3] = commands[3].m_numResultsOut;
    earlyHitFractionInOut->set(earlyHitFractions[0], earlyHitFractions[1], earlyHitFractions[2], earlyHitFractions[3]);
}

// hkTreeRayCastJobUtil::castRayBundleAabbTree<hkAabbTreeAabb16>, as castRayBundleAabbTreeFp for hkAabbTreeAabb: one
// single-ray command and one collector per ray of the bundle, the bundle cast through the tree, then each ray's hit
// count and early-out fraction handed back. The collectors' destructors run over the array in reverse order.
static void castRayBundleAabbTree16(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* aabbTree,
    hkpWorldRayCastBundleCommand* command,
    const hkpCollisionFilter* filter,
    hkVector4* earlyHitFractionInOut
) {
    using Collector16 = hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector;
    if (!aabbTree) {
        return;
    }
    hkAabbTreeData::RayBundle rayBundle;
    setRayBundle(
        &rayBundle, &command->m_rayInput[0].m_from.m_quad, &command->m_rayInput[0].m_to.m_quad,
        &command->m_rayInput[1].m_from.m_quad, &command->m_rayInput[1].m_to.m_quad,
        &command->m_rayInput[2].m_from.m_quad, &command->m_rayInput[2].m_to.m_quad,
        &command->m_rayInput[3].m_from.m_quad, &command->m_rayInput[3].m_to.m_quad, hkVector4Comparison::MASK_XYZW
    );
    hkpWorldRayCastCommand commands[4];
    hkCpuWorldRayCastCollector collectors[4];
    Collector16* collectorPtrs[4];
    for (int32_t i = 0; i < 4; ++i) {
        hkCpuWorldRayCastCollector& collector = collectors[i];

        hkpWorldRayCastCommand& single = commands[i];
        single.m_rayInput.m_from = command->m_rayInput[i].m_from;
        single.m_rayInput.m_to = command->m_rayInput[i].m_to;
        single.m_rayInput.m_enableShapeCollectionFilter = command->m_rayInput[i].m_enableShapeCollectionFilter;
        single.m_rayInput.m_filterInfo = command->m_rayInput[i].m_filterInfo;
        single.m_results = command->m_results[i];
        single.m_resultsCapacity = 1;
        single.m_numResultsOut = 0;
        single.m_useCollector = false;
        single.m_stopAfterFirstHit = command->m_stopAfterFirstHit;

        collector.m_filter = filter;
        collector.m_command = &single;
        collector.m_originalInput = &single.m_rayInput;
        collector.m_result = single.m_results;
        collector.m_nextFreeResult = single.m_results;
        collectorPtrs[i] = &collector;
    }
    float earlyHitFractions[4];
    earlyHitFractions[0] = (*earlyHitFractionInOut)(0);
    earlyHitFractions[1] = (*earlyHitFractionInOut)(1);
    earlyHitFractions[2] = (*earlyHitFractionInOut)(2);
    earlyHitFractions[3] = (*earlyHitFractionInOut)(3);
    rayBundle.m_mask = hkAabbTreeQueryUtilsData::s_numRaysToMask[command->m_numActiveRays];
    castRayBundle(aabbTree, aabbTree->m_rootNode, nullptr, rayBundle, collectorPtrs, earlyHitFractions);
    command->m_numResultsOut[0] = commands[0].m_numResultsOut;
    command->m_numResultsOut[1] = commands[1].m_numResultsOut;
    command->m_numResultsOut[2] = commands[2].m_numResultsOut;
    command->m_numResultsOut[3] = commands[3].m_numResultsOut;
    earlyHitFractionInOut->set(earlyHitFractions[0], earlyHitFractions[1], earlyHitFractions[2], earlyHitFractions[3]);
}

using hkAabbTreeQueryUtils_lhkAabbTreeData_nhkAabbTreeAabb_cunsigned_sshort_g_nCollector =
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector;

using hkAabbTreeQueryUtils_lhkAabbTreeData_nhkAabbTreeAabb16_cunsigned_sshort_g_nCollector =
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector;

static void castRayBundle(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* tree,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* nodePpu,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* escapeNode,
    const hkAabbTreeData::RayBundle& rayBundle,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector** collectors,
    float* earlyHitFractions
) {
    castRayBundleImpl(tree, nodePpu, escapeNode, rayBundle, collectors, earlyHitFractions);
}

static void castRayBundle(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* tree,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* nodePpu,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* escapeNode,
    const hkAabbTreeData::RayBundle& rayBundle,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector** collectors,
    float* earlyHitFractions
) {
    castRayBundleInternal(tree, nodePpu, escapeNode, rayBundle, collectors, earlyHitFractions);
}

// Each pass tests the node's box against the four rays (slab test on the bundle's from points and inverse
// directions); a hit inner node descends to its first child, a hit leaf goes to each collector whose ray hit it and
// lowers that ray's early-out fraction, and a miss or a leaf moves on to the node's skip node until the escape node.
// The fractions live in a local copy: the caller's array is only read.
static void castRayBundleImpl(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* tree,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* nodePpu,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* escapeNode,
    const hkAabbTreeData::RayBundle& rayBundle,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Collector** collectors,
    float* earlyHitFractions
) {
    using Node = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node;
    hkVector4 earlyOut;
    earlyOut.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_load_ss(&earlyHitFractions[0]), _mm_load_ss(&earlyHitFractions[2])),
        _mm_unpacklo_ps(_mm_load_ss(&earlyHitFractions[1]), _mm_load_ss(&earlyHitFractions[3]))
    );
    const Node* node = nodePpu;
    const Node* skipNode;
    do {
        for (;;) {
            // The quantised box back in tree space: each word shifted left by 15 and converted to float, w from the
            // zero float.
            const float fMax0 = static_cast<float>(node->m_volume.m_max[0] << 15);
            const float fMax1 = static_cast<float>(node->m_volume.m_max[1] << 15);
            const float fMax2 = static_cast<float>(node->m_volume.m_max[2] << 15);
            const float fMin0 = static_cast<float>(node->m_volume.m_min[0] << 15);
            const float fMin1 = static_cast<float>(node->m_volume.m_min[1] << 15);
            const float fMin2 = static_cast<float>(node->m_volume.m_min[2] << 15);
            const __m128 maxQuad = _mm_unpacklo_ps(
                _mm_unpacklo_ps(_mm_set_ss(fMax0), _mm_set_ss(fMax2)),
                _mm_unpacklo_ps(_mm_set_ss(fMax1), _mm_load_ss(&zero))
            );
            const __m128 minQuad = _mm_unpacklo_ps(
                _mm_unpacklo_ps(_mm_set_ss(fMin0), _mm_set_ss(fMin2)),
                _mm_unpacklo_ps(_mm_set_ss(fMin1), _mm_load_ss(&zero))
            );
            const __m128 lo = _mm_sub_ps(_mm_mul_ps(minQuad, tree->m_bitScaleInv.m_quad), tree->m_bitOffsetLow.m_quad);
            const __m128 hi = _mm_sub_ps(_mm_mul_ps(tree->m_bitScaleInv.m_quad, maxQuad), tree->m_bitOffsetLow.m_quad);
            // The tree's no-node index (hkAabbTree's clear stores it as the root).
            if (node->m_skipNodeIdx == hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::s_invalidIdx) {
                skipNode = nullptr;
            }
            else {
                skipNode = tree->m_nodeManager.getNode(node->m_skipNodeIdx);
            }
            __m128 loSplat[3];
            __m128 hiSplat[3];
            hiSplat[0] = _mm_shuffle_ps(hi, hi, _MM_SHUFFLE(0, 0, 0, 0));
            hiSplat[1] = _mm_shuffle_ps(hi, hi, _MM_SHUFFLE(1, 1, 1, 1));
            loSplat[0] = _mm_shuffle_ps(lo, lo, _MM_SHUFFLE(0, 0, 0, 0));
            loSplat[1] = _mm_shuffle_ps(lo, lo, _MM_SHUFFLE(1, 1, 1, 1));
            hiSplat[2] = _mm_shuffle_ps(hi, hi, _MM_SHUFFLE(2, 2, 2, 2));
            loSplat[2] = _mm_shuffle_ps(lo, lo, _MM_SHUFFLE(2, 2, 2, 2));
            // Per axis, the four rays' entry and exit fractions of the slab.
            __m128 tEnter[3];
            __m128 tExit[3];
            for (int32_t i = 0; i < 3; ++i) {
                const __m128 from = rayBundle.m_from.m_data[i].m_quad;
                const __m128 invDir = rayBundle.m_invDir.m_data[i].m_quad;
                const __m128 tLo = _mm_mul_ps(_mm_sub_ps(loSplat[i], from), invDir);
                const __m128 tHi = _mm_mul_ps(_mm_sub_ps(hiSplat[i], from), invDir);
                tEnter[i] = _mm_min_ps(tLo, tHi);
                tExit[i] = _mm_max_ps(tLo, tHi);
            }
            const int32_t mask = rayBundle.m_mask;
            const __m128 enter =
                _mm_max_ps(_mm_max_ps(_mm_max_ps(tEnter[0], tEnter[1]), tEnter[2]), hkVector4Zero.m_quad);
            const __m128 exit = _mm_min_ps(_mm_min_ps(_mm_min_ps(tExit[0], tExit[1]), tExit[2]), earlyOut.m_quad);
            const int32_t hits =
                _mm_movemask_ps(_mm_and_ps(_mm_cmplt_ps(enter, exit), hkVector4Comparison_s_maskFromBits[mask]));
            if ((mask & hits) == 0) {
                break;
            }
            if (!node->isLeaf()) {
                node = tree->m_nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
                continue;
            }
            for (int32_t i = 0; i < 4; ++i) {
                if (axisMaskBits_10C92E0[i] & hits) {
                    earlyOut(i) = collectors[i]->addLeaf(node);
                }
            }
            break;
        }
        node = skipNode;
    }
    while (node != escapeNode);
}

// The SDK's RayBundle(from, to, mask) constructor (hkAabbTreeData.inl), with the four rays' points transposed into
// the from and to bundles first: x, y and z of the four rays in m_data[0], [1] and [2].
static hkAabbTreeData::RayBundle* setRayBundle(
    hkAabbTreeData::RayBundle* rayBundle,
    const hkQuadReal* from0,
    const hkQuadReal* to0,
    const hkQuadReal* from1,
    const hkQuadReal* to1,
    const hkQuadReal* from2,
    const hkQuadReal* to2,
    const hkQuadReal* from3,
    const hkQuadReal* to3,
    hkVector4Comparison::Mask mask
) {
    const __m128 from01Lo = _mm_shuffle_ps(*from0, *from1, 0x44);
    const __m128 from01Hi = _mm_shuffle_ps(*from0, *from1, 0xEE);
    const __m128 from23Lo = _mm_shuffle_ps(*from2, *from3, 0x44);
    const __m128 from23Hi = _mm_shuffle_ps(*from2, *from3, 0xEE);
    const __m128 to01Lo = _mm_shuffle_ps(*to0, *to1, 0x44);
    const __m128 to01Hi = _mm_shuffle_ps(*to0, *to1, 0xEE);
    const __m128 to23Lo = _mm_shuffle_ps(*to2, *to3, 0x44);
    const __m128 to23Hi = _mm_shuffle_ps(*to2, *to3, 0xEE);
    rayBundle->m_mask = mask;
    hkAabbTreeData::Bundle to;
    to.m_data[0].m_quad = _mm_shuffle_ps(to01Lo, to23Lo, 0x88);
    to.m_data[1].m_quad = _mm_shuffle_ps(to01Lo, to23Lo, 0xDD);
    to.m_data[2].m_quad = _mm_shuffle_ps(to01Hi, to23Hi, 0x88);
    rayBundle->m_from.m_data[2].m_quad = _mm_shuffle_ps(from01Hi, from23Hi, 0x88);
    rayBundle->m_from.m_data[0].m_quad = _mm_shuffle_ps(from01Lo, from23Lo, 0x88);
    rayBundle->m_from.m_data[1].m_quad = _mm_shuffle_ps(from01Lo, from23Lo, 0xDD);
    hkAabbTreeData::Bundle delta;
    for (int32_t i = 0; i < 3; ++i) {
        delta.m_data[i].m_quad = _mm_sub_ps(to.m_data[i].m_quad, rayBundle->m_from.m_data[i].m_quad);
        const __m128 isZero = _mm_cmpeq_ps(delta.m_data[i].m_quad, hkVector4Zero.m_quad);
        const __m128 maskedDelta =
            _mm_or_ps(_mm_andnot_ps(isZero, delta.m_data[i].m_quad), _mm_and_ps(isZero, hkQuadReal1111));
        // setReciprocal4: the estimate refined by one Newton step, 2r - r*d*r.
        const __m128 estimate = _mm_rcp_ps(maskedDelta);
        const __m128 invDelta =
            _mm_sub_ps(_mm_add_ps(estimate, estimate), _mm_mul_ps(_mm_mul_ps(estimate, maskedDelta), estimate));
        // rdata_10CD670 is the constructor's QuadRealMax_Const.
        rayBundle->m_invDir.m_data[i].m_quad =
            _mm_or_ps(_mm_andnot_ps(isZero, invDelta), _mm_and_ps(isZero, rdata_10CD670));
    }
    return rayBundle;
}

// Each pass tests the node's box against the four rays (slab test on the bundle's from points and inverse
// directions); a hit inner node descends to its first child, a hit leaf goes to each collector whose ray hit it and
// lowers that ray's early-out fraction, and a miss or a leaf moves on to the node's skip node until the escape node.
// The fractions live in a local copy: the caller's array is only read.
static void castRayBundleInternal(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* tree,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* nodePpu,
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* escapeNode,
    const hkAabbTreeData::RayBundle& rayBundle,
    hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Collector** collectors,
    float* earlyHitFractions
) {
    using Tree = hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>;
    hkVector4 earlyOut;
    earlyOut.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_load_ss(&earlyHitFractions[0]), _mm_load_ss(&earlyHitFractions[2])),
        _mm_unpacklo_ps(_mm_load_ss(&earlyHitFractions[1]), _mm_load_ss(&earlyHitFractions[3]))
    );
    const Tree::Node* node = nodePpu;
    const Tree::Node* skipNode;
    do {
        for (;;) {
            const __m128 lo = node->m_volume.m_min.m_quad;
            const __m128 hi = node->m_volume.m_max.m_quad;
            if (node->m_skipNodeIdx == Tree::s_invalidIdx) {
                skipNode = nullptr;
            }
            else {
                skipNode = tree->m_nodeManager.getNode(node->m_skipNodeIdx);
            }
            __m128 loSplat[3];
            __m128 hiSplat[3];
            hiSplat[0] = _mm_shuffle_ps(hi, hi, _MM_SHUFFLE(0, 0, 0, 0));
            hiSplat[1] = _mm_shuffle_ps(hi, hi, _MM_SHUFFLE(1, 1, 1, 1));
            hiSplat[2] = _mm_shuffle_ps(hi, hi, _MM_SHUFFLE(2, 2, 2, 2));
            loSplat[0] = _mm_shuffle_ps(lo, lo, _MM_SHUFFLE(0, 0, 0, 0));
            loSplat[1] = _mm_shuffle_ps(lo, lo, _MM_SHUFFLE(1, 1, 1, 1));
            loSplat[2] = _mm_shuffle_ps(lo, lo, _MM_SHUFFLE(2, 2, 2, 2));
            // Per axis, the four rays' entry and exit fractions of the slab.
            __m128 tEnter[3];
            __m128 tExit[3];
            for (int32_t i = 0; i < 3; ++i) {
                const __m128 from = rayBundle.m_from.m_data[i].m_quad;
                const __m128 invDir = rayBundle.m_invDir.m_data[i].m_quad;
                const __m128 tLo = _mm_mul_ps(_mm_sub_ps(loSplat[i], from), invDir);
                const __m128 tHi = _mm_mul_ps(_mm_sub_ps(hiSplat[i], from), invDir);
                tEnter[i] = _mm_min_ps(tLo, tHi);
                tExit[i] = _mm_max_ps(tLo, tHi);
            }
            const int32_t mask = rayBundle.m_mask;
            const __m128 enter =
                _mm_max_ps(_mm_max_ps(_mm_max_ps(tEnter[0], tEnter[1]), tEnter[2]), hkVector4Zero.m_quad);
            const __m128 exit = _mm_min_ps(_mm_min_ps(_mm_min_ps(tExit[0], tExit[1]), tExit[2]), earlyOut.m_quad);
            const int32_t hits =
                _mm_movemask_ps(_mm_and_ps(_mm_cmplt_ps(enter, exit), hkVector4Comparison_s_maskFromBits[mask]));
            if ((mask & hits) == 0) {
                break;
            }
            if (!node->isLeaf()) {
                node = tree->m_nodeManager.getNode(node->m_nodeData.m_childIdx[0]);
                continue;
            }
            for (int32_t i = 0; i < 4; ++i) {
                if (axisMaskBits_10C92E0[i] & hits) {
                    earlyOut(i) = collectors[i]->addLeaf(node);
                }
            }
            break;
        }
        node = skipNode;
    }
    while (node != escapeNode);
}

hkCpuWorldRayCastCollector::~hkCpuWorldRayCastCollector() {}

// The handle is taken as an hkpTypedBroadPhaseHandle and its owner is the collidable: the handle's address as untyped
// bytes plus its signed byte m_ownerOffset, as void*.
float hkCpuWorldRayCastCollector::addBroadPhaseHandle(const hkpBroadPhaseHandle* broadPhaseHandle, int32_t castIndex) {
    const hkpTypedBroadPhaseHandle* pHandle = static_cast<const hkpTypedBroadPhaseHandle*>(broadPhaseHandle);
    // EXE-INLINED none: hkpTypedBroadPhaseHandle::getOwner
    const void* pOwner = reinterpret_cast<const char*>(pHandle) + pHandle->m_ownerOffset;
    testCollidable(static_cast<const hkpCollidable*>(pOwner));
    return m_hitFraction;
}

// The SDK's primitive id is the collidable's address.
hkBool hkCpuWorldRayCastCollector::addMidPhaseHit(hkPrimitiveId primitiveId) {
    return testCollidable(reinterpret_cast<const hkpCollidable*>(primitiveId));
}

hkBool hkCpuWorldRayCastCollector::hasHit() {
    return m_hit.m_bool;
}

float hkCpuWorldRayCastCollector::getHitFraction() {
    return m_hitFraction;
}

// A leaf of the world's tree holds its collidable in m_leafData; the hit fraction is the result.
float hkCpuWorldRayCastCollector::addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::Node* leaf) {
    testCollidable(static_cast<const hkpCollidable*>(leaf->m_leafData.m_userData));
    return m_hitFraction;
}

// A leaf of the world's tree holds its collidable in m_leafData; the hit fraction is the result.
float hkCpuWorldRayCastCollector::addLeaf(const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* leaf) {
    testCollidable(static_cast<const hkpCollidable*>(leaf->m_leafData.m_userData));
    return m_hitFraction;
}

// Casts the ray, in the collidable's space, into the fixed buffer collector when the command uses one, else into a
// local result that replaces the next free one, or the farthest kept one when the buffer is full and the new hit is
// nearer. When choosing the farthest kept result, NaN keeps the current one; the replacement gives up unless the kept
// one is farther than the hit (NaN gives up). The hit fraction and the replaced one are floats.
hkBool hkCpuWorldRayCastCollector::testCollidable(const hkpCollidable* apCollidable) {
    // EXE-INLINED 0xC7F690
    auto transformedInversePos = [](const hkTransform& t, const hkVector4& v) {
        const __m128 kD = _mm_sub_ps(v.m_quad, t.m_translation.m_quad);
        const __m128 kLo = _mm_unpacklo_ps(t.m_rotation.m_col0.m_quad, t.m_rotation.m_col1.m_quad);
        const __m128 kHi = _mm_unpackhi_ps(t.m_rotation.m_col0.m_quad, t.m_rotation.m_col1.m_quad);
        const __m128 kRow0 = _mm_movelh_ps(kLo, t.m_rotation.m_col2.m_quad);
        const __m128 kRow1 = _mm_shuffle_ps(_mm_movehl_ps(kRow0, kLo), t.m_rotation.m_col2.m_quad, 0xD4);
        const __m128 kRow2 = _mm_shuffle_ps(kHi, t.m_rotation.m_col2.m_quad, 0xE4);
        const __m128 kXY = _mm_add_ps(
            _mm_mul_ps(kRow1, _mm_shuffle_ps(kD, kD, 0x55)), _mm_mul_ps(_mm_shuffle_ps(kD, kD, 0x00), kRow0)
        );
        return _mm_add_ps(kXY, _mm_mul_ps(kRow2, _mm_shuffle_ps(kD, kD, 0xAA)));
    };

    const hkpShape* pShape = apCollidable->m_shape;
    // The result is false at once, the cast's result copied in after the cast.
    hkBool result;
    result.m_bool = false;
    if (!pShape) {
        return result;
    }
    if (!static_cast<const hkpRayCollidableFilter&>(*m_filter)
             .isCollisionEnabled(*m_originalInput, *apCollidable)
             .m_bool) {
        return result;
    }

    const hkTransform& kTransform = *apCollidable->m_motion.pMotion;
    hkpWorldRayCastOutput output;
    output.m_hitFraction = m_hitFraction;
    m_workInput.m_from.m_quad = transformedInversePos(kTransform, m_originalInput->m_from);
    m_workInput.m_to.m_quad = transformedInversePos(kTransform, m_originalInput->m_to);

    if (m_fixedBufferRayHitCollector) {
        pShape->castRayWithCollectorImpl(m_workInput, *apCollidable, *m_fixedBufferRayHitCollector);
        m_command->m_numResultsOut = m_fixedBufferRayHitCollector->m_numOutputs.m_storage;
        m_earlyOutHitFraction.m_quad = _mm_set1_ps(m_fixedBufferRayHitCollector->m_earlyOutHitFraction);
        return result;
    }

    // EXE-INLINED 0x632C30
    const hkBool kHit = pShape->castRayImpl(m_workInput, output);
    result.m_bool = kHit.m_bool;
    if (!kHit.m_bool) {
        return result;
    }
    hkpWorldRayCastCommand* pCommand = m_command;
    const int32_t iNumResults = pCommand->m_numResultsOut;
    hkpWorldRayCastOutput* pDest;
    float fFraction;
    if (iNumResults < pCommand->m_resultsCapacity) {
        pDest = m_nextFreeResult;
        fFraction = 1.0f;
        pCommand->m_numResultsOut = iNumResults + 1;
        m_nextFreeResult++;
    }
    else {
        pDest = m_result;
        for (int32_t i = 1; i < iNumResults; i++) {
            if (pDest->m_hitFraction < m_result[i].m_hitFraction) {
                pDest = &m_result[i];
            }
        }
        if (!(pDest->m_hitFraction > output.m_hitFraction)) {
            return result;
        }
        fFraction = pDest->m_hitFraction;
    }
    if (pDest) {
        // EXE-INLINED none: hkVector4::setRotatedDir (the normal into world space)
        const __m128 kN = output.m_normal.m_quad;
        const __m128 kXY = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(kN, kN, 0x55), kTransform.m_rotation.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(kN, kN, 0x00), kTransform.m_rotation.m_col0.m_quad)
        );
        output.m_normal.m_quad =
            _mm_add_ps(kXY, _mm_mul_ps(_mm_shuffle_ps(kN, kN, 0xAA), kTransform.m_rotation.m_col2.m_quad));
        // EXE-INLINED none: hkpWorldRayCastOutput's implicit copy assignment (a 0x60-byte block copy)
        pDest->m_normal.m_quad = output.m_normal.m_quad;
        pDest->m_hitFraction = output.m_hitFraction;
        pDest->m_extraInfo = output.m_extraInfo;
        pDest->m_pad[0] = output.m_pad[0];
        pDest->m_pad[1] = output.m_pad[1];
        for (int32_t i = 0; i < hkpShapeRayCastOutput::MAX_HIERARCHY_DEPTH; i++) {
            pDest->m_shapeKeys[i] = output.m_shapeKeys[i];
        }
        pDest->m_shapeKeyIndex = output.m_shapeKeyIndex;
        pDest->m_rootCollidable = const_cast<hkpCollidable*>(apCollidable);
        hkpWorldRayCastCommand* pCommandNow = m_command;
        if (pCommandNow->m_stopAfterFirstHit.m_bool) {
            output.m_hitFraction = 0.0f;
        }
        const float fNewFraction = pCommandNow->m_resultsCapacity == 1 ? output.m_hitFraction : fFraction;
        m_hitFraction = fNewFraction;
        m_hit.m_bool = true;
        m_earlyOutHitFraction.m_quad = _mm_set1_ps(m_hitFraction);
    }
    return result;
}

void hkCpuWorldRayCastCollector::operator delete(void* apMemory) {
    if (apMemory) {
        hkMemoryRouter::getInstance().heap().blockFree(apMemory, sizeof(hkCpuWorldRayCastCollector));
    }
}

// With a broadphase, each single command and each ray of each bundle (as a single command of one result, its hit count
// copied back) is cast through it; without one, all single commands go through both AABB trees at once
// (castRayFiltering, four collectors and four fixed buffer collectors) and then the k-d tree, and each bundle through
// the three trees. The job's command arrays are const; the job writes the commands' results through them.
hkJobQueue::JobStatus hkCpuWorldRayCastJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    HK_TIMER_BEGIN("CollQueryWorldRayCast", nullptr);
    // The queue entry holds the job: its hkJob base is the job's.
    hkpWorldRayCastJob& job = static_cast<hkpWorldRayCastJob&>(static_cast<hkJob&>(nextJobOut));
    if (job.m_broadphase) {
        // Neither collector is destroyed, as the game does.
        hkCpuWorldRayCastCollector collector;
        collector.m_filter = job.m_collisionInput->m_filter;
        // No output buffer, capacity 1.
        hkpFixedBufferRayHitCollector fixedBufferHitCollector(nullptr, 1);

        hkpWorldRayCastCommand* command = const_cast<hkpWorldRayCastCommand*>(job.m_commandArray);
        for (int32_t i = 0; i < job.m_numCommands; i++) {
            collector.m_hitFraction = 1.0f;
            collector.m_hit.m_bool = false;
            castRayBroadPhase(job.m_broadphase, command, &collector, &fixedBufferHitCollector);
            command++;
        }

        // Copies the ray input member by member (no padding).
        hkpWorldRayCastBundleCommand* bundle = const_cast<hkpWorldRayCastBundleCommand*>(job.m_bundleCommandArray);
        for (int32_t n = job.m_numBundleCommands; n > 0; n--) {
            for (int32_t r = 0; r < bundle->m_numActiveRays; r++) {
                hkpWorldRayCastCommand rayCommand;
                rayCommand.m_rayInput.m_from.m_quad = bundle->m_rayInput[r].m_from.m_quad;
                rayCommand.m_rayInput.m_to.m_quad = bundle->m_rayInput[r].m_to.m_quad;
                rayCommand.m_rayInput.m_enableShapeCollectionFilter.m_bool =
                    bundle->m_rayInput[r].m_enableShapeCollectionFilter.m_bool;
                rayCommand.m_rayInput.m_filterInfo = bundle->m_rayInput[r].m_filterInfo;
                rayCommand.m_results = bundle->m_results[r];
                rayCommand.m_resultsCapacity = 1;
                rayCommand.m_numResultsOut = 0;
                rayCommand.m_useCollector.m_bool = bundle->m_useCollector.m_bool;
                rayCommand.m_stopAfterFirstHit.m_bool = false;
                collector.m_hitFraction = 1.0f;
                collector.m_hit.m_bool = false;
                castRayBroadPhase(job.m_broadphase, &rayCommand, &collector, &fixedBufferHitCollector);
                bundle->m_numResultsOut[r] = rayCommand.m_numResultsOut;
            }
            bundle++;
        }
    }
    else {
        const hkpCollisionFilter* filter = job.m_collisionInput->m_filter;
        hkpWorldRayCastCommand* commands = const_cast<hkpWorldRayCastCommand*>(job.m_commandArray);
        const int32_t numRays = job.m_numCommands;

        hkVector4 ones;
        // The rays' start points and inverse directions. The block ends where the exe destroys the rays and the
        // collectors, which gives the rays' stack memory back before the k-d tree casts.
        {
            hkLocalArray<hkAabbTreeData::Ray> rays(numRays);
            {
                // EXE-INLINED none: hkArray<hkAabbTreeData::Ray>::setSize(numRays)
                hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                const int32_t capacity =
                    static_cast<int32_t>(rays.m_capacityAndFlags & hkArrayBase<hkAabbTreeData::Ray>::CAPACITY_MASK);
                if (capacity < numRays) {
                    const int32_t doubled = 2 * capacity;
                    hkArrayUtil::_reserve(
                        heap, &rays, numRays < doubled ? doubled : numRays,
                        static_cast<int32_t>(sizeof(hkAabbTreeData::Ray))
                    );
                }
                rays.setSizeUnchecked(numRays);
            }

            // One collector per lane of a ray bundle; castRayFiltering steps through them.
            hkCpuWorldRayCastCollector collectors[4];

            // EXE-INLINED none: hkLocalBuffer<hkpFixedBufferRayHitCollector>::hkLocalBuffer(4) (hkAllocateStack)
            hkStackMemory& bufferStack = hkMemoryRouter::getInstance().m_stack;
            // The fixed buffer collectors' storage is carved out of the stack allocator's untyped free bytes;
            // castRayFiltering constructs each one it uses.
            char* pBufferBlock = bufferStack.m_next.m_storage;
            bufferStack.m_next.m_storage =
                pBufferBlock + ((4 * static_cast<int32_t>(sizeof(hkpFixedBufferRayHitCollector)) + 127) & ~127);
            hkpFixedBufferRayHitCollector* fixedBufferCollectors =
                reinterpret_cast<hkpFixedBufferRayHitCollector*>(pBufferBlock);

            for (int32_t i = 0; i < numRays; i++) {
                // EXE-INLINED none: hkAabbTreeData::Ray::setRay: the direction's zero lanes are 1 for the reciprocal
                // estimate (w too) and the largest float in the Newton-refined inverse direction.
                hkAabbTreeData::Ray& ray = rays.m_data[i];
                ray.m_start.m_quad = commands[i].m_rayInput.m_from.m_quad;
                const __m128 delta =
                    _mm_sub_ps(commands[i].m_rayInput.m_to.m_quad, commands[i].m_rayInput.m_from.m_quad);
                const __m128 isZero = _mm_cmpeq_ps(delta, hkVector4Zero.m_quad);
                __m128 masked = _mm_or_ps(_mm_andnot_ps(isZero, delta), _mm_and_ps(isZero, hkQuadReal1111));
                masked = _mm_shuffle_ps(masked, _mm_unpackhi_ps(masked, hkQuadReal1111), _MM_SHUFFLE(3, 0, 1, 0));
                const __m128 rcp = _mm_rcp_ps(masked);
                ray.m_invDir.m_quad = _mm_or_ps(
                    _mm_andnot_ps(isZero, _mm_sub_ps(_mm_add_ps(rcp, rcp), _mm_mul_ps(_mm_mul_ps(rcp, masked), rcp))),
                    _mm_and_ps(isZero, rdata_10CD670)
                );
            }

            ones.setAll(1.0f);
            for (hkCpuWorldRayCastCollector& c : collectors) {
                c.m_earlyOutHitFraction.m_quad = ones.m_quad;
                c.m_filter = filter;
            }
            hkAabbTreeRayCastUtil<hkAabbTreeData::hkAabbTreeAabb, uint16_t>::castRayFiltering(
                job.m_aabbTreeFp, rays.m_data, commands, filter, collectors, fixedBufferCollectors, numRays
            );
            hkAabbTreeRayCastUtil<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::castRayFiltering(
                job.m_aabbTreeInt16, rays.m_data, commands, filter, collectors, fixedBufferCollectors, numRays
            );

            // EXE-INLINED none: hkLocalBuffer<hkpFixedBufferRayHitCollector>::~hkLocalBuffer (hkDeallocateStack)
            hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pBufferBlock;
        }

        const hkKdTree* kdTree = job.m_kdTree;
        // No output buffer, capacity 1; the collector is deliberately not destroyed, as the game does.
        hkpFixedBufferRayHitCollector fixedBufferHitCollector(nullptr, 1);
        hkpWorldRayCastCommand* command = const_cast<hkpWorldRayCastCommand*>(job.m_commandArray);
        for (int32_t n = job.m_numCommands; n > 0; n--) {
            hkTreeRayCastJobUtil::castRayKdTree(
                kdTree, command, filter, &fixedBufferHitCollector, nullptr, nullptr, nullptr, nullptr
            );
            command++;
        }

        hkpWorldRayCastBundleCommand* bundle = const_cast<hkpWorldRayCastBundleCommand*>(job.m_bundleCommandArray);
        for (int32_t n = job.m_numBundleCommands; n > 0; n--) {
            hkVector4 earlyHitFraction;
            earlyHitFraction.m_quad = ones.m_quad;
            castRayBundleAabbTreeFp(job.m_aabbTreeFp, bundle, filter, &earlyHitFraction);
            castRayBundleAabbTree16(job.m_aabbTreeInt16, bundle, filter, &earlyHitFraction);
            hkTreeRayCastJobUtil::castRayBundleKdTree(
                kdTree, bundle, filter, earlyHitFraction, nullptr, nullptr, nullptr, nullptr, nullptr
            );
            bundle++;
        }
    }
    HK_TIMER_END();
    return jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}

void castRayBroadPhase(
    const hkpBroadPhase* apBroadPhase,
    hkpWorldRayCastCommand* apCommand,
    hkCpuWorldRayCastCollector* apCollector,
    hkpFixedBufferRayHitCollector* apFixedBufferCollector
) {
    hkVector4 v4;
    hkpBroadPhase::hkpCastRayInput v9;
    v4.m_quad = apCommand->m_rayInput.m_from.m_quad;
    apCollector->m_originalInput = &apCommand->m_rayInput;
    v9.m_toBase = &apCommand->m_rayInput.m_to;
    apCollector->m_workInput.m_filterInfo = apCommand->m_rayInput.m_filterInfo;
    apCollector->m_result = apCommand->m_results;
    apCollector->m_nextFreeResult = apCommand->m_results;
    apCollector->m_command = apCommand;
    v9.m_toStriding = 16;
    v9.m_aabbCacheInfo = nullptr;
    v9.m_numCasts = 1;
    v9.m_from.m_quad = v4.m_quad;
    if (apCommand->m_rayInput.m_enableShapeCollectionFilter.m_bool) {
        const hkpCollisionFilter* m_filter = apCollector->m_filter;
        if (m_filter) {
            apCollector->m_workInput.m_rayShapeCollectionFilter =
                static_cast<const hkpRayShapeCollectionFilter*>(m_filter);
        }
        else {
            apCollector->m_workInput.m_rayShapeCollectionFilter = nullptr;
        }
    }
    else {
        apCollector->m_workInput.m_rayShapeCollectionFilter = nullptr;
    }
    if (apCommand->m_useCollector.m_bool) {
        if (apFixedBufferCollector) {
            apFixedBufferCollector->~hkpFixedBufferRayHitCollector();
            ::new (apFixedBufferCollector)
                hkpFixedBufferRayHitCollector(apCommand->m_results, apCommand->m_resultsCapacity);
            apCollector->m_fixedBufferRayHitCollector = apFixedBufferCollector;
        }
        else {
            apCollector->m_fixedBufferRayHitCollector = nullptr;
        }
        g_FixedBufferRayHitCollectorAddRayHitCallbackFunc = &hkpFixedBufferRayHitCollector::addRayHitImplementation;
    }
    else {
        apCollector->m_fixedBufferRayHitCollector = nullptr;
    }
    hkpWorldRayCastOutput* v8 = apCommand->m_results;
    v8->m_hitFraction = 1.0f;
    v8->m_extraInfo = -1;
    v8->m_shapeKeys[0] = -1;
    v8->m_rootCollidable = nullptr;
    v8->m_shapeKeyIndex = 0;
    apBroadPhase->castRay(v9, apCollector, 0);
}
