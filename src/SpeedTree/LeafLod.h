#pragma once

#include <cstddef>
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
    // A leaf and the leaf it is to be merged with.
    struct SLodEntry {
        const CBillboardLeaf* m_pLeaf;
        CBillboardLeaf* m_pLeafMatch;
    };

    CLeafLodEngine(
        const SIdvLeafInfo& sLeafInfo,
        float fSpacingTolerance,
        float fLeafReductionPercentage,
        float fSizeIncreaseFactor
    );
    ~CLeafLodEngine();

    // The next coarser level from vOrigLeafSet: FindPairs, then BuildNewLeaves.
    LeafArray* ComputeNextLevel(LeafArray* apResult, LeafArray vOrigLeafSet);
    // One leaf for each kept pair of m_vPairs, between the two.
    LeafArray BuildNewLeaves();
    // Pairs each leaf of vOrigLeafSet with the nearest unpaired leaf after it within m_fLeafSpacingTolerance, into
    // m_vPairs.
    void FindPairs(LeafArray vOrigLeafSet);

    std::vector<SLodEntry, std::allocator<SLodEntry>> m_vPairs;
    float m_fLeafSpacingTolerance;
    float m_fLeafReductionPercentage;
    const SIdvLeafInfo& m_sLeafInfo;
    float m_fLeafSizeIncreaseFactor;
};

// Moves [_First, _Last) to end at _Dest through CopyBackwardOpt and returns the destination's start;
// vector<SLodEntry>::_Insert_n calls it.
CLeafLodEngine::SLodEntry*
CopyBackward(CLeafLodEngine::SLodEntry* _First, CLeafLodEngine::SLodEntry* _Last, CLeafLodEngine::SLodEntry* _Dest);
// Copies [_First, _Last) backward to end at _Dest and returns the destination's start.
CLeafLodEngine::SLodEntry*
CopyBackwardOpt(CLeafLodEngine::SLodEntry* _First, CLeafLodEngine::SLodEntry* _Last, CLeafLodEngine::SLodEntry* _Dest);

using SLodEntryConstIterator =
    std::vector<CLeafLodEngine::SLodEntry, std::allocator<CLeafLodEngine::SLodEntry>>::const_iterator;
using LeafArrayConstIterator = LeafArray::const_iterator;
