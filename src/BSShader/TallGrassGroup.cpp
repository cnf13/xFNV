#include "BSShader/TallGrassGroup.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "BSShader/TallGrassTriShape.h"
#include "BSShader/TallGrassTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "SDK/MSVC/string.h"

#include <cmath>

// The instance arrays for the cached geometry's instance count, then the grass geometry with its properties and
// instance index stream, under a new multibound node.
TallGrassGroup::TallGrassGroup(TallGrassShaderProperty::CachedGeometry* apCachedGeometry, int32_t aiCellX,
	int32_t aiCellY, bool abLowDetail)
	: spCachedGeometry(apCachedGeometry), iMaxInstanceCount(static_cast<uint16_t>(apCachedGeometry->iMaxInstanceCount)),
	  iInstanceCount(0), iCellCoord{aiCellX, aiCellY}, bLowDetail(abLowDetail) {
	plTerrainChunkSeed = static_cast<int32_t*>(MemoryManager_Singleton.Allocate(sizeof(int32_t) * iMaxInstanceCount));
	pInstanceData = static_cast<GrassInstanceData*>(MemoryManager_Singleton.Allocate(sizeof(GrassInstanceData) *
		iMaxInstanceCount));
	Clear();
	NiTriBasedGeomData* pGeomData = apCachedGeometry->spTriGeomData.m_pObject;
	if (apCachedGeometry->bTristrip == true) {
		spGrassGeometry = new TallGrassTriStrips(static_cast<NiTriStripsData*>(pGeomData));
	} else {
		spGrassGeometry = new TallGrassTriShape(static_cast<NiTriShapeData*>(pGeomData));
	}
	NiAlphaProperty* pAlpha;
	if (BSShaderManager::eShaderLevel >= 2 && BSShaderManager::bHDR) {
		pAlpha = apCachedGeometry->spAlphaBlend.m_pObject;
	} else {
		pAlpha = apCachedGeometry->spAlphaNoBlend.m_pObject;
	}
	if (pAlpha) {
		spGrassGeometry.m_pObject->AttachProperty(pAlpha);
	}
	NiStencilProperty* pStencil = apCachedGeometry->spStencil.m_pObject;
	if (pStencil) {
		spGrassGeometry.m_pObject->AttachProperty(pStencil);
	}
	TallGrassShaderProperty* pProperty = new TallGrassShaderProperty();
	pProperty->spBaseTexture = apCachedGeometry->spBaseTexture;
	pProperty->pGrassGroup = this;
	// Flag 2: low detail.
	if (abLowDetail) {
		if ((pProperty->ulFlags[0] & 4u) == 0) {
			pProperty->iLastRenderPassState = 0;
		}
		pProperty->ulFlags[0] |= 4u;
	} else {
		if ((pProperty->ulFlags[0] & 4u) != 0) {
			pProperty->iLastRenderPassState = 0;
		}
		pProperty->ulFlags[0] &= ~4u;
	}
	// TGSP_FLAG_BILLBOARD.
	if (apCachedGeometry->bBillboard) {
		if ((pProperty->ulFlags[1] & 0x2000u) == 0) {
			pProperty->iLastRenderPassState = 0;
		}
		pProperty->ulFlags[1] |= 0x2000u;
	} else {
		if ((pProperty->ulFlags[1] & 0x2000u) != 0) {
			pProperty->iLastRenderPassState = 0;
		}
		pProperty->ulFlags[1] &= ~0x2000u;
	}
	spGrassGeometry.m_pObject->AttachProperty(pProperty);
	// Shader 2 (IDA: BSSM_SHADER_TALL_GRASS; no src enum value).
	pProperty->iShader = 2;
	spGrassProperty = pProperty;
	uint32_t uiIndexCount = static_cast<uint32_t>(apCachedGeometry->iVertices * apCachedGeometry->iMaxInstanceCount);
	NiAdditionalGeometryData* pData = new NiAdditionalGeometryData(static_cast<int32_t>(uiIndexCount), 1);
	pData->SetDataBlockCount(1);
	// The instance index floats are handed over as the raw bytes of the data block.
	pData->SetDataBlock(0, reinterpret_cast<uint8_t*>(apCachedGeometry->pInstanceIndex), uiIndexCount * 4, false);
	pData->SetDataStream(0, 0, 0, NiAdditionalGeometryData::AGD_NITYPE_FLOAT1, static_cast<int32_t>(uiIndexCount), 4,
		4);
	spGrassGeometry.m_pObject->m_spModelData.m_pObject->SetAdditionalGeometryData(pData);
	BSShaderManager::PrepareObject(spGrassGeometry.m_pObject, 0, 0);
	spMBGrassNode = new BSMultiBoundNode();
	BSMultiBound* pBound = new BSMultiBound();
	pBound->spShape = new BSMultiBoundAABB();
	spMBGrassNode.m_pObject->spMultiBound = pBound;
	NiNode* pGrassNode = spMBGrassNode.m_pObject;
	pGrassNode->AttachChild(spGrassGeometry.m_pObject, true);
}

// Detaches the geometry and the node, drops the references and frees the instance arrays; the members release
// nothing more.
TallGrassGroup::~TallGrassGroup() {
	spMBGrassNode.m_pObject->DetachChild(spGrassGeometry.m_pObject);
	NiNode* pParent = spMBGrassNode.m_pObject->m_pkParent;
	if (pParent) {
		pParent->DetachChild(spMBGrassNode.m_pObject);
	}
	spGrassGeometry = nullptr;
	spMBGrassNode = nullptr;
	spGrassProperty = nullptr;
	spCachedGeometry = nullptr;
	Clear();
	if (pInstanceData) {
		MemoryManager_Singleton.Deallocate(pInstanceData);
		pInstanceData = nullptr;
	}
	MemoryManager_Singleton.Deallocate(plTerrainChunkSeed);
	plTerrainChunkSeed = nullptr;
}

// The bound of the group's instances: an AABB over their positions (5e9 before the first one, 0 for none), its
// sphere given to the grass geometry and the box to the multibound's shape.
void TallGrassGroup::UpdateGroupBound() {
	float fInit = 5e+09f;
	uint16_t usCount = this->iInstanceCount;
	if (!usCount) {
		fInit = 0.0f;
	}
	// The minimum corner, then the center.
	NiPoint3 kCenter;
	kCenter.x = fInit;
	kCenter.y = fInit;
	kCenter.z = fInit;
	float fNegInit = -fInit;
	float fMaxX = fNegInit;
	float fMaxY = fNegInit;
	float fMaxZ = fNegInit;
	if (usCount) {
		TallGrassGroup::GrassInstanceData* pInstance = this->pInstanceData;
		int32_t iLeft = usCount;
		do {
			float fPos = pInstance->fXPos;
			if (!(fPos > kCenter.x)) {
				kCenter.x = fPos;
			}
			fPos = pInstance->fXPos;
			if (!(fPos < fMaxX)) {
				fMaxX = fPos;
			}
			fPos = pInstance->fYPos;
			if (!(fPos > kCenter.y)) {
				kCenter.y = fPos;
			}
			fPos = pInstance->fYPos;
			if (!(fPos < fMaxY)) {
				fMaxY = fPos;
			}
			fPos = pInstance->fZPos;
			if (!(fPos > kCenter.z)) {
				kCenter.z = fPos;
			}
			fPos = pInstance->fZPos;
			if (!(fPos < fMaxZ)) {
				fMaxZ = fPos;
			}
			++pInstance;
			--iLeft;
		} while (iLeft);
	}
	TallGrassShaderProperty::CachedGeometry* pCached = spCachedGeometry.m_pObject;
	if (!pCached) {
		return;
	}
	NiGeometry* pGeometry = spGrassGeometry.m_pObject;
	if (!pGeometry) {
		return;
	}
	float fSizeX = fMaxX - kCenter.x;
	float fSizeY = fMaxY - kCenter.y;
	float fSizeZ = fMaxZ - kCenter.z;
	// 0.5 is a double constant.
	float fHalfX = fSizeX * 0.5;
	float fHalfY = fSizeY * 0.5;
	float fHalfZ = 0.5 * fSizeZ;
	kCenter.x = kCenter.x + fHalfX;
	kCenter.y = kCenter.y + fHalfY;
	kCenter.z = kCenter.z + fHalfZ;
	fHalfX = fMaxX - kCenter.x;
	fHalfY = fMaxY - kCenter.y;
	fHalfZ = fMaxZ - kCenter.z;
	float fLengthSqr = fHalfY * fHalfY + fHalfX * fHalfX + fHalfZ * fHalfZ;
	float fLength = std::sqrt(static_cast<double>(fLengthSqr));
	NiBound kBound;
	kBound.m_kCenter = kCenter;
	kBound.m_fRadius = fLength + pCached->fBoundRadius;
	// The geometry is a TallGrassTriShape or a TallGrassTriStrips; both have the bound and count setters in the same
	// slots.
	if (pGeometry->IsKindOf(&TallGrassTriShape::ms_RTTI)) {
		TallGrassTriShape* pShape = static_cast<TallGrassTriShape*>(pGeometry);
		pShape->SetLocalBound(kBound);
		pShape->SetInstanceCount(this->iInstanceCount);
	} else if (spGrassGeometry.m_pObject) {
		TallGrassTriStrips* pStrips = static_cast<TallGrassTriStrips*>(spGrassGeometry.m_pObject
				->DynamicCastMember(&TallGrassTriStrips::ms_RTTI));
		if (pStrips) {
			pStrips->SetLocalBound(kBound);
			pStrips->SetInstanceCount(this->iInstanceCount);
		}
	}
	BSMultiBoundShape* pShapeBase = spMBGrassNode.m_pObject->spMultiBound.m_pObject->spShape.m_pObject;
	pShapeBase->SetCenter(kCenter);
	// The group's multibound shape is always the AABB its constructor made.
	BSMultiBoundAABB* pBox = static_cast<BSMultiBoundAABB*>(pShapeBase);
	pBox->HalfExtents.x = fMaxX - kCenter.x;
	pBox->HalfExtents.y = fMaxY - kCenter.y;
	pBox->HalfExtents.z = fMaxZ - kCenter.z;
}

int32_t TallGrassGroup::Remove(uint16_t aiInstanceIndex) {
	uint16_t iInstanceCount = this->iInstanceCount;
	if (aiInstanceIndex < iInstanceCount) {
		if (aiInstanceIndex != iInstanceCount - 1) {
			TallGrassGroup::GrassInstanceData* pInstanceData = this->pInstanceData;
			uint32_t* p_fXPos = reinterpret_cast<uint32_t*>(&pInstanceData[iInstanceCount - 1].fXPos);
			uint32_t* v5 = reinterpret_cast<uint32_t*>(&pInstanceData[aiInstanceIndex].fXPos);
			*v5 = *p_fXPos;
			v5[1] = p_fXPos[1];
			v5[2] = p_fXPos[2];
			v5[3] = p_fXPos[3];
			plTerrainChunkSeed[aiInstanceIndex] = plTerrainChunkSeed[this->iInstanceCount - 1];
		}
		this->pInstanceData[this->iInstanceCount - 1].fXPos = 0.0;
		this->pInstanceData[this->iInstanceCount - 1].fYPos = 0.0;
		this->pInstanceData[this->iInstanceCount - 1].fZPos = -10.0f;
		this->pInstanceData[this->iInstanceCount - 1].fColor = 0.0;
		plTerrainChunkSeed[--this->iInstanceCount] = 0;
		--uiGrassInstanceCount;
	}
	return this->iInstanceCount;
}

void TallGrassGroup::Clear() {
	std::memset(plTerrainChunkSeed, 0, sizeof(int32_t) * iMaxInstanceCount);
	std::memset(pInstanceData, 0, sizeof(TallGrassGroup::GrassInstanceData) * iMaxInstanceCount);
	int32_t v2 = 0;
	int32_t v3 = 0;
	if (iMaxInstanceCount) {
		do {
			pInstanceData[v2].fZPos = (-10.0f);
			++v3;
			++v2;
		} while (v3 < ((int32_t)iMaxInstanceCount));
	}
	uiGrassInstanceCount -= iInstanceCount;
	iInstanceCount = (uint16_t)0;
	UpdateGroupBound();
}

void TallGrassGroup::SetGrassParam(TallGrassShaderProperty::GrassParam* apGrassParam) {
	TallGrassShaderProperty* m_pObject;
	if (apGrassParam) {
		rt::memcpy(&kGrassParam, apGrassParam, sizeof(kGrassParam));
	}
	m_pObject = spGrassProperty.m_pObject;
	if (kGrassParam.bVertexLighting) {
		if ((m_pObject->ulFlags[1] & 1024) == 0) {
			m_pObject->iLastRenderPassState = (uint32_t)0;
		}
		m_pObject->ulFlags[1] |= 0x400u;
	} else {
		if ((m_pObject->ulFlags[1] & 1024) != 0) {
			m_pObject->iLastRenderPassState = (uint32_t)0;
		}
		m_pObject->ulFlags[1] &= (~0x400u);
	}
	TallGrassShaderProperty* v4 = spGrassProperty.m_pObject;
	if (kGrassParam.bUniformScaing) {
		if ((v4->ulFlags[1] & 2048) == 0) {
			v4->iLastRenderPassState = (uint32_t)0;
		}
		v4->ulFlags[1] |= 0x800u;
	} else {
		if ((v4->ulFlags[1] & 2048) != 0) {
			v4->iLastRenderPassState = (uint32_t)0;
		}
		v4->ulFlags[1] &= (~0x800u);
	}
	TallGrassShaderProperty* v5 = spGrassProperty.m_pObject;
	if (kGrassParam.bFitToSlope) {
		if ((v5->ulFlags[1] & 4096) == 0) {
			v5->iLastRenderPassState = (uint32_t)0;
		}
		v5->ulFlags[1] |= 0x1000u;
	} else {
		if ((v5->ulFlags[1] & 4096) != 0) {
			v5->iLastRenderPassState = (uint32_t)0;
		}
		v5->ulFlags[1] &= (~0x1000u);
	}
}

int32_t TallGrassGroup::RemoveTerrainChunk(int32_t alTerrainChunkSeed) {
	int32_t i;
	for (i = iInstanceCount - 1; i >= 0; --i) {
		if (alTerrainChunkSeed == plTerrainChunkSeed[i]) {
			Remove(i);
		}
	}
	return (int32_t)iInstanceCount;
}

bool TallGrassGroup::Add(TallGrassGroup::GrassInstanceData* apInstanceData, int32_t alTerrainChunkSeed) {
	uint16_t iInstanceCount;
	iInstanceCount = this->iInstanceCount;
	if ((iInstanceCount == iMaxInstanceCount) || (!apInstanceData)) {
		return (bool)0;
	}
	pInstanceData[iInstanceCount] = *apInstanceData;
	plTerrainChunkSeed[this->iInstanceCount++] = alTerrainChunkSeed;
	return (bool)1;
}
