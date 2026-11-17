#include "BSShader/WaterShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "BSShader/WaterShaderProperty.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

static bool data_11FF374;

using WaterShaderUnusedBytes = uint8_t[0x22];
static WaterShaderUnusedBytes data_11FF376{};

using WaterShaderUnusedConstantArray = NiPoint4[3];
static WaterShaderUnusedConstantArray data_11FF468{};
NiRTTI WaterShader::ms_RTTI("WaterShader", &NiD3DDefaultShader::ms_RTTI);

// The shader becomes the instance and resets the shared water state and constant enables.
WaterShader::WaterShader() {
    iShaderType = 13;
    m_bUsesNiRenderState = 1;
    WaterShader::pInstance = this;
    WaterShader::kNormalOffset.x = 0.0f;
    WaterShader::bUnderWater = 0;
    WaterShader::bNoReflections = 0;
    WaterShader::bNoRefractions = 0;
    WaterShader::kNormalOffset.y = 0.0f;
    WaterShader::kNormalOffset.z = 0.0f;
    WaterShader::kNormalOffset.w = 0.0f;
    if (BSShaderManager::eShaderLevel <= 2) {
        WaterShader::bDepth = 0;
    }
    WaterShader::bVSConstantEnable[0] = 0;
    WaterShader::bVSConstantEnable[1] = 0;
    WaterShader::bVSConstantEnable[2] = 0;
    WaterShader::bPSConstantEnable[0] = 0;
    WaterShader::bPSConstantEnable[1] = 0;
    WaterShader::bPSConstantEnable[2] = 0;
    WaterShader::bPSConstantEnable[3] = 0;
    WaterShader::bPSConstantEnable[4] = 0;
    WaterShader::bPSConstantEnable[5] = 0;
}

// The shared constant data is cleared to black; the passes, shaders and textures release themselves.
WaterShader::~WaterShader() {
    for (NiColorA& kColor : WaterShader::VertexConstantData) {
        kColor = NiColorA_BLACK;
    }
    for (NiColorA& kColor : WaterShader::PixelConstantData) {
        kColor = NiColorA_BLACK;
    }
}

const NiRTTI* WaterShader::GetRTTI() const {
    return &WaterShader::ms_RTTI;
}

// Releases the shaders, passes and constant map entries and clears the constant values to black.
bool WaterShader::Initialize() {
    bool bResult = BSShader::Initialize();
    spWaterVertexShader = nullptr;
    spWaterVertexShader_Vc = nullptr;
    spWaterVertexShader_Lighting = nullptr;
    for (uint32_t i = 0; i < 74; ++i) {
        spPass[i] = nullptr;
    }
    for (uint32_t i = 0; i < 38; ++i) {
        spWaterPixelShader[i] = nullptr;
    }
    for (NiShaderConstantMapEntry*& pEntry : pVertexConstantMapEntry) {
        if (pEntry) {
            NiDecRefCount(pEntry);
            pEntry = nullptr;
        }
    }
    for (NiShaderConstantMapEntry*& pEntry : pPixelConstantMapEntry) {
        if (pEntry) {
            NiDecRefCount(pEntry);
            pEntry = nullptr;
        }
    }
    for (NiColorA& kColor : VertexConstantData) {
        kColor = NiColorA_BLACK;
    }
    for (NiColorA& kColor : PixelConstantData) {
        kColor = NiColorA_BLACK;
    }
    return bResult;
}

void WaterShader::SetupGeometryTextures(const NiPropertyState* apState) {
    BSShaderManager::eBSSMPassEnum v2 = BSShaderManager::eCurrentPass;
    NiD3DPass* v3 = NiD3DShader::m_spCurrentPass;
    WaterShaderProperty* m_pObject = static_cast<WaterShaderProperty*>(apState->m_aspProps[3].m_pObject);
    NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject->m_spTexture = 0;
    v3->m_kStages.m_pBase[1].m_pObject->m_spTexture = 0;
    v3->m_kStages.m_pBase[2].m_pObject->m_spTexture = 0;
    v3->m_kStages.m_pBase[3].m_pObject->m_spTexture = 0;
    v3->m_kStages.m_pBase[4].m_pObject->m_spTexture = 0;
    if (v2 != BSShaderManager::BSSM_WATER_STENCIL && v2 != BSShaderManager::BSSM_WATER_STENCIL_Vc) {
        BSRenderedTexture* v5 = m_pObject->spReflectionMap.m_pObject;
        if (v5 && m_pObject->bReflections) {
            NiD3DTextureStage* v6 = v3->m_kStages.m_pBase->m_pObject;
            v6->m_spTexture = (NiTexture*)v5->GetTexture(0);
        }
        else {
            v3->m_kStages.m_pBase->m_pObject->m_spTexture = BSShaderManager::spDefaultNormalMap.m_pObject;
        }
        if (BSShaderManager::pWaterRefractionTexture) {
            if (m_pObject->bRefractions) {
                NiD3DTextureStage* v7 = v3->m_kStages.m_pBase[1].m_pObject;
                v7->m_spTexture = (NiTexture*)BSShaderManager::pWaterRefractionTexture->GetTexture(0);
            }
        }
        BSRenderedTexture* v8 = m_pObject->spNoiseNormalMap.m_pObject;
        if (v8) {
            NiD3DTextureStage* v9 = v3->m_kStages.m_pBase[2].m_pObject;
            v9->m_spTexture = (NiTexture*)v8->GetTexture(0);
        }
        else {
            v3->m_kStages.m_pBase[2].m_pObject->m_spTexture = BSShaderManager::spDefaultNormalMap.m_pObject;
        }
        if (m_pObject->bDisplacement) {
            NiRenderedTexture* v10 = m_pObject->spDisplacementNormalMap.m_pObject;
            if (v10) {
                v3->m_kStages.m_pBase[3].m_pObject->m_spTexture = v10;
            }
        }
        BSRenderedTexture* v11 = m_pObject->spDepthMap.m_pObject;
        if (v11) {
            if (m_pObject->bDepth) {
                NiD3DTextureStage* v12 = v3->m_kStages.m_pBase[4].m_pObject;
                v12->m_spTexture = (NiTexture*)v11->GetTexture(0);
            }
        }
        ApplyTextureStages();
    }
}

void WaterShader::SetupGeometryConstants(const NiPropertyState* apState) {
    BSShaderManager::eBSSMPassEnum v2 = BSShaderManager::eCurrentPass;
    NiGeometry* pgeometry = BSShaderManager::pCurrentRenderPass->pGeometry;
    BSNiBound* worldBound = pgeometry->m_kWorldBound;
    if (!worldBound) {
        worldBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
    }
    NiGeometryBufferData* pbufferData = pgeometry->m_spModelData.m_pObject->m_pkBuffData;
    const NiSkinInstance* pskin = pgeometry->m_spSkinInstance.m_pObject;
    m_pkD3DRenderer->SetModelTransform(pgeometry->m_kWorld, 0);
    float eyePos = BSShaderLightingProperty::pShaderLightConstants[30].X() - NiRenderer::PosAdjust.x;
    float eyePos_4 = BSShaderLightingProperty::pShaderLightConstants[30].Y() - NiRenderer::PosAdjust.y;
    float eyePos_8 = BSShaderLightingProperty::pShaderLightConstants[30].Z() - NiRenderer::PosAdjust.z;
    WaterShader::PixelConstantData[1].r = eyePos;
    WaterShader::PixelConstantData[1].g = eyePos_4;
    WaterShader::PixelConstantData[1].b = eyePos_8;
    WaterShader::PixelConstantData[1].a = 0.0f;
    WaterShaderProperty* pProperty = static_cast<WaterShaderProperty*>(apState->m_aspProps[3].m_pObject);
    WaterShader::PixelConstantData[2] = pProperty->pShallowColor;
    WaterShader::PixelConstantData[3] = pProperty->pDeepColor;
    WaterShader::PixelConstantData[4] = pProperty->pReflectionColor;
    WaterShader::PixelConstantData[5].r = pProperty->fFresnelAmount;
    WaterShader::PixelConstantData[5].b = pProperty->pFresnelRI.z;
    // Stores the quotient as a float, then clamps it below at 1.0 (a NaN is kept).
    float fFresnelPower = pProperty->pFresnelRI.w / 10.0;
    double dFresnelPower = fFresnelPower;
    if (fFresnelPower < 1.0) {
        dFresnelPower = 1.0;
    }
    WaterShader::PixelConstantData[5].a = dFresnelPower;
    WaterShader::PixelConstantData[6].r = pProperty->fBlendRadius;
    WaterShader::PixelConstantData[6].g = pProperty->fBlendNormalsAmount;
    WaterShader::PixelConstantData[6].b = pProperty->pBlendRadius.z;
    WaterShader::PixelConstantData[6].a = TESWaterSystem::fDisplacementDampener;
    WaterShader::PixelConstantData[7].r = NiRenderer::PosAdjust.x;
    WaterShader::VertexConstantData[9].r = NiRenderer::PosAdjust.x;
    WaterShader::PixelConstantData[7].g = NiRenderer::PosAdjust.y;
    WaterShader::VertexConstantData[9].g = NiRenderer::PosAdjust.y;
    WaterShader::PixelConstantData[7].b = NiRenderer::PosAdjust.z;
    WaterShader::VertexConstantData[9].b = NiRenderer::PosAdjust.z;
    WaterShader::PixelConstantData[8].r = pProperty->pVarAmounts.x;
    WaterShader::PixelConstantData[8].g = pProperty->pVarAmounts.y;
    WaterShader::PixelConstantData[8].b = pProperty->pVarAmounts.z;
    WaterShader::PixelConstantData[8].a = pProperty->pVarAmounts.w;
    BSFogProperty* pfog = BSShaderManager::GetFogProperty(BSShaderManager::cSceneGraph);
    if (pfog) {
        float fEndDistance = pfog->fEndDistance;
        float fPower = pfog->fPower;
        WaterShader::PixelConstantData[9].r = fEndDistance;
        WaterShader::PixelConstantData[9].g = fEndDistance - pfog->fStartDistance;
        WaterShader::PixelConstantData[9].b = pProperty->fFogFar;
        WaterShader::PixelConstantData[9].a = pProperty->fFogRange;
        WaterShader::PixelConstantData[10].r = pfog->m_kColor.r;
        WaterShader::PixelConstantData[10].g = pfog->m_kColor.g;
        WaterShader::PixelConstantData[10].b = pfog->m_kColor.b;
        WaterShader::PixelConstantData[5].g = fPower;
        WaterShader::PixelConstantData[10].a = pProperty->fFogAmount;
    }
    WaterShader::PixelConstantData[11].r = pProperty->pDepthFalloff.x;
    WaterShader::PixelConstantData[11].g = pProperty->pDepthFalloff.y;
    WaterShader::PixelConstantData[11].b = pProperty->pDepthFalloff.z;
    WaterShader::PixelConstantData[11].a = pProperty->pDepthFalloff.w;
    WaterShader::PixelConstantData[12].r = WaterShader::pSunDir.x;
    WaterShader::PixelConstantData[12].g = WaterShader::pSunDir.y;
    WaterShader::PixelConstantData[12].b = WaterShader::pSunDir.z;
    WaterShader::PixelConstantData[12].a = WaterShader::pSunDir.w;
    WaterShader::PixelConstantData[13] = WaterShader::pSunColor;
    float fUVScale = pProperty->fUVScale;
    WaterShader::VertexConstantData[8].r = fUVScale;
    WaterShader::VertexConstantData[8].g = fUVScale;
    WaterShader::VertexConstantData[8].b = fUVScale;
    WaterShader::VertexConstantData[8].a = fUVScale;
    if (v2 == BSShaderManager::BSSM_WATER_STENCIL || v2 == BSShaderManager::BSSM_WATER_STENCIL_Vc) {
        BSRenderState::SetStencilWriteMask(pProperty->iStencilMask, BSRS_NOLOCK);
    }
    else {
        if (pProperty->bDisplacement) {
            if (v2 == BSShaderManager::BSSM_WATER_WADING_SPECULAR_LIGHTING ||
                v2 == BSShaderManager::BSSM_WATER_WADING_SPECULAR_LIGHTING_Vc) {
                BSRenderState::SetStencilEnable(1, BSRS_NOLOCK);
                BSRenderState::SetStencilFunc(
                    NiStencilProperty::TEST_EQUAL, 255u, pProperty->iStencilMask, BSRS_NOLOCK
                );
                BSRenderState::SetStencilAction(
                    NiStencilProperty::ACTION_KEEP, NiStencilProperty::ACTION_KEEP, NiStencilProperty::ACTION_KEEP,
                    BSRS_NOLOCK
                );
            }
            else {
                BSRenderState::SetStencilEnable(1, BSRS_NOLOCK);
                BSRenderState::SetStencilFunc(
                    NiStencilProperty::TEST_EQUAL, 255u, pProperty->iStencilMask, BSRS_NOLOCK
                );
                BSRenderState::SetStencilAction(
                    NiStencilProperty::ACTION_KEEP, NiStencilProperty::ACTION_KEEP, NiStencilProperty::ACTION_INCREMENT,
                    BSRS_NOLOCK
                );
            }
            BSRenderState::SetStencilWriteMask(255u, BSRS_NOLOCK);
        }
    }
    float fObjectTexCoords = static_cast<float>(pProperty->bObjectTexCoords);
    WaterShader::VertexConstantData[10].r = fObjectTexCoords;
    WaterShader::VertexConstantData[10].g = fObjectTexCoords;
    WaterShader::VertexConstantData[10].b = fObjectTexCoords;
    WaterShader::VertexConstantData[10].a = fObjectTexCoords;
    WaterShader::PixelConstantData[14].r = static_cast<float>(pProperty->LightList.m_uiCount);
    // Each light fills one position constant from 15 on and one color constant from 23 on.
    NiTListItem<NiLight*>* pkIter = pProperty->LightList.m_pkHead;
    uint32_t uiLight = 0;
    while (pkIter) {
        NiLight* pLight = pkIter->m_element;
        pkIter = pkIter->m_pkNext;
        if (pLight) {
            NiPoint3 kPos = pLight->GetWorldTransform().m_Translate;
            WaterShader::PixelConstantData[15 + uiLight].r = kPos.x;
            WaterShader::PixelConstantData[15 + uiLight].g = kPos.y;
            WaterShader::PixelConstantData[15 + uiLight].b = kPos.z;
            WaterShader::PixelConstantData[15 + uiLight].a = pLight->m_kSpec.r;
            float fDimmer = pLight->m_fDimmer;
            float fRed = pLight->m_kDiff.r * fDimmer;
            float fGreen = pLight->m_kDiff.g * fDimmer;
            float fBlue = fDimmer * pLight->m_kDiff.b;
            float fScale = pProperty->pBlendRadius.w;
            fRed = fRed * fScale;
            fGreen = fScale * fGreen;
            fBlue = fScale * fBlue;
            WaterShader::PixelConstantData[23 + uiLight].r = fRed;
            WaterShader::PixelConstantData[23 + uiLight].g = fGreen;
            WaterShader::PixelConstantData[23 + uiLight].b = fBlue;
            WaterShader::PixelConstantData[23 + uiLight].a = 1.0f;
            ++uiLight;
        }
    }
    NiD3DShaderConstantMap* v13 = m_spPixelConstantMap.m_pObject;
    if (v13) {
        NiD3DPixelShader* v14 = NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject;
        if (v14) {
            v13->SetShaderConstants(
                v14, pgeometry, pskin, 0, pbufferData, apState, 0, pgeometry->m_kWorld, *worldBound, 0, 1
            );
        }
    }
    NiD3DShaderConstantMap* v15 = m_spVertexConstantMap.m_pObject;
    if (v15) {
        NiD3DVertexShader* v16 = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
        if (v16) {
            v15->SetShaderConstants(
                v16, pgeometry, pskin, 0, pbufferData, apState, 0, pgeometry->m_kWorld, *worldBound, 0, 1
            );
        }
    }
}

void WaterShader::PostGeometry(const NiPropertyState* apState) {
    WaterShaderProperty* pProperty = static_cast<WaterShaderProperty*>(apState->m_aspProps[3].m_pObject);
    BSShaderManager::eBSSMPassEnum v3 = BSShaderManager::eCurrentPass;
    if (pProperty) {
        if ((pProperty->ulFlags[1] & BSS2_Wireframe) != 0) {
            BSRenderState::SetWireframeEnable(0, BSRS_NOLOCK);
        }
    }
    if (v3 != BSShaderManager::BSSM_WATER_STENCIL && v3 != BSShaderManager::BSSM_WATER_STENCIL_Vc) {
        if (pProperty->bDisplacement) {
            BSRenderState::SetStencilEnable(0, BSRS_NOLOCK);
        }
    }
}

void WaterShader::InitPass(uint32_t auiCurrentPass) {
    switch ((int32_t)auiCurrentPass) {
        case 575: {
            NiD3DShader::m_spCurrentPass = spPass[33].m_pObject;
            break;
        }
        case 576: {
            NiD3DShader::m_spCurrentPass = spPass[34].m_pObject;
            break;
        }
        case 648: {
            NiD3DShader::m_spCurrentPass = spPass[35].m_pObject;
            break;
        }
        case 649: {
            NiD3DShader::m_spCurrentPass = spPass[71].m_pObject;
            break;
        }
        case 650: {
            NiD3DShader::m_spCurrentPass = spPass[17].m_pObject;
            break;
        }
        case 651: {
            NiD3DShader::m_spCurrentPass = spPass[20].m_pObject;
            break;
        }
        case 652: {
            NiD3DShader::m_spCurrentPass = spPass[19].m_pObject;
            break;
        }
        case 653: {
            NiD3DShader::m_spCurrentPass = spPass[18].m_pObject;
            break;
        }
        case 654: {
            NiD3DShader::m_spCurrentPass = spPass[21].m_pObject;
            break;
        }
        case 655: {
            NiD3DShader::m_spCurrentPass = spPass[24].m_pObject;
            break;
        }
        case 656: {
            NiD3DShader::m_spCurrentPass = spPass[23].m_pObject;
            break;
        }
        case 657: {
            NiD3DShader::m_spCurrentPass = spPass[22].m_pObject;
            break;
        }
        case 658: {
            NiD3DShader::m_spCurrentPass = spPass[25].m_pObject;
            break;
        }
        case 659: {
            NiD3DShader::m_spCurrentPass = spPass[28].m_pObject;
            break;
        }
        case 660: {
            NiD3DShader::m_spCurrentPass = spPass[27].m_pObject;
            break;
        }
        case 661: {
            NiD3DShader::m_spCurrentPass = spPass[26].m_pObject;
            break;
        }
        case 662: {
            NiD3DShader::m_spCurrentPass = spPass[29].m_pObject;
            break;
        }
        case 663: {
            NiD3DShader::m_spCurrentPass = spPass[32].m_pObject;
            break;
        }
        case 664: {
            NiD3DShader::m_spCurrentPass = spPass[31].m_pObject;
            break;
        }
        case 665: {
            NiD3DShader::m_spCurrentPass = spPass[30].m_pObject;
            break;
        }
        case 666: {
            NiD3DShader::m_spCurrentPass = spPass[37].m_pObject;
            break;
        }
        case 667: {
            NiD3DShader::m_spCurrentPass = spPass[55].m_pObject;
            break;
        }
        case 668: {
            NiD3DShader::m_spCurrentPass = spPass[56].m_pObject;
            break;
        }
        case 669: {
            NiD3DShader::m_spCurrentPass = spPass[57].m_pObject;
            break;
        }
        case 670: {
            NiD3DShader::m_spCurrentPass = spPass[58].m_pObject;
            break;
        }
        case 671: {
            NiD3DShader::m_spCurrentPass = spPass[59].m_pObject;
            break;
        }
        case 672: {
            NiD3DShader::m_spCurrentPass = spPass[60].m_pObject;
            break;
        }
        case 673: {
            NiD3DShader::m_spCurrentPass = spPass[61].m_pObject;
            break;
        }
        case 674: {
            NiD3DShader::m_spCurrentPass = spPass[62].m_pObject;
            break;
        }
        case 675: {
            NiD3DShader::m_spCurrentPass = spPass[63].m_pObject;
            break;
        }
        case 676: {
            NiD3DShader::m_spCurrentPass = spPass[64].m_pObject;
            break;
        }
        case 677: {
            NiD3DShader::m_spCurrentPass = spPass[65].m_pObject;
            break;
        }
        case 678: {
            NiD3DShader::m_spCurrentPass = spPass[66].m_pObject;
            break;
        }
        case 679: {
            NiD3DShader::m_spCurrentPass = spPass[67].m_pObject;
            break;
        }
        case 680: {
            NiD3DShader::m_spCurrentPass = spPass[68].m_pObject;
            break;
        }
        case 681: {
            NiD3DShader::m_spCurrentPass = spPass[69].m_pObject;
            break;
        }
        case 682: {
            NiD3DShader::m_spCurrentPass = spPass[70].m_pObject;
            break;
        }
        case 683: {
            NiD3DShader::m_spCurrentPass = spPass[73].m_pObject;
            break;
        }
        case 684: {
            NiD3DShader::m_spCurrentPass = spPass[0].m_pObject;
            break;
        }
        case 685: {
            NiD3DShader::m_spCurrentPass = spPass[3].m_pObject;
            break;
        }
        case 686: {
            NiD3DShader::m_spCurrentPass = spPass[2].m_pObject;
            break;
        }
        case 687: {
            NiD3DShader::m_spCurrentPass = spPass[1].m_pObject;
            break;
        }
        case 688: {
            NiD3DShader::m_spCurrentPass = spPass[8].m_pObject;
            break;
        }
        case 689: {
            NiD3DShader::m_spCurrentPass = spPass[11].m_pObject;
            break;
        }
        case 690: {
            NiD3DShader::m_spCurrentPass = spPass[10].m_pObject;
            break;
        }
        case 691: {
            NiD3DShader::m_spCurrentPass = spPass[9].m_pObject;
            break;
        }
        case 692: {
            NiD3DShader::m_spCurrentPass = spPass[4].m_pObject;
            break;
        }
        case 693: {
            NiD3DShader::m_spCurrentPass = spPass[5].m_pObject;
            break;
        }
        case 694: {
            NiD3DShader::m_spCurrentPass = spPass[6].m_pObject;
            break;
        }
        case 695: {
            NiD3DShader::m_spCurrentPass = spPass[7].m_pObject;
            break;
        }
        case 696: {
            NiD3DShader::m_spCurrentPass = spPass[12].m_pObject;
            break;
        }
        case 697: {
            NiD3DShader::m_spCurrentPass = spPass[15].m_pObject;
            break;
        }
        case 698: {
            NiD3DShader::m_spCurrentPass = spPass[14].m_pObject;
            break;
        }
        case 699: {
            NiD3DShader::m_spCurrentPass = spPass[13].m_pObject;
            break;
        }
        case 700: {
            NiD3DShader::m_spCurrentPass = spPass[36].m_pObject;
            break;
        }
        case 701: {
            NiD3DShader::m_spCurrentPass = spPass[16].m_pObject;
            break;
        }
        case 702: {
            NiD3DShader::m_spCurrentPass = spPass[38].m_pObject;
            break;
        }
        case 703: {
            NiD3DShader::m_spCurrentPass = spPass[41].m_pObject;
            break;
        }
        case 704: {
            NiD3DShader::m_spCurrentPass = spPass[40].m_pObject;
            break;
        }
        case 705: {
            NiD3DShader::m_spCurrentPass = spPass[39].m_pObject;
            break;
        }
        case 706: {
            NiD3DShader::m_spCurrentPass = spPass[46].m_pObject;
            break;
        }
        case 707: {
            NiD3DShader::m_spCurrentPass = spPass[49].m_pObject;
            break;
        }
        case 708: {
            NiD3DShader::m_spCurrentPass = spPass[48].m_pObject;
            break;
        }
        case 709: {
            NiD3DShader::m_spCurrentPass = spPass[47].m_pObject;
            break;
        }
        case 710: {
            NiD3DShader::m_spCurrentPass = spPass[42].m_pObject;
            break;
        }
        case 711: {
            NiD3DShader::m_spCurrentPass = spPass[43].m_pObject;
            break;
        }
        case 712: {
            NiD3DShader::m_spCurrentPass = spPass[44].m_pObject;
            break;
        }
        case 713: {
            NiD3DShader::m_spCurrentPass = spPass[45].m_pObject;
            break;
        }
        case 714: {
            NiD3DShader::m_spCurrentPass = spPass[50].m_pObject;
            break;
        }
        case 715: {
            NiD3DShader::m_spCurrentPass = spPass[53].m_pObject;
            break;
        }
        case 716: {
            NiD3DShader::m_spCurrentPass = spPass[52].m_pObject;
            break;
        }
        case 717: {
            NiD3DShader::m_spCurrentPass = spPass[51].m_pObject;
            break;
        }
        case 718: {
            NiD3DShader::m_spCurrentPass = spPass[72].m_pObject;
            break;
        }
        default: {
            return;
        }
    }
}

void WaterShader::SetupPassConstants(uint32_t auiCurrentPass) {
    uint32_t v2 = 0;
    switch (auiCurrentPass) {
        case 575u: {
            WaterShader::EnableConstantsForPass(33u, 0);
            break;
        }
        case 576u: {
            WaterShader::EnableConstantsForPass(34u, 0);
            break;
        }
        case 648u: {
            WaterShader::EnableConstantsForPass(35u, 0);
            break;
        }
        case 649u: {
            WaterShader::EnableConstantsForPass(71u, 0);
            break;
        }
        case 650u: {
            WaterShader::EnableConstantsForPass(17u, 0);
            break;
        }
        case 651u: {
            WaterShader::EnableConstantsForPass(20u, 0);
            break;
        }
        case 652u: {
            WaterShader::EnableConstantsForPass(19u, 0);
            break;
        }
        case 653u: {
            WaterShader::EnableConstantsForPass(18u, 0);
            break;
        }
        case 654u: {
            WaterShader::EnableConstantsForPass(21u, 0);
            break;
        }
        case 655u: {
            WaterShader::EnableConstantsForPass(24u, 0);
            break;
        }
        case 656u: {
            WaterShader::EnableConstantsForPass(23u, 0);
            break;
        }
        case 657u: {
            WaterShader::EnableConstantsForPass(22u, 0);
            break;
        }
        case 658u: {
            WaterShader::EnableConstantsForPass(25u, 0);
            break;
        }
        case 659u: {
            WaterShader::EnableConstantsForPass(28u, 0);
            break;
        }
        case 660u: {
            WaterShader::EnableConstantsForPass(27u, 0);
            break;
        }
        case 661u: {
            WaterShader::EnableConstantsForPass(26u, 0);
            break;
        }
        case 662u: {
            WaterShader::EnableConstantsForPass(29u, 0);
            break;
        }
        case 663u: {
            WaterShader::EnableConstantsForPass(32u, 0);
            break;
        }
        case 664u: {
            WaterShader::EnableConstantsForPass(31u, 0);
            break;
        }
        case 665u: {
            WaterShader::EnableConstantsForPass(30u, 0);
            break;
        }
        case 666u: {
            WaterShader::EnableConstantsForPass(37u, 0);
            break;
        }
        case 667u: {
            WaterShader::EnableConstantsForPass(55u, 0);
            break;
        }
        case 668u: {
            WaterShader::EnableConstantsForPass(56u, 0);
            break;
        }
        case 669u: {
            WaterShader::EnableConstantsForPass(57u, 0);
            break;
        }
        case 670u: {
            WaterShader::EnableConstantsForPass(58u, 0);
            break;
        }
        case 671u: {
            WaterShader::EnableConstantsForPass(59u, 0);
            break;
        }
        case 672u: {
            WaterShader::EnableConstantsForPass(60u, 0);
            break;
        }
        case 673u: {
            WaterShader::EnableConstantsForPass(61u, 0);
            break;
        }
        case 674u: {
            WaterShader::EnableConstantsForPass(62u, 0);
            break;
        }
        case 675u: {
            WaterShader::EnableConstantsForPass(63u, 0);
            break;
        }
        case 676u: {
            WaterShader::EnableConstantsForPass(64u, 0);
            break;
        }
        case 677u: {
            WaterShader::EnableConstantsForPass(65u, 0);
            break;
        }
        case 678u: {
            WaterShader::EnableConstantsForPass(66u, 0);
            break;
        }
        case 679u: {
            WaterShader::EnableConstantsForPass(67u, 0);
            break;
        }
        case 680u: {
            WaterShader::EnableConstantsForPass(68u, 0);
            break;
        }
        case 681u: {
            WaterShader::EnableConstantsForPass(69u, 0);
            break;
        }
        case 682u: {
            WaterShader::EnableConstantsForPass(70u, 0);
            break;
        }
        case 683u: {
            WaterShader::EnableConstantsForPass(73u, 0);
            break;
        }
        case 684u: {
            WaterShader::EnableConstantsForPass(0, 0);
            break;
        }
        case 685u: {
            WaterShader::EnableConstantsForPass(3u, 0);
            break;
        }
        case 686u: {
            WaterShader::EnableConstantsForPass(2u, 0);
            break;
        }
        case 687u: {
            WaterShader::EnableConstantsForPass(1u, 0);
            break;
        }
        case 688u: {
            WaterShader::EnableConstantsForPass(8u, 0);
            break;
        }
        case 689u: {
            WaterShader::EnableConstantsForPass(11u, 0);
            break;
        }
        case 690u: {
            WaterShader::EnableConstantsForPass(10u, 0);
            break;
        }
        case 691u: {
            WaterShader::EnableConstantsForPass(9u, 0);
            break;
        }
        case 692u: {
            WaterShader::EnableConstantsForPass(4u, 0);
            break;
        }
        case 693u: {
            WaterShader::EnableConstantsForPass(5u, 0);
            break;
        }
        case 694u: {
            WaterShader::EnableConstantsForPass(6u, 0);
            break;
        }
        case 695u: {
            WaterShader::EnableConstantsForPass(7u, 0);
            break;
        }
        case 696u: {
            WaterShader::EnableConstantsForPass(12u, 0);
            break;
        }
        case 697u: {
            WaterShader::EnableConstantsForPass(15u, 0);
            break;
        }
        case 698u: {
            WaterShader::EnableConstantsForPass(14u, 0);
            break;
        }
        case 699u: {
            WaterShader::EnableConstantsForPass(13u, 0);
            break;
        }
        case 700u: {
            WaterShader::EnableConstantsForPass(36u, 0);
            break;
        }
        case 701u: {
            v2 = 16;
            goto LABEL_75;
        }
        case 702u: {
            WaterShader::EnableConstantsForPass(38u, 0);
            break;
        }
        case 703u: {
            WaterShader::EnableConstantsForPass(41u, 0);
            break;
        }
        case 704u: {
            WaterShader::EnableConstantsForPass(40u, 0);
            break;
        }
        case 705u: {
            WaterShader::EnableConstantsForPass(39u, 0);
            break;
        }
        case 706u: {
            WaterShader::EnableConstantsForPass(46u, 0);
            break;
        }
        case 707u: {
            WaterShader::EnableConstantsForPass(49u, 0);
            break;
        }
        case 708u: {
            WaterShader::EnableConstantsForPass(48u, 0);
            break;
        }
        case 709u: {
            WaterShader::EnableConstantsForPass(47u, 0);
            break;
        }
        case 710u: {
            WaterShader::EnableConstantsForPass(42u, 0);
            break;
        }
        case 711u: {
            WaterShader::EnableConstantsForPass(43u, 0);
            break;
        }
        case 712u: {
            WaterShader::EnableConstantsForPass(44u, 0);
            break;
        }
        case 713u: {
            WaterShader::EnableConstantsForPass(45u, 0);
            break;
        }
        case 714u: {
            WaterShader::EnableConstantsForPass(50u, 0);
            break;
        }
        case 715u: {
            WaterShader::EnableConstantsForPass(53u, 0);
            break;
        }
        case 716u: {
            WaterShader::EnableConstantsForPass(52u, 0);
            break;
        }
        case 717u: {
            WaterShader::EnableConstantsForPass(51u, 0);
            break;
        }
        case 718u: {
            WaterShader::EnableConstantsForPass(72u, 0);
            break;
        }
        default: {
        LABEL_75:;
            WaterShader::EnableConstantsForPass(v2, 0);
            break;
        }
    }
}

// Creates the vertex map (the two transforms and CommonData's vertex block) and the pixel map (CommonData's pixel
// block, the variable amounts, fog, depth, sun light and the light data) when they are missing, and keeps the entries
// it looks up. The constant blocks are sub-ranges of the tables Initialize clears: CommonData::VX is
// VertexConstantData[8], CommonData::PX is PixelConstantData[1], then VarAmounts, FogData, DepthData, SunLightData and
// LightData follow it. The whole body runs in the water heap context.
void WaterShader::CreateConstantMaps() {
    TLSData* pTLS = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLS->eMemContext;
    pTLS->eMemContext = MEM_CONTEXT_WATER;
    if (!m_spVertexConstantMap.m_pObject) {
        void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
        NiD3DShaderConstantMap* pMap = nullptr;
        if (pMemory) {
            pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        }
        m_spVertexConstantMap = pMap;
        NiShaderConstantMapEntry* pEntry = nullptr;
        NiShaderConstantMapEntry* pOldEntry = nullptr;
        m_spVertexConstantMap.m_pObject->AddPredefinedEntry(
            "WorldViewProjTranspose", 0x20000009, 0, "", 0, 0, nullptr, false
        );
        // EXE-INLINED 0x66B0D0
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("WorldViewProjTranspose");
        pOldEntry = pVertexConstantMapEntry[0];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pVertexConstantMapEntry[0] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spVertexConstantMap.m_pObject->AddPredefinedEntry("WorldTranspose", 0x20000009, 4, "", 0, 0, nullptr, false);
        // EXE-INLINED 0x66B0D0
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("WorldTranspose");
        pOldEntry = pVertexConstantMapEntry[1];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pVertexConstantMapEntry[1] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spVertexConstantMap.m_pObject->AddEntry(
            "CommonData", 0x10000009, 0, 8, 3, "", 0x30, 4, &VertexConstantData[8], false
        );
        // EXE-INLINED 0x66B0D0
        pEntry = m_spVertexConstantMap.m_pObject->GetEntry("CommonData");
        pOldEntry = pVertexConstantMapEntry[2];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pVertexConstantMapEntry[2] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
    }
    if (!m_spPixelConstantMap.m_pObject) {
        void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
        NiD3DShaderConstantMap* pMap = nullptr;
        if (pMemory) {
            pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        }
        m_spPixelConstantMap = pMap;
        NiShaderConstantMapEntry* pEntry = nullptr;
        NiShaderConstantMapEntry* pOldEntry = nullptr;
        m_spPixelConstantMap.m_pObject->AddEntry(
            "CommonData", 0x10000009, 0, 1, 7, "", 0x70, 4, &PixelConstantData[1], false
        );
        // EXE-INLINED 0x66B0D0
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("CommonData");
        pOldEntry = pPixelConstantMapEntry[0];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[0] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spPixelConstantMap.m_pObject->AddEntry(
            "VarAmounts", 0x10000007, 0, 8, 1, "", 0x10, 4, &PixelConstantData[8], false
        );
        // EXE-INLINED 0x66B0D0
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("VarAmounts");
        pOldEntry = pPixelConstantMapEntry[1];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[1] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spPixelConstantMap.m_pObject->AddEntry(
            "FogData", 0x1000000C, 0, 9, 2, "", 0x20, 4, &PixelConstantData[9], false
        );
        // EXE-INLINED 0x66B0D0
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("FogData");
        pOldEntry = pPixelConstantMapEntry[2];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[2] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spPixelConstantMap.m_pObject->AddEntry(
            "DepthData", 0x10000007, 0, 11, 1, "", 0x10, 4, &PixelConstantData[11], false
        );
        // EXE-INLINED 0x66B0D0
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("DepthData");
        pOldEntry = pPixelConstantMapEntry[3];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[3] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spPixelConstantMap.m_pObject->AddEntry(
            "SunLightData", 0x1000000C, 0, 12, 2, "", 0x20, 4, &PixelConstantData[12], false
        );
        // EXE-INLINED 0x66B0D0
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("SunLightData");
        pOldEntry = pPixelConstantMapEntry[4];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[4] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spPixelConstantMap.m_pObject->AddEntry(
            "LightData", 0x10000009, 0, 14, 17, "", 0x110, 4, &PixelConstantData[14], false
        );
        // EXE-INLINED 0x66B0D0
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("LightData");
        pOldEntry = pPixelConstantMapEntry[5];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[5] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
    }
    pTLS->eMemContext = eMemContext;
}

void WaterShader::ReloadShaders() {
    LoadShaders();
    LoadStagesAndPasses();
}

void WaterShader::ClearPassStages() {
    for (uint32_t i = 0; i < 74; ++i) {
        ClearPass(spPass[i].m_pObject);
    }
}

bool WaterShader::LoadStagesAndPasses() {
    int32_t i = 0;
    NiPointer<NiD3DPass>* spPass = this->WaterShader::spPass;
    do {
        NiD3DPass* m_pObject;
        NiD3DPass* v4;
        NiD3DPass* v5;
        int32_t v9;
        if (spPass->m_pObject) {
            goto LABEL_67;
        }
        NiD3DPass::CreateNewPass(*spPass);
        {
            NiPointer<NiD3DTextureStage> spStage;
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, 1);
            spStage.m_pObject->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
            spPass->m_pObject->SetStage(spPass->m_pObject->m_uiCurrentStage, spStage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 1u, 3u, 2u, 1);
            spStage.m_pObject->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
            spPass->m_pObject->SetStage(spPass->m_pObject->m_uiCurrentStage, spStage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 2u, 1u, 2u, 1);
            spStage.m_pObject->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
            spPass->m_pObject->SetStage(spPass->m_pObject->m_uiCurrentStage, spStage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 3u, 1u, 2u, 1);
            spStage.m_pObject->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
            spPass->m_pObject->SetStage(spPass->m_pObject->m_uiCurrentStage, spStage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 4u, 3u, 2u, 1);
            spStage.m_pObject->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
            spPass->m_pObject->SetStage(spPass->m_pObject->m_uiCurrentStage, spStage.m_pObject);
            m_pObject = spPass->m_pObject;
            if (!spPass->m_pObject->m_pkRenderStateGroup) {
                m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 1u, 1);
            v4 = spPass->m_pObject;
            if (!spPass->m_pObject->m_pkRenderStateGroup) {
                v4->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v4->m_pkRenderStateGroup->SetRenderState(24u, 0, 1);
            v5 = spPass->m_pObject;
            if (!spPass->m_pObject->m_pkRenderStateGroup) {
                v5->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v5->m_pkRenderStateGroup->SetRenderState(25u, 5u, 1);
            if (i != 33) {
                if (i == 36 || (i == 72 || (i == 37 || i == 73))) {
                    NiD3DPass* v7 = spPass->m_pObject;
                    if (!spPass->m_pObject->m_pkRenderStateGroup) {
                        v7->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                    }
                    v7->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
                    NiD3DPass* v8 = spPass->m_pObject;
                    if (!spPass->m_pObject->m_pkRenderStateGroup) {
                        v8->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                    }
                    v8->m_pkRenderStateGroup->SetRenderState(19u, 5u, 1);
                    if (!v8->m_pkRenderStateGroup) {
                        v8->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                    }
                    v8->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
                }
                else {
                    NiD3DPass* v6 = spPass->m_pObject;
                    if (!spPass->m_pObject->m_pkRenderStateGroup) {
                        v6->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                    }
                    v6->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
                    spPass->m_pObject->SetSrcDestBlendRenderState(5u, 6u, 0);
                }
            }
            v9 = i;
            if ((i < 17 || i > 28) && (i < 55 || i > 66)) {
                if (i > 28 && ((i < 38 || i > 66) && (i != 36 && i != 72))) {
                    goto LABEL_47;
                }
                NiD3DPass* v11 = spPass->m_pObject;
                if (!spPass->m_pObject->m_pkRenderStateGroup) {
                    v11->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                }
                v11->m_pkRenderStateGroup->SetRenderState(52u, 1u, 1);
                NiD3DPass* v12 = spPass->m_pObject;
                if (!spPass->m_pObject->m_pkRenderStateGroup) {
                    v12->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                }
                v12->m_pkRenderStateGroup->SetRenderState(56u, 6u, 1);
                NiD3DPass* v13 = spPass->m_pObject;
                if (!spPass->m_pObject->m_pkRenderStateGroup) {
                    v13->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                }
                v13->m_pkRenderStateGroup->SetRenderState(57u, 1u, 1);
                NiD3DPass* v14 = spPass->m_pObject;
                if (!spPass->m_pObject->m_pkRenderStateGroup) {
                    v14->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                }
                v14->m_pkRenderStateGroup->SetRenderState(58u, 1u, 1);
                NiD3DPass* v15 = spPass->m_pObject;
                if (!spPass->m_pObject->m_pkRenderStateGroup) {
                    v15->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                }
                v15->m_pkRenderStateGroup->SetRenderState(55u, 1u, 1);
            }
            else {
                NiD3DPass* v10 = spPass->m_pObject;
                if (!spPass->m_pObject->m_pkRenderStateGroup) {
                    v10->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                }
                v10->m_pkRenderStateGroup->SetRenderState(52u, 1u, 1);
            }
            v9 = i;
        LABEL_47:;
            if (v9 == 35 || v9 == 71) {
                NiD3DPass* v16 = spPass->m_pObject;
                if (!spPass->m_pObject->m_pkRenderStateGroup) {
                    v16->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                }
                v16->m_pkRenderStateGroup->SetRenderState(168u, 0, 1);
                NiD3DPass* v17 = spPass->m_pObject;
                if (!spPass->m_pObject->m_pkRenderStateGroup) {
                    v17->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                }
                v17->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
                NiD3DPass* v18 = spPass->m_pObject;
                if (!spPass->m_pObject->m_pkRenderStateGroup) {
                    v18->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                }
                v18->m_pkRenderStateGroup->SetRenderState(52u, 1u, 1);
                NiD3DPass* v19 = spPass->m_pObject;
                if (!spPass->m_pObject->m_pkRenderStateGroup) {
                    v19->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                }
                v19->m_pkRenderStateGroup->SetRenderState(56u, 3u, 1);
                NiD3DPass* v20 = spPass->m_pObject;
                if (!spPass->m_pObject->m_pkRenderStateGroup) {
                    v20->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                }
                v20->m_pkRenderStateGroup->SetRenderState(57u, 255u, 1);
                NiD3DPass* v21 = spPass->m_pObject;
                if (!spPass->m_pObject->m_pkRenderStateGroup) {
                    v21->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                }
                v21->m_pkRenderStateGroup->SetRenderState(55u, 3u, 1);
                NiD3DPass* v22 = spPass->m_pObject;
                if (!spPass->m_pObject->m_pkRenderStateGroup) {
                    v22->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                }
                v22->m_pkRenderStateGroup->SetRenderState(58u, 0, 1);
            }
        }
    LABEL_67:;
        ++spPass;
        ++i;
    }
    while (i < 74);
    NiD3DPass* v24 = this->WaterShader::spPass[0].m_pObject;
    v24->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v28 = this->WaterShader::spPass[0].m_pObject;
    NiD3DPixelShader* v29 = spWaterPixelShader[0].m_pObject;
    v28->m_spPixelShader = v29;
    WaterShader::ulPixelConstantFlags[0] = 62;
    NiD3DPass* v32 = this->WaterShader::spPass[1].m_pObject;
    v32->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v36 = this->WaterShader::spPass[1].m_pObject;
    NiD3DPixelShader* v37 = spWaterPixelShader[1].m_pObject;
    v36->m_spPixelShader = v37;
    WaterShader::ulPixelConstantFlags[1] = 62;
    NiD3DPass* v40 = this->WaterShader::spPass[2].m_pObject;
    v40->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v44 = this->WaterShader::spPass[2].m_pObject;
    NiD3DPixelShader* v45 = spWaterPixelShader[2].m_pObject;
    v44->m_spPixelShader = v45;
    WaterShader::ulPixelConstantFlags[2] = 62;
    NiD3DPass* v48 = this->WaterShader::spPass[3].m_pObject;
    v48->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v52 = this->WaterShader::spPass[3].m_pObject;
    NiD3DPixelShader* v53 = spWaterPixelShader[3].m_pObject;
    v52->m_spPixelShader = v53;
    WaterShader::ulPixelConstantFlags[3] = 62;
    NiD3DPass* v56 = this->WaterShader::spPass[4].m_pObject;
    v56->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v60 = this->WaterShader::spPass[4].m_pObject;
    NiD3DPixelShader* v61 = spWaterPixelShader[4].m_pObject;
    v60->m_spPixelShader = v61;
    WaterShader::ulPixelConstantFlags[4] = 46;
    NiD3DPass* v64 = this->WaterShader::spPass[5].m_pObject;
    v64->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v68 = this->WaterShader::spPass[5].m_pObject;
    NiD3DPixelShader* v69 = spWaterPixelShader[5].m_pObject;
    v68->m_spPixelShader = v69;
    WaterShader::ulPixelConstantFlags[5] = 46;
    NiD3DPass* v72 = this->WaterShader::spPass[6].m_pObject;
    v72->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v76 = this->WaterShader::spPass[6].m_pObject;
    NiD3DPixelShader* v77 = spWaterPixelShader[6].m_pObject;
    v76->m_spPixelShader = v77;
    WaterShader::ulPixelConstantFlags[6] = 46;
    NiD3DPass* v80 = this->WaterShader::spPass[7].m_pObject;
    v80->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v84 = this->WaterShader::spPass[7].m_pObject;
    NiD3DPixelShader* v85 = spWaterPixelShader[7].m_pObject;
    v84->m_spPixelShader = v85;
    WaterShader::ulPixelConstantFlags[7] = 46;
    NiD3DPass* v88 = this->WaterShader::spPass[8].m_pObject;
    v88->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v92 = this->WaterShader::spPass[8].m_pObject;
    NiD3DPixelShader* v93 = spWaterPixelShader[8].m_pObject;
    v92->m_spPixelShader = v93;
    WaterShader::ulPixelConstantFlags[8] = 30;
    NiD3DPass* v96 = this->WaterShader::spPass[9].m_pObject;
    v96->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v100 = this->WaterShader::spPass[9].m_pObject;
    NiD3DPixelShader* v101 = spWaterPixelShader[9].m_pObject;
    v100->m_spPixelShader = v101;
    WaterShader::ulPixelConstantFlags[9] = 30;
    NiD3DPass* v104 = this->WaterShader::spPass[10].m_pObject;
    v104->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v108 = this->WaterShader::spPass[10].m_pObject;
    NiD3DPixelShader* v109 = spWaterPixelShader[10].m_pObject;
    v108->m_spPixelShader = v109;
    WaterShader::ulPixelConstantFlags[10] = 30;
    NiD3DPass* v112 = this->WaterShader::spPass[11].m_pObject;
    v112->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v116 = this->WaterShader::spPass[11].m_pObject;
    NiD3DPixelShader* v117 = spWaterPixelShader[11].m_pObject;
    v116->m_spPixelShader = v117;
    WaterShader::ulPixelConstantFlags[11] = 30;
    NiD3DPass* v120 = this->WaterShader::spPass[12].m_pObject;
    v120->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v124 = this->WaterShader::spPass[12].m_pObject;
    NiD3DPixelShader* v125 = spWaterPixelShader[12].m_pObject;
    v124->m_spPixelShader = v125;
    WaterShader::ulPixelConstantFlags[12] = 14;
    NiD3DPass* v128 = this->WaterShader::spPass[13].m_pObject;
    v128->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v132 = this->WaterShader::spPass[13].m_pObject;
    NiD3DPixelShader* v133 = spWaterPixelShader[13].m_pObject;
    v132->m_spPixelShader = v133;
    WaterShader::ulPixelConstantFlags[13] = 14;
    NiD3DPass* v136 = this->WaterShader::spPass[14].m_pObject;
    v136->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v140 = this->WaterShader::spPass[14].m_pObject;
    NiD3DPixelShader* v141 = spWaterPixelShader[14].m_pObject;
    v140->m_spPixelShader = v141;
    WaterShader::ulPixelConstantFlags[14] = 14;
    NiD3DPass* v144 = this->WaterShader::spPass[15].m_pObject;
    v144->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v148 = this->WaterShader::spPass[15].m_pObject;
    NiD3DPixelShader* v149 = spWaterPixelShader[15].m_pObject;
    v148->m_spPixelShader = v149;
    WaterShader::ulPixelConstantFlags[15] = 14;
    NiD3DPass* v152 = this->WaterShader::spPass[16].m_pObject;
    v152->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v156 = this->WaterShader::spPass[16].m_pObject;
    NiD3DPixelShader* v157 = spWaterPixelShader[16].m_pObject;
    v156->m_spPixelShader = v157;
    WaterShader::ulPixelConstantFlags[16] = 14;
    NiD3DPass* v160 = this->WaterShader::spPass[17].m_pObject;
    v160->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v164 = this->WaterShader::spPass[17].m_pObject;
    NiD3DPixelShader* v165 = spWaterPixelShader[17].m_pObject;
    v164->m_spPixelShader = v165;
    WaterShader::ulPixelConstantFlags[17] = 62;
    NiD3DPass* v168 = this->WaterShader::spPass[18].m_pObject;
    v168->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v172 = this->WaterShader::spPass[18].m_pObject;
    NiD3DPixelShader* v173 = spWaterPixelShader[18].m_pObject;
    v172->m_spPixelShader = v173;
    WaterShader::ulPixelConstantFlags[18] = 62;
    NiD3DPass* v176 = this->WaterShader::spPass[19].m_pObject;
    v176->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v180 = this->WaterShader::spPass[19].m_pObject;
    NiD3DPixelShader* v181 = spWaterPixelShader[19].m_pObject;
    v180->m_spPixelShader = v181;
    WaterShader::ulPixelConstantFlags[19] = 62;
    NiD3DPass* v184 = this->WaterShader::spPass[20].m_pObject;
    v184->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v188 = this->WaterShader::spPass[20].m_pObject;
    NiD3DPixelShader* v189 = spWaterPixelShader[20].m_pObject;
    v188->m_spPixelShader = v189;
    WaterShader::ulPixelConstantFlags[20] = 62;
    NiD3DPass* v192 = this->WaterShader::spPass[21].m_pObject;
    v192->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v196 = this->WaterShader::spPass[21].m_pObject;
    NiD3DPixelShader* v197 = spWaterPixelShader[21].m_pObject;
    v196->m_spPixelShader = v197;
    WaterShader::ulPixelConstantFlags[21] = 46;
    NiD3DPass* v200 = this->WaterShader::spPass[22].m_pObject;
    v200->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v204 = this->WaterShader::spPass[22].m_pObject;
    NiD3DPixelShader* v205 = spWaterPixelShader[22].m_pObject;
    v204->m_spPixelShader = v205;
    WaterShader::ulPixelConstantFlags[22] = 46;
    NiD3DPass* v208 = this->WaterShader::spPass[23].m_pObject;
    v208->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v212 = this->WaterShader::spPass[23].m_pObject;
    NiD3DPixelShader* v213 = spWaterPixelShader[23].m_pObject;
    v212->m_spPixelShader = v213;
    WaterShader::ulPixelConstantFlags[23] = 46;
    NiD3DPass* v216 = this->WaterShader::spPass[24].m_pObject;
    v216->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v220 = this->WaterShader::spPass[24].m_pObject;
    NiD3DPixelShader* v221 = spWaterPixelShader[24].m_pObject;
    v220->m_spPixelShader = v221;
    WaterShader::ulPixelConstantFlags[24] = 46;
    NiD3DPass* v224 = this->WaterShader::spPass[25].m_pObject;
    v224->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v228 = this->WaterShader::spPass[25].m_pObject;
    NiD3DPixelShader* v229 = spWaterPixelShader[25].m_pObject;
    v228->m_spPixelShader = v229;
    WaterShader::ulPixelConstantFlags[25] = 30;
    NiD3DPass* v232 = this->WaterShader::spPass[26].m_pObject;
    v232->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v236 = this->WaterShader::spPass[26].m_pObject;
    NiD3DPixelShader* v237 = spWaterPixelShader[26].m_pObject;
    v236->m_spPixelShader = v237;
    WaterShader::ulPixelConstantFlags[26] = 30;
    NiD3DPass* v240 = this->WaterShader::spPass[27].m_pObject;
    v240->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v244 = this->WaterShader::spPass[27].m_pObject;
    NiD3DPixelShader* v245 = spWaterPixelShader[27].m_pObject;
    v244->m_spPixelShader = v245;
    WaterShader::ulPixelConstantFlags[27] = 30;
    NiD3DPass* v248 = this->WaterShader::spPass[28].m_pObject;
    v248->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v252 = this->WaterShader::spPass[28].m_pObject;
    NiD3DPixelShader* v253 = spWaterPixelShader[28].m_pObject;
    v252->m_spPixelShader = v253;
    WaterShader::ulPixelConstantFlags[28] = 30;
    NiD3DPass* v256 = this->WaterShader::spPass[29].m_pObject;
    v256->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v260 = this->WaterShader::spPass[29].m_pObject;
    NiD3DPixelShader* v261 = spWaterPixelShader[29].m_pObject;
    v260->m_spPixelShader = v261;
    // Index 28 is stored again here and index 29 is never written, as the game does.
    WaterShader::ulPixelConstantFlags[28] = 14;
    NiD3DPass* v264 = this->WaterShader::spPass[30].m_pObject;
    v264->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v268 = this->WaterShader::spPass[30].m_pObject;
    NiD3DPixelShader* v269 = spWaterPixelShader[30].m_pObject;
    v268->m_spPixelShader = v269;
    WaterShader::ulPixelConstantFlags[30] = 14;
    NiD3DPass* v272 = this->WaterShader::spPass[31].m_pObject;
    v272->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v276 = this->WaterShader::spPass[31].m_pObject;
    NiD3DPixelShader* v277 = spWaterPixelShader[31].m_pObject;
    v276->m_spPixelShader = v277;
    WaterShader::ulPixelConstantFlags[31] = 14;
    NiD3DPass* v280 = this->WaterShader::spPass[32].m_pObject;
    v280->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v284 = this->WaterShader::spPass[32].m_pObject;
    NiD3DPixelShader* v285 = spWaterPixelShader[32].m_pObject;
    v284->m_spPixelShader = v285;
    WaterShader::ulPixelConstantFlags[32] = 14;
    NiD3DPass* v288 = this->WaterShader::spPass[33].m_pObject;
    v288->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v292 = this->WaterShader::spPass[33].m_pObject;
    NiD3DPixelShader* v293 = spWaterPixelShader[33].m_pObject;
    v292->m_spPixelShader = v293;
    WaterShader::ulPixelConstantFlags[33] = 46;
    NiD3DPass* v296 = this->WaterShader::spPass[34].m_pObject;
    v296->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v300 = this->WaterShader::spPass[34].m_pObject;
    NiD3DPixelShader* v301 = spWaterPixelShader[34].m_pObject;
    v300->m_spPixelShader = v301;
    WaterShader::ulPixelConstantFlags[34] = 46;
    NiD3DPass* v304 = this->WaterShader::spPass[35].m_pObject;
    v304->m_spVertexShader = spWaterVertexShader.m_pObject;
    NiD3DPass* v308 = this->WaterShader::spPass[35].m_pObject;
    NiD3DPixelShader* v309 = spWaterPixelShader[35].m_pObject;
    v308->m_spPixelShader = v309;
    WaterShader::ulPixelConstantFlags[35] = 0;
    NiD3DPass* v312 = this->WaterShader::spPass[36].m_pObject;
    v312->m_spVertexShader = spWaterVertexShader_Lighting.m_pObject;
    NiD3DPass* v316 = this->WaterShader::spPass[36].m_pObject;
    NiD3DPixelShader* v317 = spWaterPixelShader[36].m_pObject;
    v316->m_spPixelShader = v317;
    WaterShader::ulPixelConstantFlags[36] = 78;
    NiD3DPass* v320 = this->WaterShader::spPass[37].m_pObject;
    v320->m_spVertexShader = spWaterVertexShader_Lighting.m_pObject;
    NiD3DPass* v324 = this->WaterShader::spPass[37].m_pObject;
    NiD3DPixelShader* v325 = spWaterPixelShader[37].m_pObject;
    v324->m_spPixelShader = v325;
    WaterShader::ulPixelConstantFlags[37] = 78;
    NiD3DPass* v328 = this->WaterShader::spPass[38].m_pObject;
    v328->m_spVertexShader = spWaterVertexShader_Vc.m_pObject;
    NiD3DPass* v332 = this->WaterShader::spPass[38].m_pObject;
    NiD3DPixelShader* v333 = spWaterPixelShader[0].m_pObject;
    v332->m_spPixelShader = v333;
    WaterShader::ulPixelConstantFlags[38] = 62;
    NiD3DPass* v336 = this->WaterShader::spPass[39].m_pObject;
    v336->m_spVertexShader = spWaterVertexShader_Vc.m_pObject;
    NiD3DPass* v340 = this->WaterShader::spPass[39].m_pObject;
    NiD3DPixelShader* v341 = spWaterPixelShader[1].m_pObject;
    v340->m_spPixelShader = v341;
    WaterShader::ulPixelConstantFlags[39] = 62;
    NiD3DPass* v344 = this->WaterShader::spPass[40].m_pObject;
    v344->m_spVertexShader = spWaterVertexShader_Vc.m_pObject;
    NiD3DPass* v348 = this->WaterShader::spPass[40].m_pObject;
    NiD3DPixelShader* v349 = spWaterPixelShader[2].m_pObject;
    v348->m_spPixelShader = v349;
    WaterShader::ulPixelConstantFlags[40] = 62;
    NiD3DPass* v352 = this->WaterShader::spPass[41].m_pObject;
    v352->m_spVertexShader = spWaterVertexShader_Vc.m_pObject;
    NiD3DPass* v356 = this->WaterShader::spPass[41].m_pObject;
    NiD3DPixelShader* v357 = spWaterPixelShader[3].m_pObject;
    v356->m_spPixelShader = v357;
    WaterShader::ulPixelConstantFlags[41] = 62;
    NiD3DPass* v360 = this->WaterShader::spPass[42].m_pObject;
    v360->m_spVertexShader = spWaterVertexShader_Vc.m_pObject;
    NiD3DPass* v364 = this->WaterShader::spPass[42].m_pObject;
    NiD3DPixelShader* v365 = spWaterPixelShader[4].m_pObject;
    v364->m_spPixelShader = v365;
    WaterShader::ulPixelConstantFlags[42] = 46;
    NiD3DPass* v368 = this->WaterShader::spPass[43].m_pObject;
    v368->m_spVertexShader = spWaterVertexShader_Vc.m_pObject;
    NiD3DPass* v372 = this->WaterShader::spPass[43].m_pObject;
    NiD3DPixelShader* v373 = spWaterPixelShader[5].m_pObject;
    v372->m_spPixelShader = v373;
    WaterShader::ulPixelConstantFlags[43] = 46;
    NiD3DPass* v376 = this->WaterShader::spPass[44].m_pObject;
    v376->m_spVertexShader = spWaterVertexShader_Vc.m_pObject;
    NiD3DPass* v380 = this->WaterShader::spPass[44].m_pObject;
    NiD3DPixelShader* v381 = spWaterPixelShader[6].m_pObject;
    v380->m_spPixelShader = v381;
    WaterShader::ulPixelConstantFlags[44] = 46;
    NiD3DPass* v384 = this->WaterShader::spPass[45].m_pObject;
    v384->m_spVertexShader = spWaterVertexShader_Vc.m_pObject;
    NiD3DPass* v388 = this->WaterShader::spPass[45].m_pObject;
    NiD3DPixelShader* v389 = spWaterPixelShader[7].m_pObject;
    v388->m_spPixelShader = v389;
    WaterShader::ulPixelConstantFlags[45] = 46;
    NiD3DPass* v392 = this->WaterShader::spPass[46].m_pObject;
    v392->m_spVertexShader = spWaterVertexShader_Vc.m_pObject;
    NiD3DPass* v396 = this->WaterShader::spPass[46].m_pObject;
    NiD3DPixelShader* v397 = spWaterPixelShader[8].m_pObject;
    v396->m_spPixelShader = v397;
    WaterShader::ulPixelConstantFlags[46] = 30;
    NiD3DPass* v400 = this->WaterShader::spPass[47].m_pObject;
    v400->m_spVertexShader = spWaterVertexShader_Vc.m_pObject;
    NiD3DPass* v404 = this->WaterShader::spPass[47].m_pObject;
    NiD3DPixelShader* v405 = spWaterPixelShader[9].m_pObject;
    v404->m_spPixelShader = v405;
    WaterShader::ulPixelConstantFlags[47] = 30;
    NiD3DPass* v408 = this->WaterShader::spPass[48].m_pObject;
    v408->m_spVertexShader = spWaterVertexShader_Vc.m_pObject;
    NiD3DPass* v412 = this->WaterShader::spPass[48].m_pObject;
    NiD3DPixelShader* v413 = spWaterPixelShader[10].m_pObject;
    v412->m_spPixelShader = v413;
    WaterShader::ulPixelConstantFlags[48] = 30;
    NiD3DPass* v416 = this->WaterShader::spPass[49].m_pObject;
    v416->m_spVertexShader = spWaterVertexShader_Vc.m_pObject;
    NiD3DPass* v420 = this->WaterShader::spPass[49].m_pObject;
    NiD3DPixelShader* v421 = spWaterPixelShader[11].m_pObject;
    v420->m_spPixelShader = v421;
    WaterShader::ulPixelConstantFlags[49] = 30;
    NiD3DPass* v424 = this->WaterShader::spPass[50].m_pObject;
    v424->m_spVertexShader = spWaterVertexShader_Vc.m_pObject;
    this->WaterShader::spPass[50].m_pObject->SetPixelShader(spWaterPixelShader[12].m_pObject);
    WaterShader::ulPixelConstantFlags[50] = 14;
    this->WaterShader::spPass[51].m_pObject->SetVertexShader(spWaterVertexShader_Vc.m_pObject);
    this->WaterShader::spPass[51].m_pObject->SetPixelShader(spWaterPixelShader[13].m_pObject);
    WaterShader::ulPixelConstantFlags[51] = 14;
    this->WaterShader::spPass[52].m_pObject->SetVertexShader(spWaterVertexShader_Vc.m_pObject);
    this->WaterShader::spPass[52].m_pObject->SetPixelShader(spWaterPixelShader[14].m_pObject);
    WaterShader::ulPixelConstantFlags[52] = 14;
    this->WaterShader::spPass[53].m_pObject->SetVertexShader(spWaterVertexShader_Vc.m_pObject);
    this->WaterShader::spPass[53].m_pObject->SetPixelShader(spWaterPixelShader[15].m_pObject);
    WaterShader::ulPixelConstantFlags[53] = 14;
    this->WaterShader::spPass[54].m_pObject->SetVertexShader(spWaterVertexShader_Vc.m_pObject);
    this->WaterShader::spPass[54].m_pObject->SetPixelShader(spWaterPixelShader[16].m_pObject);
    WaterShader::ulPixelConstantFlags[54] = 14;
    this->WaterShader::spPass[55].m_pObject->SetVertexShader(spWaterVertexShader_Vc.m_pObject);
    this->WaterShader::spPass[55].m_pObject->SetPixelShader(spWaterPixelShader[17].m_pObject);
    WaterShader::ulPixelConstantFlags[55] = 62;
    this->WaterShader::spPass[56].m_pObject->SetVertexShader(spWaterVertexShader_Vc.m_pObject);
    this->WaterShader::spPass[56].m_pObject->SetPixelShader(spWaterPixelShader[18].m_pObject);
    WaterShader::ulPixelConstantFlags[56] = 62;
    this->WaterShader::spPass[57].m_pObject->SetVertexShader(spWaterVertexShader_Vc.m_pObject);
    this->WaterShader::spPass[57].m_pObject->SetPixelShader(spWaterPixelShader[19].m_pObject);
    WaterShader::ulPixelConstantFlags[57] = 62;
    this->WaterShader::spPass[58].m_pObject->SetVertexShader(spWaterVertexShader_Vc.m_pObject);
    this->WaterShader::spPass[58].m_pObject->SetPixelShader(spWaterPixelShader[20].m_pObject);
    WaterShader::ulPixelConstantFlags[58] = 62;
    this->WaterShader::spPass[59].m_pObject->SetVertexShader(spWaterVertexShader_Vc.m_pObject);
    this->WaterShader::spPass[59].m_pObject->SetPixelShader(spWaterPixelShader[21].m_pObject);
    WaterShader::ulPixelConstantFlags[59] = 46;
    this->WaterShader::spPass[60].m_pObject->SetVertexShader(spWaterVertexShader_Vc.m_pObject);
    this->WaterShader::spPass[60].m_pObject->SetPixelShader(spWaterPixelShader[22].m_pObject);
    WaterShader::ulPixelConstantFlags[60] = 46;
    this->WaterShader::spPass[61].m_pObject->SetVertexShader(spWaterVertexShader_Vc.m_pObject);
    this->WaterShader::spPass[61].m_pObject->SetPixelShader(spWaterPixelShader[23].m_pObject);
    WaterShader::ulPixelConstantFlags[61] = 46;
    this->WaterShader::spPass[62].m_pObject->SetVertexShader(spWaterVertexShader_Vc.m_pObject);
    this->WaterShader::spPass[62].m_pObject->SetPixelShader(spWaterPixelShader[24].m_pObject);
    WaterShader::ulPixelConstantFlags[62] = 46;
    this->WaterShader::spPass[63].m_pObject->SetVertexShader(spWaterVertexShader.m_pObject);
    this->WaterShader::spPass[63].m_pObject->SetPixelShader(spWaterPixelShader[25].m_pObject);
    WaterShader::ulPixelConstantFlags[63] = 30;
    this->WaterShader::spPass[64].m_pObject->SetVertexShader(spWaterVertexShader.m_pObject);
    this->WaterShader::spPass[64].m_pObject->SetPixelShader(spWaterPixelShader[26].m_pObject);
    WaterShader::ulPixelConstantFlags[64] = 30;
    this->WaterShader::spPass[65].m_pObject->SetVertexShader(spWaterVertexShader.m_pObject);
    this->WaterShader::spPass[65].m_pObject->SetPixelShader(spWaterPixelShader[27].m_pObject);
    WaterShader::ulPixelConstantFlags[65] = 30;
    this->WaterShader::spPass[66].m_pObject->SetVertexShader(spWaterVertexShader.m_pObject);
    this->WaterShader::spPass[66].m_pObject->SetPixelShader(spWaterPixelShader[28].m_pObject);
    WaterShader::ulPixelConstantFlags[66] = 30;
    this->WaterShader::spPass[67].m_pObject->SetVertexShader(spWaterVertexShader.m_pObject);
    this->WaterShader::spPass[67].m_pObject->SetPixelShader(spWaterPixelShader[29].m_pObject);
    // Index 66 is stored again here and index 67 is never written, as the game does.
    WaterShader::ulPixelConstantFlags[66] = 14;
    this->WaterShader::spPass[68].m_pObject->SetVertexShader(spWaterVertexShader.m_pObject);
    this->WaterShader::spPass[68].m_pObject->SetPixelShader(spWaterPixelShader[30].m_pObject);
    WaterShader::ulPixelConstantFlags[68] = 14;
    this->WaterShader::spPass[69].m_pObject->SetVertexShader(spWaterVertexShader.m_pObject);
    this->WaterShader::spPass[69].m_pObject->SetPixelShader(spWaterPixelShader[31].m_pObject);
    WaterShader::ulPixelConstantFlags[69] = 14;
    this->WaterShader::spPass[70].m_pObject->SetVertexShader(spWaterVertexShader.m_pObject);
    this->WaterShader::spPass[70].m_pObject->SetPixelShader(spWaterPixelShader[32].m_pObject);
    WaterShader::ulPixelConstantFlags[70] = 14;
    this->WaterShader::spPass[71].m_pObject->SetVertexShader(spWaterVertexShader_Vc.m_pObject);
    this->WaterShader::spPass[71].m_pObject->SetPixelShader(spWaterPixelShader[35].m_pObject);
    WaterShader::ulPixelConstantFlags[71] = 0;
    this->WaterShader::spPass[72].m_pObject->SetVertexShader(spWaterVertexShader_Lighting.m_pObject);
    this->WaterShader::spPass[72].m_pObject->SetPixelShader(spWaterPixelShader[36].m_pObject);
    WaterShader::ulPixelConstantFlags[72] = 78;
    this->WaterShader::spPass[73].m_pObject->SetVertexShader(spWaterVertexShader_Lighting.m_pObject);
    this->WaterShader::spPass[73].m_pObject->SetPixelShader(spWaterPixelShader[37].m_pObject);
    WaterShader::ulPixelConstantFlags[73] = 78;
    return 1;
}

// Each shader is a BSShaderMacro: its HLSL file and up to nine macros, the rest zeroed.
void WaterShader::LoadShaders() {
    BSShaderMacro kVertexShader;
    BSShaderMacro kVertexShaderVc;
    BSShaderMacro kVertexShaderLighting;
    BSShaderMacro kPixelShaders[38];
    char acPixelShaderFile[256];
    char acLightingPixelShaderFile[256];
    char acHLSLFile[260];
    char acPSOFile[260];
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_WATER;

    kVertexShader.strPath = "water\\2_ab\\v\\water.v.hlsl";
    kVertexShader.kMacros[0] = {};
    kVertexShader.kMacros[1] = {};
    std::memset(&kVertexShader.kMacros[2], 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2]));
    BSShader::MakePath(kVertexShader.strPath, acHLSLFile);
    BSsprintf(acPSOFile, sizeof(acPSOFile), "WATER000.vso");
    spWaterVertexShader = CompileVertexShaderHLSL(acHLSLFile, kVertexShader.kMacros, ShaderTarget_vs_1_1, acPSOFile);

    kVertexShaderVc.strPath = "water\\2_ab\\v\\water.v.hlsl";
    kVertexShaderVc.kMacros[0] = {"VERTEX_COLORS", ""};
    kVertexShaderVc.kMacros[1].Name = nullptr;
    std::memset(
        &kVertexShaderVc.kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    BSShader::MakePath(kVertexShaderVc.strPath, acHLSLFile);
    BSsprintf(acPSOFile, sizeof(acPSOFile), "WATER001.vso");
    spWaterVertexShader_Vc =
        CompileVertexShaderHLSL(acHLSLFile, kVertexShaderVc.kMacros, ShaderTarget_vs_1_1, acPSOFile);

    kVertexShaderLighting.strPath = "water\\2_ab\\v\\water.v.hlsl";
    kVertexShaderLighting.kMacros[0] = {"LIGHTING", ""};
    kVertexShaderLighting.kMacros[1].Name = nullptr;
    std::memset(
        &kVertexShaderLighting.kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    BSShader::MakePath(kVertexShaderLighting.strPath, acHLSLFile);
    BSsprintf(acPSOFile, sizeof(acPSOFile), "WATER002.vso");
    spWaterVertexShader_Lighting =
        CompileVertexShaderHLSL(acHLSLFile, kVertexShaderLighting.kMacros, ShaderTarget_vs_1_1, acPSOFile);

    const char* pTarget = "ps_2_0";
    if (BSShaderManager::eShaderLevel >= 7) {
        pTarget = "ps_3_0";
    }
    else if (BSShaderManager::eShaderLevel >= 6) {
        pTarget = "ps_2_b";
    }
    else if (BSShaderManager::eShaderLevel >= 5) {
        pTarget = "ps_2_a";
    }
    else if (BSShaderManager::eShaderLevel >= 2) {
        pTarget = "ps_2_0";
    }
    const char* pPixelShaderFile;
    if (BSShaderManager::eShaderLevel <= 4) {
        pPixelShaderFile = "water\\2_0\\p\\water.p.hlsl";
    }
    else {
        pPixelShaderFile = "water\\2_ab\\p\\water.p.hlsl";
    }
    BSsprintf(acPixelShaderFile, sizeof(acPixelShaderFile), pPixelShaderFile);
    BSsprintf(acLightingPixelShaderFile, sizeof(acLightingPixelShaderFile), "water\\2_ab\\p\\waterlighting.p.hlsl");

    kPixelShaders[0].strPath = acPixelShaderFile;
    kPixelShaders[0].kMacros[0] = {"WATER", ""};
    kPixelShaders[0].kMacros[1] = {"REFLECTIONS", ""};
    kPixelShaders[0].kMacros[2] = {"REFRACTIONS", ""};
    kPixelShaders[0].kMacros[3] = {"DEPTH", ""};
    kPixelShaders[0].kMacros[4] = {"FOG", ""};
    kPixelShaders[0].kMacros[5] = {};
    kPixelShaders[0].kMacros[6] = {};
    kPixelShaders[0].kMacros[7] = {};
    kPixelShaders[0].kMacros[8] = {};
    kPixelShaders[1].strPath = acPixelShaderFile;
    kPixelShaders[1].kMacros[0] = {"WATER", ""};
    kPixelShaders[1].kMacros[1] = {"REFRACTIONS", ""};
    kPixelShaders[1].kMacros[2] = {"DEPTH", ""};
    kPixelShaders[1].kMacros[3] = {"FOG", ""};
    kPixelShaders[1].kMacros[4] = {};
    kPixelShaders[1].kMacros[5] = {};
    kPixelShaders[1].kMacros[6] = {};
    kPixelShaders[1].kMacros[7] = {};
    kPixelShaders[1].kMacros[8] = {};
    kPixelShaders[2].strPath = acPixelShaderFile;
    kPixelShaders[2].kMacros[0] = {"WATER", ""};
    kPixelShaders[2].kMacros[1] = {"REFLECTIONS", ""};
    kPixelShaders[2].kMacros[2] = {"DEPTH", ""};
    kPixelShaders[2].kMacros[3] = {"FOG", ""};
    kPixelShaders[2].kMacros[4] = {};
    kPixelShaders[2].kMacros[5] = {};
    kPixelShaders[2].kMacros[6] = {};
    kPixelShaders[2].kMacros[7] = {};
    kPixelShaders[2].kMacros[8] = {};
    kPixelShaders[3].strPath = acPixelShaderFile;
    kPixelShaders[3].kMacros[0] = {"WATER", ""};
    kPixelShaders[3].kMacros[1] = {"DEPTH", ""};
    kPixelShaders[3].kMacros[2] = {"FOG", ""};
    kPixelShaders[3].kMacros[3].Name = nullptr;
    std::memset(
        &kPixelShaders[3].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kPixelShaders[4].strPath = acPixelShaderFile;
    kPixelShaders[4].kMacros[0] = {"WATER", ""};
    kPixelShaders[4].kMacros[1] = {"REFLECTIONS", ""};
    kPixelShaders[4].kMacros[2] = {"REFRACTIONS", ""};
    kPixelShaders[4].kMacros[3] = {"FOG", ""};
    kPixelShaders[4].kMacros[4] = {};
    kPixelShaders[4].kMacros[5] = {};
    kPixelShaders[4].kMacros[6] = {};
    kPixelShaders[4].kMacros[7] = {};
    kPixelShaders[4].kMacros[8] = {};
    kPixelShaders[5].strPath = acPixelShaderFile;
    kPixelShaders[5].kMacros[0] = {"WATER", ""};
    kPixelShaders[5].kMacros[1] = {"REFRACTIONS", ""};
    kPixelShaders[5].kMacros[2] = {"FOG", ""};
    kPixelShaders[5].kMacros[3].Name = nullptr;
    std::memset(
        &kPixelShaders[5].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kPixelShaders[6].strPath = acPixelShaderFile;
    kPixelShaders[6].kMacros[0] = {"WATER", ""};
    kPixelShaders[6].kMacros[1] = {"REFLECTIONS", ""};
    kPixelShaders[6].kMacros[2] = {"FOG", ""};
    kPixelShaders[6].kMacros[3].Name = nullptr;
    std::memset(
        &kPixelShaders[6].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kPixelShaders[7].strPath = acPixelShaderFile;
    kPixelShaders[7].kMacros[0] = {"WATER", ""};
    kPixelShaders[7].kMacros[1] = {"FOG", ""};
    kPixelShaders[7].kMacros[2].Name = nullptr;
    std::memset(
        &kPixelShaders[7].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kPixelShaders[8].strPath = acPixelShaderFile;
    kPixelShaders[8].kMacros[0] = {"WATER", ""};
    kPixelShaders[8].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[8].kMacros[2] = {"REFLECTIONS", ""};
    kPixelShaders[8].kMacros[3] = {"REFRACTIONS", ""};
    kPixelShaders[8].kMacros[4] = {"DEPTH", ""};
    kPixelShaders[8].kMacros[5] = {"FOG", ""};
    kPixelShaders[8].kMacros[6] = {};
    kPixelShaders[8].kMacros[7] = {};
    kPixelShaders[8].kMacros[8] = {};
    kPixelShaders[9].strPath = acPixelShaderFile;
    kPixelShaders[9].kMacros[0] = {"WATER", ""};
    kPixelShaders[9].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[9].kMacros[2] = {"REFRACTIONS", ""};
    kPixelShaders[9].kMacros[3] = {"DEPTH", ""};
    kPixelShaders[9].kMacros[4] = {"FOG", ""};
    kPixelShaders[9].kMacros[5] = {};
    kPixelShaders[9].kMacros[6] = {};
    kPixelShaders[9].kMacros[7] = {};
    kPixelShaders[9].kMacros[8] = {};
    kPixelShaders[10].strPath = acPixelShaderFile;
    kPixelShaders[10].kMacros[0] = {"WATER", ""};
    kPixelShaders[10].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[10].kMacros[2] = {"REFLECTIONS", ""};
    kPixelShaders[10].kMacros[3] = {"DEPTH", ""};
    kPixelShaders[10].kMacros[4] = {"FOG", ""};
    kPixelShaders[10].kMacros[5] = {};
    kPixelShaders[10].kMacros[6] = {};
    kPixelShaders[10].kMacros[7] = {};
    kPixelShaders[10].kMacros[8] = {};
    kPixelShaders[11].strPath = acPixelShaderFile;
    kPixelShaders[11].kMacros[0] = {"WATER", ""};
    kPixelShaders[11].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[11].kMacros[2] = {"DEPTH", ""};
    kPixelShaders[11].kMacros[3] = {"FOG", ""};
    kPixelShaders[11].kMacros[4] = {};
    kPixelShaders[11].kMacros[5] = {};
    kPixelShaders[11].kMacros[6] = {};
    kPixelShaders[11].kMacros[7] = {};
    kPixelShaders[11].kMacros[8] = {};
    kPixelShaders[12].strPath = acPixelShaderFile;
    kPixelShaders[12].kMacros[0] = {"WATER", ""};
    kPixelShaders[12].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[12].kMacros[2] = {"REFLECTIONS", ""};
    kPixelShaders[12].kMacros[3] = {"REFRACTIONS", ""};
    kPixelShaders[12].kMacros[4] = {"FOG", ""};
    kPixelShaders[12].kMacros[5] = {};
    kPixelShaders[12].kMacros[6] = {};
    kPixelShaders[12].kMacros[7] = {};
    kPixelShaders[12].kMacros[8] = {};
    kPixelShaders[13].strPath = acPixelShaderFile;
    kPixelShaders[13].kMacros[0] = {"WATER", ""};
    kPixelShaders[13].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[13].kMacros[2] = {"REFRACTIONS", ""};
    kPixelShaders[13].kMacros[3] = {"FOG", ""};
    kPixelShaders[13].kMacros[4] = {};
    kPixelShaders[13].kMacros[5] = {};
    kPixelShaders[13].kMacros[6] = {};
    kPixelShaders[13].kMacros[7] = {};
    kPixelShaders[13].kMacros[8] = {};
    kPixelShaders[14].strPath = acPixelShaderFile;
    kPixelShaders[14].kMacros[0] = {"WATER", ""};
    kPixelShaders[14].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[14].kMacros[2] = {"REFLECTIONS", ""};
    kPixelShaders[14].kMacros[3] = {"DEPTH", ""};
    kPixelShaders[14].kMacros[4] = {"FOG", ""};
    kPixelShaders[14].kMacros[5] = {};
    kPixelShaders[14].kMacros[6] = {};
    kPixelShaders[14].kMacros[7] = {};
    kPixelShaders[14].kMacros[8] = {};
    kPixelShaders[15].strPath = acPixelShaderFile;
    kPixelShaders[15].kMacros[0] = {"WATER", ""};
    kPixelShaders[15].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[15].kMacros[2] = {"FOG", ""};
    kPixelShaders[15].kMacros[3].Name = nullptr;
    std::memset(
        &kPixelShaders[15].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kPixelShaders[16].strPath = acPixelShaderFile;
    kPixelShaders[16].kMacros[0] = {"UNDERWATER", ""};
    kPixelShaders[16].kMacros[1] = {"REFRACTIONS", ""};
    kPixelShaders[16].kMacros[2] = {"FOG", ""};
    kPixelShaders[16].kMacros[3].Name = nullptr;
    std::memset(
        &kPixelShaders[16].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kPixelShaders[17].strPath = acPixelShaderFile;
    kPixelShaders[17].kMacros[0] = {"WATER", ""};
    kPixelShaders[17].kMacros[1] = {"REFLECTIONS", ""};
    kPixelShaders[17].kMacros[2] = {"REFRACTIONS", ""};
    kPixelShaders[17].kMacros[3] = {"WADING", ""};
    kPixelShaders[17].kMacros[4] = {"DEPTH", ""};
    kPixelShaders[17].kMacros[5] = {"FOG", ""};
    kPixelShaders[17].kMacros[6] = {};
    kPixelShaders[17].kMacros[7] = {};
    kPixelShaders[17].kMacros[8] = {};
    kPixelShaders[18].strPath = acPixelShaderFile;
    kPixelShaders[18].kMacros[0] = {"WATER", ""};
    kPixelShaders[18].kMacros[1] = {"REFRACTIONS", ""};
    kPixelShaders[18].kMacros[2] = {"WADING", ""};
    kPixelShaders[18].kMacros[3] = {"DEPTH", ""};
    kPixelShaders[18].kMacros[4] = {"FOG", ""};
    kPixelShaders[18].kMacros[5] = {};
    kPixelShaders[18].kMacros[6] = {};
    kPixelShaders[18].kMacros[7] = {};
    kPixelShaders[18].kMacros[8] = {};
    kPixelShaders[19].strPath = acPixelShaderFile;
    kPixelShaders[19].kMacros[0] = {"WATER", ""};
    kPixelShaders[19].kMacros[1] = {"REFLECTIONS", ""};
    kPixelShaders[19].kMacros[2] = {"WADING", ""};
    kPixelShaders[19].kMacros[3] = {"DEPTH", ""};
    kPixelShaders[19].kMacros[4] = {"FOG", ""};
    kPixelShaders[19].kMacros[5] = {};
    kPixelShaders[19].kMacros[6] = {};
    kPixelShaders[19].kMacros[7] = {};
    kPixelShaders[19].kMacros[8] = {};
    kPixelShaders[20].strPath = acPixelShaderFile;
    kPixelShaders[20].kMacros[0] = {"WATER", ""};
    kPixelShaders[20].kMacros[1] = {"WADING", ""};
    kPixelShaders[20].kMacros[2] = {"DEPTH", ""};
    kPixelShaders[20].kMacros[3] = {"FOG", ""};
    kPixelShaders[20].kMacros[4] = {};
    kPixelShaders[20].kMacros[5] = {};
    kPixelShaders[20].kMacros[6] = {};
    kPixelShaders[20].kMacros[7] = {};
    kPixelShaders[20].kMacros[8] = {};
    kPixelShaders[21].strPath = acPixelShaderFile;
    kPixelShaders[21].kMacros[0] = {"WATER", ""};
    kPixelShaders[21].kMacros[1] = {"REFLECTIONS", ""};
    kPixelShaders[21].kMacros[2] = {"REFRACTIONS", ""};
    kPixelShaders[21].kMacros[3] = {"WADING", ""};
    kPixelShaders[21].kMacros[4] = {"FOG", ""};
    kPixelShaders[21].kMacros[5] = {};
    kPixelShaders[21].kMacros[6] = {};
    kPixelShaders[21].kMacros[7] = {};
    kPixelShaders[21].kMacros[8] = {};
    kPixelShaders[22].strPath = acPixelShaderFile;
    kPixelShaders[22].kMacros[0] = {"WATER", ""};
    kPixelShaders[22].kMacros[1] = {"REFRACTIONS", ""};
    kPixelShaders[22].kMacros[2] = {"WADING", ""};
    kPixelShaders[22].kMacros[3] = {"FOG", ""};
    kPixelShaders[22].kMacros[4] = {};
    kPixelShaders[22].kMacros[5] = {};
    kPixelShaders[22].kMacros[6] = {};
    kPixelShaders[22].kMacros[7] = {};
    kPixelShaders[22].kMacros[8] = {};
    kPixelShaders[23].strPath = acPixelShaderFile;
    kPixelShaders[23].kMacros[0] = {"WATER", ""};
    kPixelShaders[23].kMacros[1] = {"REFLECTIONS", ""};
    kPixelShaders[23].kMacros[2] = {"WADING", ""};
    kPixelShaders[23].kMacros[3] = {"DEPTH", ""};
    kPixelShaders[23].kMacros[4] = {"FOG", ""};
    kPixelShaders[23].kMacros[5] = {};
    kPixelShaders[23].kMacros[6] = {};
    kPixelShaders[23].kMacros[7] = {};
    kPixelShaders[23].kMacros[8] = {};
    kPixelShaders[24].strPath = acPixelShaderFile;
    kPixelShaders[24].kMacros[0] = {"WATER", ""};
    kPixelShaders[24].kMacros[1] = {"WADING", ""};
    kPixelShaders[24].kMacros[2] = {"FOG", ""};
    kPixelShaders[24].kMacros[3].Name = nullptr;
    std::memset(
        &kPixelShaders[24].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kPixelShaders[25].strPath = acPixelShaderFile;
    kPixelShaders[25].kMacros[0] = {"WATER", ""};
    kPixelShaders[25].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[25].kMacros[2] = {"REFLECTIONS", ""};
    kPixelShaders[25].kMacros[3] = {"REFRACTIONS", ""};
    kPixelShaders[25].kMacros[4] = {"WADING", ""};
    kPixelShaders[25].kMacros[5] = {"DEPTH", ""};
    kPixelShaders[25].kMacros[6] = {"FOG", ""};
    kPixelShaders[25].kMacros[7] = {};
    kPixelShaders[25].kMacros[8] = {};
    kPixelShaders[26].strPath = acPixelShaderFile;
    kPixelShaders[26].kMacros[0] = {"WATER", ""};
    kPixelShaders[26].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[26].kMacros[2] = {"REFRACTIONS", ""};
    kPixelShaders[26].kMacros[3] = {"WADING", ""};
    kPixelShaders[26].kMacros[4] = {"DEPTH", ""};
    kPixelShaders[26].kMacros[5] = {"FOG", ""};
    kPixelShaders[26].kMacros[6] = {};
    kPixelShaders[26].kMacros[7] = {};
    kPixelShaders[26].kMacros[8] = {};
    kPixelShaders[27].strPath = acPixelShaderFile;
    kPixelShaders[27].kMacros[0] = {"WATER", ""};
    kPixelShaders[27].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[27].kMacros[2] = {"REFLECTIONS", ""};
    kPixelShaders[27].kMacros[3] = {"WADING", ""};
    kPixelShaders[27].kMacros[4] = {"DEPTH", ""};
    kPixelShaders[27].kMacros[5] = {"FOG", ""};
    kPixelShaders[27].kMacros[6] = {};
    kPixelShaders[27].kMacros[7] = {};
    kPixelShaders[27].kMacros[8] = {};
    kPixelShaders[28].strPath = acPixelShaderFile;
    kPixelShaders[28].kMacros[0] = {"WATER", ""};
    kPixelShaders[28].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[28].kMacros[2] = {"WADING", ""};
    kPixelShaders[28].kMacros[3] = {"DEPTH", ""};
    kPixelShaders[28].kMacros[4] = {"FOG", ""};
    kPixelShaders[28].kMacros[5] = {};
    kPixelShaders[28].kMacros[6] = {};
    kPixelShaders[28].kMacros[7] = {};
    kPixelShaders[28].kMacros[8] = {};
    kPixelShaders[29].strPath = acPixelShaderFile;
    kPixelShaders[29].kMacros[0] = {"WATER", ""};
    kPixelShaders[29].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[29].kMacros[2] = {"REFLECTIONS", ""};
    kPixelShaders[29].kMacros[3] = {"REFRACTIONS", ""};
    kPixelShaders[29].kMacros[4] = {"WADING", ""};
    kPixelShaders[29].kMacros[5] = {"FOG", ""};
    kPixelShaders[29].kMacros[6] = {};
    kPixelShaders[29].kMacros[7] = {};
    kPixelShaders[29].kMacros[8] = {};
    kPixelShaders[30].strPath = acPixelShaderFile;
    kPixelShaders[30].kMacros[0] = {"WATER", ""};
    kPixelShaders[30].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[30].kMacros[2] = {"REFRACTIONS", ""};
    kPixelShaders[30].kMacros[3] = {"WADING", ""};
    kPixelShaders[30].kMacros[4] = {"FOG", ""};
    kPixelShaders[30].kMacros[5] = {};
    kPixelShaders[30].kMacros[6] = {};
    kPixelShaders[30].kMacros[7] = {};
    kPixelShaders[30].kMacros[8] = {};
    kPixelShaders[31].strPath = acPixelShaderFile;
    kPixelShaders[31].kMacros[0] = {"WATER", ""};
    kPixelShaders[31].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[31].kMacros[2] = {"REFLECTIONS", ""};
    kPixelShaders[31].kMacros[3] = {"WADING", ""};
    kPixelShaders[31].kMacros[4] = {"FOG", ""};
    kPixelShaders[31].kMacros[5] = {};
    kPixelShaders[31].kMacros[6] = {};
    kPixelShaders[31].kMacros[7] = {};
    kPixelShaders[31].kMacros[8] = {};
    kPixelShaders[32].strPath = acPixelShaderFile;
    kPixelShaders[32].kMacros[0] = {"WATER", ""};
    kPixelShaders[32].kMacros[1] = {"INTERIORWATER", ""};
    kPixelShaders[32].kMacros[2] = {"WADING", ""};
    kPixelShaders[32].kMacros[3] = {"FOG", ""};
    kPixelShaders[32].kMacros[4] = {};
    kPixelShaders[32].kMacros[5] = {};
    kPixelShaders[32].kMacros[6] = {};
    kPixelShaders[32].kMacros[7] = {};
    kPixelShaders[32].kMacros[8] = {};
    kPixelShaders[33].strPath = acPixelShaderFile;
    kPixelShaders[33].kMacros[0] = {"WATER", ""};
    kPixelShaders[33].kMacros[1] = {"REFLECTIONS", ""};
    kPixelShaders[33].kMacros[2] = {"LOD", ""};
    kPixelShaders[33].kMacros[3] = {"FOG", ""};
    kPixelShaders[33].kMacros[4] = {};
    kPixelShaders[33].kMacros[5] = {};
    kPixelShaders[33].kMacros[6] = {};
    kPixelShaders[33].kMacros[7] = {};
    kPixelShaders[33].kMacros[8] = {};
    kPixelShaders[34].strPath = acPixelShaderFile;
    kPixelShaders[34].kMacros[0] = {"WATER", ""};
    kPixelShaders[34].kMacros[1] = {"LOD", ""};
    kPixelShaders[34].kMacros[2] = {"FOG", ""};
    kPixelShaders[34].kMacros[3].Name = nullptr;
    std::memset(
        &kPixelShaders[34].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kPixelShaders[35].strPath = acPixelShaderFile;
    kPixelShaders[35].kMacros[0] = {"STENCIL", ""};
    kPixelShaders[35].kMacros[1].Name = nullptr;
    std::memset(
        &kPixelShaders[35].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kPixelShaders[36].strPath = acLightingPixelShaderFile;
    kPixelShaders[36].kMacros[0].Name = nullptr;
    std::memset(
        &kPixelShaders[36].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kPixelShaders[37].strPath = acLightingPixelShaderFile;
    kPixelShaders[37].kMacros[0] = {"WADING", ""};
    kPixelShaders[37].kMacros[1].Name = nullptr;
    std::memset(
        &kPixelShaders[37].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );

    for (int32_t i = 0; i < 38; ++i) {
        if (kPixelShaders[i].strPath) {
            BSShader::MakePath(kPixelShaders[i].strPath, acHLSLFile);
            BSsprintf(acPSOFile, sizeof(acPSOFile), "WATER%03i.pso", i);
            spWaterPixelShader[i] = CompilePixelShaderHLSL(acHLSLFile, kPixelShaders[i].kMacros, pTarget, acPSOFile);
        }
    }
    pTLSData->eMemContext = eMemContext;
}

WaterShader* WaterShader::GetInstance() {
    return WaterShader::pInstance;
}

void WaterShader::EnableConstantsForPass(uint32_t auiPassEnum, bool abAlphaPass) {
    if (WaterShader::pVertexConstantMapEntry[0]) {
        WaterShader::pVertexConstantMapEntry[0]->m_bEnabled = 1;
    }
    WaterShader::bVSConstantEnable[0] = (WaterShader::ulVertexConstantFlags[auiPassEnum] & 2) != 0;
    if (WaterShader::pVertexConstantMapEntry[1]) {
        WaterShader::pVertexConstantMapEntry[1]->m_bEnabled = 1;
    }
    WaterShader::bVSConstantEnable[1] = (WaterShader::ulVertexConstantFlags[auiPassEnum] & 4) != 0;
    if (WaterShader::pVertexConstantMapEntry[2]) {
        WaterShader::pVertexConstantMapEntry[2]->m_bEnabled = 1;
    }
    WaterShader::bVSConstantEnable[2] = (WaterShader::ulVertexConstantFlags[auiPassEnum] & 8) != 0;
    if (WaterShader::pPixelConstantMapEntry[0]) {
        WaterShader::pPixelConstantMapEntry[0]->m_bEnabled = (WaterShader::ulPixelConstantFlags[auiPassEnum] & 2) != 0;
    }
    uint32_t v2 = WaterShader::ulPixelConstantFlags[auiPassEnum];
    uint32_t* v3 = &WaterShader::ulPixelConstantFlags[auiPassEnum];
    WaterShader::bPSConstantEnable[0] = (v2 & 2) != 0;
    if (WaterShader::pPixelConstantMapEntry[1]) {
        WaterShader::pPixelConstantMapEntry[1]->m_bEnabled = (v2 & 4) != 0;
    }
    uint32_t v4 = *v3;
    WaterShader::bPSConstantEnable[1] = (*v3 & 4) != 0;
    if (WaterShader::pPixelConstantMapEntry[2]) {
        WaterShader::pPixelConstantMapEntry[2]->m_bEnabled = (v4 & 8) != 0;
    }
    uint32_t v5 = *v3;
    WaterShader::bPSConstantEnable[2] = (*v3 & 8) != 0;
    if (WaterShader::pPixelConstantMapEntry[3]) {
        WaterShader::pPixelConstantMapEntry[3]->m_bEnabled = (v5 & 16) != 0;
    }
    uint32_t v6 = *v3;
    WaterShader::bPSConstantEnable[3] = (*v3 & 16) != 0;
    if (WaterShader::pPixelConstantMapEntry[4]) {
        WaterShader::pPixelConstantMapEntry[4]->m_bEnabled = (v6 & 32) != 0;
    }
    uint32_t v7 = *v3;
    WaterShader::bPSConstantEnable[4] = (*v3 & 32) != 0;
    if (WaterShader::pPixelConstantMapEntry[5]) {
        WaterShader::pPixelConstantMapEntry[5]->m_bEnabled = (v7 & 64) != 0;
    }
    WaterShader::bPSConstantEnable[5] = (*v3 & 64) != 0;
}

BSShader* WaterShader::CreateShader() {
    TLSData* v0 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v0->eMemContext;
    v0->eMemContext = MEM_CONTEXT_WATER;
    WaterShader* v2 = 0;
    NiDX9ShaderDeclaration* v3 = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 4u, 1u);
    v3->SetEntry(0u, 0u, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0u);
    v3->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0u);
    v3->SetEntry(2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0u);
    v3->SetEntry(3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0u);
    v3->GetD3DDeclaration();
    WaterShader* v4 = static_cast<WaterShader*>(NiMemObject::operator new(sizeof(WaterShader)));
    if (v4) {
        v2 = ::new (v4) WaterShader;
    }
    v2->Initialize();
    v2->LoadShaders();
    v2->LoadStagesAndPasses();
    v2->CreateConstantMaps();
    v2->SetShaderDecl(v3);
    v0->eMemContext = eMemContext;
    return v2;
}
