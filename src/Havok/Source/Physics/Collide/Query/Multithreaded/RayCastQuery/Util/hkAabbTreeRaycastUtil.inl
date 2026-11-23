#pragma once

#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpFixedBufferRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuWorldRaycastJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Util/hkAabbTreeRaycastUtil.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"

#include <new>

class hkpCollisionFilter;

void registerDefaultAddRayHitFunction();

// The generic bodies of hkAabbTreeRayCastUtil. Included and instantiated once, by hkAabbTreeRaycastUtil.cpp.

// A leaf goes to the first collector of every ray whose slab test reaches it; an inner node either splits the rays
// between its two children (when both output lanes and the task stack have room and there are more than four rays) or
// casts them, four at a time, from node to skipNode through castRayBundle. For hkAabbTreeAabb16 each node box is
// restored to world space with restoreAabb before the slab test; hkAabbTreeAabb boxes are tested as they are.
template <typename BOUNDING_VOLUME, typename hkIdxType>
void hkAabbTreeRayCastUtil<BOUNDING_VOLUME, hkIdxType>::filterRays(
    const Tree* tree,
    const Tree::Node* nodeSpu,
    const Tree::Node* skipNode,
    const hkAabbTreeData::Ray* rays,
    hkCpuWorldRayCastCollector* collectors,
    hkpFixedBufferRayHitCollector* fixedBufferCollectors,
    hkpWorldRayCastCommand* commandArray,
    const hkpCollisionFilter* filter,
    float* earlyHitFractionArray,
    int32_t* raysIdx,
    int32_t numRays,
    int32_t taskCap,
    int32_t raysIdxOutCap0,
    int32_t raysIdxOutCap1,
    int32_t* raysIdxOut0,
    int32_t* numRaysOut0,
    int32_t* raysIdxOut1,
    int32_t* numRaysOut1,
    Tree::Node* child0Out,
    Tree::Node* child1Out
) {
    using QueryUtils = hkAabbTreeQueryUtils<BOUNDING_VOLUME, hkIdxType>;
    using Collector = QueryUtils::Collector;

    *numRaysOut1 = 0;
    *numRaysOut0 = 0;
    if (nodeSpu->isLeaf()) {
        hkAabbTreeData::hkAabbTreeAabb volume;
        BOUNDING_VOLUME::restoreAabb(nodeSpu->m_volume, tree->m_bitScaleInv, tree->m_bitOffsetLow, volume);
        hkVector4 one;
        one.setAll(1.0f);
        for (int32_t i = 0; i < numRays; i++) {
            const int32_t rayIdx = raysIdx[i];
            if (volume.overlaps(rays[rayIdx], one)) {
                setCollector(collectors, &commandArray[rayIdx], filter);
                // The collector's Collector base for this tree; the returned fraction is dropped.
                Collector& treeCollector = *collectors;
                treeCollector.addLeaf(nodeSpu);
            }
        }
    }
    else if (raysIdxOutCap0 >= numRays && raysIdxOutCap1 >= numRays && taskCap >= 2 && numRays > 4) {
        *child0Out = *tree->m_nodeManager.getNode(nodeSpu->m_nodeData.m_childIdx[0]);
        *child1Out = *tree->m_nodeManager.getNode(nodeSpu->m_nodeData.m_childIdx[1]);
        hkAabbTreeData::hkAabbTreeAabb volume0;
        BOUNDING_VOLUME::restoreAabb(child0Out->m_volume, tree->m_bitScaleInv, tree->m_bitOffsetLow, volume0);
        hkAabbTreeData::hkAabbTreeAabb volume1;
        BOUNDING_VOLUME::restoreAabb(child1Out->m_volume, tree->m_bitScaleInv, tree->m_bitOffsetLow, volume1);
        hkVector4 one;
        one.setAll(1.0f);
        for (int32_t i = 0; i < numRays; i++) {
            const int32_t rayIdx = raysIdx[i];
            if (volume0.overlaps(rays[rayIdx], one)) {
                *raysIdxOut0++ = rayIdx;
                ++*numRaysOut0;
            }
            if (volume1.overlaps(rays[rayIdx], one)) {
                *raysIdxOut1++ = rayIdx;
                ++*numRaysOut1;
            }
        }
    }
    else {
        const int32_t numBundles = (numRays + 3) >> 2;
        int32_t rayIdx[4] = {0, 0, 0, 0};
        // Lanes past a bundle's ray count keep the previous bundle's values. castRayBundle loads all four fractions;
        // the lanes the bundle's mask leaves out are never used, but the first bundle's would otherwise be unwritten,
        // so they start at 1 here.
        float earlyHitFractions[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        int32_t* bundleRaysIdx = raysIdx;
        int32_t numRaysLeft = numRays;
        for (int32_t bundle = 0; bundle < numBundles; bundle++) {
            const int32_t numBundleRays = numRaysLeft < 4 ? numRaysLeft : 4;
            for (int32_t i = 0; i < numBundleRays; i++) {
                rayIdx[i] = bundleRaysIdx[i];
                hkpWorldRayCastCommand* command = &commandArray[rayIdx[i]];
                setCollector(&collectors[i], command, filter);
                collectors[i].m_earlyOutHitFraction.setAll(earlyHitFractionArray[rayIdx[i]]);
                earlyHitFractions[i] = earlyHitFractionArray[rayIdx[i]];
                if (command->m_useCollector.m_bool) {
                    // The SDK's hkpFixedBufferRayHitCollector(command->m_results, command->m_resultsCapacity).
                    hkpFixedBufferRayHitCollector* fixedBufferCollector = new (&fixedBufferCollectors[i])
                        hkpFixedBufferRayHitCollector();
                    fixedBufferCollector->m_earlyOutHitFraction = 1.0f;
                    fixedBufferCollector->m_capacity = command->m_resultsCapacity;
                    fixedBufferCollector->m_rayCastOutputBase = command->m_results;
                    fixedBufferCollector->m_nextFreeOutput = command->m_results;
                    fixedBufferCollector->m_numOutputs = 0;
                    fixedBufferCollector->m_collidableOnPpu = nullptr;
                    collectors[i].m_fixedBufferRayHitCollector = fixedBufferCollector;
                    registerDefaultAddRayHitFunction();
                }
                else {
                    collectors[i].m_fixedBufferRayHitCollector = nullptr;
                }
            }
            // Lanes past numBundleRays keep the previous bundle's ray indices (0 in the first). The mask selects the
            // first numBundleRays lanes (MASK_NONE, X, XY, XYZ, XYZW).
            const hkVector4Comparison::Mask mask = static_cast<hkVector4Comparison::Mask>((1 << numBundleRays) - 1);
            hkAabbTreeData::RayBundle rayBundle;
            bundleRaysWithInvDir(
                &rayBundle,
                rays[rayIdx[0]].m_start,
                rays[rayIdx[0]].m_invDir,
                rays[rayIdx[1]].m_start,
                rays[rayIdx[1]].m_invDir,
                rays[rayIdx[2]].m_start,
                rays[rayIdx[2]].m_invDir,
                rays[rayIdx[3]].m_start,
                rays[rayIdx[3]].m_invDir,
                mask
            );
            Collector* treeCollectors[4] = {&collectors[0], &collectors[1], &collectors[2], &collectors[3]};
            QueryUtils::castRayBundle(tree, nodeSpu, skipNode, rayBundle, treeCollectors, earlyHitFractions);
            for (int32_t i = 0; i < numBundleRays; i++) {
                earlyHitFractionArray[rayIdx[i]] = collectors[i].m_earlyOutHitFraction(0);
            }
            bundleRaysIdx += 4;
            numRaysLeft -= 4;
        }
    }
}

// A depth-first walk by a stack of tasks, each a node with the rays still active in it, whose indices are the last ones
// of one of two lanes; filterRays casts a task's rays at its node and hands back the rays and copies of the children,
// which become the next tasks: the first child's rays in lane 0, skipping to the second child past it, the second
// child's in lane 1, skipping to the node's own skip node. Every ray's early hit fraction starts at 1.
template <typename BOUNDING_VOLUME, typename hkIdxType>
void hkAabbTreeRayCastUtil<BOUNDING_VOLUME, hkIdxType>::castRayFiltering(
    const Tree* tree,
    const hkAabbTreeData::Ray* rays,
    hkpWorldRayCastCommand* commandArray,
    const hkpCollisionFilter* filter,
    hkCpuWorldRayCastCollector* collectors,
    hkpFixedBufferRayHitCollector* fixedBufferCollectors,
    int32_t numRays
) {
    using TreeTask = Task<BOUNDING_VOLUME, hkIdxType>;
    using Node = Tree::Node;
    if (!tree) {
        return;
    }

    hkLocalArray<float> earlyHitFractions(numRays);
    earlyHitFractions.setSizeUnchecked(numRays);
    for (int32_t i = 0; i < numRays; ++i) {
        earlyHitFractions.m_data[i] = 1.0f;
    }

    int32_t raysIdx[2][ACTIVESTACK_MAX];
    int32_t numRaysIdx[2] = {0, 0};

    // The task stack's capacity is TASK_MAX and filterRays splits only while taskCap leaves room: no growth test.
    hkLocalArray<TreeTask> tasks(TASK_MAX);
    TreeTask* rootTask = tasks.expandByUnchecked(1);
    rootTask->m_root = *tree->m_rootNode;
    rootTask->m_skipNode = nullptr;
    rootTask->m_activeRayLane = 0;
    rootTask->m_numActiveRays = numRays;
    for (int32_t i = 0; i < numRays; ++i) {
        raysIdx[0][numRaysIdx[0]++] = i;
    }

    int32_t taskCap = TASK_MAX - 1;
    do {
        // Pop the top task; its rays are the last ones of its lane.
        const TreeTask task = tasks.m_data[tasks.getSize() - 1];
        tasks.popBack();
        const Node node = task.m_root;
        ++taskCap;
        Node child0Out;
        child0Out.m_parentIdx = 0;
        Node child1Out;
        child1Out.m_parentIdx = 0;
        numRaysIdx[task.m_activeRayLane] -= task.m_numActiveRays;
        int32_t numRaysOut0;
        int32_t numRaysOut1;
        filterRays(
            tree,
            &node,
            task.m_skipNode,
            rays,
            collectors,
            fixedBufferCollectors,
            commandArray,
            filter,
            earlyHitFractions.m_data,
            &raysIdx[task.m_activeRayLane][numRaysIdx[task.m_activeRayLane]],
            task.m_numActiveRays,
            taskCap,
            ACTIVESTACK_MAX - numRaysIdx[0],
            ACTIVESTACK_MAX - numRaysIdx[1],
            &raysIdx[0][numRaysIdx[0]],
            &numRaysOut0,
            &raysIdx[1][numRaysIdx[1]],
            &numRaysOut1,
            &child0Out,
            &child1Out
        );
        numRaysIdx[0] += numRaysOut0;
        numRaysIdx[1] += numRaysOut1;

        if (numRaysOut0) {
            --taskCap;
            TreeTask* child = tasks.expandByUnchecked(1);
            child->m_root = child0Out;
            child->m_skipNode = tree->m_nodeManager.getNode(node.m_nodeData.m_childIdx[1]);
            child->m_activeRayLane = 0;
            child->m_numActiveRays = numRaysOut0;
        }
        if (numRaysOut1) {
            --taskCap;
            TreeTask* child = tasks.expandByUnchecked(1);
            child->m_root = child1Out;
            child->m_skipNode = (node.m_skipNodeIdx == Tree::s_invalidIdx)
                ? nullptr
                : tree->m_nodeManager.getNode(node.m_skipNodeIdx);
            child->m_activeRayLane = 1;
            child->m_numActiveRays = numRaysOut1;
        }
    }
    while (tasks.getSize() != 0);
}
