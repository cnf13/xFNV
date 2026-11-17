#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildInput.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The kd-tree builder, a class template over how the projected entries are read and written. The exe has only the
// instance below; the PDB's third argument spells its writer type hkKdTreeProjectedEntry* __restrict.
template <typename ProjEntReadPtr, typename ProjEntWritePtr, int32_t Spu> class hkKdTreeBuilder;

// hkKdTreeBuilder.cpp's file-local hkGetExtentFromBounds (PDB name; the address is matched
// by its use in splitPrimitives): extentOut = bounds' max minus min, converted to floats per lane (usercall:
// bounds in eax, extentOut in ecx; 0xDF3170).
void hkGetExtentFromBounds(const hkKdTreeProjectedEntry& bounds, hkVector4& vout);
// hkKdTreeBuilder.cpp's file-local hkString_memClear128Exactly (PDB name; matched by its
// use in splitPrimitives): zeroes the 128 bytes at dst, 16-aligned (usercall: dst in eax; 0xDF3210).
void hkString_memClear128Exactly(void* dst);
// hkKdTreeBuilder.cpp's file-local hkGetNumLeadingZero (PDB name; matched by its use in
// splitPrimitives): the number of leading zero bits of the 16-bit value x (cdecl, 0xDF3250).
int32_t hkGetNumLeadingZero(uint32_t x);
// hkKdTreeBuilder.cpp's WRITE_PROJ_ENT_WITH_MANAGER (PDB name): base[peIdx] = peToWrite, one
// 16-byte copy (usercall: peToWrite in ecx, peIdx in eax, base on the stack, the caller pops; 0xDF30F0).
void WRITE_PROJ_ENT_WITH_MANAGER(hkKdTreeProjectedEntry* base, const hkKdTreeProjectedEntry& peToWrite, int32_t peIdx);
int32_t sub_DF30A0(uint32_t* a1, int32_t r_eax);
int32_t sub_DF30B0(uint32_t* a1, int32_t r_eax);
int32_t sub_DF30C0(uint32_t* a1, int32_t r_eax);
// Not named in the PDB or IDA; name chosen here. Copies an 8-byte element (two dwords) into slot aiIndex of an array
// (usercall: aiIndex in ecx, apSource in eax, apElements on the stack; ret 0; eax the second dword on return).
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

	// Makes node nodeIdx of nodes an empty leaf (cdecl, 0xDF32A0). The exe returns with eax 0; the PDB's return
	// type is void.
	static int32_t writeEmptyLeaf(hkKdTreeNode* treeRoot, int32_t nodeIdx, hkKdTreeBuilderSparseWriter* writer);

	// Makes node nodeIdx a leaf holding primitive primIdx of entriesIn, copied to
	// projEntOut (cdecl, 0xDF32D0). Parameter names chosen here: the PDB's are garbled.
	static void writeSinglePrimitiveToLeaf(hkKdTreeNode* treeRoot, hkKdTreeProjectedEntry* entriesIn,
		hkKdTreeProjectedEntry* projEntOut, int32_t nodeIdx, int32_t primIdx, hkKdTreeBuilderSparseWriter* writer);

	// Makes node nodeIdx a leaf holding numPrims primitives from primIdx of entriesIn,
	// copied to projEntOut; returns the number of empty nodes it frees (cdecl, 0xDF3330). Parameter names chosen
	// here: the PDB's are garbled.
	static int32_t writeMultiplePrimitivesToLeaf(hkKdTreeNode* treeRoot, const hkKdTreeProjectedEntry* entriesIn,
		hkKdTreeProjectedEntry* projEntOut, int32_t nodeIdx, int32_t primIdx, int32_t numPrims,
		hkKdTreeBuilderSparseWriter* writer);

	// The PDB's and the SDK's splitTreeBuilding: splits the top of the tree of numPrimitives primitives into four
	// subtree jobs (subJobs, their sizes in subTreeSizes), sampling every first to fifth primitive by numPrimitives
	// above 10, 100, 1000 and 10000 (cdecl, ret 0, 0xDF37B0; callers 0xD9DB60 and hkCpuKdTreeBuildFastDistributedJob
	// 0xDF7E20).
	static void splitTreeBuilding(const hkKdTreeBuildInput::WorkingSet& buffersIn, hkKdTreeNode* treeOut,
		int32_t treeSize, int32_t numPrimitives, hkKdTreeBuildInput::BuildRecursiveInput* subJobs,
		int32_t subTreeSizes[4], const hkKdTreeProjectedEntry& bounds);
};

// Not named in the PDB or IDA; name chosen here. The exe's one instance of the builder.
using hkKdTreeProjectedEntryBuilder = hkKdTreeBuilder<const hkKdTreeProjectedEntry*, hkKdTreeProjectedEntry*, 0>;
