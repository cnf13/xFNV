#include "BSShader/SpeedTreeLeafShaderProperty.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SpeedTreeShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <cstring>

NiRTTI SpeedTreeLeafShaderProperty::ms_RTTI("SpeedTreeLeafShaderProperty", &SpeedTreeShaderLightingProperty::ms_RTTI);

STLSPData::STLSPData()
  : pLeafMaps(static_cast<float*>(MemoryManager::Instance().Allocate(192 * sizeof(float)))), fCurveScalar(0.0f),
    fRock(0.0f), fRustle(0.0f) {
    std::memset(pLeafMaps, 0, sizeof(float) * 192);
}

STLSPData::~STLSPData() {
    if (pLeafMaps) {
        MemoryManager::Instance().Deallocate(pLeafMaps);
    }
}

void STLSPData::SetLeafMaps(const float* apLeafMaps, uint32_t aiCount) {
    if (apLeafMaps && pLeafMaps && aiCount) {
        if (aiCount >= 0xC0) {
            aiCount = 0xC0;
        }
        std::memcpy(pLeafMaps, apLeafMaps, aiCount * sizeof(float));
        if (aiCount < 0xC0) {
            std::memset(pLeafMaps + aiCount, 0, (0xC0 - aiCount) * sizeof(float));
        }
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

NiObject* SpeedTreeLeafShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    SpeedTreeLeafShaderProperty* pClone = new SpeedTreeLeafShaderProperty(GetLevel(), GetData(), GetLeafData());
    CopyMembers(pClone, arCloneData);
    return pClone;
}

void SpeedTreeLeafShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    SpeedTreeShaderLightingProperty::GetViewerStrings(apStrings);
}

BSShaderProperty::RenderPassArray* SpeedTreeLeafShaderProperty::GetRenderPasses(
    NiGeometry* apGeom,
    int32_t aiEnabledPasses,
    uint16_t& asCount,
    uint32_t auiRenderMode,
    BSShaderAccumulator* apAccumulator,
    int32_t abUpdateList
) {
    const uint32_t uiState = aiEnabledPasses | (auiRenderMode << 8);
    if (auiRenderMode == 9) {
        return pLocalMapRenderPassList;
    }
    // Compares the enabled passes but stores the combined state.
    if (iLastRenderPassState != aiEnabledPasses || bLightListChanged) {
        CheckCreateRenderPassArray(0xFFFFFFFF);
        pRenderPassList->iSize = 0;
        ShadowSceneLight* pSun = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->pSunLight;
        if (GetNumberofActiveNonShadowLights() == 0) {
            // BSSM_LEAVES_AD
            pRenderPassList->Add(apGeom, BSShaderManager::BSSM_LEAVES_AD, true, 1, pSun, nullptr, nullptr, nullptr);
        }
        else {
            void* pIterator = nullptr;
            ShadowSceneLight* pLight = GetFirstActiveNonShadowLight(pIterator);
            // BSSM_LEAVES_ADPt
            pRenderPassList->Add(apGeom, BSShaderManager::BSSM_LEAVES_ADPt, true, 2, pSun, pLight, nullptr, nullptr);
        }
        iLastRenderPassState = uiState;
    }
    asCount = 1;
    return pRenderPassList;
}

// A new property sharing this one's level, tree data and leaf data, with its base texture.
SpeedTreeLeafShaderProperty* SpeedTreeLeafShaderProperty::CreateCopy() {
    SpeedTreeLeafShaderProperty* pCopy = new SpeedTreeLeafShaderProperty(GetLevel(), GetData(), GetLeafData());
    SpeedTreeShaderLightingProperty* pBase = pCopy;
    pBase->SetBaseTexture(spBaseTexture.m_pObject);
    return pCopy;
}

// One 16-byte float4 stream over the tree's wind info, one entry per vertex. GetVertexCount's whole return value is
// kept and sizes the block.
NiAdditionalGeometryData* SpeedTreeLeafShaderProperty::CreateRendererSpecificProperty(NiGeometry* apGeometry) {
    uint32_t usVertexCount = GetVertexCount();
    NiAdditionalGeometryData* pData = new NiAdditionalGeometryData(usVertexCount, 1);
    pData->SetDataBlockCount(1);
    // The wind info is handed over as raw bytes.
    pData->SetDataBlock(0, reinterpret_cast<uint8_t*>(GetWindInfo()), usVertexCount << 4, false);
    pData->SetDataStream(0, 0, 0, NiAdditionalGeometryData::AGD_NITYPE_FLOAT4, usVertexCount, 0x10, 0x10);
    return pData;
}

float* SpeedTreeLeafShaderProperty::GetLeafMaps() {
    return spLeafData ? spLeafData->pLeafMaps : nullptr;
}

void SpeedTreeLeafShaderProperty::SetLeafMaps(const float* apLeafMaps, uint32_t aiCount) {
    if (spLeafData) {
        spLeafData->SetLeafMaps(apLeafMaps, aiCount);
    }
}

void SpeedTreeLeafShaderProperty::CopyMembers(SpeedTreeLeafShaderProperty* apDest, NiCloningProcess& arCloneData) {
    BSShaderLightingProperty::CopyMembers(apDest, arCloneData);
    apDest->SetBaseTexture(spBaseTexture.m_pObject);
    STLSPData* m_pObject = apDest->spLeafData.m_pObject;
    if (m_pObject == spLeafData.m_pObject) {
        apDest->sLevel = sLevel;
    }
    else {
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
