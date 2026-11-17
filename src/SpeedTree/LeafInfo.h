#pragma once

#include "Reconstruction.hpp"
#include "SpeedTree/StructsSupport.h"

#include <vector>

class CTreeFileAccess;

// stdext::unchecked_uninitialized_copy of SIdvLeafTexture: copies
// [_First, _Last) into the raw storage at _Dest through _Uninit_copy and returns the end. KEEP,
// unregistered (LeafInfo.cpp).
SIdvLeafTexture* UninitializedCopy(SIdvLeafTexture* _First, SIdvLeafTexture* _Last, SIdvLeafTexture* _Dest,
	std::allocator<SIdvLeafTexture>& _Al);

// The SpeedTree leaf data of a tree species: dimming, collision type, textures, blossom and bud settings and the
// leaf vertex tables.
class SIdvLeafInfo {
public:
	enum ECollisionType : int32_t {
		NONE = 0x0,
		BRANCH = 0x1,
		TREE = 0x2,
	};

	SIdvLeafInfo();
	~SIdvLeafInfo();

	// Builds the rocking groups' time offsets and the leaf vertex and texture coordinate tables for nNumTextures
	// textures.
	void InitTables(int32_t nNumTextures);
	// Reads the general leaf information and the leaf textures from cFile.
	void Parse(CTreeFileAccess& cFile);

	bool m_bDimming;
	uint8_t padding01[3];
	float m_fDimmingScalar;
	float m_fBranchDimmingScalar;
	ECollisionType m_eCollisionType;
	// IDA: void *[6]; the PDB's is a std::vector<SIdvLeafTexture> (0x18 bytes on PC).
	std::vector<SIdvLeafTexture, std::allocator<SIdvLeafTexture>> m_vLeafTextures;
	float m_fSpacingTolerance;
	float m_fBlossomDistance;
	float m_fBlossomWeighting;
	int32_t m_nBlossomLevel;
	float m_fMinimumBudAngle;
	float m_fMaximumBudAngle;
	int32_t m_nNumRockingGroups;
	int32_t m_nNumLeafLodLevels;
	int32_t m_nNumTextures;
	float** m_pLeafVertexes;
	float* m_pLeafTexCoords;
	float* m_pTimeOffsets;
};
