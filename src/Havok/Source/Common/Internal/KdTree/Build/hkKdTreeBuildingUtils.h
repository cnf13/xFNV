#pragma once

#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildInput.h"

#include <cstdint>
#include <stdint.h>

// The kd-tree build steps: no instance data, every member is static.
class hkKdTreeBuildingUtils {
public:
	// Projects the primitives startIdx..endIdx-1 of mediator into the same indices of entriesOut, storing each one's id
	// in the entry.
	static void projectPrimitives(hkKdTreeBuildInput::Mediator* mediator, int32_t startIdx, int32_t endIdx,
		hkKdTreeProjectedEntry* entriesOut);

	// Builds tree's nodes from the projected entries in set, when it has primitives.
	static void buildTree(hkKdTreeBuildInput::WorkingSet& set, hkKdTree* tree);

	// Builds numPrimitives entries of set into the numNodes nodes at nodes and the projected entries at projEntsOut,
	// keeping numAvailableEmptyNodes empty nodes; stores the depth in depthOut.
	static void buildTree(hkKdTreeBuildInput::WorkingSet& set, hkKdTreeNode* nodes, hkKdTreeProjectedEntry* projEntsOut,
		int32_t numNodes, int32_t numPrimitives, int32_t numAvailableEmptyNodes, int32_t& depthOut);

	// Projects the mediator's primitives and builds tree through four sub-jobs of the split builder, or in one build
	// below eight primitives.
	static void buildTreeDistributed(hkKdTreeBuildInput::Mediator& mediator, hkKdTree* tree);
	// Havok SDK signature (hkInternal.lib, tied by bytes,reloc).
	static void allocateBuffers(hkKdTreeBuildInput::WorkingSet& buffersOut, int32_t numPrimitives);
	// Frees buffers' two entry arrays, out first, when numPrimitives is positive.
	static void deallocateBuffers(hkKdTreeBuildInput::WorkingSet& buffersOut, int32_t numPrimitives);
	// Havok SDK signature (hkInternal.lib, tied by bytes,reloc).
	static void completeDistributedBuild(hkKdTree* treeInOut, int32_t numPrimitives, int32_t* depths,
		int32_t* subJobSizes);
};
