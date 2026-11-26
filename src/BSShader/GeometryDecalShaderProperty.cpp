#include "BSShader/GeometryDecalShaderProperty.h"

#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SkyShaderProperty.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cmath>

NiRTTI GeometryDecalShaderProperty::ms_RTTI("GeometryDecalShaderProperty", &BSShaderLightingProperty::ms_RTTI);

GeometryDecalShaderProperty::GeometryDecalShaderProperty() : spDecalTexture(nullptr) {
    iShader = 16;
    iShaderPropertyType = 3;
}

// The decal texture releases itself.
GeometryDecalShaderProperty::~GeometryDecalShaderProperty() {}

// The clone is built in an engine block by the class's constructor; the copy body fills in the members.
NiObject* GeometryDecalShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    GeometryDecalShaderProperty* pClone = new GeometryDecalShaderProperty;
    CopyMembers(pClone, arCloneData);
    return pClone;
}

void GeometryDecalShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderLightingProperty::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(SkyShaderProperty::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v6 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v6);
}

BSShaderProperty::RenderPassArray* GeometryDecalShaderProperty::GetRenderPasses(
    NiGeometry* apGeom,
    int32_t aiEnabledPasses,
    uint16_t& asCount,
    uint32_t auiRenderMode,
    BSShaderAccumulator* apAccumulator,
    int32_t abUpdateList
) {
    double v17;
    if (auiRenderMode == 7) {
        return nullptr;
    }
    float foldCameraDistance = fCameraDistance;
    BSNiBound* m_kWorldBound = apGeom->m_kWorldBound;
    char v11 = 1;
    if (!m_kWorldBound) {
        m_kWorldBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
    }
    float v23 = m_kWorldBound->m_kCenter.x - BSShaderManager::pShadowSceneNode[0]->kEyePosition.x;
    float v24 = m_kWorldBound->m_kCenter.y - BSShaderManager::pShadowSceneNode[0]->kEyePosition.y;
    float v25 = m_kWorldBound->m_kCenter.z - BSShaderManager::pShadowSceneNode[0]->kEyePosition.z;
    float fDistanceSqr = v24 * v24 + v23 * v23 + v25 * v25;
    float fDistance = std::sqrt(static_cast<double>(fDistanceSqr));
    float fNewCameraDistance = fDistance - static_cast<NiBound&>(*m_kWorldBound).m_fRadius;
    double v12 = fNewCameraDistance;
    fCameraDistance = fNewCameraDistance;
    double v13 = foldCameraDistance;
    if (foldCameraDistance == fNewCameraDistance) {
        v17 = BSShaderManager::fSkinnedDecalLODEnd;
    }
    else {
        double v14 = BSShaderManager::fSkinnedDecalLODEnd;
        if (BSShaderManager::fSkinnedDecalLODEnd > v12 && v14 <= v13) {
            iLastRenderPassState = 0;
            v17 = BSShaderManager::fSkinnedDecalLODEnd;
        }
        else {
            if (!(v14 <= v12)) {
                v17 = BSShaderManager::fSkinnedDecalLODEnd;
            }
            else {
                bool v15 = v14 < v13;
                bool v16 = v14 == v13;
                v17 = BSShaderManager::fSkinnedDecalLODEnd;
                if (!v15 && !v16) {
                    iLastRenderPassState = 0;
                    v17 = BSShaderManager::fSkinnedDecalLODEnd;
                }
            }
        }
    }
    // The address &apGeom->m_PropertyState is never null.
    NiMaterialProperty* m_pObject = static_cast<NiMaterialProperty*>(apGeom->m_PropertyState.m_aspProps[2].m_pObject);
    float fnewAlpha = 1.0f;
    if (m_pObject) {
        fnewAlpha = m_pObject->m_fAlpha;
        if (BSShaderManager::fDecalLODEnd == 0.0f || !(BSShaderManager::fDecalLODEnd <= v12)) {
            if (BSShaderManager::fSkinnedDecalLODStartFade != 0.0f &&
                (0.0 != v17 && (BSShaderManager::fSkinnedDecalLODStartFade <= v12 && v12 < v17))) {
                fnewAlpha = FLerp(0.0f, 1.0f, v17, BSShaderManager::fSkinnedDecalLODStartFade, v12);
            }
        }
        else {
            v11 = 0;
        }
    }
    SetAlpha(fnewAlpha);
    if (iLastRenderPassState != aiEnabledPasses) {
        CheckCreateRenderPassArray(1);
        this->pRenderPassList->iSize = 0;
        if (!v11) {
            return nullptr;
        }
        BSShaderProperty::RenderPassArray* pRenderPassArray = this->pRenderPassList;
        if (apGeom->m_spSkinInstance.m_pObject) {
            pRenderPassArray->Add(apGeom, BSShaderManager::BSSM_GEOMDECAL_S, 1, 0, nullptr, nullptr, nullptr, nullptr);
        }
        else {
            pRenderPassArray->Add(apGeom, BSShaderManager::BSSM_GEOMDECAL, 1, 0, nullptr, nullptr, nullptr, nullptr);
        }
        iLastRenderPassState = aiEnabledPasses | auiRenderMode << 8;
    }
    return this->pRenderPassList;
}

void GeometryDecalShaderProperty::CopyMembers(GeometryDecalShaderProperty* pDest, NiCloningProcess& arCloneData) {
    BSShaderProperty::CopyMembers(pDest, arCloneData);
    CopyToMembers(pDest);
}
