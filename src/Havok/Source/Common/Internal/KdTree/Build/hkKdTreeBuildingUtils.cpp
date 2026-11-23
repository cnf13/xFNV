#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildingUtils.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildInput.h"
#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuilder.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"

void hkKdTreeBuildingUtils::projectPrimitives(
    hkKdTreeBuildInput::Mediator* mediator,
    int32_t startIdx,
    int32_t endIdx,
    hkKdTreeProjectedEntry* entriesOut
) {
    // Primitive i goes to entriesOut[i]: the output pointer starts at entriesOut + startPrimIdx.
    for (int32_t i = startIdx; i < endIdx; ++i) {
        hkKdTreeProjectedEntry entry;
        hkUlong primId = mediator->getPrimitiveId(i);
        mediator->projectPrimitive(primId, entry);
        entry.setPrimitiveId(primId);
        entriesOut[i] = entry;
    }
}

// A tree without primitives keeps its nodes as they are.
void hkKdTreeBuildingUtils::buildTree(hkKdTreeBuildInput::WorkingSet& set, hkKdTree* tree) {
    if (tree->m_numPrimitives > 0) {
        buildTree(
            set,
            tree->m_nodes.m_data,
            tree->m_projectedEntries.m_data,
            tree->m_nodes.m_size,
            tree->m_numPrimitives,
            tree->m_numReservedEmptyNodes,
            tree->m_maxDepth
        );
    }
}

// KEEP: an uninitialised read. Only the six bound words of the bounds local are written, then the whole entry is
// copied into input.m_bounds, m_primIdHigh and m_primIdLow among them, and input (its padding after m_endPrimIdx and
// after m_writer unwritten too) goes to buildTreeRecursive. The writer's destructor is an empty function: its call at
// the end is not written.
void hkKdTreeBuildingUtils::buildTree(
    hkKdTreeBuildInput::WorkingSet& set,
    hkKdTreeNode* nodes,
    hkKdTreeProjectedEntry* projEntsOut,
    int32_t numNodes,
    int32_t numPrimitives,
    int32_t numAvailableEmptyNodes,
    int32_t& depthOut
) {
    hkKdTreeBuildInput::TreeStream treeOut;
    hkKdTreeProjectedEntry bounds;
    hkKdTreeBuildInput::BuildRecursiveInput input;
    bounds.m_min[0] = 0;
    bounds.m_min[1] = 0;
    bounds.m_min[2] = 0;
    bounds.m_max[0] = 0xFFFF;
    bounds.m_max[1] = 0xFFFF;
    bounds.m_max[2] = 0xFFFF;
    treeOut.m_root = nodes;
    input.m_currentNodeIdx = 0;
    input.m_startPrimIdx = 0;
    input.m_allowedEmptySplits.m_mask = hkVector4Comparison::s_maskFromBits[hkVector4Comparison::MASK_XYZ];
    input.m_buffers = &set;
    int32_t numNodesOut = 1;
    input.m_endPrimIdx = numPrimitives;
    input.m_bounds = bounds;
    input.m_maxNodeIdx = numNodes;
    hkKdTreeBuilderSparseWriter writer;
    input.m_writer = &writer;
    int32_t numEmptyNodes = numAvailableEmptyNodes;
    depthOut = hkKdTreeBuilder<const hkKdTreeProjectedEntry*, hkKdTreeProjectedEntry*, 0>::buildTreeRecursive(
        input,
        &treeOut,
        projEntsOut,
        numNodesOut,
        numEmptyNodes
    );
}

// KEEP: an uninitialised read, as buildTree's: only the six bound words of the bounds local are written before it goes
// to splitTreeBuilding, whose sub-jobs carry the bounds (m_primIdHigh and m_primIdLow unwritten) into each
// input.m_bounds given to buildTreeRecursive, input's padding after m_endPrimIdx and after m_writer unwritten too. The
// two entry arrays are the SDK's hkArray locals of numPrimitives entries. The writer's destructor is an empty
// function: its call is not written.
void hkKdTreeBuildingUtils::buildTreeDistributed(hkKdTreeBuildInput::Mediator& mediator, hkKdTree* tree) {
    hkKdTreeNode* nodes = tree->m_nodes.m_data;
    hkArray<hkKdTreeProjectedEntry> entriesIn(mediator.getNumPrimitives());
    hkArray<hkKdTreeProjectedEntry> entriesOut(mediator.getNumPrimitives());

    hkKdTreeBuildInput::WorkingSet set;
    set.m_entriesIn = entriesIn.m_data;
    set.m_entriesOut = entriesOut.m_data;
    projectPrimitives(&mediator, 0, mediator.getNumPrimitives(), entriesIn.m_data);
    alignas(16) hkKdTreeProjectedEntry bounds;
    bounds.m_min[0] = 0;
    bounds.m_min[1] = 0;
    bounds.m_min[2] = 0;
    bounds.m_max[0] = 0xFFFF;
    bounds.m_max[1] = 0xFFFF;
    bounds.m_max[2] = 0xFFFF;
    if (mediator.getNumPrimitives() < 8) {
        buildTree(
            set,
            tree->m_nodes.m_data,
            tree->m_projectedEntries.m_data,
            tree->m_nodes.m_size,
            mediator.getNumPrimitives(),
            tree->m_numReservedEmptyNodes,
            tree->m_maxDepth
        );
    }
    else {
        // Four sub-jobs, each over its own pair of buffers.
        alignas(16) hkKdTreeBuildInput::WorkingSet subJobBuffers[4];
        hkKdTreeBuildInput::BuildRecursiveInput subJobs[4];
        for (int32_t i = 0; i < 4; ++i) {
            subJobs[i].m_buffers = &subJobBuffers[i];
        }
        alignas(16) int32_t subTreeSizes[4];
        const int32_t treeSize = tree->m_nodes.m_size;
        hkKdTreeProjectedEntryBuilder::splitTreeBuilding(
            set,
            nodes,
            treeSize,
            mediator.getNumPrimitives(),
            subJobs,
            subTreeSizes,
            bounds
        );
        // The reserved empty nodes split evenly, the last sub-job taking the remainder.
        const int32_t numReservedEmptyNodes = tree->m_numReservedEmptyNodes;
        const int32_t numPerJob = numReservedEmptyNodes / 4;
        alignas(16) int32_t numAvailableEmptyNodes[4] =
            {numPerJob, numPerJob, numPerJob, numReservedEmptyNodes - numPerJob - numPerJob - numPerJob};
        alignas(16) int32_t depths[4];
        int32_t nodeOffset = 8;
        for (int32_t i = 0; i < 4; ++i) {
            const hkKdTreeBuildInput::BuildRecursiveInput& subJob = subJobs[i];
            hkKdTreeBuildInput::WorkingSet buffers = *subJob.m_buffers;
            hkKdTreeBuildInput::BuildRecursiveInput input;
            input.m_buffers = &buffers;
            input.m_currentNodeIdx = subJob.m_currentNodeIdx;
            input.m_maxNodeIdx = tree->m_nodes.m_size;
            input.m_startPrimIdx = subJob.m_startPrimIdx;
            input.m_endPrimIdx = subJob.m_endPrimIdx;
            input.m_allowedEmptySplits.m_mask = hkVector4Comparison::s_maskFromBits[hkVector4Comparison::MASK_XYZ];
            input.m_bounds = subJob.m_bounds;
            hkKdTreeBuilderSparseWriter writer;
            input.m_writer = &writer;
            alignas(16) hkKdTreeBuildInput::TreeStream treeOut;
            treeOut.m_root = &nodes[nodeOffset];
            subTreeSizes[i] += numAvailableEmptyNodes[i] + (numAvailableEmptyNodes[i] & 1);
            nodeOffset += subTreeSizes[i];
            int32_t numNodes = 1;
            depths[i] = hkKdTreeProjectedEntryBuilder::buildTreeRecursive(
                            input,
                            &treeOut,
                            tree->m_projectedEntries.m_data,
                            numNodes,
                            numAvailableEmptyNodes[i]
                        ) +
                2;
        }
        completeDistributedBuild(tree, mediator.getNumPrimitives(), depths, subTreeSizes);
    }
}

void hkKdTreeBuildingUtils::allocateBuffers(hkKdTreeBuildInput::WorkingSet& buffersOut, int32_t numPrimitives) {
    buffersOut.m_entriesIn = static_cast<hkKdTreeProjectedEntry*>(
        hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkKdTreeProjectedEntry) * numPrimitives)
    );
    buffersOut.m_entriesOut = static_cast<hkKdTreeProjectedEntry*>(
        hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkKdTreeProjectedEntry) * numPrimitives)
    );
}

void hkKdTreeBuildingUtils::deallocateBuffers(hkKdTreeBuildInput::WorkingSet& buffersOut, int32_t numPrimitives) {
    if (numPrimitives > 0) {
        void* v2 = buffersOut.m_entriesOut;
        hkMemoryRouter::getInstance().m_heap->blockFree(v2, sizeof(hkKdTreeProjectedEntry) * numPrimitives);
        void* v4 = buffersOut.m_entriesIn;
        hkMemoryRouter::getInstance().m_heap->blockFree(v4, sizeof(hkKdTreeProjectedEntry) * numPrimitives);
    }
}

// Moves the roots of the four sub-job trees (nodes 8, 8+s0, 8+s0+s1 and 8+s0+s1+s2, with sN the sub-job sizes) up into
// nodes 4 to 7, an inner root's child offset rebased onto its new place; then sets the tree's depth to the deepest
// sub-job's.
void hkKdTreeBuildingUtils::completeDistributedBuild(
    hkKdTree* treeInOut,
    int32_t numPrimitives,
    int32_t* depths,
    int32_t* subJobSizes
) {
    if (numPrimitives >= 8) {
        hkKdTreeNode* nodes = treeInOut->m_nodes.begin();
        int32_t subRootIdx = 8;
        for (int32_t i = 0; i < 4; ++i) {
            if (i > 0) {
                subRootIdx += subJobSizes[i - 1];
            }
            const hkKdTreeNode* subRoot = &nodes[subRootIdx];
            hkKdTreeNode* newRoot = &nodes[4 + i];
            int32_t type = subRoot->m_data.m_leaf.m_type_primId;
            *newRoot = *subRoot;
            if (type < 0) {
                // The left child sits the byte offset in bits 2 to 30 past the sub-root; its byte offset from the new
                // root, rounded down to whole nodes, replaces it.
                ptrdiff_t byteDelta = (subRoot - newRoot) * static_cast<ptrdiff_t>(sizeof(hkKdTreeNode)) +
                    (type & 0x7FFFFFFC);
                newRoot->m_data.m_node.m_type_left_axis = static_cast<int32_t>(
                    (static_cast<uint32_t>(type) & 0x80000003u) | static_cast<uint32_t>((byteDelta >> 3) << 3)
                );
            }
        }
    }
    if (depths) {
        int32_t maxDepth23 = depths[2];
        if (maxDepth23 <= depths[3]) {
            maxDepth23 = depths[3];
        }
        int32_t maxDepth = depths[0];
        if (depths[0] <= depths[1]) {
            maxDepth = depths[1];
        }
        if (maxDepth <= maxDepth23) {
            maxDepth = maxDepth23;
        }
        treeInOut->m_maxDepth = maxDepth;
    }
}
