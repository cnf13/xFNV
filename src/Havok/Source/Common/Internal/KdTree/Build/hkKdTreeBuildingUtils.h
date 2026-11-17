#pragma once

#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildInput.h"
#include "Reconstruction.hpp"

#include <cstdint>
#include <stdint.h>

// The kd-tree build steps: no instance data, every member is static.
class hkKdTreeBuildingUtils {
public:
	// Projects the primitives startPrimIdx..endPrimIdx-1 of mediator into the same indices of entriesOut, storing
	// each one's id in the entry (cdecl, 0xD9DAB0). Parameter names chosen here: the
	// PDB's are a different build's.
	static void projectPrimitives(hkKdTreeBuildInput::Mediator* mediator, int32_t startIdx, int32_t endIdx,
		hkKdTreeProjectedEntry* entriesOut);

	// Builds tree's nodes from the projected entries in set, when it has primitives.
	static void buildTree(hkKdTreeBuildInput::WorkingSet& set, hkKdTree* tree);

	// Builds numPrimitives entries of set into the numNodes nodes at nodes and the projected entries at projEntsOut,
	// keeping numAvailableEmptyNodes empty nodes; stores the depth in *depthOut (cdecl, 0xD9D880). The parameter
	// names are chosen here: the PDB's are a different build's.
	static void buildTree(hkKdTreeBuildInput::WorkingSet& set, hkKdTreeNode* nodes, hkKdTreeProjectedEntry* projEntsOut,
		int32_t numNodes, int32_t numPrimitives, int32_t numAvailableEmptyNodes, int32_t& depthOut);

	// The PDB's buildTreeDistributed: projects the mediator's primitives and builds tree through four sub-jobs of
	// the split builder (0xDF37B0), or in one build below eight primitives; the database has no caller (cdecl,
	// ret 0, 0xD9DB60).
	static void buildTreeDistributed(hkKdTreeBuildInput::Mediator& mediator, hkKdTree* tree);
	// Havok SDK signature (hkInternal.lib, tied by bytes,reloc).
	static void allocateBuffers(hkKdTreeBuildInput::WorkingSet& buffersOut, int32_t numPrimitives);
	// Frees buffers' two entry arrays, out first, when numPrimitives is positive (the SDK's static
	// hkKdTreeBuildingUtils::deallocateBuffers(WorkingSet&, int); cdecl, 0xD9D840).
	static void deallocateBuffers(hkKdTreeBuildInput::WorkingSet& buffersOut, int32_t numPrimitives);
	// Havok SDK signature (hkInternal.lib, tied by bytes,reloc).
	static void completeDistributedBuild(hkKdTree* treeInOut, int32_t numPrimitives, int32_t* depths,
		int32_t* subJobSizes);
};
