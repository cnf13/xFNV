#include "BSShader/ShadowSceneLight.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSBound.h"
#include "BSMain/BSCullingProcess.h"
#include "BSShader/BSCubeMapCamera.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/DebugShader.h"
#include "BSShader/ImageSpaceEffectBlur.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/Lighting30Shader.h"
#include "BSShader/MTRenderingSystem.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAmbientLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"

#include <cmath>

ShadowSceneLight::ShadowSceneLight()
  : fLuminance(0.0f), fLODDimmer(0.0f), fShadowLODDimmer(0.0f), fShadowLODDimmerTarget(1.0f),
    fShadowLODDimmerElapsed(0.0f), bCastShadow(0), bCubeShadowMap(false), spSpecularAttenuationTexture(nullptr),
    bPointLight(false), bAmbientLight(false), spLight(nullptr), bDynamicLight(false), bPointPosition(NiPoint3::ZERO),
    spDynamicShadowMap(nullptr), usFrustumCull(0), spFrustumVis(nullptr), bShadowSpotlight(false), fSpotlightFOV(90.0f),
    fSpotlightFalloff(1.0f), bShowDebugTexture(0), spShadowCaster(nullptr), kGeomListFence(nullptr),
    kGeomListFenceObject(nullptr), spShadowCamera(nullptr), bClipSpacePlanesReady(false), spShadowAccumulator(nullptr) {
    std::memset(&kViewProjection, 0, sizeof(kViewProjection));
    kViewProjection._11 = 1.0f;
    kViewProjection._22 = 1.0f;
    kViewProjection._33 = 1.0f;
    kViewProjection._44 = 1.0f;
    std::memset(pfClipSpacePlanes, 0, sizeof(pfClipSpacePlanes));
}

ShadowSceneLight::~ShadowSceneLight() {
    RemoveAllTriShapes();
    if (spSpecularAttenuationTexture) {
        spSpecularAttenuationTexture->DecRefCount();
        spSpecularAttenuationTexture = nullptr;
    }
    spLight = nullptr;
    if (spDynamicShadowMap.m_pObject) {
        BSShaderManager::pTextureManager->ReturnFrustumShadowTexture(spDynamicShadowMap.m_pObject);
    }
    spDynamicShadowMap = nullptr;
    usFrustumCull = 0;
    if (spFrustumVis) {
        spFrustumVis->DecRefCount();
        spFrustumVis = nullptr;
    }
    spShadowCamera = nullptr;
    spShadowCaster = nullptr;
    spShadowAccumulator = nullptr;
    kGeomListFence = nullptr;
    kGeomListFenceObject = nullptr;
}

float ShadowSceneLight::GetLuminanceAtPoint(NiPoint3 apPoint, NiLight* apIgnore) {
    NiLight* m_pObject = spLight.m_pObject;
    // Compares the byte with 1, as IsShadowLight does.
    if (m_pObject == apIgnore || bCastShadow == 1) {
        return 0.0;
    }

    bool v4 = !bPointLight;
    float apIgnorec = m_pObject->m_fDimmer;
    float y = m_pObject->m_kWorld.m_Translate.y;
    float kLightPos = m_pObject->m_kWorld.m_Translate.x;
    float kDimmedColor = apIgnorec * m_pObject->m_kDiff.r;
    float z = m_pObject->m_kWorld.m_Translate.z;
    float kDimmedColor_4 = apIgnorec * m_pObject->m_kDiff.g;
    float kDimmedColor_8 = apIgnorec * m_pObject->m_kDiff.b;
    float fRadius = m_pObject->m_kSpec.r;
    fLuminance = 1.0;
    if (!v4) {
        double v6;
        double v7;
        float kPosDiff = apPoint.x - kLightPos;
        float kPosDiff_4 = apPoint.y - y;
        float kPosDiff_8 = apPoint.z - z;
        float apIgnored = kPosDiff_4 * kPosDiff_4 + kPosDiff * kPosDiff + kPosDiff_8 * kPosDiff_8;
        float apIgnoree = std::sqrt(static_cast<double>(apIgnored));
        float apIgnoref = apIgnoree / fRadius;
        double v5 = apIgnoref;
        if (apIgnoref < 0.0) {
            v5 = 0.0;
        }
        float apIgnorea = v5;
        if (!(apIgnorea > 1.0)) {
            v7 = apIgnorea;
            v6 = 1.0;
        }
        else {
            v6 = 1.0;
            v7 = 1.0;
        }
        float apIgnoreg = v7;
        fLuminance = v6 - apIgnoreg * apIgnoreg;
    }
    double v8 = kDimmedColor_4;
    if (!(kDimmedColor_8 < static_cast<double>(kDimmedColor_4))) {
        v8 = kDimmedColor_8;
    }
    float apIgnoreb = v8;
    double v9 = kDimmedColor;
    if (!(apIgnoreb < static_cast<double>(kDimmedColor))) {
        v9 = apIgnoreb;
    }
    float apIgnoreh = v9;
    float apIgnorei = apIgnoreh * fLuminance;
    fLuminance = apIgnorei;
    return apIgnorei;
}

void ShadowSceneLight::SetShadowCaster(NiNode* apNode) {
    NiNode* pOldCaster = spShadowCaster.m_pObject;
    if (pOldCaster == apNode) {
        return;
    }

    if (pOldCaster) {
        NiDecRefCount(pOldCaster);
    }
    spShadowCaster.m_pObject = apNode;
    if (apNode) {
        NiIncRefCount(apNode);
    }
}

// The object-type callback's result is unused: bit 0 of the shadow mask is tested whatever it returns. The shader type
// compares are signed.
bool ShadowSceneLight::Display(NiCullingProcess& arCuller) {
    if (spLight->m_uFlags & NiAVObject::APP_CULLED) {
        return true;
    }
    NiTListItem<NiPointer<NiGeometry>>* pkIter = lGeomList.m_pkHead;
    while (pkIter) {
        NiGeometry* pGeometry = pkIter->m_element.m_pObject;
        pkIter = pkIter->m_pkNext;
        if (!pGeometry) {
            continue;
        }
        BSShader* pShader = pGeometry->m_spShader;
        if (!pShader) {
            continue;
        }
        if (BSShaderManager::pObjectTypeCallback) {
            BSShaderManager::pObjectTypeCallback(pGeometry);
        }
        if (!(BSShaderManager::uiObjectTypeShadowMask & 1)) {
            continue;
        }
        bool bIsAccumulating = NiRenderer::ms_pkRenderer->m_spAccum->IsAccumulating();
        int32_t iShaderType = static_cast<int32_t>(pShader->iShaderType);
        if (iShaderType >= 1 && iShaderType <= 5) {
            if (!bIsAccumulating) {
                BSShaderManager::pCurrentRenderPass = nullptr;
                if (pGeometry->m_spSkinInstance.m_pObject) {
                    BSShaderManager::eCurrentPass = BSShaderManager::BSSM_AMBIENT_S;
                    ShadowLightShader::EnableConstantsForPass(BSShaderManager::BSSM_AMBIENT_S, false);
                }
                else {
                    BSShaderManager::eCurrentPass = BSShaderManager::BSSM_AMBIENT;
                    ShadowLightShader::EnableConstantsForPass(BSShaderManager::BSSM_AMBIENT, false);
                }
                pShader->m_spVertexConstantMap->SortByEnabled();
                pShader->m_spPixelConstantMap->SortByEnabled();
            }
        }
        else if (iShaderType == 17) {
            if (!bIsAccumulating) {
                BSShaderManager::pCurrentRenderPass = nullptr;
                if (pGeometry->m_spSkinInstance.m_pObject) {
                    BSShaderManager::eCurrentPass = BSShaderManager::BSSM_3XDEPTHMAP_S;
                    Lighting30Shader::EnableConstantsForPass(BSShaderManager::BSSM_3XDEPTHMAP_S, false);
                }
                else {
                    BSShaderManager::eCurrentPass = BSShaderManager::BSSM_3XDEPTHMAP;
                    Lighting30Shader::EnableConstantsForPass(BSShaderManager::BSSM_3XDEPTHMAP, false);
                }
                pShader->m_spVertexConstantMap->SortByEnabled();
                pShader->m_spPixelConstantMap->SortByEnabled();
            }
        }
        else {
            continue;
        }
        pGeometry->Cull(arCuller);
    }
    return true;
}

void ShadowSceneLight::SetLight(NiLight* apLight) {
    spLight = apLight;
    bPointLight = NiObject::IsKindOf(&NiPointLight::ms_RTTI, apLight);
    if (!bPointLight) {
        fLODDimmer = 1.0f;
    }
    else if (bDynamicLight) {
        bPointPosition = apLight->m_kWorld.m_Translate;
    }
    bAmbientLight = NiObject::IsKindOf(&NiAmbientLight::ms_RTTI, apLight);
}

void ShadowSceneLight::SetCastShadow(int32_t abCastShadow) {
    AutoMemContext kMemContext(MEM_CONTEXT_SHADOWS);
    // Tests and stores only the low byte, unconverted.
    uint8_t ucCastShadow = static_cast<uint8_t>(abCastShadow);
    // Bit 4 of the enabled passes is the shadow pass.
    if (((bCastShadow && !ucCastShadow) ||
         BSShaderManager::eShaderLevel < 3 ||
         !(BSShaderManager::iEnabledPasses & 0x10)) &&
        spDynamicShadowMap.m_pObject) {
        BSShaderManager::pTextureManager->ReturnFrustumShadowTexture(spDynamicShadowMap.m_pObject);
        spDynamicShadowMap = nullptr;
    }

    bCastShadow = ucCastShadow;
}

bool ShadowSceneLight::GetCastShadow() {
    return bCastShadow;
}

bool ShadowSceneLight::IsCubeShadowMap() {
    return bCubeShadowMap;
}

void ShadowSceneLight::SetFadeTarget(float afTarget, bool abForce) {
    double v3;
    double v4;
    double v5;
    float abNoFadea;
    float abNoFade;
    float abNoFadeb;
    if (afTarget != fShadowLODDimmerTarget) {
        if (abForce) {
            fShadowLODDimmerElapsed = 1.0f;
            fShadowLODDimmerTarget = afTarget;
        }
        else {
            abNoFadea = fShadowLODDimmerElapsed / BSShaderManager::fShadowFadeTime;
            v3 = abNoFadea;
            if (abNoFadea < 0.0) {
                v3 = 0.0;
            }
            abNoFade = static_cast<float>(v3);
            if (!(abNoFade > 1.0)) {
                v5 = abNoFade;
                v4 = 1.0;
            }
            else {
                v4 = 1.0;
                v5 = 1.0;
            }
            abNoFadeb = static_cast<float>(v5);
            fShadowLODDimmerElapsed = static_cast<float>(BSShaderManager::fShadowFadeTime * (v4 - abNoFadeb));
            fShadowLODDimmerTarget = afTarget;
        }
    }
}

bool ShadowSceneLight::BoundWithinLightRadius(const NiBound& apBound, NiLight* apLight, float afRadiusScale) {
    NiLight* pLight = apLight;
    if (!pLight) {
        pLight = spLight.m_pObject;
    }
    if (pLight->GetRTTI() == &NiDirectionalLight::ms_RTTI) {
        return true;
    }
    NiPoint3 kOffset = BSShaderManager::pShadowSceneNode[BSShaderManager::BSSM_SSN_WORLD]->kLightingOffset;
    float fDeltaX = pLight->m_kWorld.m_Translate.x - apBound.m_kCenter.x;
    float fDeltaY = pLight->m_kWorld.m_Translate.y - apBound.m_kCenter.y;
    float fDeltaZ = pLight->m_kWorld.m_Translate.z - apBound.m_kCenter.z;
    float fX = kOffset.x + fDeltaX;
    float fY = kOffset.y + fDeltaY;
    float fZ = kOffset.z + fDeltaZ;
    float fDistance = fY * fY + fX * fX + fZ * fZ;
    fDistance = std::sqrt(fDistance);
    float fGap = fDistance - apBound.m_fRadius * afRadiusScale;
    return pLight->m_kSpec.r > fGap;
}

// Attaches the light to apTriGeom's lighting property and adds the geometry to the light's list once (moving it in
// front of the fence when found past it). The list entry is kept in the function static sptriGeom.
void ShadowSceneLight::AddTriGeom(NiTriBasedGeom* apTriGeom) {
    static NiPointer<NiGeometry> sptriGeom;
    BSShader* m_spShader = apTriGeom->GetShader();
    if (m_spShader && NiD3DShaderInterface::DynamicCast(m_spShader, &NiD3DDefaultShader::ms_RTTI)) {
        // The address &apTriGeom->m_PropertyState is never null.
        BSShaderLightingProperty* pshprop = static_cast<BSShaderLightingProperty*>(
            apTriGeom->m_PropertyState.m_aspProps[3].m_pObject
        );
        int32_t iType = pshprop ? pshprop->iShaderPropertyType : -1;
        // Tests 1 <= iType <= 12 as one unsigned compare of iType - 1 against 11.
        if (static_cast<uint32_t>(iType - 1) <= 11) {
            NiTListItem<NiPointer<NiGeometry>>* pFence = kGeomListFence;
            NiTListItem<NiPointer<NiGeometry>>* pPos = pFence;
            if (!pFence) {
                pPos = lGeomList.m_pkHead;
            }
            while (pPos) {
                NiTListItem<NiPointer<NiGeometry>>* pItem = pPos;
                pPos = pPos->m_pkNext;
                if (pItem->m_element.m_pObject == apTriGeom) {
                    if (pFence) {
                        lGeomList.MoveToBefore(pItem, pFence);
                    }
                    BSNiBound* m_kWorldBound = apTriGeom->m_kWorldBound;
                    if (!m_kWorldBound) {
                        m_kWorldBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
                    }
                    pshprop->AttachLight(this, m_kWorldBound);
                    return;
                }
            }
            if (!pshprop->GetFlag(BSShaderProperty::BSSP_FLAG_MODELSPACENORMALS) || iType == 4) {
                sptriGeom.m_pObject = apTriGeom;
                BSNiBound* v10 = apTriGeom->m_kWorldBound;
                if (!v10) {
                    v10 = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
                }
                pshprop->AttachLight(this, v10);
                lGeomList.AddHead(sptriGeom);
                sptriGeom.m_pObject = nullptr;
            }
        }
    }
}

void ShadowSceneLight::RemoveAllTriShapes() {
    NiTListItem<NiPointer<NiGeometry>>* pPos = lGeomList.m_pkHead;
    while (pPos) {
        NiGeometry* pGeometry = pPos->m_element.m_pObject;
        pPos = pPos->m_pkNext;
        if (pGeometry) {
            // The shade property is a lighting property when its shader type is 1 to 12 (one unsigned compare).
            BSShaderProperty* pProperty = static_cast<BSShaderProperty*>(
                pGeometry->m_PropertyState.m_aspProps[3].m_pObject
            );
            if (pProperty &&
                static_cast<uint32_t>(pProperty->iShaderPropertyType - 1) <=
                    BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_LAST - 1) {
                static_cast<BSShaderLightingProperty*>(pProperty)->RemoveLight(this);
            }
        }
    }
    lGeomList.RemoveAll();
    lRootList.RemoveAll();
}

void ShadowSceneLight::SetDynamicPosition(NiPoint3 abDynamicPosition) {
    bPointPosition = abDynamicPosition;
}

// Puts the fence shape (made on first use, named "fence") at the head of the geometry list and marks it as the fence.
void ShadowSceneLight::InsertUnusedFence() {
    if (!kGeomListFenceObject.m_pObject) {
        NiTriShape* pShape;
        pShape = new NiTriShape(static_cast<NiTriShapeData*>(nullptr));
        kGeomListFenceObject = pShape;
        // kName's destructor releases the string handle.
        NiFixedString kName;
        kName.SetHandle(NiGlobalStringTable::AddString("fence"));
        kGeomListFenceObject->SetName(kName);
    }
    lGeomList.AddHead(kGeomListFenceObject);
    kGeomListFence = lGeomList.m_pkHead;
}

void ShadowSceneLight::AddObject(NiAVObject* apAVObject) {
    if (!apAVObject) {
        return;
    }
    NiNode* pNode = apAVObject->IsNode();
    if (pNode &&
        !(apAVObject->m_uFlags & NiAVObject::APP_CULLED) &&
        pNode->m_kWorldBound &&
        0.0f != pNode->m_kWorldBound->m_fRadius) {
        if (!bCastShadow) {
            ParseObjectAddRadius(pNode);
            return;
        }
        if (apAVObject != spShadowCaster.m_pObject || (apAVObject->m_uFlags & NiAVObject::NO_SHADOWS)) {
            ParseObjectAddFrustum(pNode);
            return;
        }
    }
    if (apAVObject->IsTriBasedGeom()) {
        AddTriGeom(static_cast<NiTriBasedGeom*>(apAVObject));
    }
}

void ShadowSceneLight::RemoveTriGeom(NiTriBasedGeom* apTriGeom, void* akIterator) {
    // The shade property is a lighting property when its shader type is 1 to 12 (one unsigned compare).
    BSShaderProperty* pProperty = static_cast<BSShaderProperty*>(apTriGeom->m_PropertyState.m_aspProps[3].m_pObject);
    if (pProperty &&
        static_cast<uint32_t>(pProperty->iShaderPropertyType - 1) <=
            BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_LAST - 1) {
        static_cast<BSShaderLightingProperty*>(pProperty)->RemoveLight(this);
    }
    if (akIterator) {
        NiTListItem<NiPointer<NiGeometry>>* pItem = static_cast<NiTListItem<NiPointer<NiGeometry>>*>(akIterator);
        lGeomList.RemovePos(pItem);
    }
    else {
        NiPointer<NiGeometry> spGeometry(apTriGeom);
        lGeomList.Remove(spGeometry);
    }
}

void ShadowSceneLight::RemoveUnusedGeom() {
    NiTListItem<NiPointer<NiGeometry>>* pPos = kGeomListFence;
    while (pPos) {
        NiGeometry* pGeometry = pPos->m_element.m_pObject;
        NiTListItem<NiPointer<NiGeometry>>* pItem = pPos;
        pPos = pPos->m_pkNext;
        if (pGeometry) {
            RemoveTriGeom(static_cast<NiTriBasedGeom*>(pGeometry), pItem);
        }
    }
    kGeomListFence = nullptr;
}

float ShadowSceneLight::GetLuminanceToBound(NiBound* apBound) {
    NiPoint3 kOffset = BSShaderManager::pShadowSceneNode[BSShaderManager::BSSM_SSN_WORLD]->kLightingOffset;
    NiLight* pLight = spLight.m_pObject;
    float fX = kOffset.x + pLight->m_kWorld.m_Translate.x;
    float fY = pLight->m_kWorld.m_Translate.y + kOffset.y;
    float fZ = pLight->m_kWorld.m_Translate.z + kOffset.z;
    float fDeltaX = fX - apBound->m_kCenter.x;
    float fDeltaY = fY - apBound->m_kCenter.y;
    float fDeltaZ = fZ - apBound->m_kCenter.z;
    float fDY = fDeltaY;
    float fDX = fDeltaX;
    float fDZ = fDeltaZ;
    float fSquared = fDY * fDY + fDX * fDX + fDZ * fDZ;
    float fDistance = fSquared;
    fDistance = std::sqrt(fDistance);
    float fRoot = fDistance;
    float fResult = (fRoot - apBound->m_fRadius) / pLight->m_kSpec.r;
    float fRounded = fResult;
    fLuminance = fRounded;
    return fRounded;
}

void ShadowSceneLight::TestFrustumCull(NiCullingProcess& arCuller) {
    uint16_t usNewFrustumCull = SSL_NONE;
    bool bLightCenterInFrustum = true;
    NiBound kBound;
    NiLight* pLight = spLight;
    kBound.m_fRadius = pLight->m_kSpec.r;
    kBound.m_kCenter = pLight->m_kWorld.m_Translate;
    if (GetCastShadow() && spShadowCamera) {
        usNewFrustumCull = FrustumFrustumIntersection(arCuller.m_pkCamera, spShadowCamera) ? SSL_NONE : SSL_ALL;
    }
    else {
        NiFrustumPlanes kPlanes = arCuller.GetFrustumPlanes();
        for (uint32_t i = 0; i < NiFrustumPlanes::MAX_PLANES; ++i) {
            if (kPlanes.IsPlaneActive(i)) {
                NiPlane kPlane = kPlanes.GetPlane(i);
                if (kBound.WhichSide(kPlane) == NiPlane::NEGATIVE_SIDE) {
                    usNewFrustumCull = SSL_ALL;
                    break;
                }
                if (kPlane.WhichSide(kBound.m_kCenter) == NiPlane::NEGATIVE_SIDE) {
                    bLightCenterInFrustum = false;
                }
            }
        }
        if (usNewFrustumCull != SSL_ALL &&
            !bLightCenterInFrustum &&
            GetCastShadow() &&
            BSShaderManager::GetShadows() &&
            IsCubeShadowMap()) {
            for (uint16_t usFace = 0; usFace < 6; ++usFace) {
                if (!CubeFaceFrustumFrustumIntersection(kBound, arCuller, usFace)) {
                    usNewFrustumCull |= 1 << usFace;
                }
            }
        }
    }
    float fNewLODDimmer = fLODDimmer;
    float fNewShadowLODDimmer = fShadowLODDimmer;
    if (usNewFrustumCull != SSL_ALL) {
        NiPoint3 kDiff = kBound.m_kCenter - arCuller.GetCamera()->m_kWorld.m_Translate;
        float fLength = std::sqrt(static_cast<double>(kDiff.SqrLength()));
        float fDistance = fLength - kBound.m_fRadius;
        if (BSShaderManager::GetAccumulator()->m_pkCamera) {
            fDistance = BSShaderManager::GetAccumulator()->m_pkCamera->m_fLODAdjust * fDistance;
        }
        // The fades are taken at double precision.
        if (fDistance < BSShaderManager::fLightLODStartFade || BSShaderManager::fLightLODEnd == 0.0f) {
            fNewLODDimmer = 1.0f;
        }
        else if (fDistance > BSShaderManager::fLightLODEnd) {
            fNewLODDimmer = 0.0f;
            fNewShadowLODDimmer = 0.0f;
            usNewFrustumCull = SSL_ALL;
        }
        else {
            fNewLODDimmer = 1.0 -
                (static_cast<double>(fDistance) - BSShaderManager::fLightLODStartFade) /
                    (static_cast<double>(BSShaderManager::fLightLODEnd) - BSShaderManager::fLightLODStartFade);
        }
        if (fNewLODDimmer > 0.0f && GetCastShadow()) {
            if (fDistance < BSShaderManager::fShadowLODStartFade || BSShaderManager::fShadowLODEnd == 0.0f) {
                fNewShadowLODDimmer = 1.0f;
            }
            else if (fDistance > BSShaderManager::fShadowLODEnd) {
                fNewShadowLODDimmer = 0.0f;
            }
            else {
                fNewShadowLODDimmer = 1.0 -
                    (static_cast<double>(fDistance) - BSShaderManager::fShadowLODStartFade) /
                        (static_cast<double>(BSShaderManager::fShadowLODEnd) - BSShaderManager::fShadowLODStartFade);
            }
        }
        NiNode* pShadowCaster = spShadowCaster;
        if (pShadowCaster) {
            NiMaterialProperty* pMaterial = static_cast<NiMaterialProperty*>(
                pShadowCaster->GetProperty(NiProperty::MATERIAL)
            );
            if (pMaterial) {
                fNewShadowLODDimmer = pMaterial->m_fAlpha * fNewShadowLODDimmer;
            }
        }
        float fElapsed = fShadowLODDimmerElapsed + BSShaderManager::GetTimer(true);
        fShadowLODDimmerElapsed = fElapsed;
        float fFadeProgress = fElapsed / BSShaderManager::fShadowFadeTime;
        if (fFadeProgress > 1.0f) {
            fFadeProgress = 1.0f;
        }
        if (fShadowLODDimmerTarget == 0.0f) {
            fNewShadowLODDimmer = (1.0 - fFadeProgress) * fNewShadowLODDimmer;
        }
        else {
            fNewShadowLODDimmer = fFadeProgress * fNewShadowLODDimmer;
        }
    }
    if ((usFrustumCull == SSL_ALL && usNewFrustumCull != SSL_ALL) ||
        (usFrustumCull != SSL_ALL && usNewFrustumCull == SSL_ALL) ||
        (fLODDimmer == 0.0f && fNewLODDimmer > 0.0f) ||
        (fLODDimmer > 0.0f && fNewLODDimmer == 0.0f) ||
        (fShadowLODDimmer == 0.0f && fNewShadowLODDimmer > 0.0f) ||
        (fShadowLODDimmer > 0.0f && fNewShadowLODDimmer == 0.0f)) {
        NiTListItem<NiPointer<NiGeometry>>* kPos = lGeomList.GetHeadPos();
        while (kPos) {
            NiGeometry* pGeometry = lGeomList.GetNext(kPos);
            if (pGeometry) {
                NiShadeProperty* pShade = pGeometry->m_PropertyState.GetShade();
                BSShaderLightingProperty* pLightingProperty = BSSM_ShaderPropertyIsLighting(pShade)
                    ? static_cast<BSShaderLightingProperty*>(pShade)
                    : nullptr;
                if (pLightingProperty) {
                    pLightingProperty->bLightListChanged = true;
                }
            }
        }
        if (fShadowLODDimmer == 0.0f && spDynamicShadowMap) {
            BSShaderManager::TextureManagerPtr()->ReturnFrustumShadowTexture(spDynamicShadowMap);
            spDynamicShadowMap = nullptr;
        }
    }
    usFrustumCull = usNewFrustumCull;
    fLODDimmer = fNewLODDimmer;
    fShadowLODDimmer = fNewShadowLODDimmer;
}

void ShadowSceneLight::SetDynamicShadowMap(BSRenderedTexture* apDynamicShadowMap) {
    BSRenderedTexture* m_pObject;
    m_pObject = spDynamicShadowMap.m_pObject;
    if (m_pObject != apDynamicShadowMap) {
        if (m_pObject && (!InterlockedDecrementUInt32(&m_pObject->m_uiRefCount))) {
            m_pObject->DeleteThis();
        }

        spDynamicShadowMap.m_pObject = apDynamicShadowMap;
        if (apDynamicShadowMap) {
            InterlockedIncrementUInt32(&apDynamicShadowMap->m_uiRefCount);
        }
    }
}

uint16_t ShadowSceneLight::GetFrustumCull() {
    return usFrustumCull;
}

bool ShadowSceneLight::IsFadedOut() {
    return (BSShaderManager::fShadowFadeTime <= static_cast<double>(fShadowLODDimmerElapsed)) &&
        (fShadowLODDimmerTarget <= 0.10000000149011612);
}

void ShadowSceneLight::ShowDebugTexture(bool abShow) {
    bShowDebugTexture = abShow;
}

NiLight* ShadowSceneLight::GetLight() {
    return spLight;
}

NiAVObject* ShadowSceneLight::GetShadowScene() {
    return spShadowCaster;
}

void ShadowSceneLight::ParseObjectAddFrustum(NiAVObject* apObject) {
    uint32_t m_uiActivePlanes = kShadowCameraPlanes.GetActivePlaneState();
    if (apObject != spShadowCaster.m_pObject &&
        (apObject->m_uFlags & NiAVObject::NO_SHADOWS) == 0 &&
        (!m_uiActivePlanes ||
         (apObject->m_uFlags & NiAVObject::ALWAYS_DRAW) != 0 ||
         BoundWithinLightFrustum(apObject->m_kWorldBound ? *apObject->m_kWorldBound : NiAVObject::NullBoundS))) {
        NiNode* v5 = apObject->IsNode();
        if (v5) {
            for (uint32_t v7 = 0; v7 < v5->m_kChildren.m_usSize; ++v7) {
                NiAVObject* m_pObject = v5->m_kChildren.m_pBase[v7].m_pObject;
                if (m_pObject) {
                    NiBound* v10;
                    uint32_t m_uFlags = m_pObject->m_uFlags;
                    if (((m_uFlags & NiAVObject::ALWAYS_DRAW) != 0 ||
                         ((v10 = m_pObject->m_kWorldBound) != nullptr && 0.0 != v10->m_fRadius)) &&
                        (m_uFlags & NiAVObject::APP_CULLED) == 0) {
                        ParseObjectAddFrustum(m_pObject);
                    }
                }
            }
        }
        else if (apObject->IsTriBasedGeom()) {
            AddTriGeom(static_cast<NiTriBasedGeom*>(apObject));
        }
    }
    kShadowCameraPlanes.m_uiActivePlanes = m_uiActivePlanes;
}

// Adds the geometry under apObject that lies within the light's radius; the camera planes' active state is restored
// on return.
void ShadowSceneLight::ParseObjectAddRadius(NiAVObject* apObject) {
    uint32_t uiActivePlanes = kShadowCameraPlanes.m_uiActivePlanes;
    if (!(apObject->m_uFlags & NiAVObject::ALWAYS_DRAW)) {
        NiBound* pBound = apObject->m_kWorldBound;
        if (!pBound) {
            pBound = &NiAVObject::NullBoundS;
        }
        if (!BoundWithinLightRadius(*pBound, spLight.m_pObject, 1.0f)) {
            kShadowCameraPlanes.m_uiActivePlanes = uiActivePlanes;
            return;
        }
    }
    NiNode* pNode = apObject->IsNode();
    if (pNode) {
        for (NiAVObject* pChild : pNode->m_kChildren) {
            if (!pChild) {
                continue;
            }
            uint32_t uiFlags = pChild->m_uFlags;
            if ((!(uiFlags & NiAVObject::ALWAYS_DRAW)) &&
                (!pChild->m_kWorldBound || 0.0f == pChild->m_kWorldBound->m_fRadius)) {
                continue;
            }

            if (!(uiFlags & NiAVObject::APP_CULLED)) {
                ParseObjectAddRadius(pChild);
            }
        }
    }
    else if (apObject->IsTriBasedGeom()) {
        AddTriGeom(apObject->IsTriBasedGeom());
    }
    kShadowCameraPlanes.m_uiActivePlanes = uiActivePlanes;
}

float ShadowSceneLight::GetViewLuminanceAtPoint(NiPoint3 apPoint, NiPoint3 apView, NiLight* apIgnore) {
    NiLight* m_pObject = spLight.m_pObject;
    // Compares the byte with 1, as IsShadowLight does.
    if (m_pObject == apIgnore || (bCastShadow == 1 || (m_pObject->m_uFlags & NiAVObject::APP_CULLED) != 0)) {
        return 0.0;
    }

    NiPoint3 v20;
    NiPoint3 v21;
    bool v6 = !bPointLight;
    float fDimmer = m_pObject->m_fDimmer;
    float x = m_pObject->m_kWorld.m_Translate.x;
    v21.y = m_pObject->m_kWorld.m_Translate.y;
    float r = m_pObject->m_kDiff.r;
    float b = m_pObject->m_kDiff.b;
    v21.x = x;
    float v17 = fDimmer * r;
    v21.z = m_pObject->m_kWorld.m_Translate.z;
    float v18 = fDimmer * m_pObject->m_kDiff.g;
    float v19 = fDimmer * b;
    float fRadius = m_pObject->m_kSpec.r;
    fLuminance = 1.0;
    v20.x = apPoint.x - v21.x;
    v20.y = apPoint.y - v21.y;
    v20.z = apPoint.z - v21.z;
    if (!v6) {
        float fRawRatio = v20.Length() / fRadius;
        float fDistRatio = BSMAX(static_cast<float>(0.0), fRawRatio);
        float fClampedRatio = BSMIN(static_cast<float>(1.0), fDistRatio);
        fLuminance = 1.0 - fClampedRatio * fClampedRatio;
    }
    v21.x = apView.x - apPoint.x;
    v21.y = apView.y - apPoint.y;
    v21.z = apView.z - apPoint.z;
    v21.Unitize();
    v20.Unitize();
    float apPointb = -v21.x;
    float apPoint_4 = -v21.y;
    float apPoint_8 = -v21.z;
    float fCosAngle = apPoint_4 * v20.y + apPointb * v20.x + apPoint_8 * v20.z;
    double v9 = fCosAngle;
    if (fCosAngle > 1.0) {
        v9 = 1.0;
    }
    float fClampedCos = v9;
    double v10 = fClampedCos;
    if (fClampedCos < 0.0) {
        v10 = 0.0;
    }
    float apPointa = v10;
    double v11 = v18;
    if (!(v19 < static_cast<double>(v18))) {
        v11 = v19;
    }
    float fMaxGreenBlue = v11;
    double v12 = v17;
    if (!(fMaxGreenBlue < static_cast<double>(v17))) {
        v12 = fMaxGreenBlue;
    }
    float fMaxColor = v12;
    float fResult = fLuminance * apPointa * fMaxColor;
    fLuminance = fResult;
    return fResult;
}

// The value of aTargetCenter is never read.
void ShadowSceneLight::ComputeWeightedShadowLight(
    BSShaderLightingProperty* apLightingProperty,
    NiPoint3 aTargetCenter
) {
    NiPoint3* v8;
    ShadowSceneLight* plight;
    float v66;
    float v67;
    float v68;
    NiPoint3 kExtents;
    NiPoint3 kvector;
    NiPoint3 blendedVect;
    void* kiter;
    float v82;
    float v83;
    float v84;
    NiPoint3 kdirlightvect;
    NiUpdateData UpdateData;
    NiNode* m_pObject = spShadowCaster.m_pObject;
    BSBound* pbound = static_cast<BSBound*>(m_pObject->GetExtraData(*BSBound::pTag));
    NiNode* v6 = spShadowCaster.m_pObject;
    aTargetCenter = v6->m_kWorld.m_Translate;
    if (pbound) {
        float m_fScale = v6->m_kWorld.m_fScale;
        kvector.x = pbound->Center.x * m_fScale;
        kvector.y = pbound->Center.y * m_fScale;
        kvector.z = m_fScale * pbound->Center.z;
        aTargetCenter.x = kvector.x + aTargetCenter.x;
        aTargetCenter.y = aTargetCenter.y + kvector.y;
        aTargetCenter.z = aTargetCenter.z + kvector.z;
        float v52 = v6->m_kWorld.m_fScale;
        kExtents.x = pbound->Extents.x * v52;
        kExtents.y = pbound->Extents.y * v52;
        kExtents.z = v52 * pbound->Extents.z;
        v8 = &kExtents;
    }
    else {
        v8 = &NiPoint3::ZERO;
    }
    float x = v8->x;
    float y = v8->y;
    float z = v8->z;
    kvector.x = x;
    double v12 = x;
    kvector.y = y;
    kvector.z = z;
    if (!(y < static_cast<double>(x))) {
        v12 = y;
    }
    float v61 = v12;
    double v13 = v61;
    if (!(kvector.z < static_cast<double>(v61))) {
        v13 = kvector.z;
    }
    float v72 = v13;
    float v77 = 0.0f;
    uint32_t v14 = 0;
    uint16_t v15 = 0;
    uint32_t v76 = 0;
    if (apLightingProperty) {
        plight = apLightingProperty->GetFirstActiveNonShadowLight(kiter);
    }
    else {
        plight = BSShaderManager::pShadowSceneNode[0]->GetLightAt(0);
    }
    if (plight) {
        do {
            if (v14 >= 40) {
                break;
            }
            if (!plight->bCastShadow) {
                NiLight* v17 = plight->spLight.m_pObject;
                if ((v17->m_uFlags & NiAVObject::APP_CULLED) == 0 && v17->m_fDimmer > 0.0f) {
                    kvector.x = v17->m_kWorld.m_Translate.x - aTargetCenter.x;
                    kvector.y = v17->m_kWorld.m_Translate.y - aTargetCenter.y;
                    kvector.z = v17->m_kWorld.m_Translate.z - aTargetCenter.z;
                    float v53 = kvector.x * kvector.x + kvector.y * kvector.y + kvector.z * kvector.z;
                    float v54 = std::sqrt(static_cast<double>(v53));
                    float v55 = 1.0 - (v54 - v72) / v17->m_kSpec.r;
                    double v18 = v55;
                    if (v55 < 0.0) {
                        v18 = 0.0;
                    }
                    float v62 = v18;
                    double v19 = v62;
                    if (v62 > 1.0) {
                        v19 = 1.0;
                    }
                    float v63 = v19;
                    ++v14;
                    fLightDistances[v14 - 1] = v63;
                    fLightWeights[v14 - 1] = v63;
                    v77 = v63 + v77;
                }
            }
            plight = apLightingProperty ? apLightingProperty->GetNextActiveNonShadowLight(kiter)
                                        : BSShaderManager::pShadowSceneNode[0]->GetLightAt(++v15);
        }
        while (plight);
        v76 = v14;
    }
    ShadowSceneNode* v20 = BSShaderManager::pShadowSceneNode[0];
    float v64 = 0.0f;
    blendedVect.x = NiPoint3::ZERO.x;
    float r = NiColor::BLACK.r;
    // The sun's light is a directional light.
    NiDirectionalLight* v21 = static_cast<NiDirectionalLight*>(
        BSShaderManager::pShadowSceneNode[0]->pSunLight->spLight.m_pObject
    );
    double v22 = v21->m_kWorldDir.x;
    blendedVect.y = NiPoint3::ZERO.y;
    kdirlightvect.x = -v22;
    blendedVect.z = NiPoint3::ZERO.z;
    kdirlightvect.y = -v21->m_kWorldDir.y;
    float g = NiColor::BLACK.g;
    double v23 = v21->m_kWorldDir.z;
    float b = NiColor::BLACK.b;
    kdirlightvect.z = -v23 * (v72 * 0.0625);
    kdirlightvect.Unitize();
    if (0.0f == v77) {
        blendedVect.z = kdirlightvect.z;
        float v24 = v21->m_kDiff.b;
        blendedVect.x = kdirlightvect.x;
        float v25 = v21->m_kDiff.r;
        blendedVect.y = kdirlightvect.y;
        float v26 = v21->m_kDiff.g;
        b = v24;
        r = v25;
        g = v26;
        blendedVect.Unitize();
        float v56 = v72 / 75.0;
        if (blendedVect.z < static_cast<double>(v56)) {
            blendedVect.z = v72 / 75.0;
        }
    }
    else {
        ShadowSceneLight* plight_2;
        ShadowSceneLight* plight_3;
        BSShaderLightingProperty* v27 = apLightingProperty;
        int32_t v28 = 0;
        if (apLightingProperty) {
            plight_2 = apLightingProperty->GetFirstActiveNonShadowLight(kiter);
        }
        else {
            plight_2 = v20->GetLightAt(0);
        }
        uint32_t v30 = 0;
        for (ShadowSceneLight* i = plight_2; v30 < v76; i = plight_3) {
            if (i && (!i->bCastShadow)) {
                NiLight* v32 = i->spLight.m_pObject;
                if ((v32->m_uFlags & NiAVObject::APP_CULLED) == 0) {
                    float v57 = fLightWeights[v30] / v77;
                    if (v57 > 0.0f) {
                        NiPoint3 kvector_2;
                        double dInvWeight = 1.0 - fLightWeights[v30];
                        v64 = v57 * (v32->m_kSpec.r * fLightWeights[v30] + dInvWeight * 1000.0) + v64;
                        kvector_2.x = v32->m_kWorld.m_Translate.x - aTargetCenter.x;
                        kvector_2.y = v32->m_kWorld.m_Translate.y - aTargetCenter.y;
                        kvector_2.z = v32->m_kWorld.m_Translate.z - aTargetCenter.z;
                        kvector_2.Unitize();
                        float fInvWeight = dInvWeight;
                        double v33 = fInvWeight;
                        v82 = fInvWeight * kdirlightvect.x;
                        v83 = kdirlightvect.y * fInvWeight;
                        v84 = fInvWeight * kdirlightvect.z;
                        float fWeight = fLightWeights[v30];
                        float v99 = kvector_2.x * fWeight;
                        float v100 = kvector_2.y * fWeight;
                        float v101 = fWeight * kvector_2.z;
                        kvector.x = v99 + v82;
                        kvector.y = v100 + v83;
                        kvector.z = v101 + v84;
                        kvector_2 = kvector;
                        v66 = kvector.x * v57;
                        v67 = kvector.y * v57;
                        v68 = kvector.z * v57;
                        blendedVect.x = v66 + blendedVect.x;
                        float v34 = v21->m_kDiff.r;
                        float v35 = v21->m_kDiff.g;
                        float v36 = v21->m_kDiff.b;
                        blendedVect.y = blendedVect.y + v67;
                        float v85 = v34;
                        float v37 = v32->m_kDiff.r;
                        float v86 = v35;
                        float v38 = v32->m_kDiff.g;
                        float v87 = v36;
                        blendedVect.z = v68 + blendedVect.z;
                        float v39 = v32->m_kDiff.b;
                        float v78 = v37;
                        float fInvWeight2 = v33;
                        float v79 = v38;
                        float v80 = v39;
                        v85 = v85 * fInvWeight2;
                        v86 = v86 * fInvWeight2;
                        v87 = fInvWeight2 * v87;
                        float fWeight2 = fLightWeights[v30];
                        v78 = v37 * fWeight2;
                        v79 = v38 * fWeight2;
                        v80 = fWeight2 * v39;
                        float v88 = v78 + v85;
                        float v89 = v79 + v86;
                        float v90 = v80 + v87;
                        float v93 = v88 * v57;
                        float v94 = v89 * v57;
                        float v95 = v57 * v90;
                        r = v93 + r;
                        g = v94 + g;
                        b = v95 + b;
                    }
                }
                v27 = apLightingProperty;
            }

            if (v27) {
                plight_3 = v27->GetNextActiveNonShadowLight(kiter);
            }
            else {
                plight_3 = BSShaderManager::pShadowSceneNode[0]->GetLightAt(++v28);
            }
            ++v30;
        }
        blendedVect.Unitize();
        if (blendedVect.z < 0.75) {
            blendedVect.z = 0.75f;
        }
    }
    blendedVect.Unitize();
    double v41 = v72;
    if (v72 > 256.0) {
        v41 = 256.0f;
    }
    float v58 = v41;
    NiPoint3* pLightPos = &spLight->m_kLocal.m_Translate;
    v66 = blendedVect.x * v58;
    v67 = blendedVect.y * v58;
    v68 = v58 * blendedVect.z;
    v82 = v66 * 1.399999976158142;
    v83 = v67 * 1.399999976158142;
    v84 = 1.399999976158142 * v68;
    v66 = v82 + aTargetCenter.x;
    v67 = v83 + aTargetCenter.y;
    float v44 = v67;
    double v45 = v84 + aTargetCenter.z;
    pLightPos->x = v66;
    pLightPos->y = v44;
    v68 = v45;
    pLightPos->z = v68;
    NiAVObject* v46 = spLight.m_pObject;
    UpdateData.bUpdateControllers = false;
    float v65 = v58 * 8.0;
    UpdateData.bParallelUpdate = false;
    UpdateData.bFoundParticles = false;
    UpdateData.fTime = 0.0f;
    UpdateData.bFoundMorphController = false;
    UpdateData.bSceneGraphChange = false;
    v46->Update(UpdateData);
    float v92 = g * 0.4000000059604645 + r * 0.4000000059604645 + 0.4000000059604645 * b;
    double v47 = v92;
    if (v92 < 0.800000011920929) {
        v47 = 0.8f;
    }
    float v59 = v47;
    double v48 = v59;
    if (v59 > 1.0) {
        v48 = 1.0;
    }
    // The diffuse and specular setters: each stores its color and bumps the revision.
    NiLight* v49 = spLight.m_pObject;
    float v60 = v48;
    v49->m_kDiff.r = v60;
    v49->m_kDiff.g = v60;
    v66 = v65;
    v49->m_kDiff.b = v60;
    v67 = 0.0f;
    v68 = 0.0f;
    ++v49->m_uiRevID;
    NiLight* v50 = spLight.m_pObject;
    ++v50->m_uiRevID;
    v50->m_kSpec.r = v65;
    v50->m_kSpec.g = v67;
    v50->m_kSpec.b = v68;
}

void ShadowSceneLight::AccumulateGeometry(BSCullingProcess* apCuller, int32_t aiIndex) {
    alignas(16) NiPoint3 v55;
    alignas(16) NiPoint3 v62;
    NiPoint3 arData;
    alignas(16) NiTransform kLightPos;
    bool bRender = true;
    if ((!bCastShadow || (BSShaderManager::eShaderLevel < 3 || (BSShaderManager::iEnabledPasses & 16) == 0)) &&
        !bShadowSpotlight) {
        bRender = false;
    }
    if (!spDynamicShadowMap.m_pObject) {
        BSRenderedTexture* v4 = BSShaderManager::pTextureManager->BorrowFrustumShadowTexture();
        if (!v4) {
            spLight->m_uFlags |= NiAVObject::APP_CULLED;
        }
        SetDynamicShadowMap(v4);
    }
    if (bRender && (usFrustumCull != 255 && (fShadowLODDimmer > 0.05 && spDynamicShadowMap.m_pObject))) {
        NiPoint3* v15;
        NiPoint3 kWorldUp;
        alignas(16) NiRect<float> kViewport;
        NiNode* m_pObject = spShadowCaster.m_pObject;
        double v6 = m_pObject->m_kWorld.m_Translate.x;
        kLightPos = spLight->m_kWorld;
        float x = v6 - kLightPos.m_Translate.x;
        float y = m_pObject->m_kWorld.m_Translate.y - kLightPos.m_Translate.y;
        float za = m_pObject->m_kWorld.m_Translate.z - kLightPos.m_Translate.z;
        v55.x = x;
        v55.y = y;
        v55.z = za;
        v55.Unitize();
        float zb = -NiPoint3::UNIT_Z.x;
        y = -NiPoint3::UNIT_Z.y;
        x = -NiPoint3::UNIT_Z.z;
        kWorldUp.x = zb;
        kWorldUp.y = y;
        kWorldUp.z = x;
        NiPoint3 kWorldPt = v55.UnitCross(kWorldUp);
        if (kWorldPt.x < 0.01 &&
            kWorldPt.x > -0.01 &&
            kWorldPt.y < 0.01 &&
            kWorldPt.y > -0.01 &&
            kWorldPt.z < 0.01 &&
            kWorldPt.z > -0.01) {
            kWorldUp = NiPoint3::UNIT_X;
            NiPoint3* v7 = ::new (&v62) NiPoint3(v55.UnitCross(kWorldUp));
            float v8 = v7->y;
            kWorldPt.x = v7->x;
            float v9 = v7->z;
            kWorldPt.y = v8;
            kWorldPt.z = v9;
        }
        kWorldUp = *::new (&v62) NiPoint3(v55.UnitCross(kWorldPt));
        NiFrustum kFrustum(false);
        NiNode* v10 = spShadowCaster.m_pObject;
        kFrustum.m_bOrtho = true;
        NiExtraData* ExtraData = v10->GetExtraData(*BSBound::pTag);
        NiNode* v12 = spShadowCaster.m_pObject;
        float v13 = v12->m_kWorld.m_Translate.y;
        v55.x = v12->m_kWorld.m_Translate.x;
        float v14 = v12->m_kWorld.m_Translate.z;
        v55.y = v13;
        v55.z = v14;
        if (ExtraData) {
            // The extra data is the caster's BSBound: its half extents, scaled.
            v15 = ::new (&v62) NiPoint3(static_cast<BSBound*>(ExtraData)->Extents * v12->m_kWorld.m_fScale);
        }
        else {
            v15 = &NiPoint3::ZERO;
        }
        float v16 = v15->x;
        float v17 = v15->y;
        NiBound* m_kWorldBound = v12->m_kWorldBound;
        float v19 = v15->z;
        arData.x = v16;
        arData.y = v17;
        arData.z = v19;
        if (!m_kWorldBound) {
            m_kWorldBound = &NiAVObject::NullBoundS;
        }
        float zc = arData.z * 1.5;
        float afRight = BSMAX(zc, m_kWorldBound->m_fRadius);
        float zd = arData.y * 1.5;
        float afRighta = BSMAX(zd, afRight);
        float ze = arData.x * 1.5;
        float afRightb = BSMAX(ze, afRighta);
        double v20 = BSMIN(256.0f, afRightb);
        NiLight* v21 = spLight.m_pObject;
        float v52 = v20;
        float zf = v21->m_kWorld.m_Translate.x - v55.x;
        y = v21->m_kWorld.m_Translate.y - v55.y;
        x = v21->m_kWorld.m_Translate.z - v55.z;
        v62.x = zf;
        v62.y = y;
        v62.z = x;
        double v22 = v62.Length();
        bool v23 = !bCastShadow;
        float zg = v22;
        double v24 = v52;
        fSpotlightFOV = v52 / zg * 150.0;
        float v53 = -v52;
        kFrustum.m_fLeft = v53;
        kFrustum.m_fBottom = v53;
        kFrustum.m_fRight = v24;
        kFrustum.m_fTop = v24;
        kFrustum.m_fNear = 1.0f;
        kFrustum.m_fFar = v21->m_kSpec.r;
        kViewport.m_left = 0.0f;
        kViewport.m_bottom = 0.0f;
        kViewport.m_right = 1.0f;
        kViewport.m_top = 1.0f;
        if (!v23 && BSShaderManager::GetShadows()) {
            NiPoint3 result;
            NiUpdateData kUpdateData;
            spShadowAccumulator = BSShaderManager::GetNextAvailableShadowMapAccumulator();
            spShadowAccumulator->eRenderMode = BSShaderManager::BSSM_RENDER_DEPTHMAP;
            NiPointer<NiCamera>* p_spShadowCamera = &spShadowCamera;
            if (!spShadowCamera.m_pObject) {
                NiCamera* v27;
                v27 = new NiCamera;
                *p_spShadowCamera = v27;
            }
            NiCamera* v28 = p_spShadowCamera->m_pObject;
            float v29 = kLightPos.m_Translate.y;
            v28->m_kLocal.m_Translate.x = kLightPos.m_Translate.x;
            float v30 = kLightPos.m_Translate.z;
            v28->m_kLocal.m_Translate.y = v29;
            v28->m_kLocal.m_Translate.z = v30;
            NiAVObject* v31 = p_spShadowCamera->m_pObject;
            kUpdateData.fTime = 0.0f;
            kUpdateData.bUpdateControllers = false;
            kUpdateData.bParallelUpdate = false;
            kUpdateData.bFoundParticles = false;
            kUpdateData.bFoundMorphController = false;
            kUpdateData.bSceneGraphChange = false;
            v31->Update(kUpdateData);
            NiNode* v32 = spShadowCaster.m_pObject;
            kWorldPt = v32->m_kWorld.m_Translate;
            BSBound* v33 = static_cast<BSBound*>(v32->GetExtraData(*BSBound::pTag));
            kWorldPt += *::new (&result) NiPoint3(v33->Center * spShadowCaster->m_kWorld.m_fScale);
            ::new (&v62) NiPoint3(v33->Extents * spShadowCaster->m_kWorld.m_fScale);
            if (v62.y + v62.x < v62.z) {
                v62.x = NiPoint3::UNIT_Z.x * 10.0;
                v62.y = NiPoint3::UNIT_Z.y * 10.0;
                v62.z = 10.0 * NiPoint3::UNIT_Z.z;
                kWorldPt.x = v62.x + kWorldPt.x;
                kWorldPt.y = kWorldPt.y + v62.y;
                kWorldPt.z = kWorldPt.z + v62.z;
            }
            p_spShadowCamera->m_pObject->LookAtWorldPoint(kWorldPt, kWorldUp);
            p_spShadowCamera->m_pObject->SetViewFrustum(kFrustum);
            p_spShadowCamera->m_pObject->SetViewPort(kViewport);
            NiAVObject* v34 = p_spShadowCamera->m_pObject;
            kUpdateData.fTime = 0.0f;
            kUpdateData.bUpdateControllers = false;
            kUpdateData.bParallelUpdate = false;
            kUpdateData.bFoundParticles = false;
            kUpdateData.bFoundMorphController = false;
            kUpdateData.bSceneGraphChange = false;
            v34->Update(kUpdateData);
            float m_fNear = kFrustum.m_fNear;
            BSNiBound* v35 = spShadowCaster->m_kWorldBound;
            float z = kFrustum.m_fFar;
            if (!v35) {
                v35 = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
            }
            p_spShadowCamera->m_pObject->FitNearAndFarToBound(*v35);
            NiCamera* v36 = p_spShadowCamera->m_pObject;
            kFrustum = p_spShadowCamera->m_pObject->m_kViewFrustum;
            kFrustum.m_fFar = kFrustum.m_fFar + 400.0;
            if (BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_ISFILTER) {
                kFrustum.m_fNear = kFrustum.m_fFar * 0.20000000298023224;
            }
            kShadowCameraPlanes.Set(kFrustum, v36->m_kWorld);
            NiCamera* v37 = spShadowCamera.m_pObject;
            kFrustum.m_fNear = m_fNear;
            kFrustum.m_fFar = z;
            v37->SetViewFrustum(kFrustum);
            spShadowAccumulator->SetShadowLight(this);
            MTRenderingSystem::Instance.AddAccumTask(
                spShadowCamera.m_pObject,
                nullptr,
                spShadowCaster.m_pObject,
                nullptr,
                nullptr,
                spShadowAccumulator.m_pObject,
                BSCullingProcess::BSCP_CULL_NORMAL,
                aiIndex + 17,
                0
            );
        }
    }
    else {
        if (spDynamicShadowMap.m_pObject) {
            BSShaderManager::pTextureManager->ReturnFrustumShadowTexture(spDynamicShadowMap.m_pObject);
            spDynamicShadowMap = nullptr;
            spShadowAccumulator = nullptr;
        }
    }
    MTRenderingSystem::Instance.SetThreadStage(MTRenderingSystem::THREAD_1, aiIndex + 17);
}

// Renders the shadow caster into the light's depth map from the shadow camera, builds the light's view, projection and
// view-projection matrices from the camera, then blurs the map when shadows are image-space filtered.
void ShadowSceneLight::MTRenderFrustumDepthMap(BSCullingProcess& arCuller, int32_t aiIndex) {
    MTRenderingSystem::Instance.WaitOnThreadStage(MTRenderingSystem::THREAD_2, aiIndex + 17);
    NiColorA kOrgColor = NiColorA::BLACK;
    NiRenderer::ms_pkRenderer->GetBackgroundColor(kOrgColor);
    NiColorA kWhite(1.0f, 1.0f, 1.0f, 1.0f);
    NiRenderer::ms_pkRenderer->SetBackgroundColor(kWhite);
    BSRenderedTexture::StartOffscreen(7u, spDynamicShadowMap->GetGroup());
    NiRenderer::ms_pkRenderer->SetBackgroundColor(kOrgColor);
    arCuller.spAccumulator = spShadowAccumulator.m_pObject;
    BSShaderUtil::RenderScene(spShadowCamera.m_pObject, spShadowAccumulator.m_pObject, false);
    spShadowAccumulator->spShadowLight = nullptr;
    arCuller.spAccumulator = nullptr;
    NiCamera* pCamera = spShadowCamera.m_pObject;
    NiDX9Renderer* pRenderer = BSShaderManager::pRenderer.m_pObject;
    NiPoint3 kCol2;
    kCol2.x = pCamera->m_kWorld.m_Rotate.m_pEntry[0][2];
    kCol2.y = pCamera->m_kWorld.m_Rotate.m_pEntry[1][2];
    kCol2.z = pCamera->m_kWorld.m_Rotate.m_pEntry[2][2];
    NiPoint3 kCol1;
    kCol1.x = pCamera->m_kWorld.m_Rotate.m_pEntry[0][1];
    kCol1.y = pCamera->m_kWorld.m_Rotate.m_pEntry[1][1];
    kCol1.z = pCamera->m_kWorld.m_Rotate.m_pEntry[2][1];
    NiPoint3 kCol0;
    kCol0.x = pCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
    kCol0.y = pCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
    kCol0.z = pCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
    pRenderer->SetCameraData(pCamera);
    NiLight* pLight = spLight.m_pObject;
    alignas(16) D3DXMATRIX viewMatrix = pRenderer->m_kD3DView;
    alignas(16) D3DXMATRIX projMatrix = pRenderer->m_kD3DProj;
    NiTransform kLightTransform;
    kLightTransform = pLight->m_kWorld;
    double y = kLightTransform.m_Translate.y;
    double x = kLightTransform.m_Translate.x;
    double z = kLightTransform.m_Translate.z;
    kShadowView = viewMatrix;
    // Each dot product is stored as a float before it is negated.
    float fDot = y * kCol2.y + x * kCol2.x + z * kCol2.z;
    kShadowView._41 = -fDot;
    fDot = y * kCol1.y + x * kCol1.x + z * kCol1.z;
    kShadowView._42 = -fDot;
    fDot = y * kCol0.y + x * kCol0.x + z * kCol0.z;
    kShadowView._43 = -fDot;
    kShadowProj = projMatrix;
    D3DXMatrixMultiply_Thunk(&projMatrix, &kShadowView, &kShadowProj);
    kViewProjection = projMatrix;
    BSRenderedTexture::StopOffscreen();
    if (BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_ISFILTER) {
        BSRenderedTexture* pBlur = BSShaderManager::pTextureManager->BorrowFrustumShadowTexture();
        ImageSpaceTexture kBlurTexture;
        kBlurTexture.SetTexture(pBlur);
        {
            ImageSpaceEffectParam kParam;
            ImageSpaceEffectBlur::fBlurRadius = 2.0f;
            BSShaderManager::pImageSpaceManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_BLUR5]
                ->SetTexture(2, &kBlurTexture, NiTexturingProperty::FILTER_NEAREST);
            BSShaderManager::pImageSpaceManager->RenderEffect(
                ImageSpaceManager::EFFECT_BLUR5,
                BSShaderManager::pRenderer.m_pObject,
                spDynamicShadowMap.m_pObject,
                spDynamicShadowMap.m_pObject,
                &kParam,
                1
            );
            BSShaderManager::pTextureManager->ReturnFrustumShadowTexture(pBlur);
        }
    }
    if (bShowDebugTexture) {
        NiRenderedTexture* Texture = spDynamicShadowMap->GetTexture(0);
        DebugShader* Shader = static_cast<DebugShader*>(BSShaderManager::GetShader(9));
        Shader->SetTextureToRender(Texture);
    }
    bClipSpacePlanesReady = false;
}

bool ShadowSceneLight::BoundWithinLightFrustum(const NiBound& arBound) {
    uint32_t v4 = 0;
    float* p_z = &kShadowCameraPlanes.m_akCullingPlanes[0].m_kNormal.z;
    do {
        if ((1 << v4 & kShadowCameraPlanes.GetActivePlaneState()) != 0) {
            float arBoundb = p_z[-1] * arBound.m_kCenter.y + p_z[-2] * arBound.m_kCenter.x +
                arBound.m_kCenter.z * p_z[0];
            float arBounda = arBoundb - p_z[1];
            if (-arBound.m_fRadius >= arBounda) {
                return false;
            }
            if (arBound.m_fRadius <= static_cast<double>(arBounda)) {
                kShadowCameraPlanes.m_uiActivePlanes &= ~(1 << v4);
            }
        }
        ++v4;
        p_z += 4;
    }
    while (v4 < 6);
    return true;
}

bool ShadowSceneLight::FrustumFrustumIntersection(NiCamera* apCamera1, NiCamera* apCamera2) {
    // Each camera's frustum points: the eye, then the four far corners.
    NiPoint3 akFrustum1[5];
    NiPoint3 akFrustum2[5];
    // The frustum's eight edges as pairs of point indices.
    uint8_t aucEdges[8][2] = {{0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 2}, {1, 3}, {2, 4}, {3, 4}};
    NiPoint3 kDirection;
    NiPoint3 kUp;
    NiPoint3 kRight;
    NiPoint3 kLocation;
    alignas(8) NiFrustumPlanes kPlanes1;
    alignas(8) NiFrustumPlanes kPlanes2;

    kLocation = apCamera1->GetWorldTranslate();
    kDirection = apCamera1->GetWorldDirection();
    kUp = apCamera1->GetWorldUpVector() * apCamera1->GetViewFrustum().m_fTop;
    kRight = apCamera1->GetWorldRightVector() * apCamera1->GetViewFrustum().m_fRight;
    float fFar = apCamera1->GetViewFrustum().m_fFar;
    akFrustum1[0] = kLocation;
    akFrustum1[1] = kLocation + kDirection * fFar + kUp * fFar + kRight * fFar;
    akFrustum1[2] = kLocation + kDirection * fFar + kUp * fFar - kRight * fFar;
    akFrustum1[3] = kLocation + kDirection * fFar - kUp * fFar + kRight * fFar;
    akFrustum1[4] = kLocation + kDirection * fFar - kUp * fFar - kRight * fFar;

    kLocation = apCamera2->GetWorldTranslate();
    kDirection = apCamera2->GetWorldDirection();
    kUp = apCamera2->GetWorldUpVector() * apCamera2->GetViewFrustum().m_fTop;
    kRight = apCamera2->GetWorldRightVector() * apCamera2->GetViewFrustum().m_fRight;
    fFar = apCamera2->GetViewFrustum().m_fFar;
    akFrustum2[0] = kLocation;
    akFrustum2[1] = kLocation + kDirection * fFar + kUp * fFar + kRight * fFar;
    akFrustum2[2] = kLocation + kDirection * fFar + kUp * fFar - kRight * fFar;
    akFrustum2[3] = kLocation + kDirection * fFar - kUp * fFar + kRight * fFar;
    akFrustum2[4] = kLocation + kDirection * fFar - kUp * fFar - kRight * fFar;

    alignas(8) NiCullingProcess kCuller(nullptr);
    kCuller.SetCamera(apCamera1);
    kCuller.SetFrustum(apCamera1->GetViewFrustum());
    kPlanes1 = kCuller.GetFrustumPlanes();
    kCuller.SetCamera(apCamera2);
    kCuller.SetFrustum(apCamera2->GetViewFrustum());
    kPlanes2 = kCuller.GetFrustumPlanes();

    // Each frustum's points against the other frustum's planes: a point inside all six planes intersects.
    alignas(8) NiFrustumPlanes kPlanes;
    NiPoint3* pkFrustum = nullptr;
    for (int32_t i = 0; i < 2; ++i) {
        switch (i) {
            case 0: {
                pkFrustum = akFrustum1;
                kPlanes = kPlanes2;
                break;
            }
            case 1: {
                pkFrustum = akFrustum2;
                kPlanes = kPlanes1;
                break;
            }
        }
        for (int32_t j = 0; j < 5; ++j) {
            bool bInside = true;
            for (uint32_t k = 0; k < NiFrustumPlanes::MAX_PLANES; ++k) {
                if (kPlanes.GetPlane(k).WhichSide(pkFrustum[j]) != NiPlane::POSITIVE_SIDE) {
                    bInside = false;
                    break;
                }
            }
            if (bInside) {
                return true;
            }
        }
    }

    // Each frustum's edges against the other frustum's planes: an edge lying in a plane, or crossing one at a point
    // inside the other five, intersects.
    for (int32_t i = 0; i < 2; ++i) {
        switch (i) {
            case 0: {
                pkFrustum = akFrustum1;
                kPlanes = kPlanes2;
                break;
            }
            case 1: {
                pkFrustum = akFrustum2;
                kPlanes = kPlanes1;
                break;
            }
        }
        for (int32_t j = 0; j < 8; ++j) {
            NiPoint3 kStart = pkFrustum[aucEdges[j][0]];
            NiPoint3 kEnd = pkFrustum[aucEdges[j][1]];
            for (uint32_t k = 0; k < NiFrustumPlanes::MAX_PLANES; ++k) {
                const NiPlane& kPlane = kPlanes.GetPlane(k);
                // The start point's distance is summed at double precision.
                float fStartDistance = static_cast<double>(kStart.x) * kPlane.m_kNormal.x +
                    kPlane.m_kNormal.y * static_cast<double>(kStart.y) +
                    static_cast<double>(kStart.z) * kPlane.m_kNormal.z - kPlane.m_fConstant;
                int32_t iStartSide = NiPlane::NO_SIDE;
                if (fStartDistance < 0.0f) {
                    iStartSide = NiPlane::NEGATIVE_SIDE;
                }
                else if (fStartDistance > 0.0f) {
                    iStartSide = NiPlane::POSITIVE_SIDE;
                }
                int32_t iEndSide = kPlane.WhichSide(kEnd);
                if (iStartSide == iEndSide) {
                    if (iStartSide == NiPlane::NO_SIDE && iEndSide == NiPlane::NO_SIDE) {
                        return true;
                    }
                }
                else {
                    NiPoint3 kNormal = kPlane.m_kNormal;
                    float fConstant = kPlane.m_fConstant;
                    NiPoint3 kToPlane = kNormal * fConstant - kStart;
                    NiPoint3 kEdge = kEnd - kStart;
                    float fT = kToPlane.Dot(kNormal) / kEdge.Dot(kNormal);
                    NiPoint3 kIntersection = kStart + fT * kEdge;
                    if (fT >= 0.0f && fT <= 1.0f) {
                        bool bInside = true;
                        for (uint32_t m = 0; m < NiFrustumPlanes::MAX_PLANES; ++m) {
                            if (m != k) {
                                const NiPlane& kOther = kPlanes.GetPlane(m);
                                // The z term is added at double precision.
                                float fDistance = kOther.m_kNormal.x * kIntersection.x +
                                    kOther.m_kNormal.y * kIntersection.y +
                                    static_cast<double>(kIntersection.z) * kOther.m_kNormal.z - kOther.m_fConstant;
                                if (fDistance < 0.0f) {
                                    bInside = false;
                                    break;
                                }
                            }
                        }
                        if (bInside) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool ShadowSceneLight::CubeFaceFrustumFrustumIntersection(
    NiBound aBound,
    NiCullingProcess& arCuller,
    uint16_t ausCubeFace
) {
    NiUpdateData updateData;
    NiCamera* m_pkCamera = arCuller.m_pkCamera;
    AutoMemContext kMemContext(MEM_CONTEXT_SHADOWS);
    bool v8;
    BSCubeMapCamera kCamera(0);
    kCamera.m_kLocal.m_Translate = aBound.m_kCenter;
    kCamera.SetupTransformation(static_cast<NiRenderedCubeMap::FaceID>(ausCubeFace));
    updateData.fTime = 0.0;
    updateData.bUpdateControllers = false;
    updateData.bParallelUpdate = false;
    updateData.bFoundParticles = false;
    updateData.bFoundMorphController = false;
    updateData.bSceneGraphChange = false;
    kCamera.Update(updateData);
    v8 = FrustumFrustumIntersection(m_pkCamera, &kCamera);
    return v8;
}

void ShadowSceneLight::ClearRenderPasses() {
    NiTListItem<NiPointer<NiGeometry>>* pPos = kGeomListFence;
    while (pPos) {
        NiGeometry* pGeometry = pPos->m_element.m_pObject;
        pPos = pPos->m_pkNext;
        if (pGeometry) {
            BSShaderProperty* pProperty = static_cast<BSShaderProperty*>(
                pGeometry->m_PropertyState.m_aspProps[3].m_pObject
            );
            if (pProperty) {
                // The shade property is a lighting property when its shader type is 1 to 12 (one unsigned compare); the
                // pointer or null is selected, then tested.
                BSShaderLightingProperty* pLighting = static_cast<uint32_t>(pProperty->iShaderPropertyType - 1) <=
                        BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_LAST - 1
                    ? static_cast<BSShaderLightingProperty*>(pProperty)
                    : nullptr;
                if (pLighting) {
                    pLighting->iLastRenderPassState = 0;
                }
            }
        }
    }
    kGeomListFence = nullptr;
}

int32_t ShadowSceneLight::SortByLuminance(const void* appLight1, const void* appLight2) {
    const float fFirst = (*static_cast<ShadowSceneLight* const*>(appLight1))->fLuminance;
    const float fSecond = (*static_cast<ShadowSceneLight* const*>(appLight2))->fLuminance;
    if (fSecond < fFirst) {
        return -1;
    }
    return fSecond > fFirst;
}

bool ShadowSceneLight::IsEqual(ShadowSceneLight** appLight1, ShadowSceneLight** appLight2) {
    return *appLight1 == *appLight2;
}
