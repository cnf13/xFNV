#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildInput.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"

#include <stdint.h>

// The kd-tree builder, a class template over how the projected entries are read and written.
template <typename ProjEntReadPtr, typename ProjEntWritePtr, int32_t Spu> class hkKdTreeBuilder;

// Sets vout to bounds' max minus min, converted to floats per lane.
void hkGetExtentFromBounds(const hkKdTreeProjectedEntry& bounds, hkVector4& vout);
// File-local to hkKdTreeBuilder.cpp: zeroes the 128 bytes at dst, which must be 16-aligned.
void hkString_memClear128Exactly(void* dst);
// hkKdTreeBuilder.cpp's file-local hkGetNumLeadingZero, used by splitPrimitives: the number of leading zero bits of the
// 16-bit value x.
int32_t hkGetNumLeadingZero(uint32_t x);
// base[peIdx] = peToWrite, one 16-byte copy.
void WRITE_PROJ_ENT_WITH_MANAGER(hkKdTreeProjectedEntry* base, const hkKdTreeProjectedEntry& peToWrite, int32_t peIdx);
int32_t sub_DF30A0(uint32_t* a1, int32_t r_eax);
int32_t sub_DF30B0(uint32_t* a1, int32_t r_eax);
int32_t sub_DF30C0(uint32_t* a1, int32_t r_eax);
// Copies an 8-byte element from apSource into slot aiIndex of apElements and returns the element's second 32-bit word.
uint32_t sub_DF30D0(int32_t aiIndex, uint32_t* apElements, const uint32_t* apSource);

// Where a build writes what it produces: BUFFER_SIZE vectors of nodes and projected entries written to main memory
// on flush, and how many of them the buffer holds so far. hkVector4's alignment rounds it up to 0x110. Only the
// layout is declared.
class hkKdTreeBuilderSparseWriter {
public:
	enum : int32_t {
		BUFFER_SIZE = 16,
	};

	hkKdTreeBuilderSparseWriter();

	void putProjEntToMainMemory(hkKdTreeProjectedEntry* base, int32_t peIdx, const hkKdTreeProjectedEntry& peOut);
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	void putNodeToMainMemory(hkKdTreeNode* root, int32_t currentNodeIdx, hkKdTreeNode& thisNode);

protected:
	// Writes what the buffer holds to main memory: nothing to do when the writes go there directly.
	void flush();

public:
	hkVector4 m_writeBuffer[BUFFER_SIZE];
	int32_t m_index;
};

// Whether splitPrimitives found a split, or found every primitive in the same place along each axis.
enum hkSplitPrimitivesStatus : int32_t {
	SPLIT_PRIMITIVES_OK = 0,
	SPLIT_PRIMITIVES_INDISTINGUISHABLE = 1,
};

template <> class hkKdTreeBuilder<const hkKdTreeProjectedEntry*, hkKdTreeProjectedEntry*, 0> {
public:
	// Where splitPrimitives split the primitives: the axis, the first primitive of the right half, the right half's
	// lower bound and the left half's upper bound along the axis.
	struct SplitPrimitivesResults {
		int32_t m_splitAxis;
		int32_t m_splitIndex;
		uint16_t m_splitMin;
		uint16_t m_splitMax;
	};

	// Sorts the primitives startPrimIdx..endPrimIdx-1 of buffersIn into buffersIn's out buffer on either side of
	// the cheapest split (bucketed surface area heuristic), returns the split in results and the swapped buffers
	// in buffersOut.
	static hkSplitPrimitivesStatus splitPrimitives(const hkKdTreeBuildInput::WorkingSet& buffersIn,
		int32_t startPrimIdx, int32_t endPrimIdx, SplitPrimitivesResults& results, int32_t& numAvailableEmptyNodes,
		hkKdTreeBuildInput::WorkingSet& buffersOut, const hkKdTreeProjectedEntry& bounds,
		const hkVector4Comparison& allowedEmptySplits);

	// Builds the subtree of input into treeOut, writing its leaves' primitives to projEntOut; returns its depth.
	static int32_t buildTreeRecursive(const hkKdTreeBuildInput::BuildRecursiveInput& input,
		hkKdTreeBuildInput::TreeStream* treeOut, hkKdTreeProjectedEntry* projEntOut, int32_t& numNodes,
		int32_t& numAvailableEmptyNodes);

	// splitPrimitives for fewer than eight primitives: sorts them exactly.
	static hkSplitPrimitivesStatus splitPrimitivesExactly(const hkKdTreeBuildInput::WorkingSet& buffersIn,
		int32_t startPrimIdx, int32_t endPrimIdx, SplitPrimitivesResults& results,
		hkKdTreeBuildInput::WorkingSet& buffersOut, const hkKdTreeProjectedEntry& bounds);

	// Makes node nodeIdx of treeRoot an empty leaf. Always returns 0.
	static int32_t writeEmptyLeaf(hkKdTreeNode* treeRoot, int32_t nodeIdx, hkKdTreeBuilderSparseWriter* writer);

	// Makes node nodeIdx a leaf holding primitive primIdx of entriesIn, copied to projEntOut.
	static void writeSinglePrimitiveToLeaf(hkKdTreeNode* treeRoot, hkKdTreeProjectedEntry* entriesIn,
		hkKdTreeProjectedEntry* projEntOut, int32_t nodeIdx, int32_t primIdx, hkKdTreeBuilderSparseWriter* writer);

	// Makes node nodeIdx a leaf holding numPrims primitives from primIdx of entriesIn, copied to projEntOut; returns
	// the number of empty nodes it frees.
	static int32_t writeMultiplePrimitivesToLeaf(hkKdTreeNode* treeRoot, const hkKdTreeProjectedEntry* entriesIn,
		hkKdTreeProjectedEntry* projEntOut, int32_t nodeIdx, int32_t primIdx, int32_t numPrims,
		hkKdTreeBuilderSparseWriter* writer);

	// Splits the top of the tree of numPrimitives primitives into four subtree jobs (subJobs, their sizes in
	// subTreeSizes), sampling every first to fifth primitive by numPrimitives above 10, 100, 1000 and 10000.
	static void splitTreeBuilding(const hkKdTreeBuildInput::WorkingSet& buffersIn, hkKdTreeNode* treeOut,
		int32_t treeSize, int32_t numPrimitives, hkKdTreeBuildInput::BuildRecursiveInput* subJobs,
		int32_t subTreeSizes[4], const hkKdTreeProjectedEntry& bounds);
};

using hkKdTreeProjectedEntryBuilder = hkKdTreeBuilder<const hkKdTreeProjectedEntry*, hkKdTreeProjectedEntry*, 0>;
