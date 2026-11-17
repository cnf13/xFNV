#pragma once

#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"
#include "SpeedTree/CIdvFilename.h"
#include "SpeedTree/Camera.h"
#include "SpeedTree/LeafInfo.h"
#include "SpeedTree/LibRandom_Source/IdvRandom.h"
#include "SpeedTree/WindInfo.h"

#include <memory>
#include <vector>

class CBillboardLeaf;
class CIdvBranch;
class CIndexedGeometry;
class CTreeFileAccess;
struct SIdvBranchInfo;

SIdvLeafTexture* Copy(SIdvLeafTexture* a1, SIdvLeafTexture* a2, SIdvLeafTexture* a3);
SIdvLeafTexture* CopyOpt(SIdvLeafTexture* a1, SIdvLeafTexture* a2, SIdvLeafTexture* a3);

// A tree's settings: its branch texture, near and far distances, random seed, size and size variance, and the seed of
// its flares.
class SIdvTreeInfo {
public:
	SIdvTreeInfo();

	CIdvFilename m_strBranchTextureFilename;
	float m_fFar;
	float m_fNear;
	int32_t m_nSeed;
	float m_fSize;
	float m_fSizeVariance;
	int32_t m_nFlareSeed;
};

class CTreeEngine : public CIdvCamera {
public:
	CTreeEngine(CIndexedGeometry* pBranchGeometry);
	~CTreeEngine() override;

	// Reads the tree and branch information from cFile; false on a bad file.
	bool Parse(CTreeFileAccess& cFile);
	// Reads the texture controls block.
	void ParseTextureControls(CTreeFileAccess& cFile);
	// Reads the flare block.
	void ParseFlareInfo(CTreeFileAccess& cFile);
	void GetSize(float& fSize, float& fVariance);
	void SetWind(float fStrength);
	float ComputeLod();
	// Grows the tree's branches and leaves, the leaves scaled up by fLeafSizeIncreaseFactor.
	void Compute(float fLeafSizeIncreaseFactor);
	void ParseFlareSeed(CTreeFileAccess& cFile);
	// 0 picks a random seed, 1 keeps the current one.
	void SetSeed(uint32_t nSeed);
	// Frees the tree's transient data.
	void FreeTransientData();
	// Sets the near and far LOD distances.
	void SetLodLimits(float fNear, float fFar);
	void SetSize(float fSize, float fVariance);
	void ParseTreeInfo(CTreeFileAccess& cFile);
	void BuildBranchLods();
	// Builds each leaf LOD level after the first from the one before it.
	void BuildLeafLods(float fLeafSizeIncreaseFactor);
	// Reads the leaf LOD levels' billboard leaves.
	void ParseLeafCluster(CTreeFileAccess& cFile);
	void ParseLodInfo(CTreeFileAccess& cFile);
	void ParseBranchInfo(CTreeFileAccess& cFile);

	float m_fBillboardSize;
	float m_fLod;
	float m_fOverrideTreeSize;
	float m_fOverrideTreeVariance;
	// The empty CIdvRandom object occupies one byte on PC.
	CIdvRandom m_cRandom;
	bool m_bTransientDataIntact;
	uint8_t padding22[2];
	// PC SIdvTreeInfo occupies 0x34 bytes, four fewer than Xbox's record.
	SIdvTreeInfo m_sTreeInfo;
	// The PC constructor places these pointers at 0x58 and 0x5C, not Xbox's 0x5C and 0x60.
	CIdvBranch* m_pBranch;
	CIndexedGeometry* m_pBranchGeometry;
	// PC STL vectors occupy 0x18 bytes rather than Xbox's 0x14. The exe's FreeTransientData (0xB2B940) deletes each
	// element through its virtual destructor (slot 0) and the element stride is 4: pointers.
	std::vector<SIdvBranchInfo*, std::allocator<SIdvBranchInfo*>> m_vBranchInfo;
	uint32_t m_nNumBranchLodLevels;
	// PC m_vBillboardLeaves and SIdvLeafInfo occupy 0x18 and 0x58 bytes respectively.
	std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>> m_vBillboardLeaves;
	SIdvLeafInfo m_sLeafInfo;
	// The PC pointer m_vLeafLods is at 0xEC, not Xbox's 0xE4: an array of m_nNumLeafLodLevels vectors (stride 0x18,
	// element 4 bytes, deleted virtually in 0xB2B940), allocated by the exe, not a single vector.
	std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>* m_vLeafLods;
	// These fields are eight bytes later on PC than in Xbox's layout.
	bool m_bParsedLeafLods;
	uint8_t paddingF1[3];
	float m_fMinBranchVolumePercent;
	float m_fMaxBranchVolumePercent;
	float m_fLeafReductionPercentage;
	float m_fBranchReductionFuzziness;
	float m_fLargeBranchPercent;
	int32_t m_nBranchesWeightLevel;
	// PC SIdvWindInfo is at 0x10C, eight bytes later than Xbox's 0x104.
	SIdvWindInfo m_sWindInfo;
};

inline char* g_pszHeader = const_cast<char*>("__IdvSpt_02_");
inline constexpr fnvd::Vtable CTreeEngine_vftable{};
