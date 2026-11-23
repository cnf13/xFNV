#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ImageSpaceEffectMotionBlur.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/Lighting30Shader.h"
#include "BSShader/Lighting30ShaderProperty.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/ShadowSceneNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"

void Lighting30Shader::UpdateDiffuseNormalStages(uint32_t auiCurrentPass, Lighting30ShaderProperty* apProp) {
    NiD3DPass* v3 = NiD3DShader::m_spCurrentPass;
    uint32_t v4 = auiCurrentPass;
    if ((auiCurrentPass - BSShaderManager::BSSM_3XENVMAP > 36 ||
         ((auiCurrentPass >= BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW &&
           auiCurrentPass <= BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_VcS) ||
          auiCurrentPass - BSShaderManager::BSSM_3XDEPTHMAP <= 3)) &&
        (auiCurrentPass < BSShaderManager::BSSM_WATER_DEPTH ||
         auiCurrentPass > BSShaderManager::BSSM_NLWATER_DEPTH_AtVc_S)) {
        NiD3DTextureStage* m_pObject = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
        m_pObject->m_spTexture = apProp->spTextures[0]->m_pObject;
        SetTextureMode(m_pObject, apProp);
        NiD3DTextureStage* pstage = v3->m_kStages.m_pBase[1].m_pObject;
        if (pstage) {
            NiTexture* v7 = apProp->spTextures[1]->m_pObject;
            if (!v7) {
                v7 = BSShaderManager::spDefaultNormalMap.m_pObject;
            }
            pstage->m_spTexture = v7;
            SetTextureMode(pstage, apProp);
        }
        if ((BSShaderManager::eCurrentPass >= BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW &&
             BSShaderManager::eCurrentPass <= BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_VcS) &&
            apProp->GetUnk68()) {
            ShadowSceneLight* v8 = *BSShaderManager::pCurrentRenderPass->ppSceneLights;
            if (v8->bCastShadow) {
                BSRenderedTexture* v9 = v8->spDynamicShadowMap.m_pObject;
                if (v9 && BSShaderManager::pCurrentRenderPass->ucNumLights) {
                    NiD3DTextureStage* pstage_2 = v3->m_kStages.m_pBase[2].m_pObject;
                    pstage_2->m_spTexture = v9->GetTexture(0);
                    pstage_2->SetFilterMode(
                        static_cast<NiTexturingProperty::FilterMode>(
                            BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_LOOPFILTER
                        )
                    );
                    pstage_2->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
                    v4 = auiCurrentPass;
                    Lighting30Shader::kPSConstants[20].w = static_cast<float>(
                        apProp->GetFlag(BSShaderProperty::BSSP_ALPHATEX)
                    );
                }
            }
        }
    }
    if (v4 == BSShaderManager::BSSM_3XWATER_DEPTH_At ||
        (v4 == BSShaderManager::BSSM_3XWATER_DEPTH_SAt ||
         (v4 == BSShaderManager::BSSM_3XWATER_DEPTH_AtVc || v4 == BSShaderManager::BSSM_3XWATER_DEPTH_SAtVc))) {
        NiD3DTextureStage* v12 = v3->m_kStages.m_pBase->m_pObject;
        v12->m_spTexture = apProp->spTextures[0]->m_pObject;
        SetTextureMode(v12, apProp);
    }
    if (v4 - BSShaderManager::BSSM_3XCONSTALPHA <= 3) {
        NiD3DTextureStage* v13 = v3->m_kStages.m_pBase->m_pObject;
        v13->m_spTexture = apProp->spTextures[0]->m_pObject;
        SetTextureMode(v13, apProp);
    }
}

void Lighting30Shader::UpdatePassTextureStages(uint32_t auiCurrentPass, Lighting30ShaderProperty* apProp) {
    NiD3DPass* v3 = NiD3DShader::m_spCurrentPass;
    if (auiCurrentPass > BSShaderManager::BSSM_3XLIGHTING_H) {
        switch (auiCurrentPass) {
            case BSShaderManager::BSSM_3XLIGHTING_Px:
            case BSShaderManager::BSSM_3XLIGHTING_Pxo:
            case BSShaderManager::BSSM_3XLIGHTING_VcPx:
            case BSShaderManager::BSSM_3XLIGHTING_VcPxo:
            case BSShaderManager::BSSM_3XLIGHTING_PxSpc:
            case BSShaderManager::BSSM_3XLIGHTING_PxoSpc:
            case BSShaderManager::BSSM_3XLIGHTING_VcPxSpc:
            case BSShaderManager::BSSM_3XLIGHTING_VcPxoSpc: {
                SetHeightMap(NiD3DShader::m_spCurrentPass, apProp, 4u);
                return;
            }
            case BSShaderManager::BSSM_3XLIGHTING_G:
            case BSShaderManager::BSSM_3XLIGHTING_SG:
            case BSShaderManager::BSSM_3XLIGHTING_VcG:
            case BSShaderManager::BSSM_3XLIGHTING_VcSG:
            case BSShaderManager::BSSM_3XLIGHTING_GSpc:
            case BSShaderManager::BSSM_3XLIGHTING_SGSpc:
            case BSShaderManager::BSSM_3XLIGHTING_VcGSpc:
            case BSShaderManager::BSSM_3XLIGHTING_VcSGSpc: {
                NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[6]->m_spTexture = apProp->spTextures[2]->m_pObject;
                return;
            }
            case BSShaderManager::BSSM_3XLIGHTING_HSpc: {
                SetHairMaps(NiD3DShader::m_spCurrentPass, apProp);
                return;
            }
            case BSShaderManager::BSSM_3XLIGHTING_HShp:
            case BSShaderManager::BSSM_3XLIGHTING_HShpSpc: {
                SetHairMaps(NiD3DShader::m_spCurrentPass, apProp);
                SetProjShadowMaps(v3);
                return;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxShp:
            case BSShaderManager::BSSM_3XLIGHTING_PxoShp:
            case BSShaderManager::BSSM_3XLIGHTING_VcPxShp:
            case BSShaderManager::BSSM_3XLIGHTING_VcPxoShp:
            case BSShaderManager::BSSM_3XLIGHTING_PxShpSpc:
            case BSShaderManager::BSSM_3XLIGHTING_PxoShpSpc:
            case BSShaderManager::BSSM_3XLIGHTING_VcPxShpSpc:
            case BSShaderManager::BSSM_3XLIGHTING_VcPxoShpSpc: {
                SetProjShadowMaps(NiD3DShader::m_spCurrentPass);
                SetHeightMap(v3, apProp, 4u);
                return;
            }
            case BSShaderManager::BSSM_3XLIGHTING_FgShp:
            case BSShaderManager::BSSM_3XLIGHTING_FgShpSpc: {
                SetProjShadowMaps(NiD3DShader::m_spCurrentPass);
                [[fallthrough]];
            }
            case BSShaderManager::BSSM_3XLIGHTING_Fg:
            case BSShaderManager::BSSM_3XLIGHTING_FgSpc: {
                v3->m_kStages.m_pBase[4]->m_spTexture = apProp->spTextures[0][1].m_pObject;
                v3->m_kStages.m_pBase[5]->m_spTexture = apProp->spTextures[1][1].m_pObject;
                return;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SFgShp:
            case BSShaderManager::BSSM_3XLIGHTING_SFgShpSpc: {
                SetProjShadowMaps(NiD3DShader::m_spCurrentPass);
                [[fallthrough]];
            }
            case BSShaderManager::BSSM_3XLIGHTING_SFg:
            case BSShaderManager::BSSM_3XLIGHTING_SFgSpc: {
                SetupStandardTextures(v3, apProp, true);
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_GShp:
            case BSShaderManager::BSSM_3XLIGHTING_SGShp:
            case BSShaderManager::BSSM_3XLIGHTING_VcGShp:
            case BSShaderManager::BSSM_3XLIGHTING_VcSGShp:
            case BSShaderManager::BSSM_3XLIGHTING_GShpSpc:
            case BSShaderManager::BSSM_3XLIGHTING_SGShpSpc:
            case BSShaderManager::BSSM_3XLIGHTING_VcGShpSpc:
            case BSShaderManager::BSSM_3XLIGHTING_VcSGShpSpc: {
                NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[6]->m_spTexture = apProp->spTextures[2]->m_pObject;
                [[fallthrough]];
            }
            case BSShaderManager::BSSM_3XLIGHTING_Shp:
            case BSShaderManager::BSSM_3XLIGHTING_SShp:
            case BSShaderManager::BSSM_3XLIGHTING_VcShp:
            case BSShaderManager::BSSM_3XLIGHTING_VcSShp:
            case BSShaderManager::BSSM_3XLIGHTING_ShpSpc:
            case BSShaderManager::BSSM_3XLIGHTING_SShpSpc:
            case BSShaderManager::BSSM_3XLIGHTING_VcShpSpc:
            case BSShaderManager::BSSM_3XLIGHTING_VcSShpSpc: {
                SetProjShadowMaps(v3);
                break;
            }
            case BSShaderManager::BSSM_3XENVMAP:
            case BSShaderManager::BSSM_3XENVMAP_W:
            case BSShaderManager::BSSM_3XENVMAP_Vc:
            case BSShaderManager::BSSM_3XENVMAP_WVc:
            case BSShaderManager::BSSM_3XENVMAP_S:
            case BSShaderManager::BSSM_3XENVMAP_SVc:
            case BSShaderManager::BSSM_3XENVMAP_EYE: {
                SetEnvironmentMap(NiD3DShader::m_spCurrentPass, apProp, auiCurrentPass);
                SetEnvironmentMask(v3, apProp);
                SetNormalMap(v3, apProp, 0);
                break;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS:
            case BSShaderManager::BSSM_3XRENDERNORMALS_S:
            case BSShaderManager::BSSM_3XRENDERNORMALS_FIRE: {
                SetRenderNormalsMaps(NiD3DShader::m_spCurrentPass, apProp);
                break;
            }
            case BSShaderManager::BSSM_3XTEXEFFECT:
            case BSShaderManager::BSSM_3XTEXEFFECT_S: {
                UpdatePipelineTexEffect(apProp, NiD3DShader::m_spCurrentPass);
                break;
            }
            default: {
                return;
            }
        }
    }
    else {
        if (auiCurrentPass == BSShaderManager::BSSM_3XLIGHTING_H) {
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[4]->m_spTexture = BSShaderManager::spAnisotropicMap;
            v3->m_kStages.m_pBase[5]->m_spTexture = apProp->spTextures[2]->m_pObject;
        }
        else {
            if (auiCurrentPass >= BSShaderManager::BSSM_3XZONLY_TEXEFFECT &&
                auiCurrentPass <= BSShaderManager::BSSM_3XZONLY_TEXEFFECT_S) {
                UpdatePipelineZOnlyTexEffect(apProp, NiD3DShader::m_spCurrentPass);
            }
        }
    }
}

NiD3DPass*
Lighting30Shader::UpdatePass(BSShaderManager::eBSSMPassEnum aePassEnum, Lighting30ShaderProperty* apL30Prop) {
    NiD3DPass* pPass = nullptr;
    if (aePassEnum > BSShaderManager::BSSM_3XOCCLUSION) {
        switch (aePassEnum) {
            case BSShaderManager::BSSM_3XLIGHTING: {
                return Lighting30Shader::spPass[7].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_S: {
                return Lighting30Shader::spPass[8].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_H: {
                pPass = Lighting30Shader::spPass[9].m_pObject;
                Lighting30Shader::kPSConstants[2].x = apL30Prop->pHairTintColor.r;
                Lighting30Shader::kPSConstants[2].y = apL30Prop->pHairTintColor.g;
                Lighting30Shader::kPSConstants[2].z = apL30Prop->pHairTintColor.b;
                Lighting30Shader::kPSConstants[2].w = apL30Prop->pHairTintColor.a;
                pPass->m_kStages.m_pBase[4]->m_spTexture = BSShaderManager::spAnisotropicMap;
                pPass->m_kStages.m_pBase[5]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Px: {
                pPass = Lighting30Shader::spPass[10].m_pObject;
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Pxo: {
                pPass = Lighting30Shader::spPass[11].m_pObject;
                Lighting30Shader::kPSConstants[6].x = apL30Prop->fParallaxOccMaxPasses;
                Lighting30Shader::kPSConstants[6].y = apL30Prop->fParallaxOccScale;
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Fg: {
                pPass = Lighting30Shader::spPass[12].m_pObject;
                pPass->m_kStages.m_pBase[4]->m_spTexture = apL30Prop->spTextures[0][1].m_pObject;
                pPass->m_kStages.m_pBase[5]->m_spTexture = apL30Prop->spTextures[1][1].m_pObject;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SFg: {
                pPass = Lighting30Shader::spPass[13].m_pObject;
                pPass->m_kStages.m_pBase[4]->m_spTexture = apL30Prop->spTextures[0][1].m_pObject;
                pPass->m_kStages.m_pBase[5]->m_spTexture = apL30Prop->spTextures[1][1].m_pObject;
                NiD3DTextureStage* pStage = pPass->m_kStages.m_pBase[6].m_pObject;
                pStage->m_spTexture = apL30Prop->GetTexture(BSShaderProperty::TEXTYPE_GLOW, 0);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_G: {
                pPass = Lighting30Shader::spPass[14].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SG: {
                pPass = Lighting30Shader::spPass[15].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Vc: {
                return Lighting30Shader::spPass[16].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcS: {
                return Lighting30Shader::spPass[17].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPx: {
                pPass = Lighting30Shader::spPass[18].m_pObject;
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxo: {
                pPass = Lighting30Shader::spPass[19].m_pObject;
                Lighting30Shader::kPSConstants[6].x = apL30Prop->fParallaxOccMaxPasses;
                Lighting30Shader::kPSConstants[6].y = apL30Prop->fParallaxOccScale;
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcG: {
                pPass = Lighting30Shader::spPass[20].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSG: {
                pPass = Lighting30Shader::spPass[21].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Spc: {
                return Lighting30Shader::spPass[22].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SSpc: {
                return Lighting30Shader::spPass[23].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_HSpc: {
                pPass = Lighting30Shader::spPass[24].m_pObject;
                Lighting30Shader::kPSConstants[2].x = apL30Prop->pHairTintColor.r;
                Lighting30Shader::kPSConstants[2].y = apL30Prop->pHairTintColor.g;
                Lighting30Shader::kPSConstants[2].z = apL30Prop->pHairTintColor.b;
                Lighting30Shader::kPSConstants[2].w = apL30Prop->pHairTintColor.a;
                pPass->m_kStages.m_pBase[4]->m_spTexture = BSShaderManager::spAnisotropicMap;
                pPass->m_kStages.m_pBase[5]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxSpc: {
                pPass = Lighting30Shader::spPass[25].m_pObject;
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxoSpc: {
                pPass = Lighting30Shader::spPass[26].m_pObject;
                Lighting30Shader::kPSConstants[6].x = apL30Prop->fParallaxOccMaxPasses;
                Lighting30Shader::kPSConstants[6].y = apL30Prop->fParallaxOccScale;
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_FgSpc: {
                pPass = Lighting30Shader::spPass[27].m_pObject;
                pPass->m_kStages.m_pBase[4]->m_spTexture = apL30Prop->spTextures[0][1].m_pObject;
                pPass->m_kStages.m_pBase[5]->m_spTexture = apL30Prop->spTextures[1][1].m_pObject;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SFgSpc: {
                pPass = Lighting30Shader::spPass[28].m_pObject;
                pPass->m_kStages.m_pBase[4]->m_spTexture = apL30Prop->spTextures[0][1].m_pObject;
                pPass->m_kStages.m_pBase[5]->m_spTexture = apL30Prop->spTextures[1][1].m_pObject;
                NiD3DTextureStage* pStage = pPass->m_kStages.m_pBase[6].m_pObject;
                pStage->m_spTexture = apL30Prop->GetTexture(BSShaderProperty::TEXTYPE_GLOW, 0);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_GSpc: {
                pPass = Lighting30Shader::spPass[29].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SGSpc: {
                pPass = Lighting30Shader::spPass[30].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSpc: {
                return Lighting30Shader::spPass[31].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSSpc: {
                return Lighting30Shader::spPass[32].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxSpc: {
                pPass = Lighting30Shader::spPass[33].m_pObject;
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxoSpc: {
                pPass = Lighting30Shader::spPass[34].m_pObject;
                Lighting30Shader::kPSConstants[6].x = apL30Prop->fParallaxOccMaxPasses;
                Lighting30Shader::kPSConstants[6].y = apL30Prop->fParallaxOccScale;
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcGSpc: {
                pPass = Lighting30Shader::spPass[35].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSGSpc: {
                pPass = Lighting30Shader::spPass[21].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Shp: {
                pPass = Lighting30Shader::spPass[37].m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SShp: {
                pPass = Lighting30Shader::spPass[38].m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_HShp: {
                pPass = Lighting30Shader::spPass[39].m_pObject;
                Lighting30Shader::kPSConstants[2].x = apL30Prop->pHairTintColor.r;
                Lighting30Shader::kPSConstants[2].y = apL30Prop->pHairTintColor.g;
                Lighting30Shader::kPSConstants[2].z = apL30Prop->pHairTintColor.b;
                Lighting30Shader::kPSConstants[2].w = apL30Prop->pHairTintColor.a;
                pPass->m_kStages.m_pBase[4]->m_spTexture = BSShaderManager::spAnisotropicMap;
                pPass->m_kStages.m_pBase[5]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxShp: {
                pPass = Lighting30Shader::spPass[40].m_pObject;
                SetProjShadowMaps(pPass);
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxoShp: {
                pPass = Lighting30Shader::spPass[41].m_pObject;
                Lighting30Shader::kPSConstants[6].x = apL30Prop->fParallaxOccMaxPasses;
                Lighting30Shader::kPSConstants[6].y = apL30Prop->fParallaxOccScale;
                SetProjShadowMaps(pPass);
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_FgShp: {
                pPass = Lighting30Shader::spPass[42].m_pObject;
                SetProjShadowMaps(pPass);
                pPass->m_kStages.m_pBase[4]->m_spTexture = apL30Prop->spTextures[0][1].m_pObject;
                pPass->m_kStages.m_pBase[5]->m_spTexture = apL30Prop->spTextures[1][1].m_pObject;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SFgShp: {
                pPass = Lighting30Shader::spPass[43].m_pObject;
                SetProjShadowMaps(pPass);
                pPass->m_kStages.m_pBase[4]->m_spTexture = apL30Prop->spTextures[0][1].m_pObject;
                pPass->m_kStages.m_pBase[5]->m_spTexture = apL30Prop->spTextures[1][1].m_pObject;
                NiD3DTextureStage* pStage = pPass->m_kStages.m_pBase[6].m_pObject;
                pStage->m_spTexture = apL30Prop->GetTexture(BSShaderProperty::TEXTYPE_GLOW, 0);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_GShp: {
                pPass = Lighting30Shader::spPass[44].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SGShp: {
                pPass = Lighting30Shader::spPass[45].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcShp: {
                pPass = Lighting30Shader::spPass[46].m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSShp: {
                pPass = Lighting30Shader::spPass[47].m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxShp: {
                pPass = Lighting30Shader::spPass[48].m_pObject;
                SetProjShadowMaps(pPass);
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxoShp: {
                pPass = Lighting30Shader::spPass[49].m_pObject;
                Lighting30Shader::kPSConstants[6].x = apL30Prop->fParallaxOccMaxPasses;
                Lighting30Shader::kPSConstants[6].y = apL30Prop->fParallaxOccScale;
                SetProjShadowMaps(pPass);
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcGShp: {
                pPass = Lighting30Shader::spPass[50].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSGShp: {
                pPass = Lighting30Shader::spPass[51].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_ShpSpc: {
                pPass = Lighting30Shader::spPass[52].m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SShpSpc: {
                pPass = Lighting30Shader::spPass[53].m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_HShpSpc: {
                pPass = Lighting30Shader::spPass[54].m_pObject;
                Lighting30Shader::kPSConstants[2].x = apL30Prop->pHairTintColor.r;
                Lighting30Shader::kPSConstants[2].y = apL30Prop->pHairTintColor.g;
                Lighting30Shader::kPSConstants[2].z = apL30Prop->pHairTintColor.b;
                Lighting30Shader::kPSConstants[2].w = apL30Prop->pHairTintColor.a;
                pPass->m_kStages.m_pBase[4]->m_spTexture = BSShaderManager::spAnisotropicMap;
                pPass->m_kStages.m_pBase[5]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxShpSpc: {
                pPass = Lighting30Shader::spPass[55].m_pObject;
                SetProjShadowMaps(pPass);
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxoShpSpc: {
                pPass = Lighting30Shader::spPass[56].m_pObject;
                Lighting30Shader::kPSConstants[6].x = apL30Prop->fParallaxOccMaxPasses;
                Lighting30Shader::kPSConstants[6].y = apL30Prop->fParallaxOccScale;
                SetProjShadowMaps(pPass);
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_FgShpSpc: {
                pPass = Lighting30Shader::spPass[57].m_pObject;
                SetProjShadowMaps(pPass);
                pPass->m_kStages.m_pBase[4]->m_spTexture = apL30Prop->spTextures[0][1].m_pObject;
                pPass->m_kStages.m_pBase[5]->m_spTexture = apL30Prop->spTextures[1][1].m_pObject;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SFgShpSpc: {
                pPass = Lighting30Shader::spPass[58].m_pObject;
                SetProjShadowMaps(pPass);
                pPass->m_kStages.m_pBase[4]->m_spTexture = apL30Prop->spTextures[0][1].m_pObject;
                pPass->m_kStages.m_pBase[5]->m_spTexture = apL30Prop->spTextures[1][1].m_pObject;
                NiD3DTextureStage* pStage = pPass->m_kStages.m_pBase[6].m_pObject;
                pStage->m_spTexture = apL30Prop->GetTexture(BSShaderProperty::TEXTYPE_GLOW, 0);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_GShpSpc: {
                pPass = Lighting30Shader::spPass[59].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SGShpSpc: {
                pPass = Lighting30Shader::spPass[60].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcShpSpc: {
                pPass = Lighting30Shader::spPass[61].m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSShpSpc: {
                pPass = Lighting30Shader::spPass[62].m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxShpSpc: {
                pPass = Lighting30Shader::spPass[63].m_pObject;
                SetProjShadowMaps(pPass);
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxoShpSpc: {
                pPass = Lighting30Shader::spPass[64].m_pObject;
                Lighting30Shader::kPSConstants[6].x = apL30Prop->fParallaxOccMaxPasses;
                Lighting30Shader::kPSConstants[6].y = apL30Prop->fParallaxOccScale;
                SetProjShadowMaps(pPass);
                SetHeightMap(pPass, apL30Prop, 4u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcGShpSpc: {
                pPass = Lighting30Shader::spPass[65].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSGShpSpc: {
                pPass = Lighting30Shader::spPass[66].m_pObject;
                pPass->m_kStages.m_pBase[6]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(pPass);
                return pPass;
            }
            case BSShaderManager::BSSM_3XSILHOUETTE: {
                pPass = Lighting30Shader::spPass[67].m_pObject;
                ClearUnusedStages(pPass, 1u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XSILHOUETTE_Vc: {
                pPass = Lighting30Shader::spPass[68].m_pObject;
                ClearUnusedStages(pPass, 1u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XENVMAP: {
                pPass = Lighting30Shader::spPass[69].m_pObject;
                SetEnvironmentMap(pPass, apL30Prop, aePassEnum);
                SetEnvironmentMask(pPass, apL30Prop);
                SetNormalMap(pPass, apL30Prop, 0);
                return pPass;
            }
            case BSShaderManager::BSSM_3XENVMAP_W: {
                pPass = Lighting30Shader::spPass[70].m_pObject;
                SetEnvironmentMap(pPass, apL30Prop, aePassEnum);
                SetEnvironmentMask(pPass, apL30Prop);
                SetNormalMap(pPass, apL30Prop, 0);
                return pPass;
            }
            case BSShaderManager::BSSM_3XENVMAP_Vc: {
                pPass = Lighting30Shader::spPass[71].m_pObject;
                SetEnvironmentMap(pPass, apL30Prop, aePassEnum);
                SetEnvironmentMask(pPass, apL30Prop);
                SetNormalMap(pPass, apL30Prop, 0);
                return pPass;
            }
            case BSShaderManager::BSSM_3XENVMAP_WVc: {
                pPass = Lighting30Shader::spPass[72].m_pObject;
                SetEnvironmentMap(pPass, apL30Prop, aePassEnum);
                SetEnvironmentMask(pPass, apL30Prop);
                SetNormalMap(pPass, apL30Prop, 0);
                return pPass;
            }
            case BSShaderManager::BSSM_3XENVMAP_S: {
                pPass = Lighting30Shader::spPass[73].m_pObject;
                SetEnvironmentMap(pPass, apL30Prop, aePassEnum);
                SetEnvironmentMask(pPass, apL30Prop);
                SetNormalMap(pPass, apL30Prop, 0);
                return pPass;
            }
            case BSShaderManager::BSSM_3XENVMAP_SVc: {
                pPass = Lighting30Shader::spPass[74].m_pObject;
                SetEnvironmentMap(pPass, apL30Prop, aePassEnum);
                SetEnvironmentMask(pPass, apL30Prop);
                SetNormalMap(pPass, apL30Prop, 0);
                return pPass;
            }
            case BSShaderManager::BSSM_3XENVMAP_EYE: {
                pPass = Lighting30Shader::spPass[75].m_pObject;
                SetEnvironmentMap(pPass, apL30Prop, aePassEnum);
                SetEnvironmentMask(pPass, apL30Prop);
                SetNormalMap(pPass, apL30Prop, 0);
                BSNiBound* m_kWorldBound = BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorldBound;
                if (!m_kWorldBound) {
                    m_kWorldBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
                }
                float v64 = m_kWorldBound->m_kCenter.x - NiRenderer::PosAdjust.x;
                float v65 = m_kWorldBound->m_kCenter.y - NiRenderer::PosAdjust.y;
                float v66 = m_kWorldBound->m_kCenter.z - NiRenderer::PosAdjust.z;
                Lighting30Shader::kVSConstants[39].x = v64;
                Lighting30Shader::kVSConstants[39].y = v65;
                Lighting30Shader::kVSConstants[39].z = v66;
                return pPass;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW: {
                return Lighting30Shader::spPass[76].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_Vc: {
                return Lighting30Shader::spPass[77].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_S: {
                return Lighting30Shader::spPass[78].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_VcS: {
                return Lighting30Shader::spPass[79].m_pObject;
            }
            case BSShaderManager::BSSM_3XDEPTHMAP: {
                pPass = Lighting30Shader::spPass[80].m_pObject;
                pPass->m_kStages.m_pBase->m_pObject->m_spTexture = apL30Prop->spTextures[0]->m_pObject;
                ClearUnusedStages(pPass, 1u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XDEPTHMAP_S: {
                pPass = Lighting30Shader::spPass[82].m_pObject;
                pPass->m_kStages.m_pBase->m_pObject->m_spTexture = apL30Prop->spTextures[0]->m_pObject;
                ClearUnusedStages(pPass, 1u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS: {
                pPass = Lighting30Shader::spPass[84].m_pObject;
                NiTexture* pNormalMap = apL30Prop->spTextures[1]->m_pObject;
                if (!pNormalMap) {
                    pNormalMap = BSShaderManager::spDefaultNormalMap.m_pObject;
                }
                pPass->m_kStages.m_pBase->m_pObject->m_spTexture = pNormalMap;
                ClearUnusedStages(pPass, 1u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS_S: {
                pPass = Lighting30Shader::spPass[85].m_pObject;
                NiTexture* pNormalMap = apL30Prop->spTextures[1]->m_pObject;
                if (!pNormalMap) {
                    pNormalMap = BSShaderManager::spDefaultNormalMap.m_pObject;
                }
                pPass->m_kStages.m_pBase->m_pObject->m_spTexture = pNormalMap;
                ClearUnusedStages(pPass, 1u);
                return pPass;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS_FIRE: {
                pPass = Lighting30Shader::spPass[86].m_pObject;
                NiTexture* pNormalMap = apL30Prop->spTextures[1]->m_pObject;
                if (!pNormalMap) {
                    pNormalMap = BSShaderManager::spDefaultNormalMap.m_pObject;
                }
                pPass->m_kStages.m_pBase->m_pObject->m_spTexture = pNormalMap;
                ClearUnusedStages(pPass, 1u);
                break;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS_CLEAR: {
                pPass = Lighting30Shader::spPass[87].m_pObject;
                ClearUnusedStages(pPass, 0);
                break;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS_CLEAR_S: {
                pPass = Lighting30Shader::spPass[88].m_pObject;
                ClearUnusedStages(pPass, 0);
                break;
            }
            case BSShaderManager::BSSM_3XCONSTALPHA: {
                pPass = Lighting30Shader::spPass[89].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XCONSTALPHA_Vc: {
                pPass = Lighting30Shader::spPass[90].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XCONSTALPHA_S: {
                pPass = Lighting30Shader::spPass[91].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XCONSTALPHA_SVc: {
                pPass = Lighting30Shader::spPass[92].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XDEPTH: {
                pPass = Lighting30Shader::spPass[93].m_pObject;
                ClearUnusedStages(pPass, 0);
                break;
            }
            case BSShaderManager::BSSM_3XDEPTH_Vc: {
                pPass = Lighting30Shader::spPass[94].m_pObject;
                ClearUnusedStages(pPass, 0);
                break;
            }
            case BSShaderManager::BSSM_3XDEPTH_S: {
                pPass = Lighting30Shader::spPass[95].m_pObject;
                ClearUnusedStages(pPass, 0);
                ShadowLightShader::UpdateFarNear();
                break;
            }
            case BSShaderManager::BSSM_3XDEPTH_SVc: {
                pPass = Lighting30Shader::spPass[96].m_pObject;
                ClearUnusedStages(pPass, 0);
                ShadowLightShader::UpdateFarNear();
                break;
            }
            case BSShaderManager::BSSM_3XVELOCITY: {
                pPass = Lighting30Shader::spPass[97].m_pObject;
                ClearUnusedStages(pPass, 0);
                Lighting30Shader::kVSConstants[41].y = static_cast<ImageSpaceEffectMotionBlur*>(
                                                           BSShaderManager::pImageSpaceManager->pEffectList
                                                               .m_pBase[ImageSpaceManager::EFFECT_MOTION_BLUR]
                )
                                                           ->GetObjectBlurConst();
                Lighting30Shader::kVSConstants[41].z = static_cast<ImageSpaceEffectMotionBlur*>(
                                                           BSShaderManager::pImageSpaceManager->pEffectList
                                                               .m_pBase[ImageSpaceManager::EFFECT_MOTION_BLUR]
                )
                                                           ->Params.r;
                Lighting30Shader::kVSConstants[41].w = 0.0f;
                ShadowLightShader::UpdateFarNear();
                break;
            }
            case BSShaderManager::BSSM_3XVELOCITY_S: {
                pPass = Lighting30Shader::spPass[98].m_pObject;
                ClearUnusedStages(pPass, 0);
                Lighting30Shader::kVSConstants[41].y = static_cast<ImageSpaceEffectMotionBlur*>(
                                                           BSShaderManager::pImageSpaceManager->pEffectList
                                                               .m_pBase[ImageSpaceManager::EFFECT_MOTION_BLUR]
                )
                                                           ->GetObjectBlurConst();
                Lighting30Shader::kVSConstants[41].z = static_cast<ImageSpaceEffectMotionBlur*>(
                                                           BSShaderManager::pImageSpaceManager->pEffectList
                                                               .m_pBase[ImageSpaceManager::EFFECT_MOTION_BLUR]
                )
                                                           ->Params.r;
                Lighting30Shader::kVSConstants[41].w = 0.0f;
                break;
            }
            case BSShaderManager::BSSM_3XLOCALMAP: {
                pPass = Lighting30Shader::spPass[99].m_pObject;
                ClearUnusedStages(pPass, 0);
                break;
            }
            case BSShaderManager::BSSM_3XLOCALMAP_S: {
                pPass = Lighting30Shader::spPass[100].m_pObject;
                ClearUnusedStages(pPass, 0);
                break;
            }
            case BSShaderManager::BSSM_3XLOCALMAP_CLEAR: {
                pPass = Lighting30Shader::spPass[101].m_pObject;
                ClearUnusedStages(pPass, 0);
                break;
            }
            case BSShaderManager::BSSM_3XTEXEFFECT: {
                pPass = Lighting30Shader::spPass[102].m_pObject;
                UpdatePipelineTexEffect(apL30Prop, pPass);
                break;
            }
            case BSShaderManager::BSSM_3XTEXEFFECT_S: {
                pPass = Lighting30Shader::spPass[103].m_pObject;
                UpdatePipelineTexEffect(apL30Prop, pPass);
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH: {
                pPass = Lighting30Shader::spPass[104].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_S: {
                pPass = Lighting30Shader::spPass[105].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_At: {
                pPass = Lighting30Shader::spPass[106].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_SAt: {
                pPass = Lighting30Shader::spPass[107].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_Vc: {
                pPass = Lighting30Shader::spPass[108].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_SVc: {
                pPass = Lighting30Shader::spPass[109].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_AtVc: {
                pPass = Lighting30Shader::spPass[110].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_SAtVc: {
                pPass = Lighting30Shader::spPass[111].m_pObject;
                break;
            }
            default: {
                return pPass;
            }
        }
    }
    else if (aePassEnum == BSShaderManager::BSSM_3XOCCLUSION) {
        return Lighting30Shader::spPass[0].m_pObject;
    }
    else {
        switch (aePassEnum) {
            case BSShaderManager::BSSM_3XZONLY: {
                return Lighting30Shader::spPass[1].m_pObject;
            }
            case BSShaderManager::BSSM_3XZONLY_S: {
                return Lighting30Shader::spPass[2].m_pObject;
            }
            case BSShaderManager::BSSM_3XZONLY_Vc: {
                return Lighting30Shader::spPass[3].m_pObject;
            }
            case BSShaderManager::BSSM_3XZONLY_SVc: {
                return Lighting30Shader::spPass[4].m_pObject;
            }
            case BSShaderManager::BSSM_3XZONLY_TEXEFFECT: {
                pPass = Lighting30Shader::spPass[5].m_pObject;
                UpdatePipelineZOnlyTexEffect(apL30Prop, pPass);
                break;
            }
            case BSShaderManager::BSSM_3XZONLY_TEXEFFECT_S: {
                pPass = Lighting30Shader::spPass[6].m_pObject;
                UpdatePipelineZOnlyTexEffect(apL30Prop, pPass);
                break;
            }
            default: {
                break;
            }
        }
    }
    return pPass;
}

void Lighting30Shader::SetHairMaps(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop) {
    apPass->m_kStages.m_pBase[4]->m_spTexture = BSShaderManager::spAnisotropicMap;
    apPass->m_kStages.m_pBase[5]->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
}

void Lighting30Shader::SetNormalMap(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop, uint32_t auiStageNum) {
    NiSourceTexture* m_pObject;
    m_pObject = static_cast<NiSourceTexture*>(apL30Prop->spTextures[1]->m_pObject);
    NiD3DTextureStage* v5 = apPass->m_kStages.m_pBase[auiStageNum].m_pObject;
    if (!m_pObject) {
        m_pObject = BSShaderManager::spDefaultNormalGlossMap;
        if ((apL30Prop->ulFlags[0] & BSS1_Environment_Mapping) == 0) {
            m_pObject = static_cast<NiSourceTexture*>(BSShaderManager::spDefaultNormalMap.m_pObject);
        }
    }
    v5->m_spTexture = m_pObject;
    SetTextureMode(v5, apL30Prop);
}

// The stage is read before the property's height map texture is fetched.
void Lighting30Shader::SetHeightMap(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop, uint32_t auiStageNum) {
    NiD3DTextureStage* pStage = apPass->m_kStages.m_pBase[auiStageNum].m_pObject;
    pStage->m_spTexture = apL30Prop->GetTexture(BSShaderProperty::TEXTYPE_HEIGHT, 0);
    SetTextureMode(pStage, apL30Prop);
}

// Stage 3 gets the environment mask, or the error map with the mask constant cleared when the property has none.
void Lighting30Shader::SetEnvironmentMask(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop) {
    NiTexture* pTexture = apL30Prop->GetTexture(BSShaderProperty::TEXTYPE_ENV_MASK, 0);
    NiD3DTextureStage* pStage = apPass->m_kStages.m_pBase[3].m_pObject;
    kPSConstants[7].w = 1.0f;
    if (!pTexture) {
        pTexture = BSShaderManager::spDefaultErrorMap;
        kPSConstants[7].w = 0.0f;
    }
    pStage->m_spTexture = pTexture;
    SetTextureMode(pStage, apL30Prop);
    kPSConstants[7].z = apL30Prop->fEnvmapScale;
}

void Lighting30Shader::SetRenderNormalsMaps(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop) {
    NiSourceTexture* m_pObject = static_cast<NiSourceTexture*>(apL30Prop->spTextures[1]->m_pObject);
    if (!m_pObject) {
        m_pObject = static_cast<NiSourceTexture*>(BSShaderManager::spDefaultNormalMap.m_pObject);
    }
    apPass->m_kStages.m_pBase->m_pObject->m_spTexture = m_pObject;
}

void Lighting30Shader::SetProjShadowMaps(NiD3DPass* apPass) {
    apPass->m_kStages.m_pBase[2]->m_spTexture = BSShaderManager::pProjectedShadowTexture.m_pObject;
    apPass->m_kStages.m_pBase[3]->m_spTexture = static_cast<NiTexture*>(
        BSShaderManager::pProjectedShadowMaskTexture.m_pObject
    );
}

void Lighting30Shader::SetEnvironmentMap(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop, uint32_t auiPass) {
    ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode;
    NiTexture* pReflectionMap = apL30Prop->GetTexture(BSShaderProperty::TEXTYPE_ENV, 0);
    if (!pReflectionMap) {
        if (auiPass == BSShaderManager::BSSM_3XENVMAP_W || auiPass == BSShaderManager::BSSM_3XENVMAP_WVc) {
            if (pShadowSceneNode) {
                pReflectionMap = BSShaderManager::spArmorReflectionCubeMap;
            }
        }
        else if (apL30Prop->GetFlag(BSShaderProperty::BSSP_FLAG_EYEREFLECT)) {
            pReflectionMap = BSShaderManager::spEyeReflectionCubeMap;
        }
        else if (BSShaderManager::bInterior && apL30Prop->GetFlag(BSShaderProperty::BSSP_FLAG_WINDOWREFLECT)) {
            pReflectionMap = BSShaderManager::spArmorReflectionCubeMap;
        }
        else if (pShadowSceneNode) {
            pReflectionMap = BSShaderManager::spArmorReflectionCubeMap;
        }
        if (!pReflectionMap) {
            pReflectionMap = BSShaderManager::spArmorReflectionCubeMap;
        }
    }
    apPass->m_kStages.m_pBase[1]->m_spTexture = pReflectionMap;
}
