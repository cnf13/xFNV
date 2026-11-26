#include "BSShader/BSShaderBloodSplatter.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderBloodSplatterProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffectHDR.h"
#include "BSShader/ImageSpaceTexture.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"

NiPointer<NiTexture> BSShaderBloodSplatter::spDefaultFlareTexture;
NiRTTI BSShaderBloodSplatter::ms_RTTI("BSShaderBloodSplatter", &NiD3DDefaultShader::ms_RTTI);

// Shader type 23; the shared light location and the default flare texture are set on each construction.
BSShaderBloodSplatter::BSShaderBloodSplatter() {
    iShaderType = 23;
    BSShaderBloodSplatter::Constant::LightLoc.x = 0.9f;
    BSShaderBloodSplatter::Constant::LightLoc.y = 0.9f;
    BSShaderBloodSplatter::Constant::LightLoc.z = 0.0f;
    BSShaderBloodSplatter::Constant::LightLoc.w = 0.1f;
    BSShaderManager::GetTexture(
        "Textures\\Effects\\WhiteFlash.dds",
        true,
        BSShaderBloodSplatter::spDefaultFlareTexture,
        true,
        false
    );
}

// The passes go back to the pass pool at their last reference, the shaders and the shared flare texture are released.
BSShaderBloodSplatter::~BSShaderBloodSplatter() {
    for (NiPointer<NiD3DPass>& rspPass : BSShaderBloodSplatter::spPass) {
        NiD3DPass* pPass = rspPass.m_pObject;
        if (pPass) {
            if (pPass->m_uiRefCount-- == 1) {
                pPass->ReturnPassToPool();
            }
            rspPass.m_pObject = nullptr;
        }
    }
    for (NiPointer<NiD3DVertexShader>& rspShader : spVertexShader) {
        rspShader = nullptr;
    }
    for (NiPointer<NiD3DPixelShader>& rspShader : spPixelShader) {
        rspShader = nullptr;
    }
    BSShaderBloodSplatter::spDefaultFlareTexture = nullptr;
}

const NiRTTI* BSShaderBloodSplatter::GetRTTI() const {
    return &BSShaderBloodSplatter::ms_RTTI;
}

void BSShaderBloodSplatter::SetupGeometryTextures(const NiPropertyState* apState) {
    BSShaderBloodSplatterProperty* pprop = static_cast<BSShaderBloodSplatterProperty*>(
        apState->m_aspProps[3].m_pObject
    );
    BSShaderBloodSplatter* v3 = this;
    if (BSShaderManager::eCurrentPass == BSShaderManager::BSSM_BLOOD_SPLATTER) {
        NiD3DPass* apProperties = v3->BSShaderBloodSplatter::spPass[0].m_pObject;
        uint32_t v5 = 0;
        NiPointer<NiTexture>* spTexture = pprop->spTexture;
        do {
            NiD3DTextureStage* pstage = apProperties->m_kStages.m_pBase[v5].m_pObject;
            pstage->m_spTexture = spTexture->m_pObject;
            NiTexturingProperty::ClampMode v8 = pprop->GetClampNI();
            pstage->SetClampMode(v8);
            ++v5;
            ++spTexture;
        }
        while (v5 < 2);
        v3 = this;
    }
    else {
        if (BSShaderManager::eCurrentPass == BSShaderManager::BSSM_BLOOD_SPLATTER_FLARE) {
            NiD3DPass* v9 = this->BSShaderBloodSplatter::spPass[1].m_pObject;
            NiD3DTextureStage* pstage_2 = v9->m_kStages.m_pBase->m_pObject;
            pstage_2->m_spTexture = pprop->spTexture[0].m_pObject;
            NiTexturingProperty::ClampMode v11 = pprop->GetClampNI();
            pstage_2->SetClampMode(v11);
            NiD3DTextureStage* pstage_3 = v9->m_kStages.m_pBase[1].m_pObject;
            if (ImageSpaceEffectHDR::kAdaptedLight.GetTexture()) {
                pstage_3->m_spTexture = ImageSpaceEffectHDR::kAdaptedLight.GetTexture();
            }
            else {
                pstage_3->m_spTexture = BSShaderBloodSplatter::spDefaultFlareTexture.m_pObject;
            }
            NiTexturingProperty::ClampMode v13 = pprop->GetClampNI();
            pstage_3->SetClampMode(v13);
        }
    }
    v3->ApplyTextureStages();
}

void BSShaderBloodSplatter::SetupGeometryConstants(const NiPropertyState* apState) {
    NiGeometry* pgeometry = BSShaderManager::pCurrentRenderPass->pGeometry;
    BSNiBound* pWorldBound = BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorldBound;
    if (!pWorldBound) {
        pWorldBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
    }
    NiGeometryBufferData* pbufferData = pgeometry->m_spModelData->m_pkBuffData;
    const NiSkinInstance* pskin = pgeometry->m_spSkinInstance.m_pObject;
    m_pkD3DRenderer->SetModelTransform(pgeometry->m_kWorld, false);
    // The geometry's shader property: its fade alpha source times its alpha.
    BSShaderBloodSplatterProperty* pprop = static_cast<BSShaderBloodSplatterProperty*>(
        apState->m_aspProps[3].m_pObject
    );
    float v9 = *pprop->pfFadeAlpha * pprop->fAlpha;
    BSShaderBloodSplatter::Constant::Alpha.x = v9 * BSShaderBloodSplatter::fGlobalAlpha;
    if (BSShaderManager::eCurrentPass == BSShaderManager::BSSM_BLOOD_SPLATTER_FLARE) {
        BSShaderBloodSplatter::Constant::Alpha.x = BSShaderBloodSplatter::fFlareMult *
            BSShaderBloodSplatter::fFlareAlpha * BSShaderBloodSplatter::Constant::Alpha.x;
    }
    NiD3DShaderConstantMap* m_pObject = m_spPixelConstantMap.m_pObject;
    if (m_pObject) {
        NiD3DPixelShader* v10 = NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject;
        if (v10) {
            m_pObject->SetShaderConstants(
                v10,
                pgeometry,
                pskin,
                nullptr,
                pbufferData,
                apState,
                nullptr,
                pgeometry->m_kWorld,
                *pWorldBound,
                0,
                true
            );
        }
    }
    NiD3DShaderConstantMap* v6 = m_spVertexConstantMap.m_pObject;
    if (v6) {
        NiD3DVertexShader* v11 = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
        if (v11) {
            v6->SetShaderConstants(
                v11,
                pgeometry,
                pskin,
                nullptr,
                pbufferData,
                apState,
                nullptr,
                pgeometry->m_kWorld,
                *pWorldBound,
                0,
                true
            );
        }
    }
}

// Pass 0x2F0 renders with the second pass and 0x2F1 with the first; any other pass leaves the current one.
void BSShaderBloodSplatter::InitPass(uint32_t auiCurrentPass) {
    if (auiCurrentPass == BSShaderManager::BSSM_BLOOD_SPLATTER_FLARE) {
        NiD3DShader::m_spCurrentPass = spPass[1].m_pObject;
    }
    else if (auiCurrentPass == BSShaderManager::BSSM_BLOOD_SPLATTER) {
        NiD3DShader::m_spCurrentPass = spPass[0].m_pObject;
    }
}

// Creates the pixel map (the global alpha) and the vertex map (the world-view-projection transform, the light location
// and the flare offset scale) when they are missing.
void BSShaderBloodSplatter::CreateConstantMaps() {
    if (!m_spPixelConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        m_spPixelConstantMap = pMap;
        m_spPixelConstantMap->AddEntry("alpha", 0x10000007, 0, 0, 1, "", 0x10, 4, &Constant::Alpha, false);
    }
    if (!m_spVertexConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        m_spVertexConstantMap = pMap;
        m_spVertexConstantMap->AddEntry("WorldViewProjTranspose", 0x20000009, 0, 0, 4, nullptr, 0, 0, nullptr, false);
        m_spVertexConstantMap->AddEntry("lightloc", 0x10000007, 0, 4, 1, "", 0x10, 4, &Constant::LightLoc, false);
        m_spVertexConstantMap->AddEntry("ctrl", 0x10000007, 0, 5, 1, "", 0x10, 4, &Constant::Ctrl, false);
    }
}

void BSShaderBloodSplatter::ReloadShaders() {
    LoadShaders();
    LoadStagesAndPasses();
}

void BSShaderBloodSplatter::ClearPassStages() {
    for (const NiPointer<NiD3DPass>& spEntry : spPass) {
        ClearPass(spEntry.m_pObject);
    }
}

// Each macro list is aggregate-initialised.
void BSShaderBloodSplatter::LoadShaders() {
    BSShaderMacro v14;
    BSShaderMacro pixelShaderParamsFlare;
    BSShaderMacro v16;
    BSShaderMacro pixelShaderParams;
    char apDest[260];
    char Str1[260];
    v16.strPath = "BloodSplatter\\BloodSplatter.v.hlsl";
    v16.kMacros[0].Name = nullptr;
    std::memset(&v16.kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition));
    pixelShaderParams.strPath = "BloodSplatter\\BloodSplatter.p.hlsl";
    pixelShaderParams.kMacros[0].Name = nullptr;
    std::memset(
        &pixelShaderParams.kMacros[0].Definition,
        0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    v14.strPath = "BloodSplatter\\BloodSplatter.v.hlsl";
    v14.kMacros[0].Name = "FLARE";
    v14.kMacros[0].Definition = "";
    v14.kMacros[1].Name = nullptr;
    std::memset(&v14.kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
    pixelShaderParamsFlare.strPath = "BloodSplatter\\BloodSplatter.p.hlsl";
    pixelShaderParamsFlare.kMacros[0].Name = "FLARE";
    pixelShaderParamsFlare.kMacros[0].Definition = "";
    pixelShaderParamsFlare.kMacros[1].Name = nullptr;
    std::memset(
        &pixelShaderParamsFlare.kMacros[1].Definition,
        0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    BSShader::MakePath("BloodSplatter\\BloodSplatter.v.hlsl", Str1);
    BSsprintf(apDest, 260, "BS.vso");
    NiD3DVertexShader* v2 = CompileVertexShaderHLSL(Str1, v16.kMacros, ShaderTarget_vs_1_1, apDest);
    spVertexShader[0] = v2;
    BSShader::MakePath("BloodSplatter\\BloodSplatter.v.hlsl", Str1);
    BSsprintf(apDest, 260, "BSF.vso");
    NiD3DVertexShader* v5 = CompileVertexShaderHLSL(Str1, v14.kMacros, ShaderTarget_vs_1_1, apDest);
    spVertexShader[1] = v5;
    BSShader::MakePath(pixelShaderParams.strPath, Str1);
    BSsprintf(apDest, 260, "BS.pso");
    NiD3DPixelShader* v8 = CompilePixelShaderHLSL(Str1, pixelShaderParams.kMacros, "ps_2_a", apDest);
    spPixelShader[0] = v8;
    BSShader::MakePath(pixelShaderParamsFlare.strPath, Str1);
    BSsprintf(apDest, 260, "BSF.pso");
    NiD3DPixelShader* v11 = CompilePixelShaderHLSL(Str1, pixelShaderParamsFlare.kMacros, "ps_2_a", apDest);
    spPixelShader[1] = v11;
}

bool BSShaderBloodSplatter::LoadStagesAndPasses() {
    NiPointer<NiD3DPass>* spPass = this->BSShaderBloodSplatter::spPass;
    NiD3DPass::CreateNewPass(*this->BSShaderBloodSplatter::spPass);
    for (uint32_t i = 0; i < 2; ++i) {
        NiPointer<NiD3DTextureStage> spTexStage;
        NiD3DTextureStage::CreateNewStage(spTexStage);
        BSShader::SetupDefaultStage(spTexStage.m_pObject, i, 3u, 2u, true);
        spTexStage->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
        spPass->m_pObject->SetStage(spPass->m_pObject->m_uiCurrentStage, spTexStage.m_pObject);
    }
    NiD3DPass* v5 = spPass->m_pObject;
    if (!spPass->m_pObject->m_pkRenderStateGroup) {
        v5->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v5->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
    NiD3DPass* v6 = spPass->m_pObject;
    if (!spPass->m_pObject->m_pkRenderStateGroup) {
        v6->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v6->m_pkRenderStateGroup->SetRenderState(7u, 0, true);
    NiD3DPass* v7 = spPass->m_pObject;
    if (!spPass->m_pObject->m_pkRenderStateGroup) {
        v7->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v7->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
    NiD3DPass* v8 = spPass->m_pObject;
    if (!spPass->m_pObject->m_pkRenderStateGroup) {
        v8->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v8->m_pkRenderStateGroup->SetRenderState(19u, 1u, true);
    if (!v8->m_pkRenderStateGroup) {
        v8->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v8->m_pkRenderStateGroup->SetRenderState(20u, 3u, true);
    NiD3DPass* v9 = spPass->m_pObject;
    NiD3DVertexShader* v10 = spVertexShader[0].m_pObject;
    NiD3DVertexShader* v11 = spPass->m_pObject->m_spVertexShader.m_pObject;
    if (v11 != v10) {
        if (v11) {
            NiDecRefCount(v11);
        }
        v9->m_spVertexShader.m_pObject = v10;
        if (v10) {
            NiIncRefCount(v10);
        }
    }
    NiD3DPass* v13 = spPass->m_pObject;
    NiD3DPixelShader* v14 = this->spPixelShader[0].m_pObject;
    v13->m_spPixelShader = v14;
    NiD3DPass** p_m_pObject = &this->BSShaderBloodSplatter::spPass[1].m_pObject;
    NiD3DPass::CreateNewPass(this->BSShaderBloodSplatter::spPass[1]);
    for (uint32_t j = 0; j < 2; ++j) {
        NiPointer<NiD3DTextureStage> aspD3DTextureStage;
        NiD3DTextureStage::CreateNewStage(aspD3DTextureStage);
        BSShader::SetupDefaultStage(aspD3DTextureStage.m_pObject, j, 3u, 2u, true);
        aspD3DTextureStage->SetFilterMode(NiTexturingProperty::FILTER_BILERP);
        (*p_m_pObject)->SetStage((*p_m_pObject)->m_uiCurrentStage, aspD3DTextureStage.m_pObject);
    }
    NiD3DPass* v19 = *p_m_pObject;
    if (!(*p_m_pObject)->m_pkRenderStateGroup) {
        v19->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v19->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
    NiD3DPass* v20 = *p_m_pObject;
    if (!(*p_m_pObject)->m_pkRenderStateGroup) {
        v20->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v20->m_pkRenderStateGroup->SetRenderState(7u, 0, true);
    NiD3DPass* v21 = *p_m_pObject;
    if (!(*p_m_pObject)->m_pkRenderStateGroup) {
        v21->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v21->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
    NiD3DPass* v22 = *p_m_pObject;
    if (!(*p_m_pObject)->m_pkRenderStateGroup) {
        v22->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v22->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
    if (!v22->m_pkRenderStateGroup) {
        v22->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v22->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
    NiD3DVertexShader* v23 = this->spVertexShader[1].m_pObject;
    (*p_m_pObject)->m_spVertexShader = v23;
    NiD3DPass* v26 = *p_m_pObject;
    NiD3DPixelShader* v27 = this->spPixelShader[1].m_pObject;
    v26->m_spPixelShader = v27;
    return true;
}

void BSShaderBloodSplatter::SetFlareMult(float afFlareMult) {
    BSShaderBloodSplatter::fFlareMult = afFlareMult;
}

void BSShaderBloodSplatter::SetGlobalAlpha(float afAlpha) {
    BSShaderBloodSplatter::fGlobalAlpha = afAlpha;
}

float BSShaderBloodSplatter::GetGlobalAlpha() {
    return BSShaderBloodSplatter::fGlobalAlpha;
}

void BSShaderBloodSplatter::SetFlareOffsetScale(float afScale) {
    BSShaderBloodSplatter::Constant::Ctrl.x = afScale;
}

void BSShaderBloodSplatter::ComputeScreenSpaceLightLoc(NiCamera* apCamera) {
    NiPoint3 worlddir;
    NiPoint3 loc;
    BSShaderBloodSplatter::fFlareAlpha = 0.0f;
    worlddir.x = apCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
    worlddir.y = apCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
    worlddir.z = apCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
    NiPoint3* pTranslate = &apCamera->m_kWorld.m_Translate;
    if (BSShaderManager::pShadowSceneNode[0]->GetAverageLightPos(apCamera->m_kWorld.m_Translate, worlddir, loc)) {
        float fx;
        float fy;
        float fz;
        NiPoint3 lightvect;
        worlddir.x = apCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
        worlddir.y = apCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
        worlddir.z = apCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
        lightvect.x = loc.x - pTranslate->x;
        lightvect.y = loc.y - pTranslate->y;
        lightvect.z = loc.z - pTranslate->z;
        worlddir.Unitize();
        lightvect.Unitize();
        BSShaderBloodSplatter::fFlareAlpha = lightvect.y * worlddir.y + lightvect.x * worlddir.x +
            lightvect.z * worlddir.z;
        if (apCamera->WorldPtToScreenPt3(loc, fx, fy, fz, 1e-05f)) {
            if (fx < 0.0f) {
                fx = 0.0f;
            }
            else if (fx > 1.0f) {
                fx = 1.0f;
            }
            if (fy < 0.0f) {
                fy = 0.0f;
            }
            else if (fy > 1.0f) {
                fy = 1.0f;
            }
            loc.x = fx * 2.0 - 1.0;
            BSShaderBloodSplatter::Constant::LightLoc.x = loc.x;
            loc.y = 2.0 * fy - 1.0;
            BSShaderBloodSplatter::Constant::LightLoc.y = loc.y;
            loc.z = fz;
            BSShaderBloodSplatter::Constant::LightLoc.z = fz;
            BSShaderBloodSplatter::Constant::LightLoc.w = 0.1f;
            return;
        }
        loc.x = 0.0f;
        loc.y = 0.0f;
        BSShaderBloodSplatter::Constant::LightLoc.x = 0.0f;
        BSShaderBloodSplatter::Constant::LightLoc.y = 0.0f;
        BSShaderBloodSplatter::Constant::LightLoc.z = 0.0f;
        BSShaderBloodSplatter::Constant::LightLoc.w = 0.1f;
    }
    else {
        BSShaderBloodSplatter::Constant::LightLoc.x = 0.0f;
        BSShaderBloodSplatter::Constant::LightLoc.y = 0.0f;
        BSShaderBloodSplatter::Constant::LightLoc.z = 0.0f;
        BSShaderBloodSplatter::Constant::LightLoc.w = 0.1f;
    }
}

BSShader* BSShaderBloodSplatter::CreateShader() {
    BSShaderBloodSplatter* v0 = nullptr;
    NiDX9ShaderDeclaration* v1 = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 2u, 1u);
    v1->SetEntry(0u, 0u, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0u);
    v1->SetEntry(1u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0u);
    v1->GetD3DDeclaration();
    v0 = new BSShaderBloodSplatter;
    v0->Initialize();
    v0->LoadShaders();
    v0->LoadStagesAndPasses();
    v0->CreateConstantMaps();
    v0->SetShaderDecl(v1);
    return v0;
}
