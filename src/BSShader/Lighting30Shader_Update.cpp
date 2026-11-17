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

void Lighting30Shader::UpdateDiffuseNormalStages(uint32_t a1, Lighting30ShaderProperty* a2) {
    NiD3DPass* v3 = NiD3DShader::m_spCurrentPass;
    uint32_t v4 = a1;
    if ((a1 - 445 > 36 || ((a1 >= 452 && a1 <= 455) || a1 - 456 <= 3)) && (a1 < 728 || a1 > 751)) {
        NiD3DTextureStage* m_pObject = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
        m_pObject->m_spTexture = a2->spTextures[0]->m_pObject;
        SetTextureMode(m_pObject, a2);
        NiD3DTextureStage* pstage = v3->m_kStages.m_pBase[1].m_pObject;
        if (pstage) {
            NiTexture* v7 = a2->spTextures[1]->m_pObject;
            if (!v7) {
                v7 = BSShaderManager::spDefaultNormalMap.m_pObject;
            }
            pstage->m_spTexture = v7;
            SetTextureMode(pstage, a2);
        }
        if (BSShaderManager::eCurrentPass >= BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW &&
            BSShaderManager::eCurrentPass <= BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_VcS) {
            if (a2->GetUnk68()) {
                ShadowSceneLight* v8 = *BSShaderManager::pCurrentRenderPass->ppSceneLights;
                if (v8->bCastShadow) {
                    BSRenderedTexture* v9 = v8->spDynamicShadowMap.m_pObject;
                    if (v9) {
                        if (BSShaderManager::pCurrentRenderPass->ucNumLights) {
                            NiD3DTextureStage* pstage_2 = v3->m_kStages.m_pBase[2].m_pObject;
                            pstage_2->m_spTexture = (NiTexture*)v9->GetTexture(0);
                            pstage_2->SetFilterMode(
                                static_cast<NiTexturingProperty::FilterMode>(
                                    BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_LOOPFILTER
                                )
                            );
                            pstage_2->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
                            v4 = a1;
                            Lighting30Shader::kPSConstants[20].w = static_cast<float>(a2->GetFlag(8u));
                        }
                    }
                }
            }
        }
    }
    if (v4 == 736 || (v4 == 737 || (v4 == 748 || v4 == 749))) {
        NiD3DTextureStage* v12 = v3->m_kStages.m_pBase->m_pObject;
        v12->m_spTexture = a2->spTextures[0]->m_pObject;
        SetTextureMode(v12, a2);
    }
    if (v4 - 467 <= 3) {
        NiD3DTextureStage* v13 = v3->m_kStages.m_pBase->m_pObject;
        v13->m_spTexture = a2->spTextures[0]->m_pObject;
        SetTextureMode(v13, a2);
    }
}

void Lighting30Shader::UpdatePassTextureStages(uint32_t a1, Lighting30ShaderProperty* a2) {
    NiD3DPass* v3 = NiD3DShader::m_spCurrentPass;
    if (a1 > 385) {
        switch (a1) {
            case 386u:
            case 387u:
            case 394u:
            case 395u:
            case 401u:
            case 402u:
            case 409u:
            case 410u: {
                SetHeightMap(NiD3DShader::m_spCurrentPass, a2, 4u);
                return;
            }
            case 388u:
            case 403u: {
                goto LABEL_16;
            }
            case 389u:
            case 404u: {
                goto LABEL_18;
            }
            case 390u:
            case 391u:
            case 396u:
            case 397u:
            case 405u:
            case 406u:
            case 411u:
            case 412u: {
                NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    a2->spTextures[2]->m_pObject;
                return;
            }
            case 400u: {
                SetHairMaps(NiD3DShader::m_spCurrentPass, a2);
                return;
            }
            case 413u:
            case 414u:
            case 422u:
            case 423u:
            case 428u:
            case 429u:
            case 437u:
            case 438u: {
                goto LABEL_12;
            }
            case 415u:
            case 430u: {
                SetHairMaps(NiD3DShader::m_spCurrentPass, a2);
                SetProjShadowMaps(v3);
                return;
            }
            case 416u:
            case 417u:
            case 424u:
            case 425u:
            case 431u:
            case 432u:
            case 439u:
            case 440u: {
                SetProjShadowMaps(NiD3DShader::m_spCurrentPass);
                SetHeightMap(v3, a2, 4u);
                return;
            }
            case 418u:
            case 433u: {
                SetProjShadowMaps(NiD3DShader::m_spCurrentPass);
            LABEL_16:;
                v3->m_kStages.m_pBase[4].m_pObject->m_spTexture = a2->spTextures[0][1].m_pObject;
                v3->m_kStages.m_pBase[5].m_pObject->m_spTexture = a2->spTextures[1][1].m_pObject;
                return;
            }
            case 419u:
            case 434u: {
                SetProjShadowMaps(NiD3DShader::m_spCurrentPass);
            LABEL_18:;
                SetupStandardTextures(v3, a2, true);
                break;
            }
            case 420u:
            case 421u:
            case 426u:
            case 427u:
            case 435u:
            case 436u:
            case 441u:
            case 442u: {
                NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    a2->spTextures[2]->m_pObject;
            LABEL_12:;
                SetProjShadowMaps(v3);
                break;
            }
            case 445u:
            case 446u:
            case 447u:
            case 448u:
            case 449u:
            case 450u:
            case 451u: {
                SetEnvironmentMap(NiD3DShader::m_spCurrentPass, a2, a1);
                SetEnvironmentMask(v3, a2);
                SetNormalMap(v3, a2, 0);
                break;
            }
            case 460u:
            case 462u:
            case 464u: {
                SetRenderNormalsMaps(NiD3DShader::m_spCurrentPass, a2);
                break;
            }
            case 480u:
            case 481u: {
                UpdatePipelineTexEffect(a2, NiD3DShader::m_spCurrentPass);
                break;
            }
            default: {
                return;
            }
        }
    }
    else {
        if (a1 == 385) {
            NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[4].m_pObject->m_spTexture =
                BSShaderManager::spAnisotropicMap;
            v3->m_kStages.m_pBase[5].m_pObject->m_spTexture = a2->spTextures[2]->m_pObject;
        }
        else {
            if (a1 >= 14 && a1 <= 15) {
                UpdatePipelineZOnlyTexEffect(a2, NiD3DShader::m_spCurrentPass);
            }
        }
    }
}

NiD3DPass*
Lighting30Shader::UpdatePass(BSShaderManager::eBSSMPassEnum aePassEnum, Lighting30ShaderProperty* apL30Prop) {
    NiD3DPass* result;
    NiD3DPass* ppass = nullptr;
    if (aePassEnum > BSShaderManager::BSSM_3XOCCLUSION) {
        NiD3DPass* m_pObject;
        NiD3DPass* v10;
        NiD3DPass* v11;
        NiD3DPass* v12;
        NiD3DPass* v23;
        NiD3DPass* v25;
        NiD3DPass* ppass_2;
        NiD3DPass* v54;
        switch (aePassEnum) {
            case BSShaderManager::BSSM_3XLIGHTING: {
                return Lighting30Shader::spPass[7].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_S: {
                return Lighting30Shader::spPass[8].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_H: {
                m_pObject = Lighting30Shader::spPass[9].m_pObject;
                goto LABEL_15;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Px: {
                NiD3DPass* v9 = Lighting30Shader::spPass[10].m_pObject;
                SetHeightMap(Lighting30Shader::spPass[10].m_pObject, apL30Prop, 4u);
                return v9;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Pxo: {
                v10 = Lighting30Shader::spPass[11].m_pObject;
                goto LABEL_40;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Fg: {
                v11 = Lighting30Shader::spPass[12].m_pObject;
                goto LABEL_65;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SFg: {
                v12 = Lighting30Shader::spPass[13].m_pObject;
                goto LABEL_67;
            }
            case BSShaderManager::BSSM_3XLIGHTING_G: {
                result = Lighting30Shader::spPass[14].m_pObject;
                Lighting30Shader::spPass[14].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                return result;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SG: {
                NiD3DPass* v13 = Lighting30Shader::spPass[15].m_pObject;
                Lighting30Shader::spPass[15].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                return v13;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Vc: {
                return Lighting30Shader::spPass[16].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcS: {
                return Lighting30Shader::spPass[17].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPx: {
                NiD3DPass* v14 = Lighting30Shader::spPass[18].m_pObject;
                SetHeightMap(Lighting30Shader::spPass[18].m_pObject, apL30Prop, 4u);
                return v14;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxo: {
                v10 = Lighting30Shader::spPass[19].m_pObject;
                goto LABEL_40;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcG: {
                NiD3DPass* v15 = Lighting30Shader::spPass[20].m_pObject;
                Lighting30Shader::spPass[20].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                return v15;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSG:
            case BSShaderManager::BSSM_3XLIGHTING_VcSGSpc: {
                NiD3DPass* v20 = Lighting30Shader::spPass[21].m_pObject;
                Lighting30Shader::spPass[21].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                return v20;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Spc: {
                return Lighting30Shader::spPass[22].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SSpc: {
                return Lighting30Shader::spPass[23].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_HSpc: {
                m_pObject = Lighting30Shader::spPass[24].m_pObject;
            LABEL_15:;
                Lighting30Shader::kPSConstants[2].x = apL30Prop->pHairTintColor.r;
                Lighting30Shader::kPSConstants[2].y = apL30Prop->pHairTintColor.g;
                Lighting30Shader::kPSConstants[2].z = apL30Prop->pHairTintColor.b;
                Lighting30Shader::kPSConstants[2].w = apL30Prop->pHairTintColor.a;
                m_pObject->m_kStages.m_pBase[4].m_pObject->m_spTexture = (NiTexture*)BSShaderManager::spAnisotropicMap;
                m_pObject->m_kStages.m_pBase[5].m_pObject->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                return m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxSpc: {
                NiD3DPass* v16 = Lighting30Shader::spPass[25].m_pObject;
                SetHeightMap(Lighting30Shader::spPass[25].m_pObject, apL30Prop, 4u);
                return v16;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxoSpc: {
                v10 = Lighting30Shader::spPass[26].m_pObject;
                goto LABEL_40;
            }
            case BSShaderManager::BSSM_3XLIGHTING_FgSpc: {
                v11 = Lighting30Shader::spPass[27].m_pObject;
                goto LABEL_65;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SFgSpc: {
                v12 = Lighting30Shader::spPass[28].m_pObject;
                goto LABEL_67;
            }
            case BSShaderManager::BSSM_3XLIGHTING_GSpc: {
                result = Lighting30Shader::spPass[29].m_pObject;
                Lighting30Shader::spPass[29].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                return result;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SGSpc: {
                NiD3DPass* v17 = Lighting30Shader::spPass[30].m_pObject;
                Lighting30Shader::spPass[30].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                return v17;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSpc: {
                return Lighting30Shader::spPass[31].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSSpc: {
                return Lighting30Shader::spPass[32].m_pObject;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxSpc: {
                NiD3DPass* v18 = Lighting30Shader::spPass[33].m_pObject;
                SetHeightMap(Lighting30Shader::spPass[33].m_pObject, apL30Prop, 4u);
                return v18;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxoSpc: {
                v10 = Lighting30Shader::spPass[34].m_pObject;
            LABEL_40:;
                Lighting30Shader::kPSConstants[6].x = apL30Prop->fParallaxOccMaxPasses;
                Lighting30Shader::kPSConstants[6].y = apL30Prop->fParallaxOccScale;
                SetHeightMap(v10, apL30Prop, 4u);
                return v10;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcGSpc: {
                NiD3DPass* v19 = Lighting30Shader::spPass[35].m_pObject;
                Lighting30Shader::spPass[35].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                return v19;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Shp: {
                NiD3DPass* v21 = Lighting30Shader::spPass[37].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[37].m_pObject);
                return v21;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SShp: {
                NiD3DPass* v22 = Lighting30Shader::spPass[38].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[38].m_pObject);
                return v22;
            }
            case BSShaderManager::BSSM_3XLIGHTING_HShp: {
                v23 = Lighting30Shader::spPass[39].m_pObject;
                goto LABEL_61;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxShp: {
                NiD3DPass* v24 = Lighting30Shader::spPass[40].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[40].m_pObject);
                SetHeightMap(v24, apL30Prop, 4u);
                return v24;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxoShp: {
                v25 = Lighting30Shader::spPass[41].m_pObject;
                goto LABEL_74;
            }
            case BSShaderManager::BSSM_3XLIGHTING_FgShp: {
                NiD3DPass* v26 = Lighting30Shader::spPass[42].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[42].m_pObject);
                v26->m_kStages.m_pBase[4].m_pObject->m_spTexture = apL30Prop->spTextures[0][1].m_pObject;
                v26->m_kStages.m_pBase[5].m_pObject->m_spTexture = apL30Prop->spTextures[1][1].m_pObject;
                return v26;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SFgShp: {
                NiD3DPass* v27 = Lighting30Shader::spPass[43].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[43].m_pObject);
                v27->m_kStages.m_pBase[4].m_pObject->m_spTexture = apL30Prop->spTextures[0][1].m_pObject;
                v27->m_kStages.m_pBase[5].m_pObject->m_spTexture = apL30Prop->spTextures[1][1].m_pObject;
                NiD3DTextureStage* v28 = v27->m_kStages.m_pBase[6].m_pObject;
                v28->m_spTexture = apL30Prop->GetTexture(BSShaderProperty::TEXTYPE_GLOW, 0);
                return v27;
            }
            case BSShaderManager::BSSM_3XLIGHTING_GShp: {
                NiD3DPass* v29 = Lighting30Shader::spPass[44].m_pObject;
                Lighting30Shader::spPass[44].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(v29);
                return v29;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SGShp: {
                NiD3DPass* v30 = Lighting30Shader::spPass[45].m_pObject;
                Lighting30Shader::spPass[45].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(v30);
                return v30;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcShp: {
                NiD3DPass* v31 = Lighting30Shader::spPass[46].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[46].m_pObject);
                return v31;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSShp: {
                NiD3DPass* v32 = Lighting30Shader::spPass[47].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[47].m_pObject);
                return v32;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxShp: {
                NiD3DPass* v33 = Lighting30Shader::spPass[48].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[48].m_pObject);
                SetHeightMap(v33, apL30Prop, 4u);
                return v33;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxoShp: {
                v25 = Lighting30Shader::spPass[49].m_pObject;
                goto LABEL_74;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcGShp: {
                NiD3DPass* v34 = Lighting30Shader::spPass[50].m_pObject;
                Lighting30Shader::spPass[50].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(v34);
                return v34;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSGShp: {
                NiD3DPass* v35 = Lighting30Shader::spPass[51].m_pObject;
                NiD3DPass* v62 = Lighting30Shader::spPass[51].m_pObject;
                Lighting30Shader::spPass[51].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(v62);
                return v35;
            }
            case BSShaderManager::BSSM_3XLIGHTING_ShpSpc: {
                NiD3DPass* v36 = Lighting30Shader::spPass[52].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[52].m_pObject);
                return v36;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SShpSpc: {
                NiD3DPass* v37 = Lighting30Shader::spPass[53].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[53].m_pObject);
                return v37;
            }
            case BSShaderManager::BSSM_3XLIGHTING_HShpSpc: {
                v23 = Lighting30Shader::spPass[54].m_pObject;
            LABEL_61:;
                Lighting30Shader::kPSConstants[2].x = apL30Prop->pHairTintColor.r;
                Lighting30Shader::kPSConstants[2].y = apL30Prop->pHairTintColor.g;
                Lighting30Shader::kPSConstants[2].z = apL30Prop->pHairTintColor.b;
                Lighting30Shader::kPSConstants[2].w = apL30Prop->pHairTintColor.a;
                v23->m_kStages.m_pBase[4].m_pObject->m_spTexture = (NiTexture*)BSShaderManager::spAnisotropicMap;
                v23->m_kStages.m_pBase[5].m_pObject->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(v23);
                return v23;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxShpSpc: {
                NiD3DPass* v38 = Lighting30Shader::spPass[55].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[55].m_pObject);
                SetHeightMap(v38, apL30Prop, 4u);
                return v38;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxoShpSpc: {
                v25 = Lighting30Shader::spPass[56].m_pObject;
                goto LABEL_74;
            }
            case BSShaderManager::BSSM_3XLIGHTING_FgShpSpc: {
                v11 = Lighting30Shader::spPass[57].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[57].m_pObject);
            LABEL_65:;
                v11->m_kStages.m_pBase[4].m_pObject->m_spTexture = apL30Prop->spTextures[0][1].m_pObject;
                v11->m_kStages.m_pBase[5].m_pObject->m_spTexture = apL30Prop->spTextures[1][1].m_pObject;
                return v11;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SFgShpSpc: {
                v12 = Lighting30Shader::spPass[58].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[58].m_pObject);
            LABEL_67:;
                v12->m_kStages.m_pBase[4].m_pObject->m_spTexture = apL30Prop->spTextures[0][1].m_pObject;
                v12->m_kStages.m_pBase[5].m_pObject->m_spTexture = apL30Prop->spTextures[1][1].m_pObject;
                NiD3DTextureStage* v39 = v12->m_kStages.m_pBase[6].m_pObject;
                v39->m_spTexture = apL30Prop->GetTexture(BSShaderProperty::TEXTYPE_GLOW, 0);
                return v12;
            }
            case BSShaderManager::BSSM_3XLIGHTING_GShpSpc: {
                NiD3DPass* v40 = Lighting30Shader::spPass[59].m_pObject;
                Lighting30Shader::spPass[59].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(v40);
                return v40;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SGShpSpc: {
                NiD3DPass* v41 = Lighting30Shader::spPass[60].m_pObject;
                Lighting30Shader::spPass[60].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(v41);
                return v41;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcShpSpc: {
                NiD3DPass* v42 = Lighting30Shader::spPass[61].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[61].m_pObject);
                return v42;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSShpSpc: {
                NiD3DPass* v43 = Lighting30Shader::spPass[62].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[62].m_pObject);
                return v43;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxShpSpc: {
                NiD3DPass* v44 = Lighting30Shader::spPass[63].m_pObject;
                SetProjShadowMaps(Lighting30Shader::spPass[63].m_pObject);
                SetHeightMap(v44, apL30Prop, 4u);
                return v44;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxoShpSpc: {
                v25 = Lighting30Shader::spPass[64].m_pObject;
            LABEL_74:;
                Lighting30Shader::kPSConstants[6].x = apL30Prop->fParallaxOccMaxPasses;
                Lighting30Shader::kPSConstants[6].y = apL30Prop->fParallaxOccScale;
                SetProjShadowMaps(v25);
                SetHeightMap(v25, apL30Prop, 4u);
                return v25;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcGShpSpc: {
                NiD3DPass* v45 = Lighting30Shader::spPass[65].m_pObject;
                Lighting30Shader::spPass[65].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(v45);
                return v45;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSGShpSpc: {
                NiD3DPass* v46 = Lighting30Shader::spPass[66].m_pObject;
                NiD3DPass* v63 = Lighting30Shader::spPass[66].m_pObject;
                Lighting30Shader::spPass[66].m_pObject->m_kStages.m_pBase[6].m_pObject->m_spTexture =
                    apL30Prop->spTextures[2]->m_pObject;
                SetProjShadowMaps(v63);
                return v46;
            }
            case BSShaderManager::BSSM_3XSILHOUETTE: {
                v54 = Lighting30Shader::spPass[67].m_pObject;
                goto LABEL_111;
            }
            case BSShaderManager::BSSM_3XSILHOUETTE_Vc: {
                v54 = Lighting30Shader::spPass[68].m_pObject;
                goto LABEL_111;
            }
            case BSShaderManager::BSSM_3XENVMAP: {
                ppass_2 = Lighting30Shader::spPass[69].m_pObject;
                goto LABEL_83;
            }
            case BSShaderManager::BSSM_3XENVMAP_W: {
                ppass_2 = Lighting30Shader::spPass[70].m_pObject;
                goto LABEL_83;
            }
            case BSShaderManager::BSSM_3XENVMAP_Vc: {
                ppass_2 = Lighting30Shader::spPass[71].m_pObject;
                goto LABEL_83;
            }
            case BSShaderManager::BSSM_3XENVMAP_WVc: {
                ppass_2 = Lighting30Shader::spPass[72].m_pObject;
                goto LABEL_83;
            }
            case BSShaderManager::BSSM_3XENVMAP_S: {
                ppass_2 = Lighting30Shader::spPass[73].m_pObject;
                goto LABEL_83;
            }
            case BSShaderManager::BSSM_3XENVMAP_SVc: {
                ppass_2 = Lighting30Shader::spPass[74].m_pObject;
            LABEL_83:;
                SetEnvironmentMap(ppass_2, apL30Prop, aePassEnum);
                SetEnvironmentMask(ppass_2, apL30Prop);
                SetNormalMap(ppass_2, apL30Prop, 0);
                return ppass_2;
            }
            case BSShaderManager::BSSM_3XENVMAP_EYE: {
                NiD3DPass* ppass_3 = Lighting30Shader::spPass[75].m_pObject;
                SetEnvironmentMap(Lighting30Shader::spPass[75].m_pObject, apL30Prop, aePassEnum);
                SetEnvironmentMask(ppass_3, apL30Prop);
                SetNormalMap(ppass_3, apL30Prop, 0);
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
                return ppass_3;
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
                NiD3DPass* v52 = Lighting30Shader::spPass[80].m_pObject;
                Lighting30Shader::spPass[80].m_pObject->m_kStages.m_pBase->m_pObject->m_spTexture =
                    apL30Prop->spTextures[0]->m_pObject;
                ClearUnusedStages(v52, 1u);
                return v52;
            }
            case BSShaderManager::BSSM_3XDEPTHMAP_S: {
                NiD3DPass* v53 = Lighting30Shader::spPass[82].m_pObject;
                Lighting30Shader::spPass[82].m_pObject->m_kStages.m_pBase->m_pObject->m_spTexture =
                    apL30Prop->spTextures[0]->m_pObject;
                ClearUnusedStages(v53, 1u);
                return v53;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS: {
                v54 = Lighting30Shader::spPass[84].m_pObject;
                goto LABEL_108;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS_S: {
                NiTexture* v56 = apL30Prop->spTextures[1]->m_pObject;
                NiD3DPass* v57 = Lighting30Shader::spPass[85].m_pObject;
                if (!v56) {
                    v56 = BSShaderManager::spDefaultNormalMap.m_pObject;
                }
                Lighting30Shader::spPass[85].m_pObject->m_kStages.m_pBase->m_pObject->m_spTexture = v56;
                ClearUnusedStages(v57, 1u);
                return v57;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS_FIRE: {
                NiTexture* v55;
                v54 = Lighting30Shader::spPass[86].m_pObject;
            LABEL_108:;
                v55 = apL30Prop->spTextures[1]->m_pObject;
                if (!v55) {
                    v55 = BSShaderManager::spDefaultNormalMap.m_pObject;
                }
                v54->m_kStages.m_pBase->m_pObject->m_spTexture = v55;
            LABEL_111:;
                ClearUnusedStages(v54, 1u);
                return v54;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS_CLEAR: {
                ppass = Lighting30Shader::spPass[87].m_pObject;
                goto LABEL_129;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS_CLEAR_S: {
                ppass = Lighting30Shader::spPass[88].m_pObject;
                goto LABEL_129;
            }
            case BSShaderManager::BSSM_3XCONSTALPHA: {
                return Lighting30Shader::spPass[89].m_pObject;
            }
            case BSShaderManager::BSSM_3XCONSTALPHA_Vc: {
                return Lighting30Shader::spPass[90].m_pObject;
            }
            case BSShaderManager::BSSM_3XCONSTALPHA_S: {
                return Lighting30Shader::spPass[91].m_pObject;
            }
            case BSShaderManager::BSSM_3XCONSTALPHA_SVc: {
                return Lighting30Shader::spPass[92].m_pObject;
            }
            case BSShaderManager::BSSM_3XDEPTH: {
                ppass = Lighting30Shader::spPass[93].m_pObject;
                goto LABEL_129;
            }
            case BSShaderManager::BSSM_3XDEPTH_Vc: {
                ppass = Lighting30Shader::spPass[94].m_pObject;
                goto LABEL_129;
            }
            case BSShaderManager::BSSM_3XDEPTH_S: {
                NiD3DPass* v58 = Lighting30Shader::spPass[95].m_pObject;
                ClearUnusedStages(Lighting30Shader::spPass[95].m_pObject, 0);
                ShadowLightShader::UpdateFarNear();
                return v58;
            }
            case BSShaderManager::BSSM_3XDEPTH_SVc: {
                NiD3DPass* v59 = Lighting30Shader::spPass[96].m_pObject;
                ClearUnusedStages(Lighting30Shader::spPass[96].m_pObject, 0);
                ShadowLightShader::UpdateFarNear();
                return v59;
            }
            case BSShaderManager::BSSM_3XVELOCITY: {
                NiD3DPass* v60 = Lighting30Shader::spPass[97].m_pObject;
                ClearUnusedStages(Lighting30Shader::spPass[97].m_pObject, 0);
                Lighting30Shader::kVSConstants[41].y =
                    static_cast<ImageSpaceEffectMotionBlur*>(
                        BSShaderManager::pImageSpaceManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_MOTION_BLUR]
                    )
                        ->GetObjectBlurConst();
                Lighting30Shader::kVSConstants[41].z =
                    static_cast<ImageSpaceEffectMotionBlur*>(
                        BSShaderManager::pImageSpaceManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_MOTION_BLUR]
                    )
                        ->Params.r;
                Lighting30Shader::kVSConstants[41].w = 0.0f;
                ShadowLightShader::UpdateFarNear();
                return v60;
            }
            case BSShaderManager::BSSM_3XVELOCITY_S: {
                NiD3DPass* v61 = Lighting30Shader::spPass[98].m_pObject;
                ClearUnusedStages(Lighting30Shader::spPass[98].m_pObject, 0);
                Lighting30Shader::kVSConstants[41].y =
                    static_cast<ImageSpaceEffectMotionBlur*>(
                        BSShaderManager::pImageSpaceManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_MOTION_BLUR]
                    )
                        ->GetObjectBlurConst();
                Lighting30Shader::kVSConstants[41].z =
                    static_cast<ImageSpaceEffectMotionBlur*>(
                        BSShaderManager::pImageSpaceManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_MOTION_BLUR]
                    )
                        ->Params.r;
                Lighting30Shader::kVSConstants[41].w = 0.0f;
                return v61;
            }
            case BSShaderManager::BSSM_3XLOCALMAP: {
                ppass = Lighting30Shader::spPass[99].m_pObject;
                goto LABEL_129;
            }
            case BSShaderManager::BSSM_3XLOCALMAP_S: {
                ppass = Lighting30Shader::spPass[100].m_pObject;
                goto LABEL_129;
            }
            case BSShaderManager::BSSM_3XLOCALMAP_CLEAR: {
                ppass = Lighting30Shader::spPass[101].m_pObject;
            LABEL_129:;
                ClearUnusedStages(ppass, 0);
                goto LABEL_130;
            }
            case BSShaderManager::BSSM_3XTEXEFFECT: {
                NiD3DPass* v50 = Lighting30Shader::spPass[102].m_pObject;
                UpdatePipelineTexEffect(apL30Prop, Lighting30Shader::spPass[102].m_pObject);
                return v50;
            }
            case BSShaderManager::BSSM_3XTEXEFFECT_S: {
                NiD3DPass* v51 = Lighting30Shader::spPass[103].m_pObject;
                UpdatePipelineTexEffect(apL30Prop, Lighting30Shader::spPass[103].m_pObject);
                return v51;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH: {
                return Lighting30Shader::spPass[104].m_pObject;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_S: {
                return Lighting30Shader::spPass[105].m_pObject;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_At: {
                return Lighting30Shader::spPass[106].m_pObject;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_SAt: {
                return Lighting30Shader::spPass[107].m_pObject;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_Vc: {
                return Lighting30Shader::spPass[108].m_pObject;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_SVc: {
                return Lighting30Shader::spPass[109].m_pObject;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_AtVc: {
                return Lighting30Shader::spPass[110].m_pObject;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_SAtVc: {
                return Lighting30Shader::spPass[111].m_pObject;
            }
            default: {
            LABEL_130:;
                result = ppass;
                break;
            }
        }
    }
    else {
        if (aePassEnum == BSShaderManager::BSSM_3XOCCLUSION) {
            return Lighting30Shader::spPass[0].m_pObject;
        }
        else {
            switch (aePassEnum) {
                case BSShaderManager::BSSM_3XZONLY: {
                    result = Lighting30Shader::spPass[1].m_pObject;
                    break;
                }
                case BSShaderManager::BSSM_3XZONLY_S: {
                    result = Lighting30Shader::spPass[2].m_pObject;
                    break;
                }
                case BSShaderManager::BSSM_3XZONLY_Vc: {
                    result = Lighting30Shader::spPass[3].m_pObject;
                    break;
                }
                case BSShaderManager::BSSM_3XZONLY_SVc: {
                    result = Lighting30Shader::spPass[4].m_pObject;
                    break;
                }
                case BSShaderManager::BSSM_3XZONLY_TEXEFFECT: {
                    NiD3DPass* v6 = Lighting30Shader::spPass[5].m_pObject;
                    UpdatePipelineZOnlyTexEffect(apL30Prop, Lighting30Shader::spPass[5].m_pObject);
                    result = v6;
                    break;
                }
                case BSShaderManager::BSSM_3XZONLY_TEXEFFECT_S: {
                    NiD3DPass* v7 = Lighting30Shader::spPass[6].m_pObject;
                    UpdatePipelineZOnlyTexEffect(apL30Prop, Lighting30Shader::spPass[6].m_pObject);
                    result = v7;
                    break;
                }
                default: {
                    goto LABEL_130;
                }
            }
        }
    }
    return result;
}

void Lighting30Shader::SetHairMaps(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop) {
    apPass->m_kStages.m_pBase[4].m_pObject->m_spTexture = BSShaderManager::spAnisotropicMap;
    apPass->m_kStages.m_pBase[5].m_pObject->m_spTexture = apL30Prop->spTextures[2]->m_pObject;
}

void Lighting30Shader::SetNormalMap(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop, uint32_t auiStageNum) {
    NiSourceTexture* m_pObject;
    m_pObject = static_cast<NiSourceTexture*>(apL30Prop->spTextures[1]->m_pObject);
    NiD3DTextureStage* v5 = apPass->m_kStages.m_pBase[auiStageNum].m_pObject;
    if (!m_pObject) {
        m_pObject = BSShaderManager::spDefaultNormalGlossMap;
        if ((apL30Prop->ulFlags[0] & 128) == 0) {
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
    apPass->m_kStages.m_pBase[2].m_pObject->m_spTexture = BSShaderManager::pProjectedShadowTexture.m_pObject;
    apPass->m_kStages.m_pBase[3].m_pObject->m_spTexture =
        static_cast<NiTexture*>(BSShaderManager::pProjectedShadowMaskTexture.m_pObject);
}

void Lighting30Shader::SetEnvironmentMap(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop, uint32_t auiPass) {
    NiSourceCubeMap* preflectionmap;
    ShadowSceneNode* pActiveShadowSceneNode = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode;
    NiTexture* preflectionmap_2 = apL30Prop->GetTexture(BSShaderProperty::TEXTYPE_ENV, 0);
    if (preflectionmap_2) {
        goto LABEL_12;
    }
    preflectionmap = BSShaderManager::spArmorReflectionCubeMap;
    if (auiPass == 446 || auiPass == 448) {
        goto LABEL_8;
    }
    if ((apL30Prop->ulFlags[0] & BSS1_Eye_Environment_Mapping) != 0) {
        preflectionmap_2 = BSShaderManager::spEyeReflectionCubeMap;
        goto LABEL_10;
    }
    if (!BSShaderManager::bInterior || !apL30Prop->GetFlag(21u)) {
    LABEL_8:;
        if (!pActiveShadowSceneNode) {
        LABEL_11:;
            preflectionmap_2 = preflectionmap;
            goto LABEL_12;
        }
    }
    preflectionmap_2 = preflectionmap;
LABEL_10:;
    if (!preflectionmap_2) {
        goto LABEL_11;
    }
LABEL_12:;
    apPass->m_kStages.m_pBase[1].m_pObject->m_spTexture = preflectionmap_2;
}
