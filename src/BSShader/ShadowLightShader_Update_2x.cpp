#include "BSShader/BSFogProperty.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/ImageSpaceEffectMotionBlur.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/ShadowSceneLight.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"

// Params.g over the default target's height, divided in double and rounded to float. The unsigned height is converted
// as a signed value plus the float 2^32 when negative, as the game does; that addition rounds at the thread's precision
// control (24 bits under Direct3D), where a 64-bit conversion of the height would not.
float ImageSpaceEffectMotionBlur::GetObjectBlurConst() {
    NiRenderTargetGroup* pGroup = BSShaderManager::pRenderer.m_pObject->GetDefaultRenderTargetGroup();
    double dBlur = Params.g;
    int32_t iHeight = static_cast<int32_t>(pGroup->GetHeight(0));
    double dHeight = iHeight;
    if (iHeight < 0) {
        dHeight += 4294967296.0f;
    }
    return static_cast<float>(dBlur / dHeight);
}

void ShadowLightShader::UpdateDiffuseNormalStages(uint32_t auiCurrentPass, BSShaderPPLightingProperty* apProp) {
    NiD3DTextureStage* m_pObject;
    int32_t v5;
    NiD3DTextureStage* v6;
    NiSourceTexture* v7;
    int32_t v8;
    ShadowSceneLight* v9;
    BSRenderedTexture* v10;
    NiD3DTextureStage* v11;
    NiD3DPass* v3 = NiD3DShader::m_spCurrentPass;
    if (((((uint32_t)(((int32_t)auiCurrentPass) - 445)) > 36) ||
         (((auiCurrentPass >= 452) && (auiCurrentPass <= 455)) ||
          (((uint32_t)(((int32_t)auiCurrentPass) - 456)) <= 3))) &&
        ((auiCurrentPass < 728) || (auiCurrentPass > 751))) {
        m_pObject = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
        m_pObject->m_spTexture = apProp->spTextures[0]->m_pObject;
        v5 = (int32_t)apProp->GetClampNI();
        m_pObject->SetClampMode(static_cast<NiTexturingProperty::ClampMode>(v5));
        v6 = v3->m_kStages.m_pBase[1].m_pObject;
        if (v6) {
            v7 = static_cast<NiSourceTexture*>(apProp->spTextures[1]->m_pObject);
            if (!v7) {
                v7 = static_cast<NiSourceTexture*>(BSShaderManager::spDefaultNormalMap.m_pObject);
            }
            v6->m_spTexture = v7;
            v8 = (int32_t)apProp->GetClampNI();
            v6->SetClampMode(static_cast<NiTexturingProperty::ClampMode>(v8));
        }
        if (((uint32_t)(BSShaderManager::eCurrentPass - 452)) <= 3) {
            if (apProp->GetUnk68()) {
                v9 = *BSShaderManager::pCurrentRenderPass->ppSceneLights;
                if (v9->bCastShadow) {
                    v10 = v9->spDynamicShadowMap.m_pObject;
                    if (v10) {
                        if (BSShaderManager::pCurrentRenderPass->ucNumLights) {
                            v11 = v3->m_kStages.m_pBase[2].m_pObject;
                            v11->m_spTexture = v10->GetTexture(0);
                            v11->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
                            v11->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
                        }
                    }
                }
            }
        }
    }
}

void ShadowLightShader::UpdateTogglesConstant(NiGeometry* a1, BSFogProperty* a2, NiMaterialProperty* a3) {
    double m_fShine;
    float v6;
    float g = NiColorA_BLACK.g;
    BSShaderProperty* pShaderProp = static_cast<BSShaderProperty*>(a1->m_PropertyState.m_aspProps[3].m_pObject);
    if ((pShaderProp->ulFlags[1] & BSS2_Vertex_Colors) != 0) {
        v6 = 1.0f;
    }
    else {
        v6 = 0.0f;
    }
    if (a2) {
        g = 1.0f;
    }
    if (a3) {
        m_fShine = a3->m_fShine;
    }
    else {
        m_fShine = 30.0f;
    }
    float v7 = m_fShine;
    BSShaderLightingProperty::pShaderLightConstants[33].y = g;
    BSShaderLightingProperty::pShaderLightConstants[33].x = v6;
    BSShaderLightingProperty::pShaderLightConstants[33].z = v7;
    BSShaderLightingProperty::pShaderLightConstants[33].w = NiColorA_BLACK.a;
}

void ShadowLightShader::UpdateTogglesConstantLand(
    NiGeometry* apGeometry,
    BSFogProperty* apFog,
    BSShaderPPLightingProperty* apShaderProp,
    int32_t aiTextureIndex
) {
    float g;
    g = NiColorA_BLACK.g;
    if (apFog) {
        g = (float)1.0;
    }
    float v7 = (float)(double)apShaderProp->pSpecularExponent[aiTextureIndex];
    int32_t v6 = (int32_t)apShaderProp->QLandscapeTextureCount();
    BSShaderLightingProperty::pShaderLightConstants[33].y = g;
    BSShaderLightingProperty::pShaderLightConstants[33].x = (float)1.0;
    BSShaderLightingProperty::pShaderLightConstants[33].z = v7;
    float v8 = (float)(((double)v6) + 1.0);
    BSShaderLightingProperty::pShaderLightConstants[33].w = v8;
}

void ShadowLightShader::EnableConstantsForPass_2x(uint32_t auiPassEnum) {
    ShadowLightShader::EnableConstantsForPass_1x(auiPassEnum);
    ShadowLightShader::spPixelConstantMapEntries[15]->m_bEnabled =
        (bool)((uint8_t)((((int32_t)auiPassEnum) == 193) || (((int32_t)auiPassEnum) == 194)));
}
