#include "SpeedTree/LeafLod.h"

#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/exception.h"
#include "SpeedTree/BillboardLeaf.h"
#include "SpeedTree/LeafInfo.h"
#include "SpeedTree/LibRandom_Source/IdvRandom.h"
#include "SpeedTree/StructsSupport.h"
#include "SpeedTree/TreeEngine.h"

#include <memory>
#include <utility>
#include <vector>

using SLodEntryVector = std::vector<CLeafLodEngine::SLodEntry, std::allocator<CLeafLodEngine::SLodEntry>>;
// The found-flags' vector under one name.
using BoolVector = std::vector<bool, std::allocator<bool>>;

static uint32_t uiBuildNewLeavesGuard;

// The leaf information is held by reference; the pairs' vector starts empty.
CLeafLodEngine::CLeafLodEngine(const SIdvLeafInfo& sLeafInfo, float fSpacingTolerance, float fLeafReductionPercentage,
	float fSizeIncreaseFactor)
	: m_fLeafSpacingTolerance(fSpacingTolerance), m_fLeafReductionPercentage(fLeafReductionPercentage),
	  m_sLeafInfo(sLeafInfo), m_fLeafSizeIncreaseFactor(fSizeIncreaseFactor) {}

// The pairs' vector is destroyed by its own destructor.
CLeafLodEngine::~CLeafLodEngine() {}

LeafArray* CLeafLodEngine::ComputeNextLevel(LeafArray* apResult, LeafArray vOrigLeafSet) {
	LeafArray vNextLevel;
	LeafArray vArgument(vOrigLeafSet);
	FindPairs(vArgument);
	FNVD_INLINED_LOCAL(LeafArray, vNewLeaves, std::nullopt, std::nullopt);
	LeafArray* pNewLeaves = BuildNewLeaves(&vNewLeaves);
	// The temporary, raw storage BuildNewLeaves constructed, is destroyed here; the other locals' and the argument's
	// own destructors run at the end of their scope.
	vNextLevel = *pNewLeaves;
	vNewLeaves.~LeafArray();
	::new (apResult) LeafArray(vNextLevel);
	return apResult;
}

// cDice is the function's guarded static. The texture index is the leaf's m_ucTextureIndex halved (signed division by
// 2). The sums and halves are float stores; 0.5 is a double in fAdjust, 0.5f elsewhere. The result is copy-constructed
// at apResult, then vLeaves is destroyed.
LeafArray* CLeafLodEngine::BuildNewLeaves(LeafArray* apResult) {
	static CIdvRandom cDice;
	LeafArray vLeaves;
	SLodEntryVector::iterator i = m_vPairs.begin();
	for (;;) {
		SLodEntryVector::iterator kEnd = m_vPairs.end();
		if (i == kEnd) {
			break;
		}
		if (cDice.GetUniform(0.0f, 1.0f) > m_fLeafReductionPercentage) {
			CBillboardLeaf* pNewLeaf = (*i).m_pLeaf->Clone();
			const CVec3& cMatchPosition = (*i).m_pLeafMatch->m_cPosition;
			const CVec3& cLeafPosition = (*i).m_pLeaf->m_cPosition;
			float fSumX = cMatchPosition.m_afData[0] + cLeafPosition.m_afData[0];
			float fSumY = cMatchPosition.m_afData[1] + cLeafPosition.m_afData[1];
			float fSumZ = cMatchPosition.m_afData[2] + cLeafPosition.m_afData[2];
			CVec3 cNewPosition;
			cNewPosition.m_afData[0] = 0.5f * fSumX;
			cNewPosition.m_afData[1] = 0.5f * fSumY;
			cNewPosition.m_afData[2] = 0.5f * fSumZ;
			int32_t nTextureIndex = static_cast<int32_t>((*i).m_pLeaf->m_ucTextureIndex) / 2;
			const SIdvLeafTexture& sTexture = m_sLeafInfo.m_vLeafTextures[nTextureIndex];
			float fAdjust = static_cast<float>((sTexture.m_cOrigin.m_afData[1] - 0.5) *
				(static_cast<double>(m_fLeafSizeIncreaseFactor) * sTexture.m_cSizeUsed.m_afData[1] * 0.5));
			cNewPosition.m_afData[2] += fAdjust;
			pNewLeaf->m_cPosition = cNewPosition;
			const SLodEntry& sMatchEntry = *i;
			const SLodEntry& sLeafEntry = *i;
			CVec3 cMatchColor = sMatchEntry.m_pLeafMatch->GetColor();
			CVec3 cLeafColor = sLeafEntry.m_pLeaf->GetColor();
			float fColorX = cMatchColor.m_afData[0] + cLeafColor.m_afData[0];
			float fColorY = cMatchColor.m_afData[1] + cLeafColor.m_afData[1];
			float fColorZ = cMatchColor.m_afData[2] + cLeafColor.m_afData[2];
			CVec3 cNewColor;
			cNewColor.m_afData[0] = 0.5f * fColorX;
			cNewColor.m_afData[1] = 0.5f * fColorY;
			cNewColor.m_afData[2] = 0.5f * fColorZ;
			pNewLeaf->SetColor(cNewColor, false);
			vLeaves.push_back(pNewLeaf);
		}
		++i;
	}
	::new (apResult) LeafArray(vLeaves);
	return apResult;
}

// The inner loop tests vFound[nSearchIndex] in its condition and again in its body, as speedtree-2 does. Both distance
// tests are strict and false for NaN. At the end vFound is destroyed, then vOrigLeafSet.
void CLeafLodEngine::FindPairs(LeafArray vOrigLeafSet) {
	BoolVector vFound;
	FNVD_RECONSTRUCT(BoolVector)(&vFound, vOrigLeafSet.size(), false);
	bool bFalse = false;
	BoolVector::iterator kLast = vFound.end();
	BoolVector::iterator kFirst = vFound.begin();
	std::fill(kFirst, kLast, bFalse);
	LeafArrayConstIterator i = vOrigLeafSet.begin();
	for (;;) {
		LeafArray::iterator kEnd = vOrigLeafSet.end();
		if (i == kEnd) {
			break;
		}
		SLodEntry sLod;
		sLod.m_pLeaf = *i;
		sLod.m_pLeafMatch = nullptr;
		float fShortestDistance = 3.4028235e+38f;
		int32_t nPairedIndex = -1;
		LeafArray::iterator kBegin = vOrigLeafSet.begin();
		int32_t nSearchIndex = static_cast<int32_t>(i - kBegin) + 1;
		LeafArrayConstIterator kNext = i;
		kNext += 1;
		LeafArrayConstIterator j = kNext;
		for (;;) {
			LeafArray::iterator kInnerEnd = vOrigLeafSet.end();
			if (j == kInnerEnd) {
				break;
			}
			if (vFound[nSearchIndex]) {
				break;
			}
			if (!vFound[nSearchIndex]) {
				const CVec3& cOtherPosition = (*j)->m_cPosition;
				const CVec3& cPosition = (*i)->m_cPosition;
				float fDistance = cPosition.Distance(cOtherPosition);
				if (fDistance < m_fLeafSpacingTolerance && fDistance < fShortestDistance) {
					fShortestDistance = fDistance;
					sLod.m_pLeafMatch = *j;
					nPairedIndex = nSearchIndex;
				}
			}
			++nSearchIndex;
			++j;
		}
		if (sLod.m_pLeafMatch) {
			vFound[nPairedIndex] = true;
			m_vPairs.push_back(sLod);
		}
		++i;
	}
}

CLeafLodEngine::SLodEntry* CopyBackward(CLeafLodEngine::SLodEntry* _First, CLeafLodEngine::SLodEntry* _Last,
	CLeafLodEngine::SLodEntry* _Dest) {
	return CopyBackwardOpt(_First, _Last, _Dest);
}

// Copies the SLodEntry entries backward to end at _Dest; the result, _Dest less the source's count, is computed before
// the loop.
CLeafLodEngine::SLodEntry* CopyBackwardOpt(CLeafLodEngine::SLodEntry* _First, CLeafLodEngine::SLodEntry* _Last,
	CLeafLodEngine::SLodEntry* _Dest) {
	CLeafLodEngine::SLodEntry* _Result = _Dest - (_Last - _First);
	while (_First != _Last) {
		*--_Dest = *--_Last;
	}
	return _Result;
}
