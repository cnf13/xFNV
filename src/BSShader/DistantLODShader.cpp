#include "BSShader/DistantLODShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/DistantLODGroup.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/ehvec.h"

NiRTTI DistantLODShader::ms_RTTI("DistantLODShader", &NiD3DDefaultShader::ms_RTTI);

// The instance data array holds 80 instances below shader level 2, else 228.
DistantLODShader::DistantLODShader(NiDX9ShaderDeclaration* apShaderDecl)
  : spPass(nullptr), fWindTimer(0.0f), pDistantLODInstanceDataEntry(nullptr),
    iInstanceDataCount(BSShaderManager::eShaderLevel < 2 ? 80 : 228), pfAmbientColor(NiColorA_BLACK),
    pfDiffuseDir{0.0f, 0.0f, 0.0f, 0.0f}, pfDiffuseColor(0.0f, 0.0f, 0.0f, 0.0f),
    pfAlphaTestRef(0.0f, 0.0f, 0.0f, 0.0f), pEyeVector{0.0f, 0.0f, 0.0f, 0.0f}, pEyePosition{0.0f, 0.0f, 0.0f, 0.0f} {
    iShaderType = 7;
    m_spShaderDecl = apShaderDecl;
    pDistantLODInstanceData = new DistantLODGroup::DistantLODInstanceData[iInstanceDataCount];
}

// The instance data array is freed; the shader pointers release their objects themselves.
DistantLODShader::~DistantLODShader() {
    pDistantLODInstanceDataEntry = nullptr;
    delete[] pDistantLODInstanceData;
}

const NiRTTI* DistantLODShader::GetRTTI() const {
    return &DistantLODShader::ms_RTTI;
}

void DistantLODShader::RecreateRendererData() {
    if (m_spShaderDecl.m_pObject) {
        NiPointer<NiD3DVertexShader>* spVertexShaders = this->spVertexShaders;
        int32_t v3 = 4;
        do {
            if (spVertexShaders->m_pObject) {
                spVertexShaders->m_pObject->RecreateRendererData();
            }
            ++spVertexShaders;
            --v3;
        }
        while (v3);
    }
    NiPointer<NiD3DPixelShader>* spPixelShaders = this->spPixelShaders;
    int32_t v5 = 2;
    do {
        if (spPixelShaders->m_pObject) {
            spPixelShaders->m_pObject->RecreateRendererData();
        }
        ++spPixelShaders;
        --v5;
    }
    while (v5);
    NiD3DShader::RecreateRendererData();
}

// Each entry's variable name is an empty string.
void DistantLODShader::CreateConstantMaps() {
    if (!m_spVertexConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        m_spVertexConstantMap = pMap;
        m_spVertexConstantMap.m_pObject->AddEntry(
            "WorldViewProjTranspose", 0x20000009, 0, 0, 4, nullptr, 0, 0, nullptr, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "Diffuse Light direction", 0x10000007, 0, 4, 1, "", 16, 4, &pfDiffuseDir, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "Diffuse Light color", 0x10000007, 0, 5, 1, "", 16, 4, &pfDiffuseColor, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "ambient color", 0x10000007, 0, 13, 1, "", 16, 4, &pfAmbientColor, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry("eye vector", 0x10000007, 0, 14, 1, "", 16, 4, &pEyeVector, false);
        m_spVertexConstantMap.m_pObject->AddEntry(
            "eye right vector", 0x10000007, 0, 9, 1, "", 16, 4, &pEyePosition, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "alpha param param", 0x10000007, 0, 12, 1, "", 16, 4, &TESTerrainLODManager_kShaderParams, false
        );
        // The fog parameters and the fog colour are registers 28 and 29 of the lighting shaders' shared constants.
        m_spVertexConstantMap.m_pObject->AddEntry(
            "fogparam", 0x10000007, 0, 11, 1, "", 16, 4, &BSShaderLightingProperty::pShaderLightConstants[28], false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "fogcolor", 0x10000007, 0, 10, 1, "", 16, 4, &BSShaderLightingProperty::pShaderLightConstants[29], false
        );
        uint32_t uiCount = iInstanceDataCount;
        m_spVertexConstantMap.m_pObject->AddEntry(
            "instance data", 0x10000009, 0, 20, uiCount, "", uiCount << 4, 4, pDistantLODInstanceData, false
        );
        pDistantLODInstanceDataEntry = m_spVertexConstantMap.m_pObject->GetEntry("instance data");
    }
    if (!m_spPixelConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        m_spPixelConstantMap = pMap;
        m_spPixelConstantMap.m_pObject->AddEntry(
            "alpha test ref", 0x10000007, 0, 3, 1, "", 16, 4, &pfAlphaTestRef, false
        );
    }
}

void DistantLODShader::ReloadShaders() {
    LoadShaders();
}

[[clang::disable_tail_calls]] void DistantLODShader::ClearPassStages() {
    ClearPass(spPass.m_pObject);
}

void DistantLODShader::SetCurrentPass(uint32_t aPass, bool aFirstPass) {
    ePass = static_cast<BSShaderManager::eBSSMPassEnum>(aPass);
    bFirstPass = (bool)((int8_t)aFirstPass);
}

// Each macro list holds its first macros, then the rest cleared by memset. Shader level 1 builds vertex shaders 0-1 and
// pixel shader 0, the other levels vertex shaders 2-3 and pixel shader 1.
void DistantLODShader::LoadShaders() {
    BSShaderMacro kPixelMacros[2];
    BSShaderMacro kVertexMacros[4];
    char Str1[260];
    char apDest[260];
    kVertexMacros[0].strPath = "tallgrass\\1x\\v\\DistantLOD.v.hlsl";
    kVertexMacros[0].kMacros[0].Name = nullptr;
    std::memset(
        &kVertexMacros[0].kMacros[0].Definition, 0,
        sizeof(kVertexMacros[0].kMacros) - sizeof(kVertexMacros[0].kMacros[0].Name)
    );
    kVertexMacros[1].strPath = "tallgrass\\1x\\v\\DistantLOD.v.hlsl";
    kVertexMacros[1].kMacros[0].Name = "BILLBOARD";
    kVertexMacros[1].kMacros[0].Definition = nullptr;
    kVertexMacros[1].kMacros[1].Name = nullptr;
    std::memset(
        &kVertexMacros[1].kMacros[1].Definition, 0,
        sizeof(kVertexMacros[1].kMacros) - sizeof(D3DXMACRO) - sizeof(kVertexMacros[1].kMacros[1].Name)
    );
    kVertexMacros[2].strPath = "tallgrass\\1x\\v\\DistantLOD.v.hlsl";
    kVertexMacros[2].kMacros[0].Name = "VS_2_0";
    kVertexMacros[2].kMacros[0].Definition = nullptr;
    kVertexMacros[2].kMacros[1].Name = nullptr;
    std::memset(
        &kVertexMacros[2].kMacros[1].Definition, 0,
        sizeof(kVertexMacros[2].kMacros) - sizeof(D3DXMACRO) - sizeof(kVertexMacros[2].kMacros[1].Name)
    );
    kVertexMacros[3].strPath = "tallgrass\\1x\\v\\DistantLOD.v.hlsl";
    kVertexMacros[3].kMacros[0].Name = "VS_2_0";
    kVertexMacros[3].kMacros[0].Definition = nullptr;
    kVertexMacros[3].kMacros[1].Name = "BILLBOARD";
    kVertexMacros[3].kMacros[1].Definition = nullptr;
    kVertexMacros[3].kMacros[2].Name = nullptr;
    std::memset(
        &kVertexMacros[3].kMacros[2].Definition, 0,
        sizeof(kVertexMacros[3].kMacros) - 2 * sizeof(D3DXMACRO) - sizeof(kVertexMacros[3].kMacros[2].Name)
    );
    kPixelMacros[0].strPath = "tallgrass\\1x\\p\\highDetail.p.hlsl";
    kPixelMacros[0].kMacros[0].Name = nullptr;
    kPixelMacros[0].kMacros[0].Definition = nullptr;
    std::memset(&kPixelMacros[0].kMacros[1], 0, sizeof(kPixelMacros[0].kMacros) - sizeof(D3DXMACRO));
    kPixelMacros[1].strPath = "tallgrass\\1x\\p\\highDetail.p.hlsl";
    kPixelMacros[1].kMacros[0].Name = "PS_2_0";
    kPixelMacros[1].kMacros[0].Definition = nullptr;
    kPixelMacros[1].kMacros[1].Name = nullptr;
    kPixelMacros[1].kMacros[1].Definition = nullptr;
    std::memset(&kPixelMacros[1].kMacros[2], 0, sizeof(kPixelMacros[1].kMacros) - 2 * sizeof(D3DXMACRO));
    const char* ptarget = BSShaderManager::GetPSTarget(true);
    if (BSShaderManager::eShaderLevel == 1) {
        for (int32_t i = 0; i < 2; ++i) {
            BSShader::MakePath(kVertexMacros[i].strPath, Str1);
            BSsprintf(apDest, 260, "DISTLOD1%03i.vso", i);
            spVertexShaders[i] = CompileVertexShaderHLSL(Str1, kVertexMacros[i].kMacros, ShaderTarget_vs_1_1, apDest);
        }
        BSShader::MakePath(kPixelMacros[0].strPath, Str1);
        BSsprintf(apDest, 260, "DISTLOD1%03i.pso", 0);
        spPixelShaders[0] = CompilePixelShaderHLSL(Str1, kPixelMacros[0].kMacros, ptarget, apDest);
    }
    else {
        for (int32_t i = 2; i < 4; ++i) {
            BSShader::MakePath(kVertexMacros[i].strPath, Str1);
            BSsprintf(apDest, 260, "DISTLOD2%03i.vso", i);
            spVertexShaders[i] = CompileVertexShaderHLSL(Str1, kVertexMacros[i].kMacros, ShaderTarget_vs_2_0, apDest);
        }
        BSShader::MakePath(kPixelMacros[1].strPath, Str1);
        BSsprintf(apDest, 260, "DISTLOD2%03i.pso", 1);
        spPixelShaders[1] = CompilePixelShaderHLSL(Str1, kPixelMacros[1].kMacros, ptarget, apDest);
    }
}

bool DistantLODShader::LoadStagesAndPasses() {
    NiD3DPass::CreateNewPass(spPass);
    PresetStages();
    return true;
}

void DistantLODShader::PresetStages() {
    NiPointer<NiD3DTextureStage> a1;
    NiD3DTextureStage::CreateNewStage(a1);
    BSShader::SetupDefaultStage(a1.m_pObject, 0, 3u, 2u, true);
    this->DistantLODShader::spPass.m_pObject->SetStage(
        this->DistantLODShader::spPass.m_pObject->m_uiCurrentStage, a1.m_pObject
    );
    NiD3DPass* m_pObject = this->DistantLODShader::spPass.m_pObject;
    NiD3DVertexShader* v3 = spVertexShaders[0].m_pObject;
    m_pObject->m_spVertexShader = v3;
    NiD3DPass* v5 = this->DistantLODShader::spPass.m_pObject;
    NiD3DPixelShader* v6 = spPixelShaders[0].m_pObject;
    v5->m_spPixelShader = v6;
    if (BSShaderManager::eShaderLevel >= 2) {
        NiD3DPass* v8 = this->DistantLODShader::spPass.m_pObject;
        if (!v8->m_pkRenderStateGroup) {
            v8->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v8->m_pkRenderStateGroup->SetRenderState(24u, 10u, true);
    }
    NiD3DPass* v9 = this->DistantLODShader::spPass.m_pObject;
    if (!v9->m_pkRenderStateGroup) {
        v9->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v9->m_pkRenderStateGroup->SetRenderState(168u, 7u, true);
    NiD3DPass* v10 = this->DistantLODShader::spPass.m_pObject;
    if (!v10->m_pkRenderStateGroup) {
        v10->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v10->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
    NiD3DPass* v11 = this->DistantLODShader::spPass.m_pObject;
    if (!v11->m_pkRenderStateGroup) {
        v11->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v11->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
    NiD3DPass* v12 = this->DistantLODShader::spPass.m_pObject;
    if (!v12->m_pkRenderStateGroup) {
        v12->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v12->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
    NiD3DPass* v13 = this->DistantLODShader::spPass.m_pObject;
    if (!v13->m_pkRenderStateGroup) {
        v13->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v13->m_pkRenderStateGroup->SetRenderState(14u, 1u, true);
}

BSShader* DistantLODShader::CreateShader() {
    DistantLODShader* pshader = nullptr;
    NiDX9ShaderDeclaration* spShaderDecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 4u, 1u);
    spShaderDecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    spShaderDecl->SetEntry(1u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
    spShaderDecl->SetEntry(
        2u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    spShaderDecl->SetEntry(
        0, 3u, 6u, NiShaderDeclaration::SHADERPARAM_EXTRA_DATA_MASK, NiShaderDeclaration::SPTYPE_FLOAT1,
        NiShaderDeclaration::SPUSAGE_TEXCOORD, 1u, NiD3DShaderDeclaration::SPTESS_DEFAULT
    );
    spShaderDecl->GetD3DDeclaration();
    pshader = new DistantLODShader(spShaderDecl);
    pshader->Initialize();
    pshader->LoadShaders();
    pshader->CreateConstantMaps();
    pshader->LoadStagesAndPasses();
    pshader->SetShaderDecl(spShaderDecl);
    return pshader;
}
