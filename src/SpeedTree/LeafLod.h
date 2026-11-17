#pragma once

#include "Reconstruction.hpp"

#include <__stddef_ptrdiff_t.h>
#include <__stddef_size_t.h>
#include <memory>
#include <stdint.h>
#include <vector>

class CBillboardLeaf;
class SIdvLeafInfo;

// A list of billboard leaves: one leaf LOD level (speedtree-2 BillboardLeaf.h).
using LeafArray = std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>;

// Builds each coarser leaf LOD level by pairing nearby leaves of the previous level and merging the pairs
// (speedtree-2 LeafLod.h). Members are public here so that their offsets can be pinned.
class CLeafLodEngine {
public:
	// A leaf and the leaf it is to be merged with (speedtree-2: private; nothing in src needs it hidden).
	struct SLodEntry {
		const CBillboardLeaf* m_pLeaf;
		CBillboardLeaf* m_pLeafMatch;
	};

	CLeafLodEngine(const SIdvLeafInfo& sLeafInfo, float fSpacingTolerance, float fLeafReductionPercentage,
		float fSizeIncreaseFactor);
	~CLeafLodEngine();

	// The next coarser level from vOrigLeafSet: FindPairs, then BuildNewLeaves.
	LeafArray* ComputeNextLevel(LeafArray* apResult, LeafArray vOrigLeafSet);
	// One leaf for each kept pair of m_vPairs, between the two.
	LeafArray* BuildNewLeaves(LeafArray* apResult);
	// Pairs each leaf of vOrigLeafSet with the nearest unpaired leaf after it within m_fLeafSpacingTolerance, into
	// m_vPairs (thiscall, ret 0x18: vOrigLeafSet by value, which the function destroys).
	void FindPairs(LeafArray vOrigLeafSet);

	std::vector<SLodEntry, std::allocator<SLodEntry>> m_vPairs;
	float m_fLeafSpacingTolerance;
	float m_fLeafReductionPercentage;
	const SIdvLeafInfo& m_sLeafInfo;
	float m_fLeafSizeIncreaseFactor;
};

// _Unchecked_move_backward of SLodEntry: moves [_First, _Last) to end
// at _Dest through CopyBackwardOpt and returns the destination's start; vector<SLodEntry>::_Insert_n
// calls it. KEEP, unregistered (LeafLod.cpp).
CLeafLodEngine::SLodEntry* CopyBackward(CLeafLodEngine::SLodEntry* _First, CLeafLodEngine::SLodEntry* _Last,
	CLeafLodEngine::SLodEntry* _Dest);
// _Move_backward_opt of SLodEntry: copies [_First, _Last) backward to
// end at _Dest and returns the destination's start. KEEP, unregistered (LeafLod.cpp).
CLeafLodEngine::SLodEntry* CopyBackwardOpt(CLeafLodEngine::SLodEntry* _First, CLeafLodEngine::SLodEntry* _Last,
	CLeafLodEngine::SLodEntry* _Dest);

// The pairs' and the leaves' iterators under one name each. The hybrid spells the VC9 class itself, as the
// registrations below name it.
using SLodEntryConstIterator =
	std::vector<CLeafLodEngine::SLodEntry, std::allocator<CLeafLodEngine::SLodEntry>>::const_iterator;
using LeafArrayConstIterator = LeafArray::const_iterator;
