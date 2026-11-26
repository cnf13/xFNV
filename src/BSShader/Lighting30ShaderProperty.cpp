#include "BSShader/Lighting30ShaderProperty.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI Lighting30ShaderProperty::ms_RTTI("Lighting30ShaderProperty", &BSShaderPPLightingProperty::ms_RTTI);

Lighting30ShaderProperty::Lighting30ShaderProperty() {
    pRenderDepthPass = nullptr;
}

Lighting30ShaderProperty::Lighting30ShaderProperty(BSShaderPPLightingProperty* aPPLightingProp) {
    aPPLightingProp->CopyTo(this);
    iShader = 29;
    iShaderPropertyType = 12;
}

Lighting30ShaderProperty::~Lighting30ShaderProperty() {}

// The clone is built while the thread's heap context is MEM_CONTEXT_HAIRSHADER.
NiObject* Lighting30ShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext memContext(MEM_CONTEXT_HAIRSHADER);
    Lighting30ShaderProperty* pClone = new Lighting30ShaderProperty;
    CopyMembers(pClone, arCloneData);
    return pClone;
}

void Lighting30ShaderProperty::LoadBinary(NiStream& kStream) {
    BSShaderPPLightingProperty::LoadBinary(kStream);
}

void Lighting30ShaderProperty::LinkObject(NiStream& kStream) {
    BSShaderPPLightingProperty::LinkObject(kStream);
}

bool Lighting30ShaderProperty::RegisterStreamables(NiStream& kStream) {
    return BSShaderPPLightingProperty::RegisterStreamables(kStream);
}

void Lighting30ShaderProperty::SaveBinary(NiStream& kStream) {
    BSShaderPPLightingProperty::SaveBinary(kStream);
}

void Lighting30ShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderPPLightingProperty::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

void Lighting30ShaderProperty::PostLinkObject(NiStream& kStream) {
    BSShaderPPLightingProperty::PostLinkObject(kStream);
}

bool Lighting30ShaderProperty::SetupGeometry(NiGeometry* apGeometry) {
    bool bResult = BSShaderPPLightingProperty::SetupGeometry(apGeometry);
    // BSSP_FLAG_DYNAMICALPHA
    if (ulFlags[0] & BSS1_Dynamic_Alpha) {
        fDepthBias = 1.0f;
    }
    NiProperty* pProperty = apGeometry->GetProperty(NiProperty::VERTEX_COLOR);
    if (pProperty) {
        apGeometry->DetachProperty(pProperty);
    }
    return bResult;
}

// The passes of a geometry for the accumulator's render mode. The depth-map, constant-alpha, local-map and silhouette
// modes build their lists once. The normal mode measures the camera distance, drops the cached list when the distance
// crosses an LOD end (eye or plain env map, decal, specular), fades decals out, refreshes the lights of the cached
// passes, and rebuilds the list when the state (enabled passes and mode) differs from the last one built.
BSShaderProperty::RenderPassArray* Lighting30ShaderProperty::GetRenderPasses(
    NiGeometry* apGeom,
    int32_t aiEnabledPasses,
    uint16_t& asCount,
    uint32_t auiRenderMode,
    BSShaderAccumulator* apAccumulator,
    int32_t abUpdateList
) {
    const uint32_t uiState = static_cast<uint32_t>(aiEnabledPasses) | (auiRenderMode << 8);
    const uint32_t uiFlags1 = ulFlags[1];
    const bool bStinger = (uiFlags1 & BSS2_stinger_prop) != 0;
    bool bEnvMapInRange = true;
    bool bDecalInRange = true;
    bool bFaded = false;

    switch (auiRenderMode) {
        case 7: {
            const uint32_t uiFlags0 = ulFlags[0];
            if ((uiFlags0 & BSS1_Refraction) || (uiFlags0 & BSS1_Fire_Refraction)) {
                return nullptr;
            }
            InitRenderPassArray_depthMap(1);
            RenderPassArray* pArray = pDepthMapRenderPassList;
            if (pArray->iSize) {
                RenderPass* pPass = pArray->pPassA.m_pBase[0];
                if (pPass->ucNumLights) {
                    pPass->ppSceneLights[0] = static_cast<BSShaderAccumulator*>(
                                                  NiRenderer::ms_pkRenderer->m_spAccum.m_pObject
                    )
                                                  ->spShadowLight.m_pObject;
                }
            }
            else if (apGeom->m_spModelData->m_pkColor) {
                const bool bSkinned = GetFlag(BSSP_SKINNED);
                ShadowSceneLight* pLight = static_cast<BSShaderAccumulator*>(
                                               NiRenderer::ms_pkRenderer->m_spAccum.m_pObject
                )
                                               ->spShadowLight.m_pObject;
                pArray->Add(
                    apGeom,
                    bSkinned ? BSShaderManager::BSSM_3XDEPTHMAP_VcS : BSShaderManager::BSSM_3XDEPTHMAP_Vc,
                    true,
                    1,
                    pLight,
                    nullptr,
                    nullptr,
                    nullptr
                );
            }
            else {
                const bool bSkinned = GetFlag(BSSP_SKINNED);
                ShadowSceneLight* pLight = static_cast<BSShaderAccumulator*>(
                                               NiRenderer::ms_pkRenderer->m_spAccum.m_pObject
                )
                                               ->spShadowLight.m_pObject;
                pArray->Add(
                    apGeom,
                    bSkinned ? BSShaderManager::BSSM_3XDEPTHMAP_S : BSShaderManager::BSSM_3XDEPTHMAP,
                    true,
                    1,
                    pLight,
                    nullptr,
                    nullptr,
                    nullptr
                );
            }
            return pDepthMapRenderPassList;
        }
        case 8: {
            const uint32_t uiFlags0 = ulFlags[0];
            // Refraction, fire refraction or refraction tint: the list as it is.
            if ((uiFlags0 & BSS1_Refraction) ||
                (uiFlags0 & BSS1_Fire_Refraction) ||
                (uiFlags1 & BSS2_Refraction_Tint)) {
                return pConstAlphaRenderPassList;
            }
            InitRenderPassArray_constAlpha(1);
            RenderPassArray* pArray = pConstAlphaRenderPassList;
            if (pArray->iSize) {
                RenderPass* pPass = pArray->pPassA.m_pBase[0];
                if (pPass->ucNumLights) {
                    pPass->ppSceneLights[0] = static_cast<BSShaderAccumulator*>(
                                                  NiRenderer::ms_pkRenderer->m_spAccum.m_pObject
                    )
                                                  ->spShadowLight.m_pObject;
                }
            }
            else {
                const bool bVertexColors = apGeom->m_spModelData->m_pkColor != nullptr;
                const bool bSkinned = GetFlag(BSSP_SKINNED);
                ShadowSceneLight* pLight = static_cast<BSShaderAccumulator*>(
                                               NiRenderer::ms_pkRenderer->m_spAccum.m_pObject
                )
                                               ->spShadowLight.m_pObject;
                uint16_t usPass;
                if (bSkinned) {
                    usPass = bVertexColors ? BSShaderManager::BSSM_3XCONSTALPHA_SVc
                                           : BSShaderManager::BSSM_3XCONSTALPHA_S;
                }
                else {
                    usPass = bVertexColors ? BSShaderManager::BSSM_3XCONSTALPHA_Vc : BSShaderManager::BSSM_3XCONSTALPHA;
                }
                pArray->Add(apGeom, usPass, true, 1, pLight, nullptr, nullptr, nullptr);
            }
            return pConstAlphaRenderPassList;
        }
        case 9: {
            InitRenderPassArray_localMap(1);
            RenderPassArray* pArray = pLocalMapRenderPassList;
            if (!pArray->iSize) {
                if (ulFlags[0] & BSS1_Localmap_Hide_Secret) {
                    pArray->Add(
                        apGeom,
                        BSShaderManager::BSSM_3XLOCALMAP_CLEAR,
                        true,
                        0,
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr
                    );
                }
                else {
                    pArray->Add(
                        apGeom,
                        GetFlag(BSSP_SKINNED) ? BSShaderManager::BSSM_3XLOCALMAP_S : BSShaderManager::BSSM_3XLOCALMAP,
                        true,
                        0,
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr
                    );
                }
            }
            return pLocalMapRenderPassList;
        }
        case 15: {
            CheckSilhouetteRenderPassArray(1);
            RenderPassArray* pArray = pSilhouettePassList;
            if (!pArray->iSize && !apGeom->m_spSkinInstance.m_pObject) {
                pArray->Add(
                    apGeom,
                    GetFlag(BSSP_FLAG_VERTEXCOLORS) ? 444 : 443,
                    true,
                    0,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr
                );
            }
            return pSilhouettePassList;
        }
        default: break;
    }

    // The LOD fades that apply, from the flags and the LOD ends.
    const float fOldDistance = fCameraDistance;
    const uint32_t uiFlags0 = ulFlags[0];
    const bool bEnvMap = (uiFlags0 & BSS1_Environment_Mapping) && BSShaderManager::fEnvMapLODEnd > 0.0f;
    const bool bEyeEnvMap = (uiFlags0 & BSS1_Eye_Environment_Mapping) && BSShaderManager::fEyeEnvMapLODEnd > 0.0f;
    const bool bDecal = ((uiFlags0 & BSS1_Decal_Single_Pass) || GetFlag(BSSP_FLAG_DYNAMIC_DECAL)) &&
        BSShaderManager::fDecalLODEnd > 0.0f &&
        !GetFlag(BSSP_FLAG_NOFADE);
    bool bSpecular = (uiFlags0 & BSS1_Specular) && BSShaderManager::fSpecularLODEnd > 0.0f;

    if (bEyeEnvMap || bEnvMap || bDecal || bSpecular) {
        const NiPoint3 kEyePos = apAccumulator->pActiveShadowSceneNode->kEyePosition;
        const NiBound* pWorldBound = apGeom->m_kWorldBound;
        if (!pWorldBound) {
            pWorldBound = &NiAVObject::NullBoundS;
        }
        const NiBound kBound = *pWorldBound;
        NiPoint3 kDiff = kBound.m_kCenter - kEyePos;
        fCameraDistance = kDiff.Length() - kBound.m_fRadius;
        if (BSShaderManager::GetAccumulator()->m_pkCamera) {
            fCameraDistance = BSShaderManager::GetAccumulator()->m_pkCamera->m_fLODAdjust * fCameraDistance;
        }

        // A distance that crossed an LOD end since the last call, either way, drops the cached list.
        if (fOldDistance != fCameraDistance) {
            if (bEyeEnvMap) {
                const float fEnd = BSShaderManager::fEyeEnvMapLODEnd;
                if ((fEnd > fCameraDistance && fEnd <= fOldDistance) ||
                    (fCameraDistance >= fEnd && fEnd > fOldDistance)) {
                    iLastRenderPassState = 0;
                }
            }
            else if (bEnvMap) {
                const float fEnd = BSShaderManager::fEnvMapLODEnd;
                if ((fEnd > fCameraDistance && fEnd <= fOldDistance) ||
                    (fCameraDistance >= fEnd && fEnd > fOldDistance)) {
                    iLastRenderPassState = 0;
                }
            }
            if (bDecal) {
                const float fEnd = BSShaderManager::fDecalLODEnd;
                if ((fEnd > fCameraDistance && fEnd <= fOldDistance) ||
                    (fCameraDistance >= fEnd && fEnd > fOldDistance)) {
                    iLastRenderPassState = 0;
                }
            }
            if (bSpecular && !bStinger) {
                const float fEnd = BSShaderManager::fSpecularLODEnd;
                if ((fEnd > fCameraDistance && fEnd <= fOldDistance) ||
                    (fCameraDistance >= fEnd && fEnd > fOldDistance)) {
                    iLastRenderPassState = 0;
                }
                if (fCameraDistance >= BSShaderManager::fSpecularLODEnd) {
                    bSpecular = false;
                }
            }
        }
    }
    if (!bStinger && BSShaderManager::fEnvMapLODEnd != 0.0f && fCameraDistance >= BSShaderManager::fEnvMapLODEnd) {
        bEnvMapInRange = false;
    }

    // The alpha: decals fade out between fDecalLODStartFade and fDecalLODEnd and are dropped past it.
    NiPropertyState* pState = &apGeom->m_PropertyState;
    if (pState) {
        float fNewAlpha = 1.0f;
        NiMaterialProperty* pMaterial = static_cast<NiMaterialProperty*>(
            pState->m_aspProps[NiProperty::MATERIAL].m_pObject
        );
        if (!pMaterial) {
            fNewAlpha = fFadeAlpha;
        }
        else if (!bDecal) {
            fNewAlpha = pMaterial->m_fAlpha * fFadeAlpha;
        }
        else if (BSShaderManager::fDecalLODEnd != 0.0f && fCameraDistance >= BSShaderManager::fDecalLODEnd) {
            bDecalInRange = false;
        }
        else if (
            BSShaderManager::fDecalLODStartFade != 0.0f &&
            BSShaderManager::fDecalLODEnd != 0.0f &&
            fCameraDistance >= BSShaderManager::fDecalLODStartFade &&
            fCameraDistance < BSShaderManager::fDecalLODEnd
        ) {
            fNewAlpha = FLerp(
                0.0f,
                1.0f,
                BSShaderManager::fDecalLODEnd,
                BSShaderManager::fDecalLODStartFade,
                fCameraDistance
            );
        }
        else {
            fNewAlpha = pMaterial->m_fAlpha * fFadeAlpha;
        }
        SetAlpha(fNewAlpha);
        if (fNewAlpha < 1.0f) {
            bFaded = true;
        }
    }

    if (bLightListChanged) {
        const NiBound* pWorldBound = apGeom->m_kWorldBound;
        if (!pWorldBound) {
            pWorldBound = &NiAVObject::NullBoundS;
        }
        NiBound kObjBound = *pWorldBound;
        ResortLights(&kObjBound);
    }

    // The same state with new lights: put the lights into the cached passes.
    if (iLastRenderPassState == uiState && bLightListChanged) {
        ShadowSceneLight* pSunLight = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->pSunLight;
        for (uint32_t i = 0; i < pRenderPassList->iSize; ++i) {
            RenderPass* pPass = pRenderPassList->pPassA.m_pBase[i];
            const uint16_t usPass = pPass->usPassEnum;
            if (usPass >= BSShaderManager::BSSM_3XLIGHTING && usPass <= BSShaderManager::BSSM_3XLIGHTING_VcSGShpSpc) {
                void* pIter;
                uint8_t ucNewLights = 1;
                if (GetFirstActiveNonShadowLight(pIter)) {
                    do {
                        ++ucNewLights;
                    }
                    while (GetNextActiveNonShadowLight(pIter));
                    if (!ucNewLights) {
                        pPass->ucNumLights = 0;
                        continue;
                    }
                }
                if (pPass->ucMaxNumLights < ucNewLights) {
                    pPass->ClearSceneLights();
                    pPass->AllocateLightArray(ucNewLights);
                    pPass->ucMaxNumLights = ucNewLights;
                }
                ShadowSceneLight* pLight = GetFirstActiveNonShadowLight(pIter);
                pPass->ppSceneLights[0] = pSunLight;
                int32_t iLight = 1;
                for (; pLight && iLight < ucNewLights; pLight = GetNextActiveNonShadowLight(pIter)) {
                    pPass->ppSceneLights[iLight++] = pLight;
                }
                pPass->ucNumLights = static_cast<uint8_t>(iLight);
            }
            else if (usPass >= BSShaderManager::BSSM_3XENVMAP && usPass <= BSShaderManager::BSSM_3XENVMAP_EYE) {
                if (pPass->ucNumLights == 1) {
                    void* pIter;
                    ShadowSceneLight* pLight = GetFirstActiveNonShadowLight(pIter);
                    pPass->ppSceneLights[0] = pLight ? pLight : pSunLight;
                }
            }
            else if (
                (usPass >= BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW &&
                 usPass <= BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_VcS) &&
                (pPass->ucNumLights == 1)
            ) {
                void* pIter;
                ShadowSceneLight* pLight = GetFirstActiveLight(pIter);
                // No shadow-casting light left, or its light is culled: rebuild the list.
                if (!pLight || !pLight->bCastShadow || (pLight->spLight->m_uFlags & NiAVObject::APP_CULLED)) {
                    iLastRenderPassState = 0;
                    break;
                }
                pPass->ppSceneLights[0] = pLight;
            }
        }
        bLightListChanged = false;
    }

    if (iLastRenderPassState == uiState) {
        return pRenderPassList;
    }

    CheckCreateRenderPassArray(-1);
    pRenderPassList->iSize = 0;
    bLightListChanged = false;
    const bool bLit = !(ulFlags[0] & BSS1_Localmap_Hide_Secret) || BSShaderManager::bLiteBrite;
    const bool bDecalDrawn = bDecalInRange && (GetFlag(BSSP_FLAG_DECAL) || GetFlag(BSSP_FLAG_DYNAMIC_DECAL));
    if (fAlpha == 0.0f || !bLit || (bDecal && !bDecalDrawn)) {
        return pRenderPassList;
    }

    const bool bSkinned = GetFlag(BSSP_SKINNED);
    const bool bEnvMapPass = bEnvMapInRange && GetFlag(BSSP_ENVMAP);
    const bool bEyeReflect = GetFlag(BSSP_FLAG_EYEREFLECT);
    const bool bWindowReflect = GetFlag(BSSP_FLAG_WINDOWREFLECT);
    const bool bRefraction = GetFlag(BSSP_FLAG_REFRACTION);
    const bool bRefractionFire = GetFlag(BSSP_FLAG_REFRACTION_FIRE);
    const bool bParallax = GetFlag(BSSP_FLAG_PARALLAX);
    const bool bParallaxOcc = GetFlag(BSSP_FLAG_PARALLAXOCCLUSION);
    const bool bFaceGen = GetFlag(BSSP_FLAG_FACEGENBLEND);
    const bool bGlowMap = spTextures[TEXTYPE_GLOW][0].m_pObject != nullptr;
    const bool bSpecularPass = GetFlag(BSSP_SPECULAR) && (uiState & 8) && bSpecular;
    // Reads the alpha property without a null test.
    const bool bAlphaBlend = bFaded ||
        (static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[NiProperty::ALPHA].m_pObject)->m_uFlags &
         NiAlphaProperty::ALPHA_BLEND_MASK);
    bool bProjShadow = BSShaderManager::GetUseProjectedShadow() &&
        !GetFlag(BSSP_FLAG_NOPROJSHADOW) &&
        ShadowLightShader::GetProjectedShadowScale() &&
        BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->cSceneGraphIndex != 1 &&
        BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->cSceneGraphIndex != 3;
    const bool bVertexColors = apGeom->m_spModelData->m_pkColor || GetFlag(BSSP_FLAG_VERTEXCOLORS);
    if (spTexEffectData.m_pObject) {
        bProjShadow = false;
    }

    if (bRefraction) {
        if (bRefractionFire) {
            pRenderPassList
                ->Add(apGeom, BSShaderManager::BSSM_3XRENDERNORMALS_FIRE, true, 0, nullptr, nullptr, nullptr, nullptr);
        }
        else {
            pRenderPassList->Add(
                apGeom,
                bVertexColors ? BSShaderManager::BSSM_3XRENDERNORMALS_Vc + 2 * bSkinned
                              : BSShaderManager::BSSM_3XRENDERNORMALS + 2 * bSkinned,
                true,
                0,
                nullptr,
                nullptr,
                nullptr,
                nullptr
            );
        }
        pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint = BSSP_HINT_GROUP_REFRACT;
        iLastRenderPassState = uiState;
        return pRenderPassList;
    }

    if (ulFlags[1] & BSS2_1st_person) {
        pRenderPassList->Add(
            apGeom,
            BSShaderManager::BSSM_3XRENDERNORMALS_CLEAR + bSkinned,
            true,
            0,
            nullptr,
            nullptr,
            nullptr,
            nullptr
        );
        pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]
            ->eAccumulationHint = BSSP_HINT_GROUP_1STPERSONNOREFRACT;
    }

    // The main pass: the sun first, then every active light that casts no shadow.
    ShadowSceneLight* pSunLight = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->pSunLight;
    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_EMPTY, true, 0, nullptr, nullptr, nullptr, nullptr);
    RenderPass* pPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
    void* pIter;
    pPass->ucNumLights = 1;
    for (ShadowSceneLight* pLight = GetFirstActiveLight(pIter); pLight; pLight = GetNextActiveLight(pIter)) {
        if (!pLight->bCastShadow) {
            ++pPass->ucNumLights;
        }
    }
    if (pPass->ucNumLights) {
        if (pPass->ucMaxNumLights < pPass->ucNumLights) {
            pPass->ClearSceneLights();
            pPass->AllocateLightArray(pPass->ucNumLights);
            pPass->ucMaxNumLights = pPass->ucNumLights;
        }
        ShadowSceneLight* pLight = GetFirstActiveLight(pIter);
        pPass->ppSceneLights[0] = pSunLight;
        uint8_t ucLight = 1;
        for (; pLight; pLight = GetNextActiveLight(pIter)) {
            if (!pLight->bCastShadow) {
                pPass->ppSceneLights[ucLight++] = pLight;
            }
        }
        pPass->ucNumLights = ucLight;
    }
    else {
        pPass->ppSceneLights = nullptr;
    }
    SetFlag(BSSP_FLAG_SHADOWMAP, false);

    // The main pass's shader. Skinned parallax has none of its own and keeps pass 0.
    if (GetFlag(BSSP_FLAG_HAIR)) {
        if (bSpecularPass) {
            pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_HShpSpc
                                            : BSShaderManager::BSSM_3XLIGHTING_HSpc;
        }
        else {
            pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_HShp
                                            : BSShaderManager::BSSM_3XLIGHTING_H;
        }
    }
    else if (bSpecularPass) {
        if (bSkinned) {
            if (bParallax) {}
            else if (bFaceGen) {
                pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_SFgShpSpc
                                                : BSShaderManager::BSSM_3XLIGHTING_SFgSpc;
            }
            else if (bGlowMap) {
                if (bVertexColors) {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_VcSGShpSpc
                                                    : BSShaderManager::BSSM_3XLIGHTING_VcSGSpc;
                }
                else {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_SGShpSpc
                                                    : BSShaderManager::BSSM_3XLIGHTING_SGSpc;
                }
            }
            else {
                if (bVertexColors) {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_VcSShpSpc
                                                    : BSShaderManager::BSSM_3XLIGHTING_VcSSpc;
                }
                else {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_SShpSpc
                                                    : BSShaderManager::BSSM_3XLIGHTING_SSpc;
                }
            }
        }
        else {
            if (bParallax) {
                if (bVertexColors) {
                    if (bProjShadow) {
                        pPass->usPassEnum = bParallaxOcc ? BSShaderManager::BSSM_3XLIGHTING_VcPxoShpSpc
                                                         : BSShaderManager::BSSM_3XLIGHTING_VcPxShpSpc;
                    }
                    else {
                        pPass->usPassEnum = bParallaxOcc ? BSShaderManager::BSSM_3XLIGHTING_VcPxoSpc
                                                         : BSShaderManager::BSSM_3XLIGHTING_VcPxSpc;
                    }
                }
                else {
                    if (bProjShadow) {
                        pPass->usPassEnum = bParallaxOcc ? BSShaderManager::BSSM_3XLIGHTING_PxoShpSpc
                                                         : BSShaderManager::BSSM_3XLIGHTING_PxShpSpc;
                    }
                    else {
                        pPass->usPassEnum = bParallaxOcc ? BSShaderManager::BSSM_3XLIGHTING_PxoSpc
                                                         : BSShaderManager::BSSM_3XLIGHTING_PxSpc;
                    }
                }
            }
            else if (bFaceGen) {
                pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_FgShpSpc
                                                : BSShaderManager::BSSM_3XLIGHTING_FgSpc;
            }
            else if (bGlowMap) {
                if (bVertexColors) {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_VcGShpSpc
                                                    : BSShaderManager::BSSM_3XLIGHTING_VcGSpc;
                }
                else {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_GShpSpc
                                                    : BSShaderManager::BSSM_3XLIGHTING_GSpc;
                }
            }
            else {
                if (bVertexColors) {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_VcShpSpc
                                                    : BSShaderManager::BSSM_3XLIGHTING_VcSpc;
                }
                else {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_ShpSpc
                                                    : BSShaderManager::BSSM_3XLIGHTING_Spc;
                }
            }
        }
    }
    else {
        if (bSkinned) {
            if (bParallax) {}
            else if (bFaceGen) {
                pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_SFgShp
                                                : BSShaderManager::BSSM_3XLIGHTING_SFg;
            }
            else if (bGlowMap) {
                if (bVertexColors) {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_VcSGShp
                                                    : BSShaderManager::BSSM_3XLIGHTING_VcSG;
                }
                else {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_SGShp
                                                    : BSShaderManager::BSSM_3XLIGHTING_SG;
                }
            }
            else {
                if (bVertexColors) {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_VcSShp
                                                    : BSShaderManager::BSSM_3XLIGHTING_VcS;
                }
                else {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_SShp
                                                    : BSShaderManager::BSSM_3XLIGHTING_S;
                }
            }
        }
        else {
            if (bParallax) {
                if (bVertexColors) {
                    if (bProjShadow) {
                        pPass->usPassEnum = bParallaxOcc ? BSShaderManager::BSSM_3XLIGHTING_VcPxoShp
                                                         : BSShaderManager::BSSM_3XLIGHTING_VcPxShp;
                    }
                    else {
                        pPass->usPassEnum = bParallaxOcc ? BSShaderManager::BSSM_3XLIGHTING_VcPxo
                                                         : BSShaderManager::BSSM_3XLIGHTING_VcPx;
                    }
                }
                else {
                    if (bProjShadow) {
                        pPass->usPassEnum = bParallaxOcc ? BSShaderManager::BSSM_3XLIGHTING_PxoShp
                                                         : BSShaderManager::BSSM_3XLIGHTING_PxShp;
                    }
                    else {
                        pPass->usPassEnum = bParallaxOcc ? BSShaderManager::BSSM_3XLIGHTING_Pxo
                                                         : BSShaderManager::BSSM_3XLIGHTING_Px;
                    }
                }
            }
            else if (bFaceGen) {
                pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_FgShp
                                                : BSShaderManager::BSSM_3XLIGHTING_Fg;
            }
            else if (bGlowMap) {
                if (bVertexColors) {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_VcGShp
                                                    : BSShaderManager::BSSM_3XLIGHTING_VcG;
                }
                else {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_GShp
                                                    : BSShaderManager::BSSM_3XLIGHTING_G;
                }
            }
            else {
                if (bVertexColors) {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_VcShp
                                                    : BSShaderManager::BSSM_3XLIGHTING_Vc;
                }
                else {
                    pPass->usPassEnum = bProjShadow ? BSShaderManager::BSSM_3XLIGHTING_Shp
                                                    : BSShaderManager::BSSM_3XLIGHTING;
                }
            }
        }
    }

    if (GetFlag(BSSP_FLAG_DECAL)) {
        pPass->eAccumulationHint = GetFlag(BSSP_FLAG_ALPHA_DECAL) ? BSSP_HINT_SORT : BSSP_HINT_GROUP_DECAL;
    }
    else if (GetFlag(BSSP_FLAG_DYNAMIC_DECAL)) {
        pPass->eAccumulationHint = GetFlag(BSSP_FLAG_ALPHA_DECAL) ? BSSP_HINT_SORT : BSSP_HINT_GROUP_DYNAMIC_DECAL;
    }
    else if (bAlphaBlend) {
        // NaN sorts.
        pPass->eAccumulationHint = fFadeAlpha < 1.0f ? BSSP_HINT_GROUP_FADING : BSSP_HINT_SORT;
    }
    else if (
        apAccumulator->eAccumulationMode == BSShaderAccumulator::BSSA_ACCUMLOD ||
        GetFlag(BSSP_FLAG_LODBUILDING) ||
        GetFlag(BSSP_FLAG_LODLANDSCAPE)
    ) {
        pPass->eAccumulationHint = BSSP_HINT_GROUP_LOD;
    }

    // One shadow pass per active shadow-casting light.
    if (BSShaderManager::GetShadows()) {
        for (ShadowSceneLight* pLight = GetFirstActiveLight(pIter); pLight; pLight = GetNextActiveLight(pIter)) {
            if (!pLight->bCastShadow) {
                continue;
            }
            pRenderPassList->Add(apGeom, BSShaderManager::BSSM_EMPTY, false, 1, pLight, nullptr, nullptr, nullptr);
            RenderPass* pShadowPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
            pShadowPass->usPassEnum = bVertexColors ? BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_Vc + 2 * bSkinned
                                                    : BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW + 2 * bSkinned;
            if (GetFlag(BSSP_FLAG_DECAL)) {
                pShadowPass->eAccumulationHint = GetFlag(BSSP_FLAG_ALPHA_DECAL) ? BSSP_HINT_SORT
                                                                                : BSSP_HINT_GROUP_DECAL;
            }
            else if (GetFlag(BSSP_FLAG_DYNAMIC_DECAL)) {
                pShadowPass->eAccumulationHint = GetFlag(BSSP_FLAG_ALPHA_DECAL) ? BSSP_HINT_SORT
                                                                                : BSSP_HINT_GROUP_DYNAMIC_DECAL;
            }
        }
    }

    BSShaderPPLightingProperty::TextureEffectData* pTexEffectData = spTexEffectData.m_pObject;
    if (pTexEffectData && pTexEffectData->spBlockOutTexture.m_pObject) {
        pRenderPassList->Add(apGeom, bSkinned + 14, true, 0, nullptr, nullptr, nullptr, nullptr);
        pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint = BSSP_HINT_GROUP_FADING;
    }
    else if (GetFlag(BSSP_ZPREPASS) || fFadeAlpha < 1.0f) {
        NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(
            apGeom->m_PropertyState.m_aspProps[NiProperty::ALPHA].m_pObject
        );
        if (!pAlpha || !(pAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK)) {
            pRenderPassList->Add(
                apGeom,
                bVertexColors ? bSkinned + 12 : bSkinned + 10,
                true,
                0,
                nullptr,
                nullptr,
                nullptr,
                nullptr
            );
            RenderPass* pDepthPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
            pDepthPass->eAccumulationHint = GetFlag(BSSP_ZPREPASS) ? BSSP_HINT_SORT : BSSP_HINT_GROUP_FADING;
            if (fFadeAlpha < 1.0f || fAlpha < 1.0f) {
                pDepthPass->eAccumulationHint = BSSP_HINT_GROUP_FADING;
            }
        }
    }

    if (spTexEffectData.m_pObject) {
        pRenderPassList
            ->Add(apGeom, BSShaderManager::BSSM_3XTEXEFFECT + bSkinned, true, 0, nullptr, nullptr, nullptr, nullptr);
        pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint = BSSP_HINT_SORT;
    }

    if (bEnvMapPass) {
        RenderPass* pEnvPass;
        if (bWindowReflect) {
            const uint16_t usPass = BSShaderManager::BSSM_3XENVMAP_W + 2 * bVertexColors;
            pRenderPassList->Add(apGeom, BSShaderManager::BSSM_EMPTY, false, 0, nullptr, nullptr, nullptr, nullptr);
            pEnvPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
            pEnvPass->usPassEnum = usPass;
        }
        else if (bEyeReflect) {
            ShadowSceneLight* pLight = GetFirstActiveLight(pIter);
            pRenderPassList->Add(
                apGeom,
                BSShaderManager::BSSM_EMPTY,
                false,
                1,
                pLight ? pLight : pSunLight,
                nullptr,
                nullptr,
                nullptr
            );
            pEnvPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
            pEnvPass->usPassEnum = BSShaderManager::BSSM_3XENVMAP_EYE;
        }
        else {
            const uint16_t usPass = bSkinned ? BSShaderManager::BSSM_3XENVMAP_S + bVertexColors
                                             : BSShaderManager::BSSM_3XENVMAP + 2 * bVertexColors;
            pRenderPassList->Add(apGeom, BSShaderManager::BSSM_EMPTY, false, 0, nullptr, nullptr, nullptr, nullptr);
            pEnvPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
            pEnvPass->usPassEnum = usPass;
        }
        if (bAlphaBlend) {
            pEnvPass->eAccumulationHint = BSSP_HINT_SORT;
        }
    }

    iLastRenderPassState = uiState;
    return pRenderPassList;
}

BSShaderProperty::RenderPass* Lighting30ShaderProperty::GetRenderDepthPass(NiGeometry* apGeom) {
    // BSSP_FLAG_DECAL, BSSP_FLAG_DYNAMIC_DECAL
    if (ulFlags[0] & BSS1_Decal_Single_Pass) {
        return nullptr;
    }
    if (ulFlags[0] & BSS1_Dynamic_Decal_Single_Pass) {
        return nullptr;
    }
    const bool bVertexColors = apGeom->m_spModelData->m_pkColor != nullptr;
    if (!pRenderDepthPass) {
        uint32_t uiPass;
        // BSSP_SKINNED
        if (ulFlags[0] & BSS1_Skinned) {
            uiPass = bVertexColors ? BSShaderManager::BSSM_3XDEPTH_SVc : BSShaderManager::BSSM_3XDEPTH_S;
        }
        else {
            uiPass = bVertexColors ? BSShaderManager::BSSM_3XDEPTH_Vc : BSShaderManager::BSSM_3XDEPTH;
        }
        pRenderDepthPass = new RenderPass(apGeom, uiPass, true, 0, nullptr);
    }
    return pRenderDepthPass;
}

BSShaderProperty::RenderPassArray* Lighting30ShaderProperty::GetWaterFogPassList(NiGeometry* apGeom) {
    // NaN builds the list.
    if (fRefractionPower > 0.0f) {
        return nullptr;
    }
    const NiAlphaProperty* pAlpha = static_cast<const NiAlphaProperty*>(
        apGeom->m_PropertyState.m_aspProps[NiProperty::ALPHA].m_pObject
    );
    const bool bAlphaTest = pAlpha && pAlpha->GetAlphaTesting();
    const bool bVertexColors = apGeom->m_spModelData->m_pkColor != nullptr;
    CheckCreateWaterFogRenderPassArray(1);
    if (pWaterFogRenderPassList->iSize == 0) {
        uint32_t uiPass;
        // BSSP_SKINNED
        if (ulFlags[0] & BSS1_Skinned) {
            if (bVertexColors) {
                uiPass = bAlphaTest ? BSShaderManager::BSSM_3XWATER_DEPTH_SAtVc
                                    : BSShaderManager::BSSM_3XWATER_DEPTH_SVc;
            }
            else {
                uiPass = bAlphaTest ? BSShaderManager::BSSM_3XWATER_DEPTH_SAt : BSShaderManager::BSSM_3XWATER_DEPTH_S;
            }
        }
        else {
            if (bVertexColors) {
                uiPass = bAlphaTest ? BSShaderManager::BSSM_3XWATER_DEPTH_AtVc : BSShaderManager::BSSM_3XWATER_DEPTH_Vc;
            }
            else {
                uiPass = bAlphaTest ? BSShaderManager::BSSM_3XWATER_DEPTH_At : BSShaderManager::BSSM_3XWATER_DEPTH;
            }
        }
        pWaterFogRenderPassList->Add(apGeom, uiPass, true, 0, nullptr, nullptr, nullptr, nullptr);
    }
    return pWaterFogRenderPassList;
}

void Lighting30ShaderProperty::CopyMembers(Lighting30ShaderProperty* pDest, NiCloningProcess& arCloneData) {
    BSShaderLightingProperty::CopyMembers(pDest, arCloneData);
    CopyToMembers(pDest);
}

// The stream's factory: runs the class's default constructor in an engine block.
NiObject* Lighting30ShaderProperty::CreateObject() {
    return new Lighting30ShaderProperty;
}
