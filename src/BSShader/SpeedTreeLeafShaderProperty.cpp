#include "BSShader/SpeedTreeLeafShaderProperty.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SpeedTreeShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <cstring>

NiRTTI SpeedTreeLeafShaderProperty::ms_RTTI("SpeedTreeLeafShaderProperty", &SpeedTreeShaderLightingProperty::ms_RTTI);

STLSPData::STLSPData()
	: pLeafMaps(static_cast<float*>(MemoryManager_Singleton.Allocate(192 * sizeof(float)))), fCurveScalar(0.0f),
	  fRock(0.0f), fRustle(0.0f) {
	std::memset(pLeafMaps, 0, sizeof(float) * 192);
}

STLSPData::~STLSPData() {
	if (pLeafMaps) {
		MemoryManager_Singleton.Deallocate(pLeafMaps);
	}
}

void STLSPData::SetLeafMaps(const float* apLeafMaps, uint32_t aiCount) {
	if (apLeafMaps && pLeafMaps && aiCount) {
		if (aiCount >= 0xC0)
			aiCount = 0xC0;
		std::memcpy(pLeafMaps, apLeafMaps, aiCount * sizeof(float));
		if (aiCount < 0xC0)
			std::memset(pLeafMaps + aiCount, 0, (0xC0 - aiCount) * sizeof(float));
	}
}

void STLSPData::SetCurveScalar(float afCurveScalar) {
	fCurveScalar = afCurveScalar;
}

void STLSPData::SetRock(float afRock) {
	fRock = afRock;
}

void STLSPData::SetRustle(float afRustle) {
	fRustle = afRustle;
}

void STLSPData::SetRockSpeed(float afRockSpeed) {
	fRockSpeed = afRockSpeed;
}

void STLSPData::SetRustleSpeed(float afRustleSpeed) {
	fRustleSpeed = afRustleSpeed;
}

SpeedTreeLeafShaderProperty::SpeedTreeLeafShaderProperty(uint16_t asLevel, STSPData* apData, STLSPData* apLeafData)
	: SpeedTreeShaderLightingProperty(apData), spLeafData(apLeafData), sLevel(asLevel) {
	iShader = 6;
	iShaderPropertyType = 6;
}

SpeedTreeLeafShaderProperty::~SpeedTreeLeafShaderProperty() {
	spLeafData = nullptr;
}

// The exe makes the three virtual calls last argument first. Its cleanup-only EH frame is left out.
NiObject* SpeedTreeLeafShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
	SpeedTreeLeafShaderProperty* pClone;
	void* pMemory = NiMemObject::operator new(sizeof(SpeedTreeLeafShaderProperty));
	if (pMemory) {
		STLSPData* pLeafData = GetLeafData();
		STSPData* pData = GetData();
		uint16_t sLevelCopy = GetLevel();
		pClone = ::new (pMemory) SpeedTreeLeafShaderProperty(sLevelCopy, pData, pLeafData);
	} else {
		pClone = nullptr;
	}
	CopyMembers(pClone, arCloneData);
	return pClone;
}

// The exe is a jump to the parent's GetViewerStrings (0xBB3390).
void SpeedTreeLeafShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	SpeedTreeShaderLightingProperty::GetViewerStrings(apStrings);
}

BSShaderProperty::RenderPassArray* SpeedTreeLeafShaderProperty::GetRenderPasses(NiGeometry* apGeom,
	int32_t aiEnabledPasses, uint16_t& asCount, uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator,
	int32_t abUpdateList) {
	const uint32_t uiState = aiEnabledPasses | (auiRenderMode << 8);
	if (auiRenderMode == 9)
		return pLocalMapRenderPassList;
	// The exe compares the enabled passes but stores the combined state.
	if (iLastRenderPassState != aiEnabledPasses || bLightListChanged) {
		CheckCreateRenderPassArray(0xFFFFFFFF);
		pRenderPassList->iSize = 0;
		ShadowSceneLight* pSun = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->pSunLight;
		if (static_cast<uint16_t>(GetNumberofActiveNonShadowLights()) == 0) {
			// BSSM_LEAVES_AD
			pRenderPassList->Add(apGeom, 0x3E, true, 1, pSun, nullptr, nullptr, nullptr);
		} else {
			void* pIterator = nullptr;
			ShadowSceneLight* pLight = GetFirstActiveNonShadowLight(pIterator);
			// BSSM_LEAVES_ADPt
			pRenderPassList->Add(apGeom, 0x3F, true, 2, pSun, pLight, nullptr, nullptr);
		}
		iLastRenderPassState = uiState;
	}
	asCount = 1;
	return pRenderPassList;
}

// A new property sharing this one's level, tree data and leaf data, with its base texture. The exe makes the three
// virtual calls last argument first and calls SetBaseTexture even when the allocation failed. Its cleanup-only EH
// frame is left out.
SpeedTreeLeafShaderProperty* SpeedTreeLeafShaderProperty::CreateCopy() {
	SpeedTreeLeafShaderProperty* pCopy;
	void* pMemory = NiMemObject::operator new(sizeof(SpeedTreeLeafShaderProperty));
	if (pMemory) {
		STLSPData* pLeafData = GetLeafData();
		STSPData* pData = GetData();
		uint16_t sLevelCopy = GetLevel();
		pCopy = ::new (pMemory) SpeedTreeLeafShaderProperty(sLevelCopy, pData, pLeafData);
	} else {
		pCopy = nullptr;
	}
	SpeedTreeShaderLightingProperty* pBase = pCopy;
	pBase->SetBaseTexture(spBaseTexture.m_pObject);
	return pCopy;
}

// One 16-byte float4 stream over the tree's wind info, one entry per vertex. The exe keeps GetVertexCount's whole eax
// and sizes the block from it. Its cleanup-only EH frame is left out.
NiAdditionalGeometryData* SpeedTreeLeafShaderProperty::CreateRendererSpecificProperty(NiGeometry* apGeometry) {
	uint32_t usVertexCount = GetVertexCount();
	void* pMemory = NiMemObject::operator new(sizeof(NiAdditionalGeometryData));
	NiAdditionalGeometryData* pData;
	if (pMemory)
		pData = ::new (pMemory) NiAdditionalGeometryData(usVertexCount, 1);
	else
		pData = nullptr;
	pData->SetDataBlockCount(1);
	// The wind info is handed over as raw bytes, the way the exe passes the block.
	pData->SetDataBlock(0, reinterpret_cast<uint8_t*>(GetWindInfo()), static_cast<uint32_t>(usVertexCount) << 4, false);
	pData->SetDataStream(0, 0, 0, NiAdditionalGeometryData::AGD_NITYPE_FLOAT4, usVertexCount, 0x10, 0x10);
	return pData;
}

float* SpeedTreeLeafShaderProperty::GetLeafMaps() {
	return spLeafData ? spLeafData->pLeafMaps : nullptr;
}

void SpeedTreeLeafShaderProperty::SetLeafMaps(const float* apLeafMaps, uint32_t aiCount) {
	if (spLeafData)
		spLeafData->SetLeafMaps(apLeafMaps, aiCount);
}

void SpeedTreeLeafShaderProperty::CopyMembers(SpeedTreeLeafShaderProperty* apDest, NiCloningProcess& arCloneData) {
	BSShaderLightingProperty::CopyMembers(apDest, arCloneData);
	apDest->SetBaseTexture(spBaseTexture.m_pObject);
	STLSPData* m_pObject = apDest->spLeafData.m_pObject;
	if (m_pObject == spLeafData.m_pObject) {
		apDest->sLevel = sLevel;
	} else {
		if (m_pObject) {
			NiDecRefCount(m_pObject);
		}
		STLSPData* v5 = spLeafData.m_pObject;
		apDest->spLeafData.m_pObject = v5;
		if (v5) {
			NiIncRefCount(v5);
		}
		apDest->sLevel = sLevel;
	}
}
