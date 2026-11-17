#pragma once

#include "SDK/MSVC/ehvec.h"
#include "SpeedTree/SpeedTreeRT.h"

#include <vector>

class CBillboardLeaf;
class SIdvLeafInfo;

class CLeafGeometry {
public:
	// One LOD's leaf data, with the leaf centres before the wind was applied.
	struct SLodGeometry : CSpeedTreeRT::SGeometry::SLeaf {
		SLodGeometry();
		~SLodGeometry();

		// A single LOD and an array of them are both freed through ourFree.
		static void operator delete(void* apMemory);
		static void operator delete[](void* apMemory);

		bool m_bValid;
		float* m_pOrigCenterCoords;
	};

	CLeafGeometry(CWindEngine* pWindEngine);
	~CLeafGeometry();

	// Marks every LOD's leaf data as needing a rebuild.
	void Invalidate();
	// Copies the leaf vertices of each texture and rocking group into the vertex program table and returns it, with
	// its entry count in nEntryCount when both exist.
	const float* GetLeafBillboardTable(uint32_t& nEntryCount);
	void SetTextureCoords(uint32_t nLeafMapIndex, const float* pTexCoords);
	// Bethesda's addition: copies the leaf data of the LOD usLodLevel into sGeometry, marked active, when the leaves
	// are set up.
	void SmallUpdate(CSpeedTreeRT::SGeometry::SLeaf& sGeometry, uint16_t usLodLevel, float fAzimuth, float fPitch,
		float fSizeIncreaseFactor);
	// The full update that GetLeafGeometry runs on every LOD.
	void Update(CSpeedTreeRT::SGeometry::SLeaf& sGeometry, uint16_t usLodLevel, float fAzimuth, float fPitch,
		float fSizeIncreaseFactor);
	// Frees the leaf data of every LOD.
	void FreeLODDataArrays();
	// Moves every LOD's leaf centres by cTransform and scales the leaf dimensions by its first matrix entry.
	void Transform(const CTransform& cTransform);
	// Sets up usNumLeafLods LODs from the leaves pvLeafLods and the leaf info sLeafInfo.
	void Init(uint16_t usNumLeafLods, const std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>* pvLeafLods,
		const SIdvLeafInfo& sLeafInfo);
	// Grows cExtents by every leaf of every LOD.
	void ComputeExtents(CRegion& cExtents) const;

protected:
	// Moves the LOD's leaf centres by their wind matrices, weighted per leaf.
	void ComputeWindEffect(uint16_t nLodLevel);
	// Builds usNumLeafLods LODs of leaf data from the leaves of each LOD in pvLeafLods.
	void InitLods(uint16_t usNumLeafLods,
		const std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>* pvLeafLods);

public:
	bool m_bManualLighting;
	bool m_bVertexWeighting;
	uint8_t padding02[2];
	CWindEngine* m_pWindEngine;
	uint16_t m_usNumRockingGroups;
	uint8_t padding0A[2];
	float* m_pTimeOffsets;
	float** m_pLeafVertexes;
	float* m_pLeafTexCoords;
	float* m_pVertexProgramTable;
	uint16_t m_usNumTextures;
	uint8_t padding1E[2];
	CVec3* m_pLeafDimensions;
	CVec3* m_pLeafOrigins;
	uint16_t m_usNumLods;
	uint8_t padding2A[2];
	SLodGeometry* m_pLods;
};
