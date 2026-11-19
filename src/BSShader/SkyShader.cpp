#include "BSShader/SkyShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderData.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/SkyShaderProperty.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

NiRTTI SkyShader::ms_RTTI("SkyShader", &NiD3DDefaultShader::ms_RTTI);

SkyShader::SkyShader() : spPass(), spVertexShader(), spPixelShader() {
    iShaderType = 11;
    pBlendColor[0] = NiColorA_BLACK;
    pBlendColor[1] = NiColorA_BLACK;
    pBlendColor[2] = NiColorA_BLACK;
    Sky::BlendColor[0] = NiColorA_BLACK;
    Sky::BlendColor[1] = NiColorA_BLACK;
    Sky::BlendColor[2] = NiColorA_BLACK;
    kEyePosition = NiColorA_BLACK;
    m_bUsesNiRenderState = true;
}

SkyShader::~SkyShader() {
    for (NiPointer<NiD3DPass>& rspPass : spPass) {
        rspPass = nullptr;
    }
    for (NiD3DVertexShader*& rpShader : spVertexShader) {
        if (rpShader) {
            rpShader->DecRefCount();
            rpShader = nullptr;
        }
    }
    for (NiD3DPixelShader*& rpShader : spPixelShader) {
        if (rpShader) {
            rpShader->DecRefCount();
            rpShader = nullptr;
        }
    }
}

const NiRTTI* SkyShader::GetRTTI() const {
    return &SkyShader::ms_RTTI;
}

bool SkyShader::Initialize() {
    bool v2 = BSShader::Initialize();
    NiD3DPass* m_pObject = SkyShader::spPass[0].m_pObject;
    if (m_pObject) {
        if (m_pObject->m_uiRefCount-- == 1) {
            m_pObject->ReturnPassToPool();
        }
        SkyShader::spPass[0].m_pObject = nullptr;
    }
    return v2;
}

void SkyShader::SetupGeometryTextures(const NiPropertyState* apState) {
    NiD3DPass* v2 = NiD3DShader::m_spCurrentPass;
    SkyShaderProperty* pprop = static_cast<SkyShaderProperty*>(apState->m_aspProps[3].m_pObject);
    NiSourceTexture* m_pObject = pprop->spBaseMap.m_pObject;
    if (m_pObject) {
        NiD3DTextureStage* pstage = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
        pstage->m_spTexture = m_pObject;
        NiTexturingProperty::ClampMode v6 = pprop->GetClampNI();
        pstage->SetClampMode(v6);
        pstage->ConfigureStage(false);
    }
    NiSourceTexture* v7 = pprop->spBlendTexture.m_pObject;
    if (v7 ||
        ((BSShaderManager::eCurrentPass == BSShaderManager::BSSM_SELFILLUM_SKY_CLOUDS ||
          (BSShaderManager::eCurrentPass == BSShaderManager::BSSM_SKY_TEXTURE ||
           (BSShaderManager::eCurrentPass == BSShaderManager::BSSM_SKY_MOON_STARS_MASK ||
            (BSShaderManager::eCurrentPass == BSShaderManager::BSSM_SKY_SUNGLARE ||
             (BSShaderManager::eCurrentPass == BSShaderManager::BSSM_SKY_CLOUDS ||
              BSShaderManager::eCurrentPass == BSShaderManager::BSSM_SKY_CLOUDS_UNDERWATER))))) &&
         (v7 = pprop->spBaseMap.m_pObject) != nullptr)) {
        NiD3DTextureStage* pstage_2 = v2->m_kStages.m_pBase[1].m_pObject;
        if (pstage_2) {
            pstage_2->m_spTexture = v7;
            NiTexturingProperty::ClampMode v9 = pprop->GetClampNI();
            pstage_2->SetClampMode(v9);
            pstage_2->ConfigureStage(false);
        }
    }
}

void SkyShader::SetupGeometryConstants(const NiPropertyState* apState) {
    NiTransform trans;
    NiGeometry* pgeometry = BSShaderManager::pCurrentRenderPass->pGeometry;
    BSShaderManager::eBSSMPassEnum uiPassType = BSShaderManager::eCurrentPass;
    NiTransform* p_m_kWorld = &BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorld;
    BSNiBound* pBound = BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorldBound;
    if (!pBound) {
        pBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
    }
    const NiPropertyState* v4 = apState;
    NiGeometryBufferData* pbufferData = pgeometry->m_spModelData.m_pObject->m_pkBuffData;
    const NiSkinInstance* pskin = pgeometry->m_spSkinInstance.m_pObject;
    SkyShaderProperty* pShaderProp = static_cast<SkyShaderProperty*>(apState->m_aspProps[3].m_pObject);
    if (BSShaderManager::GetCurrentAccumulator()->bWaterReflection) {
        NiPoint3 kPosDiff;
        trans = *p_m_kWorld;
        kPosDiff.x = trans.m_Translate.x - BSShaderManager::spMainCamera.m_pObject->m_kWorld.m_Translate.x;
        kPosDiff.y = trans.m_Translate.y - BSShaderManager::spMainCamera.m_pObject->m_kWorld.m_Translate.y;
        kPosDiff.z = trans.m_Translate.z - BSShaderManager::spMainCamera.m_pObject->m_kWorld.m_Translate.z;
        NiCamera* m_pkCamera = BSShaderManager::GetCurrentAccumulator()->m_pkCamera;
        if (m_pkCamera) {
            kPosDiff.x = m_pkCamera->m_kWorld.m_Translate.x + kPosDiff.x;
            kPosDiff.y = m_pkCamera->m_kWorld.m_Translate.y + kPosDiff.y;
            kPosDiff.z = m_pkCamera->m_kWorld.m_Translate.z + kPosDiff.z;
        }
        trans.m_Translate = kPosDiff;
        m_pkD3DRenderer->SetModelTransform(trans, false);
        v4 = apState;
    }
    else {
        m_pkD3DRenderer->SetModelTransform(pgeometry->m_kWorld, false);
    }
    float kPosDiffa = BSShaderLightingProperty::pShaderLightConstants[30].X() - NiRenderer::PosAdjust.x;
    float kPosDiff_4 = BSShaderLightingProperty::pShaderLightConstants[30].Y() - NiRenderer::PosAdjust.y;
    float kPosDiff_8a = BSShaderLightingProperty::pShaderLightConstants[30].Z() - NiRenderer::PosAdjust.z;
    SkyShader::kEyePosition.r = kPosDiffa;
    SkyShader::kEyePosition.g = kPosDiff_4;
    SkyShader::kEyePosition.b = kPosDiff_8a;
    SkyShader::kEyePosition.a = 0.0f;
    if (uiPassType == BSShaderManager::BSSM_ZONLY_TEXEFFECT_S) {
        SkyShader::kEyePosition.b = -SkyShader::fPlayerDepth / 7.0;
    }
    SkyShaderProperty* v7 = pShaderProp;
    SkyShader* v8 = this;
    if (pShaderProp->soType == 3) {
        Params.x = pShaderProp->fBlendValue;
        TexCoordYOff = SkyShader::pCloudTexCoordYOffset[pShaderProp->usCloudLayer];
    }
    else {
        Params.x = 0.0f;
    }
    if (BSShaderManager::bHDR) {
        Params.y = SkyShader::pLUMRamp[0];
    }
    switch (pShaderProp->soType) {
        case 0u:
        case 1u:
        case 3u:
        case 5u:
        case 6u:
        case 7u: {
            SkyShader::pBlendColor[0] = pShaderProp->VertexColor;
            break;
        }
        case 2u: {
            SkyShader::pBlendColor[0] = Sky::BlendColor[0];
            SkyShader::pBlendColor[1] = Sky::BlendColor[1];
            SkyShader::pBlendColor[2] = Sky::BlendColor[2];
            break;
        }
        default: {
            SkyShader::pBlendColor[0].r = 0.0f;
            SkyShader::pBlendColor[0].g = 0.0f;
            SkyShader::pBlendColor[0].b = 0.0f;
            SkyShader::pBlendColor[0].a = 1.0f;
            SkyShader::pBlendColor[1].r = 0.0f;
            SkyShader::pBlendColor[1].g = 0.0f;
            SkyShader::pBlendColor[1].b = 0.0f;
            SkyShader::pBlendColor[2].r = 0.0f;
            SkyShader::pBlendColor[2].g = 0.0f;
            SkyShader::pBlendColor[2].b = 0.0f;
            break;
        }
    }
    NiD3DShaderConstantMap* m_pObject = m_spPixelConstantMap.m_pObject;
    if (m_pObject) {
        NiD3DPixelShader* v10 = NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject;
        if (v10) {
            m_pObject->SetShaderConstants(
                v10, pgeometry, pskin, nullptr, pbufferData, v4, nullptr, pgeometry->m_kWorld, *pBound, 0, true
            );
            v4 = apState;
            v7 = pShaderProp;
        }
        v8 = this;
    }
    NiD3DShaderConstantMap* v11 = v8->m_spVertexConstantMap.m_pObject;
    if (v11) {
        NiD3DVertexShader* v12 = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
        if (v12) {
            v11->SetShaderConstants(
                v12, pgeometry, pskin, nullptr, pbufferData, v4, nullptr, pgeometry->m_kWorld, *pBound, 0, true
            );
            v7 = pShaderProp;
        }
    }
    uint32_t uiSkyObjectType = v7->soType;
    if (uiSkyObjectType == 0 || uiSkyObjectType == 6) {
        BSRenderState::SetAlphaBlendFunc(NiAlphaProperty::ALPHA_SRCALPHA, NiAlphaProperty::ALPHA_ONE, BSRS_NOLOCK);
    }
    NiStencilProperty* v14 = static_cast<NiStencilProperty*>(v4->m_aspProps[4].m_pObject);
    if ((v14) && ((v14->m_uFlags & 1) != 0)) {
        m_pkD3DRenderState->ApplyStencil(v14);
    }
}

void SkyShader::PostGeometry(const NiPropertyState* apState) {
    SkyShaderProperty* pShaderProp = static_cast<SkyShaderProperty*>(apState->m_aspProps[3].m_pObject);
    if (pShaderProp->soType == SkyShaderProperty::SO_SUN || pShaderProp->soType == SkyShaderProperty::SO_MOON) {
        BSRenderState::SetAlphaBlendFunc(
            NiAlphaProperty::ALPHA_SRCALPHA, NiAlphaProperty::ALPHA_INVSRCALPHA, BSRS_NOLOCK
        );
    }
    NiStencilProperty* pStencil = static_cast<NiStencilProperty*>(apState->m_aspProps[4].m_pObject);
    if ((pStencil) && ((pStencil->m_uFlags & 1) != 0)) {
        BSRenderState::SetStencilEnable(false, BSRS_NOLOCK);
    }
}

void SkyShader::InitPass(uint32_t auiCurrentPass) {
    if (auiCurrentPass > 569) {
        switch ((int32_t)auiCurrentPass) {
            case 570: {
                NiD3DShader::m_spCurrentPass = spPass[2].m_pObject;
                break;
            }
            case 571: {
                NiD3DShader::m_spCurrentPass = spPass[3].m_pObject;
                break;
            }
            case 572: {
                NiD3DShader::m_spCurrentPass = spPass[6].m_pObject;
                break;
            }
            case 573: {
                NiD3DShader::m_spCurrentPass = spPass[4].m_pObject;
                break;
            }
            case 574: {
                NiD3DShader::m_spCurrentPass = spPass[5].m_pObject;
                break;
            }
            case 722: {
                NiD3DShader::m_spCurrentPass = spPass[1].m_pObject;
                break;
            }
            case 723: {
                NiD3DShader::m_spCurrentPass = spPass[9].m_pObject;
                break;
            }
            case 724: {
                NiD3DShader::m_spCurrentPass = spPass[10].m_pObject;
                break;
            }
            case 725: {
                NiD3DShader::m_spCurrentPass = spPass[8].m_pObject;
                break;
            }
            default: {
                return;
            }
        }
    }
    else {
        if (((int32_t)auiCurrentPass) == 569) {
            NiD3DShader::m_spCurrentPass = spPass[7].m_pObject;
        }
        else {
            if (((int32_t)auiCurrentPass) == 9) {
                NiD3DShader::m_spCurrentPass = spPass[0].m_pObject;
            }
        }
    }
}

// Creates the pixel map (the blend parameters) and the vertex map (the two transforms, the three blend colours, the eye
// position and the vertex parameters) when they are missing.
void SkyShader::CreateConstantMaps() {
    if (!m_spPixelConstantMap.m_pObject) {
        void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
        NiD3DShaderConstantMap* pMap = nullptr;
        if (pMemory) {
            pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        }
        m_spPixelConstantMap = pMap;
        m_spPixelConstantMap.m_pObject->AddEntry("Params", 0x10000005, 0, 4, 1, "", 8, 4, &Params, false);
    }
    if (!m_spVertexConstantMap.m_pObject) {
        void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
        NiD3DShaderConstantMap* pMap = nullptr;
        if (pMemory) {
            pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        }
        m_spVertexConstantMap = pMap;
        m_spVertexConstantMap.m_pObject->AddEntry(
            "WorldViewProjTranspose", 0x20000009, 0, 0, 4, nullptr, 0, 0, nullptr, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry("WorldTranspose", 0x20000009, 0, 8, 4, nullptr, 0, 0, nullptr, false);
        m_spVertexConstantMap.m_pObject->AddEntry("Color0", 0x10000007, 0, 4, 1, "", 0x10, 4, &pBlendColor[0], false);
        m_spVertexConstantMap.m_pObject->AddEntry("Color1", 0x10000007, 0, 5, 1, "", 0x10, 4, &pBlendColor[1], false);
        m_spVertexConstantMap.m_pObject->AddEntry("Color2", 0x10000007, 0, 6, 1, "", 0x10, 4, &pBlendColor[2], false);
        m_spVertexConstantMap.m_pObject->AddEntry("eyepos", 0x10000007, 0, 7, 1, "", 0x10, 4, &kEyePosition, false);
        m_spVertexConstantMap.m_pObject->AddEntry("Params", 0x10000004, 0, 12, 1, "", 4, 4, &TexCoordYOff, false);
    }
}

void SkyShader::ReloadShaders() {
    LoadShaders();
    LoadStagesAndPasses();
}

void SkyShader::ClearPassStages() {
    for (uint32_t i = 0; i < 11; ++i) {
        ClearPass(spPass[i].m_pObject);
    }
}

bool SkyShader::LoadStagesAndPasses() {
    NiPointer<NiD3DTextureStage> spStage;
    NiD3DPass* m_pObject = this->SkyShader::spPass[0].m_pObject;
    NiPointer<NiD3DPass>* spPass = this->SkyShader::spPass;
    if (!m_pObject) {
        NiD3DPass::CreateNewPass(*this->SkyShader::spPass);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
        spPass->m_pObject->SetStage(spPass->m_pObject->m_uiCurrentStage, spStage.m_pObject);
        NiD3DPass* v3 = spPass->m_pObject;
        if (!spPass->m_pObject->m_pkRenderStateGroup) {
            v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v3->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        NiD3DPass* v4 = spPass->m_pObject;
        if (!spPass->m_pObject->m_pkRenderStateGroup) {
            v4->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v4->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!v4->m_pkRenderStateGroup) {
            v4->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v4->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        NiD3DPass* v5 = spPass->m_pObject;
        if (!spPass->m_pObject->m_pkRenderStateGroup) {
            v5->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v5->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        NiD3DPass* v6 = spPass->m_pObject;
        if (!spPass->m_pObject->m_pkRenderStateGroup) {
            v6->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v6->m_pkRenderStateGroup->SetRenderState(168u, 0, true);
    }
    NiD3DPass* v7 = spPass->m_pObject;
    NiD3DVertexShader* v8 = this->spVertexShader[5];
    NiD3DVertexShader* v9 = spPass->m_pObject->m_spVertexShader.m_pObject;
    if (v9 != v8) {
        if (v9) {
            NiDecRefCount(v9);
        }
        v7->m_spVertexShader.m_pObject = v8;
        if (v8) {
            NiIncRefCount(v8);
        }
    }
    NiD3DPass* v10 = spPass->m_pObject;
    NiD3DPixelShader* v11 = this->spPixelShader[3];
    v10->m_spPixelShader = v11;
    NiD3DPass** p_m_pObject = &this->SkyShader::spPass[8].m_pObject;
    if (!this->SkyShader::spPass[8].m_pObject) {
        NiD3DPass::CreateNewPass(this->SkyShader::spPass[8]);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
        (*p_m_pObject)->SetStage((*p_m_pObject)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DPass* v14 = *p_m_pObject;
        if (!(*p_m_pObject)->m_pkRenderStateGroup) {
            v14->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v14->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        NiD3DPass* v15 = *p_m_pObject;
        if (!(*p_m_pObject)->m_pkRenderStateGroup) {
            v15->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v15->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
        NiD3DPass* v16 = *p_m_pObject;
        if (!(*p_m_pObject)->m_pkRenderStateGroup) {
            v16->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v16->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        NiD3DPass* v17 = *p_m_pObject;
        if (!(*p_m_pObject)->m_pkRenderStateGroup) {
            v17->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v17->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        NiD3DPass* v18 = *p_m_pObject;
        if (!(*p_m_pObject)->m_pkRenderStateGroup) {
            v18->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v18->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        NiD3DPass* v19 = *p_m_pObject;
        if (!(*p_m_pObject)->m_pkRenderStateGroup) {
            v19->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v19->m_pkRenderStateGroup->SetRenderState(168u, 8u, true);
    }
    NiD3DPass* v20 = *p_m_pObject;
    NiD3DVertexShader* v21 = this->spVertexShader[10];
    NiD3DVertexShader* v22 = (*p_m_pObject)->m_spVertexShader.m_pObject;
    if (v22 != v21) {
        if (v22) {
            NiDecRefCount(v22);
        }
        v20->m_spVertexShader.m_pObject = v21;
        if (v21) {
            NiIncRefCount(v21);
        }
    }
    NiD3DPass* v23 = *p_m_pObject;
    NiD3DPixelShader* v24 = this->spPixelShader[7];
    v23->m_spPixelShader = v24;
    NiD3DPass** v26 = &this->SkyShader::spPass[9].m_pObject;
    if (!this->SkyShader::spPass[9].m_pObject) {
        NiD3DPass::CreateNewPass(this->SkyShader::spPass[9]);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
        (*v26)->SetStage((*v26)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DPass* v27 = *v26;
        if (!(*v26)->m_pkRenderStateGroup) {
            v27->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v27->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        NiD3DPass* v28 = *v26;
        if (!(*v26)->m_pkRenderStateGroup) {
            v28->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v28->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
        if (!v28->m_pkRenderStateGroup) {
            v28->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v28->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        NiD3DPass* v29 = *v26;
        if (!(*v26)->m_pkRenderStateGroup) {
            v29->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v29->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        NiD3DPass* v30 = *v26;
        if (!(*v26)->m_pkRenderStateGroup) {
            v30->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v30->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        NiD3DPass* v31 = *v26;
        if (!(*v26)->m_pkRenderStateGroup) {
            v31->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v31->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        NiD3DPass* v32 = *v26;
        if (!(*v26)->m_pkRenderStateGroup) {
            v32->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v32->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        NiD3DPass* v33 = *v26;
        if (!(*v26)->m_pkRenderStateGroup) {
            v33->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v33->m_pkRenderStateGroup->SetRenderState(168u, 8u, true);
    }
    NiD3DPass* v34 = *v26;
    NiD3DVertexShader* v35 = this->spVertexShader[8];
    NiD3DVertexShader* v36 = (*v26)->m_spVertexShader.m_pObject;
    if (v36 != v35) {
        if (v36) {
            NiDecRefCount(v36);
        }
        v34->m_spVertexShader.m_pObject = v35;
        if (v35) {
            NiIncRefCount(v35);
        }
    }
    NiD3DPass* v37 = *v26;
    NiD3DPixelShader* v38 = this->spPixelShader[5];
    v37->m_spPixelShader = v38;
    NiD3DPass** v40 = &this->SkyShader::spPass[10].m_pObject;
    if (!this->SkyShader::spPass[10].m_pObject) {
        NiD3DPass::CreateNewPass(this->SkyShader::spPass[10]);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
        (*v40)->SetStage((*v40)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 1u, 3u, 2u, true);
        (*v40)->SetStage((*v40)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DPass* v41 = *v40;
        if (!(*v40)->m_pkRenderStateGroup) {
            v41->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v41->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        NiD3DPass* v42 = *v40;
        if (!(*v40)->m_pkRenderStateGroup) {
            v42->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v42->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        NiD3DPass* v43 = *v40;
        if (!(*v40)->m_pkRenderStateGroup) {
            v43->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v43->m_pkRenderStateGroup->SetRenderState(19u, 1u, true);
        if (!v43->m_pkRenderStateGroup) {
            v43->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v43->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        NiD3DPass* v44 = *v40;
        if (!(*v40)->m_pkRenderStateGroup) {
            v44->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v44->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        NiD3DPass* v45 = *v40;
        if (!(*v40)->m_pkRenderStateGroup) {
            v45->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v45->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        NiD3DPass* v46 = *v40;
        if (!(*v40)->m_pkRenderStateGroup) {
            v46->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v46->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        NiD3DPass* v47 = *v40;
        if (!(*v40)->m_pkRenderStateGroup) {
            v47->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v47->m_pkRenderStateGroup->SetRenderState(168u, 8u, true);
    }
    NiD3DPass* v48 = *v40;
    NiD3DVertexShader* v49 = this->spVertexShader[9];
    NiD3DVertexShader* v50 = (*v40)->m_spVertexShader.m_pObject;
    if (v50 != v49) {
        if (v50) {
            NiDecRefCount(v50);
        }
        v48->m_spVertexShader.m_pObject = v49;
        if (v49) {
            NiIncRefCount(v49);
        }
    }
    NiD3DPass* v51 = *v40;
    NiD3DPixelShader* v52 = this->spPixelShader[6];
    v51->m_spPixelShader = v52;
    NiD3DPass** v54 = &this->SkyShader::spPass[1].m_pObject;
    if (!this->SkyShader::spPass[1].m_pObject) {
        NiD3DPass::CreateNewPass(this->SkyShader::spPass[1]);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
        (*v54)->SetStage((*v54)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 1u, 3u, 2u, true);
        (*v54)->SetStage((*v54)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DPass* v55 = *v54;
        if (!(*v54)->m_pkRenderStateGroup) {
            v55->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v55->m_pkRenderStateGroup->SetRenderState(7u, 0, true);
        NiD3DPass* v56 = *v54;
        if (!(*v54)->m_pkRenderStateGroup) {
            v56->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v56->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        NiD3DPass* v57 = *v54;
        if (!(*v54)->m_pkRenderStateGroup) {
            v57->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v57->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        NiD3DPass* v58 = *v54;
        if (!(*v54)->m_pkRenderStateGroup) {
            v58->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v58->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!v58->m_pkRenderStateGroup) {
            v58->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v58->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        NiD3DPass* v59 = *v54;
        if (!(*v54)->m_pkRenderStateGroup) {
            v59->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v59->m_pkRenderStateGroup->SetRenderState(52u, 0, true);
        NiD3DPass* v60 = *v54;
        if (!(*v54)->m_pkRenderStateGroup) {
            v60->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v60->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        NiD3DPass* v61 = *v54;
        if (!(*v54)->m_pkRenderStateGroup) {
            v61->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v61->m_pkRenderStateGroup->SetRenderState(168u, 7u, true);
    }
    NiD3DPass* v62 = *v54;
    NiD3DVertexShader* v63 = this->spVertexShader[1];
    NiD3DVertexShader* v64 = (*v54)->m_spVertexShader.m_pObject;
    if (v64 != v63) {
        if (v64) {
            NiDecRefCount(v64);
        }
        v62->m_spVertexShader.m_pObject = v63;
        if (v63) {
            NiIncRefCount(v63);
        }
    }
    NiD3DPass* v65 = *v54;
    NiD3DPixelShader* v66 = this->spPixelShader[1];
    v65->m_spPixelShader = v66;
    NiD3DPass** v68 = &this->SkyShader::spPass[3].m_pObject;
    if (!this->SkyShader::spPass[3].m_pObject) {
        NiD3DPass::CreateNewPass(this->SkyShader::spPass[3]);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
        (*v68)->SetStage((*v68)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DPass* v69 = *v68;
        if (!(*v68)->m_pkRenderStateGroup) {
            v69->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v69->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        NiD3DPass* v70 = *v68;
        if (!(*v68)->m_pkRenderStateGroup) {
            v70->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v70->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        NiD3DPass* v71 = *v68;
        if (!(*v68)->m_pkRenderStateGroup) {
            v71->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v71->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!v71->m_pkRenderStateGroup) {
            v71->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v71->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
        NiD3DPass* v72 = *v68;
        if (!(*v68)->m_pkRenderStateGroup) {
            v72->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v72->m_pkRenderStateGroup->SetRenderState(168u, 7u, true);
    }
    NiD3DPass* v73 = *v68;
    NiD3DVertexShader* v74 = this->spVertexShader[4];
    NiD3DVertexShader* v75 = (*v68)->m_spVertexShader.m_pObject;
    if (v75 != v74) {
        if (v75) {
            NiDecRefCount(v75);
        }
        v73->m_spVertexShader.m_pObject = v74;
        if (v74) {
            NiIncRefCount(v74);
        }
    }
    NiD3DPass* v76 = *v68;
    NiD3DPixelShader* v77 = this->spPixelShader[4];
    v76->m_spPixelShader = v77;
    NiD3DPass** v79 = &this->SkyShader::spPass[4].m_pObject;
    if (!this->SkyShader::spPass[4].m_pObject) {
        NiD3DPass::CreateNewPass(this->SkyShader::spPass[4]);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
        (*v79)->SetStage((*v79)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 1u, 3u, 2u, true);
        (*v79)->SetStage((*v79)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DPass* v80 = *v79;
        if (!(*v79)->m_pkRenderStateGroup) {
            v80->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v80->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        NiD3DPass* v81 = *v79;
        if (!(*v79)->m_pkRenderStateGroup) {
            v81->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v81->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        NiD3DPass* v82 = *v79;
        if (!(*v79)->m_pkRenderStateGroup) {
            v82->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v82->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!v82->m_pkRenderStateGroup) {
            v82->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v82->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        NiD3DPass* v83 = *v79;
        if (!(*v79)->m_pkRenderStateGroup) {
            v83->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v83->m_pkRenderStateGroup->SetRenderState(168u, 7u, true);
    }
    NiD3DPass* v84 = *v79;
    NiD3DVertexShader* v85 = this->spVertexShader[6];
    NiD3DVertexShader* v86 = (*v79)->m_spVertexShader.m_pObject;
    if (v86 != v85) {
        if (v86) {
            NiDecRefCount(v86);
        }
        v84->m_spVertexShader.m_pObject = v85;
        if (v85) {
            NiIncRefCount(v85);
        }
    }
    NiD3DPass* v87 = *v79;
    NiD3DPixelShader* v88 = this->spPixelShader[1];
    v87->m_spPixelShader = v88;
    NiD3DPass** v90 = &this->SkyShader::spPass[5].m_pObject;
    if (!this->SkyShader::spPass[5].m_pObject) {
        NiD3DPass::CreateNewPass(this->SkyShader::spPass[5]);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
        (*v90)->SetStage((*v90)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 1u, 3u, 2u, true);
        (*v90)->SetStage((*v90)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DPass* v91 = *v90;
        if (!(*v90)->m_pkRenderStateGroup) {
            v91->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v91->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        NiD3DPass* v92 = *v90;
        if (!(*v90)->m_pkRenderStateGroup) {
            v92->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v92->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        NiD3DPass* v93 = *v90;
        if (!(*v90)->m_pkRenderStateGroup) {
            v93->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v93->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!v93->m_pkRenderStateGroup) {
            v93->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v93->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        NiD3DPass* v94 = *v90;
        if (!(*v90)->m_pkRenderStateGroup) {
            v94->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v94->m_pkRenderStateGroup->SetRenderState(168u, 7u, true);
    }
    NiD3DPass* v95 = *v90;
    NiD3DVertexShader* v96 = this->spVertexShader[7];
    NiD3DVertexShader* v97 = (*v90)->m_spVertexShader.m_pObject;
    if (v97 != v96) {
        if (v97) {
            NiDecRefCount(v97);
        }
        v95->m_spVertexShader.m_pObject = v96;
        if (v96) {
            NiIncRefCount(v96);
        }
    }
    NiD3DPass* v98 = *v90;
    NiD3DPixelShader* v99 = this->spPixelShader[2];
    v98->m_spPixelShader = v99;
    NiD3DPass** v101 = &this->SkyShader::spPass[6].m_pObject;
    if (!this->SkyShader::spPass[6].m_pObject) {
        NiD3DPass::CreateNewPass(this->SkyShader::spPass[6]);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
        (*v101)->SetStage((*v101)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 1u, 3u, 2u, true);
        (*v101)->SetStage((*v101)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DPass* v102 = *v101;
        if (!(*v101)->m_pkRenderStateGroup) {
            v102->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v102->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        NiD3DPass* v103 = *v101;
        if (!(*v101)->m_pkRenderStateGroup) {
            v103->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v103->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        NiD3DPass* v104 = *v101;
        if (!(*v101)->m_pkRenderStateGroup) {
            v104->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v104->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!v104->m_pkRenderStateGroup) {
            v104->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v104->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        NiD3DPass* v105 = *v101;
        if (!(*v101)->m_pkRenderStateGroup) {
            v105->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v105->m_pkRenderStateGroup->SetRenderState(168u, 7u, true);
    }
    NiD3DPass* v106 = *v101;
    NiD3DVertexShader* v107 = this->spVertexShader[1];
    NiD3DVertexShader* v108 = (*v101)->m_spVertexShader.m_pObject;
    if (v108 != v107) {
        if (v108) {
            NiDecRefCount(v108);
        }
        v106->m_spVertexShader.m_pObject = v107;
        if (v107) {
            NiIncRefCount(v107);
        }
    }
    NiD3DPass* v109 = *v101;
    NiD3DPixelShader* v110 = this->spPixelShader[1];
    v109->m_spPixelShader = v110;
    NiD3DPass** v112 = &this->SkyShader::spPass[2].m_pObject;
    if (!this->SkyShader::spPass[2].m_pObject) {
        NiD3DPass::CreateNewPass(this->SkyShader::spPass[2]);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
        (*v112)->SetStage((*v112)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 1u, 3u, 2u, true);
        (*v112)->SetStage((*v112)->m_uiCurrentStage, spStage.m_pObject);
        NiD3DPass* v113 = *v112;
        if (!(*v112)->m_pkRenderStateGroup) {
            v113->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v113->m_pkRenderStateGroup->SetRenderState(15u, 1u, true);
        NiD3DPass* v114 = *v112;
        if (!(*v112)->m_pkRenderStateGroup) {
            v114->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v114->m_pkRenderStateGroup->SetRenderState(24u, 0, true);
        NiD3DPass* v115 = *v112;
        if (!(*v112)->m_pkRenderStateGroup) {
            v115->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v115->m_pkRenderStateGroup->SetRenderState(25u, 5u, true);
        NiD3DPass* v116 = *v112;
        if (!(*v112)->m_pkRenderStateGroup) {
            v116->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v116->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        NiD3DPass* v117 = *v112;
        if (!(*v112)->m_pkRenderStateGroup) {
            v117->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v117->m_pkRenderStateGroup->SetRenderState(19u, 1u, true);
        if (!v117->m_pkRenderStateGroup) {
            v117->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v117->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
    }
    NiD3DPass* v118 = *v112;
    NiD3DVertexShader* v119 = this->spVertexShader[2];
    NiD3DVertexShader* v120 = (*v112)->m_spVertexShader.m_pObject;
    if (v120 != v119) {
        if (v120) {
            NiDecRefCount(v120);
        }
        v118->m_spVertexShader.m_pObject = v119;
        if (v119) {
            NiIncRefCount(v119);
        }
    }
    NiD3DPass* v121 = *v112;
    NiD3DPixelShader* v122 = this->spPixelShader[1];
    v121->m_spPixelShader = v122;
    NiPointer<NiD3DPass>* v124 = &this->SkyShader::spPass[7];
    if (!this->SkyShader::spPass[7].m_pObject) {
        NiD3DPass::CreateNewPass(this->SkyShader::spPass[7]);
        NiD3DPass* v125 = v124->m_pObject;
        if (!v124->m_pObject->m_pkRenderStateGroup) {
            v125->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v125->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
        NiD3DPass* v126 = v124->m_pObject;
        if (!v124->m_pObject->m_pkRenderStateGroup) {
            v126->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v126->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        NiD3DPass* v127 = v124->m_pObject;
        if (!v124->m_pObject->m_pkRenderStateGroup) {
            v127->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v127->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!v127->m_pkRenderStateGroup) {
            v127->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v127->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        NiD3DPass* v128 = v124->m_pObject;
        if (!v124->m_pObject->m_pkRenderStateGroup) {
            v128->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v128->m_pkRenderStateGroup->SetRenderState(168u, 7u, true);
    }
    NiD3DPass* v129 = v124->m_pObject;
    NiD3DVertexShader* v130 = this->spVertexShader[0];
    NiD3DVertexShader* v131 = v124->m_pObject->m_spVertexShader.m_pObject;
    if (v131 != v130) {
        if (v131) {
            NiDecRefCount(v131);
        }
        v129->m_spVertexShader.m_pObject = v130;
        if (v130) {
            NiIncRefCount(v130);
        }
    }
    NiD3DPass* v132 = v124->m_pObject;
    NiD3DPixelShader* v133 = this->spPixelShader[0];
    v132->m_spPixelShader = v133;
    return true;
}

// Each macro list is written by storing its macros and their {0, 0} terminator one by one, then clearing the rest of
// the list.
void SkyShader::LoadShaders() {
    BSShaderMacro v74;
    BSShaderMacro v75;
    BSShaderMacro v76;
    BSShaderMacro v77;
    BSShaderMacro v78;
    BSShaderMacro v79;
    BSShaderMacro v80;
    BSShaderMacro v81;
    BSShaderMacro v82;
    BSShaderMacro v83;
    BSShaderMacro v84;
    BSShaderMacro v85;
    BSShaderMacro v86;
    BSShaderMacro v87;
    BSShaderMacro v88;
    BSShaderMacro v89;
    BSShaderMacro v90;
    BSShaderMacro v91;
    char apDest[260];
    char Str1[260];
    char v94[352];
    v90.strPath = "sky\\v\\sky.v.hlsl";
    v90.kMacros[0].Name = nullptr;
    v90.kMacros[0].Definition = nullptr;
    std::memset(&v90.kMacros[1], 0, 8 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\v\\sky.v.hlsl", Str1);
    BSsprintf(apDest, 260, "SKY.vso");
    const char* VSTarget = BSShaderManager::GetVSTarget();
    NiD3DVertexShader* v3 = CompileVertexShaderHLSL(Str1, v90.kMacros, VSTarget, apDest);
    NiD3DVertexShader* v4 = spVertexShader[0];
    if (v4 != v3) {
        if (v4) {
            NiDecRefCount(v4);
        }
        spVertexShader[0] = v3;
        if (v3) {
            NiIncRefCount(v3);
        }
    }
    v84.strPath = "sky\\v\\sky.v.hlsl";
    v84.kMacros[0].Name = "TEX";
    v84.kMacros[0].Definition = nullptr;
    v84.kMacros[1].Name = nullptr;
    v84.kMacros[1].Definition = nullptr;
    std::memset(&v84.kMacros[2], 0, 7 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\v\\sky.v.hlsl", Str1);
    BSsprintf(apDest, 260, "SKYT.vso");
    const char* v6 = BSShaderManager::GetVSTarget();
    NiD3DVertexShader* v7 = CompileVertexShaderHLSL(Str1, v84.kMacros, v6, apDest);
    NiD3DVertexShader* v8 = spVertexShader[1];
    if (v8 != v7) {
        if (v8) {
            NiDecRefCount(v8);
        }
        spVertexShader[1] = v7;
        if (v7) {
            NiIncRefCount(v7);
        }
    }
    v81.strPath = "sky\\v\\sky.v.hlsl";
    v81.kMacros[0].Name = "DEPTH_VALUE";
    v81.kMacros[0].Definition = "0.99995f";
    v81.kMacros[1].Name = "TEX";
    v81.kMacros[1].Definition = nullptr;
    v81.kMacros[2].Name = nullptr;
    v81.kMacros[2].Definition = nullptr;
    std::memset(&v81.kMacros[3], 0, 6 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\v\\sky.v.hlsl", Str1);
    BSsprintf(apDest, 260, "SKYMM.vso");
    const char* v10 = BSShaderManager::GetVSTarget();
    NiD3DVertexShader* v11 = CompileVertexShaderHLSL(Str1, v81.kMacros, v10, apDest);
    NiD3DVertexShader* v12 = spVertexShader[2];
    if (v12 != v11) {
        if (v12) {
            NiDecRefCount(v12);
        }
        spVertexShader[2] = v11;
        if (v11) {
            NiIncRefCount(v11);
        }
    }
    v77.strPath = "sky\\v\\sky.v.hlsl";
    v77.kMacros[0].Name = "DEPTH_VALUE";
    v77.kMacros[0].Definition = "1.0f";
    v77.kMacros[1].Name = "HORIZFADE";
    v77.kMacros[1].Definition = nullptr;
    v77.kMacros[2].Name = nullptr;
    v77.kMacros[2].Definition = nullptr;
    std::memset(&v77.kMacros[3], 0, 6 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\v\\sky.v.hlsl", Str1);
    BSsprintf(apDest, 260, "SKYSTARS.vso");
    const char* v14 = BSShaderManager::GetVSTarget();
    NiD3DVertexShader* v15 = CompileVertexShaderHLSL(Str1, v77.kMacros, v14, apDest);
    NiD3DVertexShader* v16 = spVertexShader[4];
    if (v16 != v15) {
        if (v16) {
            NiDecRefCount(v16);
        }
        spVertexShader[4] = v15;
        if (v15) {
            NiIncRefCount(v15);
        }
    }
    v89.strPath = "sky\\v\\sky.v.hlsl";
    v89.kMacros[0].Name = "OCCLUSION";
    v89.kMacros[0].Definition = nullptr;
    v89.kMacros[1].Name = nullptr;
    v89.kMacros[1].Definition = nullptr;
    std::memset(&v89.kMacros[2], 0, 7 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\v\\sky.v.hlsl", Str1);
    BSsprintf(apDest, 260, "SKYOCC.vso");
    const char* v18 = BSShaderManager::GetVSTarget();
    NiD3DVertexShader* v19 = CompileVertexShaderHLSL(Str1, v89.kMacros, v18, apDest);
    NiD3DVertexShader* v20 = spVertexShader[5];
    if (v20 != v19) {
        if (v20) {
            NiDecRefCount(v20);
        }
        spVertexShader[5] = v19;
        if (v19) {
            NiIncRefCount(v19);
        }
    }
    v76.strPath = "sky\\v\\sky.v.hlsl";
    v76.kMacros[0].Name = "TEX";
    v76.kMacros[0].Definition = nullptr;
    v76.kMacros[1].Name = "CLOUDS";
    v76.kMacros[1].Definition = nullptr;
    v76.kMacros[2].Name = nullptr;
    v76.kMacros[2].Definition = nullptr;
    std::memset(&v76.kMacros[3], 0, 6 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\v\\sky.v.hlsl", Str1);
    BSsprintf(apDest, 260, "SKYCLOUDS.vso");
    const char* v22 = BSShaderManager::GetVSTarget();
    NiD3DVertexShader* v23 = CompileVertexShaderHLSL(Str1, v76.kMacros, v22, apDest);
    NiD3DVertexShader* v24 = spVertexShader[6];
    if (v24 != v23) {
        if (v24) {
            NiDecRefCount(v24);
        }
        spVertexShader[6] = v23;
        if (v23) {
            NiIncRefCount(v23);
        }
    }
    // EXE-INLINED none: memset
    for (int32_t i = 4; i < 9; ++i) {
        v75.kMacros[i].Name = nullptr;
        v75.kMacros[i].Definition = nullptr;
    }
    v75.strPath = "sky\\v\\sky.v.hlsl";
    v75.kMacros[0].Name = "HORIZFADE";
    v75.kMacros[0].Definition = nullptr;
    v75.kMacros[1].Name = "TEX";
    v75.kMacros[1].Definition = nullptr;
    v75.kMacros[2].Name = "CLOUDS";
    v75.kMacros[2].Definition = nullptr;
    v75.kMacros[3].Name = nullptr;
    v75.kMacros[3].Definition = nullptr;
    BSShader::MakePath(v76.strPath, Str1);
    BSsprintf(apDest, 260, "SKYCLOUDSFADE.vso");
    const char* v26 = BSShaderManager::GetVSTarget();
    NiD3DVertexShader* v27 = CompileVertexShaderHLSL(Str1, v75.kMacros, v26, apDest);
    NiD3DVertexShader* v28 = spVertexShader[7];
    if (v28 != v27) {
        if (v28) {
            NiDecRefCount(v28);
        }
        spVertexShader[7] = v27;
        if (v27) {
            NiIncRefCount(v27);
        }
    }
    v91.strPath = "sky\\p\\sky.p.hlsl";
    v91.kMacros[0].Name = nullptr;
    v91.kMacros[0].Definition = nullptr;
    std::memset(&v91.kMacros[1], 0, 8 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\p\\sky.p.hlsl", Str1);
    BSsprintf(apDest, 260, "SKY.pso");
    const char* PSTarget = BSShaderManager::GetPSTarget(false);
    NiD3DPixelShader* v31 = CompilePixelShaderHLSL(Str1, v91.kMacros, PSTarget, apDest);
    NiD3DPixelShader* v32 = spPixelShader[0];
    if (v32 != v31) {
        if (v32) {
            NiDecRefCount(v32);
        }
        spPixelShader[0] = v31;
        if (v31) {
            NiIncRefCount(v31);
        }
    }
    v82.strPath = "sky\\p\\sky.p.hlsl";
    v82.kMacros[0].Name = "TEX";
    v82.kMacros[0].Definition = nullptr;
    v82.kMacros[1].Name = nullptr;
    v82.kMacros[1].Definition = nullptr;
    std::memset(&v82.kMacros[2], 0, 7 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\p\\sky.p.hlsl", Str1);
    BSsprintf(apDest, 260, "SKYTEX.pso");
    const char* v34 = BSShaderManager::GetPSTarget(false);
    NiD3DPixelShader* v35 = CompilePixelShaderHLSL(Str1, v82.kMacros, v34, apDest);
    NiD3DPixelShader* v36 = spPixelShader[1];
    if (v36 != v35) {
        if (v36) {
            NiDecRefCount(v36);
        }
        spPixelShader[1] = v35;
        if (v35) {
            NiIncRefCount(v35);
        }
    }
    v83.strPath = "sky\\p\\sky.p.hlsl";
    v83.kMacros[0].Name = "HORIZFADE";
    v83.kMacros[0].Definition = nullptr;
    v83.kMacros[1].Name = "TEX";
    v83.kMacros[1].Definition = nullptr;
    v83.kMacros[2].Name = nullptr;
    v83.kMacros[2].Definition = nullptr;
    std::memset(&v83.kMacros[3], 0, 6 * sizeof(D3DXMACRO));
    BSShader::MakePath(v82.strPath, Str1);
    BSsprintf(apDest, 260, "SKYTEXFADE.pso");
    const char* v38 = BSShaderManager::GetPSTarget(false);
    NiD3DPixelShader* v39 = CompilePixelShaderHLSL(Str1, v83.kMacros, v38, apDest);
    NiD3DPixelShader* v40 = spPixelShader[2];
    if (v40 != v39) {
        if (v40) {
            NiDecRefCount(v40);
        }
        spPixelShader[2] = v39;
        if (v39) {
            NiIncRefCount(v39);
        }
    }
    // Formats the sun brightness here; the next list does not use the text.
    _gcvt(SkyShader::g_fSunBrightness_BlurShader, 12, v94);
    v88.strPath = "sky\\p\\sky.p.hlsl";
    v88.kMacros[0].Name = "OCCLUSION";
    v88.kMacros[0].Definition = nullptr;
    v88.kMacros[1].Name = nullptr;
    v88.kMacros[1].Definition = nullptr;
    std::memset(&v88.kMacros[2], 0, 7 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\p\\sky.p.hlsl", Str1);
    BSsprintf(apDest, 260, "SKYSUNOCCL.pso");
    const char* v42 = BSShaderManager::GetPSTarget(false);
    NiD3DPixelShader* v43 = CompilePixelShaderHLSL(Str1, v88.kMacros, v42, apDest);
    NiD3DPixelShader* v44 = spPixelShader[3];
    if (v44 != v43) {
        if (v44) {
            NiDecRefCount(v44);
        }
        spPixelShader[3] = v43;
        if (v43) {
            NiIncRefCount(v43);
        }
    }
    v86.strPath = "sky\\p\\sky.p.hlsl";
    v86.kMacros[0].Name = "HORIZFADE";
    v86.kMacros[0].Definition = nullptr;
    v86.kMacros[1].Name = nullptr;
    v86.kMacros[1].Definition = nullptr;
    std::memset(&v86.kMacros[2], 0, 7 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\p\\sky.p.hlsl", Str1);
    BSsprintf(apDest, 260, "SKYSHORIZFADE.pso");
    const char* v46 = BSShaderManager::GetPSTarget(false);
    NiD3DPixelShader* v47 = CompilePixelShaderHLSL(Str1, v86.kMacros, v46, apDest);
    NiD3DPixelShader* v48 = spPixelShader[4];
    if (v48 != v47) {
        if (v48) {
            NiDecRefCount(v48);
        }
        spPixelShader[4] = v47;
        if (v47) {
            NiIncRefCount(v47);
        }
    }
    v80.strPath = "sky\\v\\sky.v.hlsl";
    v80.kMacros[0].Name = "DEPTH_VALUE";
    v80.kMacros[0].Definition = "0.999999";
    v80.kMacros[1].Name = "SI";
    v80.kMacros[1].Definition = nullptr;
    v80.kMacros[2].Name = nullptr;
    v80.kMacros[2].Definition = nullptr;
    std::memset(&v80.kMacros[3], 0, 6 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\v\\sky.v.hlsl", Str1);
    BSsprintf(apDest, 260, "SKYFAR.vso");
    const char* v50 = BSShaderManager::GetVSTarget();
    NiD3DVertexShader* v51 = CompileVertexShaderHLSL(Str1, v80.kMacros, v50, apDest);
    NiD3DVertexShader* v52 = spVertexShader[8];
    if (v52 != v51) {
        if (v52) {
            NiDecRefCount(v52);
        }
        spVertexShader[8] = v51;
        if (v51) {
            NiIncRefCount(v51);
        }
    }
    // EXE-INLINED none: memset
    for (int32_t i = 4; i < 9; ++i) {
        v74.kMacros[i].Name = nullptr;
        v74.kMacros[i].Definition = nullptr;
    }
    v74.strPath = "sky\\v\\sky.v.hlsl";
    v74.kMacros[0].Name = "DEPTH_VALUE";
    v74.kMacros[0].Definition = "0.999999";
    v74.kMacros[1].Name = "SI";
    v74.kMacros[1].Definition = nullptr;
    v74.kMacros[2].Name = "SI_CLOUDS";
    v74.kMacros[2].Definition = "";
    v74.kMacros[3].Name = nullptr;
    v74.kMacros[3].Definition = nullptr;
    BSShader::MakePath("sky\\v\\sky.v.hlsl", Str1);
    BSsprintf(apDest, 260, "SKYCLOUDSI.vso");
    const char* v54 = BSShaderManager::GetVSTarget();
    NiD3DVertexShader* v55 = CompileVertexShaderHLSL(Str1, v74.kMacros, v54, apDest);
    NiD3DVertexShader* v56 = spVertexShader[9];
    if (v56 != v55) {
        if (v56) {
            NiDecRefCount(v56);
        }
        spVertexShader[9] = v55;
        if (v55) {
            NiIncRefCount(v55);
        }
    }
    v87.strPath = "sky\\v\\sky_quad.v.hlsl";
    v87.kMacros[0].Name = "SI";
    v87.kMacros[0].Definition = nullptr;
    v87.kMacros[1].Name = nullptr;
    v87.kMacros[1].Definition = nullptr;
    std::memset(&v87.kMacros[2], 0, 7 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\v\\sky_quad.v.hlsl", Str1);
    BSsprintf(apDest, 260, "SKYQUADSI.vso");
    const char* v58 = BSShaderManager::GetVSTarget();
    NiD3DVertexShader* v59 = CompileVertexShaderHLSL(Str1, v87.kMacros, v58, apDest);
    NiD3DVertexShader* v60 = spVertexShader[10];
    if (v60 != v59) {
        if (v60) {
            NiDecRefCount(v60);
        }
        spVertexShader[10] = v59;
        if (v59) {
            NiIncRefCount(v59);
        }
    }
    _gcvt(SkyShader::fSkyBrightness, 12, v94);
    v85.strPath = "sky\\p\\sky.p.hlsl";
    v85.kMacros[0].Name = "SI";
    v85.kMacros[0].Definition = v94;
    v85.kMacros[1].Name = nullptr;
    v85.kMacros[1].Definition = nullptr;
    std::memset(&v85.kMacros[2], 0, 7 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\p\\sky.p.hlsl", Str1);
    BSsprintf(apDest, 260, "SKYSI.pso");
    const char* v62 = BSShaderManager::GetPSTarget(false);
    NiD3DPixelShader* v63 = CompilePixelShaderHLSL(Str1, v85.kMacros, v62, apDest);
    NiD3DPixelShader* v64 = spPixelShader[7];
    if (v64 != v63) {
        if (v64) {
            NiDecRefCount(v64);
        }
        spPixelShader[7] = v63;
        if (v63) {
            NiIncRefCount(v63);
        }
    }
    _gcvt(SkyShader::g_fSunBrightness_BlurShader, 12, v94);
    v78.strPath = "sky\\p\\sky.p.hlsl";
    v78.kMacros[0].Name = "SI";
    v78.kMacros[0].Definition = v94;
    v78.kMacros[1].Name = "SI_SUN";
    v78.kMacros[1].Definition = nullptr;
    v78.kMacros[2].Name = nullptr;
    v78.kMacros[2].Definition = nullptr;
    std::memset(&v78.kMacros[3], 0, 6 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\p\\sky.p.hlsl", Str1);
    BSsprintf(apDest, 260, "SKYSISUN.pso");
    const char* v66 = BSShaderManager::GetPSTarget(false);
    NiD3DPixelShader* v67 = CompilePixelShaderHLSL(Str1, v78.kMacros, v66, apDest);
    NiD3DPixelShader* v68 = spPixelShader[5];
    if (v68 != v67) {
        if (v68) {
            NiDecRefCount(v68);
        }
        spPixelShader[5] = v67;
        if (v67) {
            NiIncRefCount(v67);
        }
    }
    v79.strPath = "sky\\p\\sky.p.hlsl";
    v79.kMacros[0].Name = "SI";
    v79.kMacros[0].Definition = nullptr;
    v79.kMacros[1].Name = "SI_CLOUDS";
    v79.kMacros[1].Definition = nullptr;
    v79.kMacros[2].Name = nullptr;
    v79.kMacros[2].Definition = nullptr;
    std::memset(&v79.kMacros[3], 0, 6 * sizeof(D3DXMACRO));
    BSShader::MakePath("sky\\p\\sky.p.hlsl", Str1);
    BSsprintf(apDest, 260, "SKYSICLOUDS.pso");
    const char* v70 = BSShaderManager::GetPSTarget(false);
    NiD3DPixelShader* v71 = CompilePixelShaderHLSL(Str1, v79.kMacros, v70, apDest);
    NiD3DPixelShader* v72 = spPixelShader[6];
    if (v72 != v71) {
        if (v72) {
            NiDecRefCount(v72);
        }
        spPixelShader[6] = v71;
        if (v71) {
            NiIncRefCount(v71);
        }
    }
}

void SkyShader::SetSunBrightness(float value) {
    SkyShader::g_fSunBrightness_BlurShader = value;
}

void SkyShader::SetSkyBrightness(float value) {
    SkyShader::fSkyBrightness = value;
}

void SkyShader::CreateSkyQuad(NiPointer<NiTriShape>& aspTriShape) {
    NiPoint3* pVertices = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(4 * sizeof(NiPoint3)));
    pVertices[0].x = -1.0f;
    pVertices[0].y = -1.0f;
    pVertices[0].z = 0.999999f;
    pVertices[1].x = 1.0f;
    pVertices[1].y = -1.0f;
    pVertices[1].z = 0.999999f;
    pVertices[2].x = 1.0f;
    pVertices[2].y = 1.0f;
    pVertices[2].z = 0.999999f;
    pVertices[3].x = -1.0f;
    pVertices[3].y = 1.0f;
    pVertices[3].z = 0.999999f;
    uint16_t* pTriangles = static_cast<uint16_t*>(MemoryManager_Singleton.Allocate(6 * sizeof(uint16_t)));
    pTriangles[0] = 0;
    pTriangles[1] = 1;
    pTriangles[3] = 3;
    pTriangles[2] = 2;
    pTriangles[4] = 0;
    pTriangles[5] = 2;
    void* pColorMemory = MemoryManager_Singleton.Allocate(4 * sizeof(NiColorA));
    NiColorA* pColors = nullptr;
    if (pColorMemory) {
        pColors = ::new (pColorMemory) NiColorA[4];
    }
    pColors[0] = NiColorA_BLACK;
    pColors[1] = NiColorA_BLACK;
    pColors[2] = NiColorA_BLACK;
    pColors[3] = NiColorA_BLACK;
    void* pMemory = NiMemObject::operator new(sizeof(NiTriShape));
    NiTriShape* pShape = pMemory
        ? ::new (pMemory)
              NiTriShape(4, pVertices, nullptr, pColors, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, 2, pTriangles)
        : nullptr;
    NiPointer<NiTriShape> spShape(pShape);
    NiGeometryData* pData = pShape->m_spModelData.m_pObject;
    pData->m_kBound.m_kCenter.x = 0.0f;
    pData->m_kBound.m_kCenter.y = 0.0f;
    pData->m_kBound.m_kCenter.z = 0.0f;
    pData->m_kBound.m_fRadius = 1000.0f;
    BSShaderData kShaderData;
    kShaderData.~BSShaderData();
    ::new (&kShaderData) BSShaderData;
    kShaderData.eLightingShader = BSShaderData::LS_SKY;
    kShaderData.iSkyTYpe = SkyShaderProperty::SO_SKYQUAD;
    kShaderData.Convert(pShape, BSShaderData::PROP_SHADER);
    aspTriShape = pShape;
}

void SkyShader::SetCloudTexCoordOffset(uint16_t auID, float afOffset) {
    if (auID < 4u) {
        SkyShader::pCloudTexCoordYOffset[auID] = SkyShader::pCloudTexCoordYOffset[auID] + afOffset;
    }
}

BSShader* SkyShader::CreateShader() {
    SkyShader* v0 = nullptr;
    NiDX9ShaderDeclaration* v1 = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 3u, 1u);
    v1->SetEntry(0u, 0u, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0u);
    v1->SetEntry(1u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0u);
    v1->SetEntry(2u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0u);
    v1->GetD3DDeclaration();
    SkyShader* v2 = static_cast<SkyShader*>(NiMemObject::operator new(sizeof(SkyShader)));
    if (v2) {
        v0 = ::new (v2) SkyShader;
    }
    v0->Initialize();
    v0->LoadShaders();
    v0->LoadStagesAndPasses();
    v0->CreateConstantMaps();
    v0->SetShaderDecl(v1);
    return v0;
}
