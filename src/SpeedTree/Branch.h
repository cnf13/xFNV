#pragma once

#include "SpeedTree/BillboardLeaf.h"
#include "SpeedTree/LibRandom_Source/IdvRandom.h"
#include "SpeedTree/StructsSupport.h"

#include <algorithm>
#include <memory>
#include <vector>

class CFrondEngine;
class CIdvBranch;
class CIndexedGeometry;
class CLightingEngine;
struct SIdvBranchInfo;
class SIdvLeafInfo;

// A child branch: where on the parent it starts and the branch itself.
struct SIdvBranch {
    int32_t m_nPreVertexIndex;
    float m_fPercent;
    CIdvBranch* m_pBranch;
};

// The vector types of CIdvBranch's statics.
using IntVector = std::vector<int32_t, std::allocator<int32_t>>;
using BranchInfoVector = std::vector<const SIdvBranchInfo*, std::allocator<const SIdvBranchInfo*>>;
using LeafPtrVector = std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>;

class CIdvBranch {
public:
    CIdvBranch(CIdvBranch* pParent);
    ~CIdvBranch();

    void ComputeVolume();
    // Grows the branch of level nLevel from cBasePos: its vertices along the level's splines, its cross sections
    // in pBranchGeometry (or a frond guide), then its child branches, recursively, or its buds.
    void Compute(
        uint32_t nSeed,
        float fSize,
        int32_t nLevel,
        const CVec3& cBasePos,
        float fPercentOfParent,
        float fCombinedPercentOfParent,
        const CRotTransform& cBaseTransform,
        const CVec3& cZAxis,
        CIndexedGeometry* pBranchGeometry,
        LeafPtrVector& vLeaves,
        float fWindWeight,
        int32_t nWindGroup,
        float fParentRadius
    );
    // The wind weight of a vertex at fProgress along a branch of level nLevel: 1 - fProgress * fFlexibility on the
    // weight level, else 1.
    float ComputeVertexWeight(int32_t nLevel, float fProgress, float fFlexibility);
    // Fills the blossom and non-blossom level vectors.
    void BuildBlossomVectors();
    // The flare entries of the branch from sInfo.
    void ComputeFlareEntries(const SIdvBranchInfo& sInfo);
    // The vertex before fChildGrowthPos along the branch and the percent past it, into
    // sBranch.
    void FillBranch(SIdvBranch& sBranch, float fChildGrowthPos) const;
    // Grows a bud, a two-vertex branch, and its leaf, which goes to vLeaves.
    void ComputeBud(
        const CIdvBranch* pParent,
        float fSize,
        int32_t nLevel,
        const CVec3& cBasePos,
        float fPercentOfParent,
        const CRotTransform& cBaseTransform,
        const CVec3& cZAxis,
        LeafPtrVector& vLeaves,
        float fWindWeight,
        int32_t nWindGroup
    ) const;
    // Adds the cross section of pVertex to pBranchGeometry: nSegments + 1 vertices with
    // their normals, texture coordinates and wind.
    void BuildCrossSection(
        const SIdvBranchVertex* pVertex,
        float fBranchProgress,
        float fCombinedBranchProgress,
        int32_t nSegments,
        CIndexedGeometry* pBranchGeometry,
        const CVec3& cTile,
        float fWindWeight,
        int32_t nWindGroup,
        float fTileOffset,
        int32_t nLevel
    ) const;
    // fBaseWeight - fBaseWeight * fFlexibility (Compute's flexibility-propagation weight).
    float ComputeHighLevelVertexWeight(float fBaseWeight, float fFlexibility);
    // Appends this branch and then, depth first, every child branch to vAllBranches.
    void BuildBranchVector(std::vector<CIdvBranch*, std::allocator<CIdvBranch*>>& vAllBranches);

protected:
    // Sets each vertex normal of the branch's usSegments + 1 vertices per cross section: the cross product of the
    // direction around the cross section and the direction along the branch.
    void ComputeBranchNormals(CIndexedGeometry* pBranchGeometry, uint16_t usSegments);

public:
    // Adds the triangle strip of LOD level nDiscreteLodLevel to pGeometry: two indices per cross-section vertex and
    // a degenerate pair between cross sections.
    void ComputeLod(int32_t nDiscreteLodLevel, CIndexedGeometry* pGeometry);
    // Whether a leaf grown fPercentOfParent along pParent is a blossom: past the leaf info's blossom distance and
    // within the blossom weighting by a random draw.
    bool IsBlossom(const CIdvBranch* pParent, float fPercentOfParent) const;
    // Adds a leaf at cPos to vLeaves (and to m_vLocalLeaves under branch collision) when there is room for it: its
    // texture, colour, normal, tangent and binormal.
    void MakeLeaf(
        const CVec3& cPos,
        float fPercent,
        const CIdvBranch* pParent,
        const CVec3& cNormal,
        const CVec3& cParentDir,
        float fWindWeight,
        int32_t nWindGroup,
        LeafPtrVector& vLeaves
    ) const;
    // Whether no leaf of vLeaves lies within the spacing tolerance (times the larger used side of texture
    // nTextureIndex) of cPos.
    bool RoomForLeaf(const CVec3& cPos, int32_t nTextureIndex, LeafPtrVector& vLeaves) const;

    // Sorts vAllBranches by descending fuzzy volume through std::sort.
    static void SortBranchVector(std::vector<CIdvBranch*, std::allocator<CIdvBranch*>>& vAllBranches);

    CIdvBranch* m_pParent;
    float m_fPercentOfParent;
    // BuildBranchVector divides the PC vector's byte span by 0xC.
    std::vector<SIdvBranch, std::allocator<SIdvBranch>> m_vChildren;
    SIdvBranchVertex* m_pVertices;
    int32_t m_nVertexCount;
    uint16_t m_usCrossSectionSegments;
    uint8_t padding2A[2];
    int32_t m_nStartingOffset;
    float m_fVolume;
    float m_fFuzzyVolume;
    // The PC m_vFlares vector likewise occupies 0x18 bytes.
    std::vector<SIdvBranchFlare, std::allocator<SIdvBranchFlare>> m_vFlares;

    static IntVector m_vBlossoms;
    static IntVector m_vNonBlossoms;
    static BranchInfoVector m_vBranchInfo;
    static inline bool m_bComputeLeaves = true;
    static inline SIdvLeafInfo* m_pLeafInfo;
    static LeafPtrVector m_vLocalLeaves;
    static inline CLightingEngine* m_pLightingEngine;
    static CIdvRandom m_cRandom;
    static inline int32_t m_nWeightLevel = 1;
    static inline int32_t m_nRoundRobinWindLevel;
    static inline CFrondEngine* m_pFrondEngine;
    // LoadTree's token 22000 writes it and Compute reads it.
    static inline bool m_bPropagateFlexibility;
};

// Orders branches by descending fuzzy volume: the comparator SortBranchVector hands to std::sort. A stateless functor
// (one byte). operator() is defined in Branch.cpp.
class CBranchVolumeSorter {
public:
    bool operator()(const CIdvBranch* pLeft, const CIdvBranch* pRight) const;

    // The empty functor's one byte. The algorithm instances pass it on by value, and an empty class has no byte to
    // copy, so it is modelled as a field.
    uint8_t unk0;
};

// The point fPercent of the way from cStart to cEnd. Defined in Branch.cpp.
CVec3 VecInterpolate(const CVec3& cStart, const CVec3& cEnd, float fPercent);
// A uniform random number between fMin and fMax, from the CRT's rand.
float GetRandom(float fMin, float fMax);
// _Unchecked_move_backward over CVec3, used by vector<CVec3>::_Insert_n.
CVec3* UncheckedCopyBackward(CVec3* _First, CVec3* _Last, CVec3* _Dest);
SIdvBranchFlare* UncheckedCopyBackward(SIdvBranchFlare* _First, SIdvBranchFlare* _Last, SIdvBranchFlare* _Dest);
// Copies the 24-byte elements of [_First, _Last) backward to end at
// _Dest and returns the destination's start; UncheckedCopyBackward calls it.
SIdvBranchFlare* CopyBackwardOpt(SIdvBranchFlare* _First, SIdvBranchFlare* _Last, SIdvBranchFlare* _Dest);
// The percent of the tree pBranch grows at: 0 for the trunk, else its parent's percent plus its own share of the rest.
float ComputeDimPercent(const CIdvBranch* pBranch);
// std::sort over two checked _Vector_iterator<CIdvBranch*> passed by value (each its container proxy, then its pointer)
// and the comparator by value. a1, a3: the iterators' _Myownedaux proxies; a2, a4: their pointers; _Pred: the empty
// functor, passed on to _Sort.
void sort(void* a1, CIdvBranch** a2, void* a3, CIdvBranch** a4, CBranchVolumeSorter _Pred);
// The copy _Umove of the branch flares: _Uninit_copy<SIdvBranchFlare>;
// vector<SIdvBranchFlare>::_Insert_n's copies pass flare pointers and the vector's allocator.
SIdvBranchFlare* UninitializedCopy(
    SIdvBranchFlare* _First,
    SIdvBranchFlare* _Last,
    SIdvBranchFlare* _Dest,
    std::allocator<SIdvBranchFlare>& _Al
);
// Called by the branch sort's _Insertion_sort1.
CIdvBranch** CopyBackwardOpt(CIdvBranch** _First, CIdvBranch** _Last, CIdvBranch** _Dest);
// Copies [_First, _Last) to end at _Dest, last element first; returns the start of the copy.
CVec3* CopyBackwardOpt(const CVec3* _First, const CVec3* _Last, CVec3* _Dest);

// Branch.cpp's constants Compute reads (speedtree-2's g_cOut and g_cDown), set by their dynamic initializers.
extern const CVec3 g_cOut;
