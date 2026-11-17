#pragma once

#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppSplitter.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// An element on a free list: the freed object's first word links to the next one.
class hkpFreeListElem {
public:
	hkpFreeListElem* m_next;
};

// A list of freed objects of type T (a pointer type), linked through their first word.
template <class T> class hkpFreeList {
public:
	// Pushes elem on the list.
	void addElem(T elem) {
		hkpFreeListElem* freeElem = reinterpret_cast<hkpFreeListElem*>(elem);
		freeElem->m_next = m_elems;
		m_freeElems++;
		m_elems = freeElem;
	}

	// Pops the most recently freed element. While on the list an element's memory is untyped: its first word is
	// the link (as addElem writes it), so the pointer is reinterpreted back to T.
	T getElem() {
		hkpFreeListElem* elem = m_elems;
		m_elems = elem->m_next;
		m_freeElems--;
		return reinterpret_cast<T>(elem);
	}

	int32_t getNumFreeElems() const { return m_freeElems; }

	hkpFreeListElem* m_elems;
	int32_t m_freeElems;
};

// The splitter the MOPP compiler uses: it cuts along a set of plane directions, keeping freed nodes and terminals on
// free lists.
class hkpMoppDefaultSplitter : public hkpMoppSplitter {
public:
	class hkpMoppMaxList;

	// Which side of a cut split() builds (hkpinternal.pdb).
	enum hkpMoppDsSide : int32_t {
		HK_MOPP_DS_LEFT = 0,
		HK_MOPP_DS_RIGHT = 1,
	};

	// A range of primitives the splitter works on and the splits it may still make (hkpinternal.pdb, sizeof 0xC).
	struct hkpMoppPrimitiveArray {
		hkpMoppCompilerPrimitive* m_primitives;
		int32_t m_numPrimitives;
		int32_t m_maxPrimitiveSplits;
	};

	hkpMoppDefaultSplitter();
	~hkpMoppDefaultSplitter() override;

	void releaseNode(hkpMoppTreeNode* nodeToRelease) override;
	int32_t getFreeNodes() override;
	hkpMoppTreeNode* buildTree(hkpMoppMediator* mediator, hkpMoppCostFunction* costFunction,
		hkpMoppAssembler* assembler, const hkpMoppSplitter::hkpMoppSplitParams& in,
		hkpMoppSplitter::hkpMoppScratchArea& temp) override;

	// The PDB's sortLeftAndRight (thiscall, ret 0x24, 0xE0A950). Moves the primitive at current by its position
	// against the node's plane: side 0 and 2 rotate it to the ends of the ranges, 1 keeps it, 3 splits it in two
	// through the mediator. The four cursors are the caller's primitive range ends.
	void sortLeftAndRight(int32_t switchPosition, hkpMoppBasicNode* bestNode, int32_t depth, int32_t& maxSplits,
		int32_t& numSplits, hkpMoppCompilerPrimitive*& beginUnsorted, hkpMoppCompilerPrimitive*& endUnsorted,
		hkpMoppCompilerPrimitive*& beginRight, hkpMoppCompilerPrimitive*& beginMiddle);
	// The order of the node's axes by length, the longest first, and each one's cost against it; the node's own
	// plane's axis counts at 0.66 of its length.
	void resortAxis(const hkpMoppTreeInternalNode* parentNode, int32_t* directionsOut, float* extraCostsOut);
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	hkpMoppTreeTerminal* createTerminal(hkpMoppTreeInternalNode* parentNode,
		const hkpMoppDefaultSplitter::hkpMoppPrimitiveArray& params);
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	hkpMoppTreeInternalNode* createNode();
	// Builds the subtree of the primitives: a terminal for one, else an inner node split recursively.
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	hkpMoppTreeNode* split(hkpMoppTreeInternalNode* parentNode,
		const hkpMoppDefaultSplitter::hkpMoppPrimitiveArray& params, hkpMoppDefaultSplitter::hkpMoppDsSide side,
		int32_t depth);
	// Spreads the middle primitives over the left and right ranges.
	void distributeMiddle(hkpMoppCompilerPrimitive*& startLeft, hkpMoppCompilerPrimitive*& endLeft,
		hkpMoppCompilerPrimitive*& startRight, hkpMoppCompilerPrimitive*& endRight,
		hkpMoppCompilerPrimitive*& startMiddle, hkpMoppCompilerPrimitive*& endMiddle, const hkpMoppBasicNode* bestNode);
	// Sorts the primitives into the node's two sides and gives each its share of the splits.
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	void groupPrimitives(const hkpMoppDefaultSplitter::hkpMoppPrimitiveArray& in, hkpMoppBasicNode* bestNode,
		int32_t depth, hkpMoppDefaultSplitter::hkpMoppPrimitiveArray& leftOut,
		hkpMoppDefaultSplitter::hkpMoppPrimitiveArray& rightOut);
	// Havok SDK signature (hkpInternal.lib, tied by bytes,reloc).
	void calculateMaxMinId(const hkpMoppDefaultSplitter::hkpMoppPrimitiveArray& params, hkpMoppTreeNode* node);
	// The PDB's findSplittingPlanePositions: walks the node's primitives along one plane direction with the cost
	// function (0xE0A6C0, 0xE0A750, 0xE0A850) and the max list, then empties the list (thiscall, ret 0x14, 0xE09BE0).
	// Only caller: split 0xE0A270. depth is stored as hkpPlanesParams::m_currentDepth.
	void findSplittingPlanePositions(hkpMoppBasicNode* node, const hkpMoppSplittingPlaneDirection* plane,
		const hkpMoppDefaultSplitter::hkpMoppPrimitiveArray& primitives, const hkpMoppExtent& extents,
		int32_t treeDepth);

	const hkpMoppSplitter::hkpMoppSplitParams* m_compileParams;
	hkpMoppMaxList* m_maxList;
	hkpFreeList<hkpMoppTreeInternalNode*> m_freeNodes;
	hkpFreeList<hkpMoppTreeTerminal*> m_freeTerminals;
	hkpMoppCostFunction* m_costFunction;
	hkpMoppAssembler* m_assembler;
	hkpMoppMediator* m_mediator;
	hkpMoppTreeInternalNode* m_rootNode;
	int32_t m_numPlaneDirections;
	const hkpMoppSplittingPlaneDirection* m_planeDirections;
	int32_t m_optimalDepth;
};

// The primitives the splitter sorts by their extent's maximum, smallest last, so that those below a plane drop off
// the end. Members are the PDB's; the exe's addElement 0xE09670 and removeElementsLessThan 0xE097A0 read the count at
// +0, the buffer at +8 and store the last removed element at +0xC.
class hkpMoppDefaultSplitter::hkpMoppMaxList {
public:
	// Inserts apElement in order of its extent's maximum, shifting the smaller ones down.
	void addElement(hkpMoppCompilerPrimitive* compPrimitive);
	// Drops the elements from the end whose extent's maximum is below afValue, keeping the
	// last one dropped in lastRemovedElement.
	void removeElementsLessThan(float position);

	int32_t m_currentNumElements;
	int32_t m_sizeOfArray;
	hkpMoppCompilerPrimitive** buffer;
	hkpMoppCompilerPrimitive* lastRemovedElement;
};

inline constexpr fnvd::Vtable hkpMoppDefaultSplitter_vftable{};
