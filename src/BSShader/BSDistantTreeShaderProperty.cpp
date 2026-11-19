#include "BSShader/BSDistantTreeShaderProperty.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "Fallout Shared/Distant Terrain System/BGSDistantTreeBlock.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cmath>

NiRTTI BSDistantTreeShaderProperty::ms_RTTI("BSDistantTreeShaderProperty", &BSShaderProperty::ms_RTTI);

// Shader 8, property type 24, with room for the distant tree block's maximum instance count.
BSDistantTreeShaderProperty::BSDistantTreeShaderProperty()
  : uiInstanceCount(static_cast<uint32_t>(BGSDistantTreeBlock::GetMaxInstanceCount())),
    pInstancesA(
        static_cast<BSDistantTreeShader::InstanceData*>(
            (&MemoryManager_Singleton)->Allocate(sizeof(BSDistantTreeShader::InstanceData) * uiInstanceCount)
        )
    ) {
    iShader = 8;
    iShaderPropertyType = 24;
}

BSDistantTreeShaderProperty::~BSDistantTreeShaderProperty() {
    (&MemoryManager_Singleton)->Deallocate(pInstancesA);
}

NiObject* BSDistantTreeShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    BSDistantTreeShaderProperty* pClone = new BSDistantTreeShaderProperty;
    CopyMembers(pClone, arCloneData);
    return pClone;
}

void BSDistantTreeShaderProperty::LoadBinary(NiStream& kStream) {
    BSShaderProperty::LoadBinary(kStream);
}

void BSDistantTreeShaderProperty::LinkObject(NiStream& kStream) {
    BSShaderProperty::LinkObject(kStream);
}

bool BSDistantTreeShaderProperty::RegisterStreamables(NiStream& kStream) {
    return BSShaderProperty::RegisterStreamables(kStream);
}

void BSDistantTreeShaderProperty::SaveBinary(NiStream& kStream) {
    BSShaderProperty::SaveBinary(kStream);
}

void BSDistantTreeShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderProperty::GetViewerStrings(apStrings);
    // Deliberately titles this class with the no-lighting property's RTTI name, as the game does.
    apStrings->Add(NiGetViewerString(BSShaderNoLightingProperty::ms_RTTI.GetName()));
}

// Skips BSShaderProperty::PostLinkObject and calls NiObjectNET::PostLinkObject directly, as the game does.
void BSDistantTreeShaderProperty::PostLinkObject(NiStream& kStream) {
    NiObjectNET::PostLinkObject(kStream);
}

BSShaderProperty::RenderPassArray* BSDistantTreeShaderProperty::GetRenderPasses(
    NiGeometry* apGeom,
    int32_t aiEnabledPasses,
    uint16_t& asCount,
    uint32_t auiRenderMode,
    BSShaderAccumulator* apAccumulator,
    int32_t abUpdateList
) {
    uint32_t uiState = aiEnabledPasses | (auiRenderMode << 8);
    if (auiRenderMode == 7) {
        return nullptr;
    }
    NiMaterialProperty* pMaterial = static_cast<NiMaterialProperty*>(apGeom->m_PropertyState.m_aspProps[2].m_pObject);
    SetAlpha(pMaterial ? pMaterial->m_fAlpha * fFadeAlpha : fFadeAlpha);
    if (auiRenderMode == 0xF) {
        CheckSilhouetteRenderPassArray(1);
        if (pSilhouettePassList->iSize == 0) {
            pSilhouettePassList->Add(
                apGeom, BSShaderManager::BSSM_DISTANT_TREE_SILHOUETTE, true, 0, nullptr, nullptr, nullptr, nullptr
            );
            pSilhouettePassList->pPassA.m_pBase[0]->eAccumulationHint = BSSP_HINT_NORMAL;
        }
        return pSilhouettePassList;
    }
    if (iLastRenderPassState != uiState) {
        CheckCreateRenderPassArray(1);
        pRenderPassList->iSize = 0;
        pRenderPassList->Add(
            apGeom, BSShaderManager::BSSM_DISTANT_TREE_BASE, true, 0, nullptr, nullptr, nullptr, nullptr
        );
        pRenderPassList->pPassA.m_pBase[0]->eAccumulationHint = BSSP_HINT_GROUP_LOD;
        iLastRenderPassState = uiState;
    }
    return pRenderPassList;
}

// Each instance takes x, y, z, scale, cos and sin of the rotation.
void BSDistantTreeShaderProperty::SetInstanceData(uint32_t auiInstanceID, NiPoint3 arPos, float fRotZ, float fScale) {
    pInstancesA[auiInstanceID].fx = arPos.x;
    pInstancesA[auiInstanceID].fy = arPos.y;
    pInstancesA[auiInstanceID].fz = arPos.z;
    pInstancesA[auiInstanceID].fScale = fScale;
    float fCos = std::cos(fRotZ);
    pInstancesA[auiInstanceID].fzrot1 = fCos;
    float fSin = std::sin(fRotZ);
    pInstancesA[auiInstanceID].fzrot2 = fSin;
}

void BSDistantTreeShaderProperty::CopyMembers(BSDistantTreeShaderProperty* pDest, NiCloningProcess& arCloneData) {
    NiShadeProperty::CopyMembers(pDest, arCloneData);
    pDest->ulFlags[0] = ulFlags[0];
    pDest->ulFlags[1] = ulFlags[1];
    pDest->fAlpha = fAlpha;
    double fFadeAlpha = this->fFadeAlpha;
    pDest->iLastRenderPassState = 0;
    pDest->fFadeAlpha = fFadeAlpha;
    pDest->iShader = iShader;
    pDest->fDepthBias = fDepthBias;
}

NiObject* BSDistantTreeShaderProperty::CreateObject() {
    return new BSDistantTreeShaderProperty;
}
