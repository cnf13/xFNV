#include "BSShader/BeamShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStageGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/string.h"

NiRTTI BeamShader::ms_RTTI("BeamShader", &NiD3DDefaultShader::ms_RTTI);
NiPointer<NiTriShape> BeamShader::spBaseGeometry;

BeamShader::BeamShader(NiDX9ShaderDeclaration* apShaderDecl)
  : ePass(BSShaderManager::BSSM_BEAM), fTimer(0.0f), fVars{}, spBeamPass(nullptr), spBeamVertexShader(nullptr),
    spBeamPixelShader(nullptr) {
    iShaderType = 16;
    m_bUsesNiRenderState = true;
    m_spShaderDecl = apShaderDecl;
}

// The shared beam geometry is released with the shader; the members release their objects themselves.
BeamShader::~BeamShader() {
    if (spBaseGeometry) {
        NiDecRefCount(spBaseGeometry.m_pObject);
        spBaseGeometry.m_pObject = nullptr;
    }
}

const NiRTTI* BeamShader::GetRTTI() const {
    return &BeamShader::ms_RTTI;
}

bool BeamShader::Initialize() {
    return BSShader::Initialize();
}

void BeamShader::RecreateRendererData() {
    if (m_spShaderDecl.m_pObject) {
        spBeamVertexShader->RecreateRendererData();
        spBeamPixelShader->RecreateRendererData();
    }
    NiD3DShader::RecreateRendererData();
}

// Creates the vertex map (transform and vars) and an empty pixel map when they are missing. The vars' variable name is
// an empty string.
void BeamShader::CreateConstantMaps() {
    if (!m_spVertexConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        m_spVertexConstantMap = pMap;
        m_spVertexConstantMap->AddEntry(
            "WorldViewProjTranspose",
            0x20000009,
            0,
            PAS_VC_WORLDVIEWTRANSFORM,
            4,
            nullptr,
            0,
            0,
            nullptr,
            false
        );
        m_spVertexConstantMap->AddEntry("fVars", 0x10000009, 0, PAS_VC_VARS, 4, "", 0x40, 4, fVars, false);
    }
    if (!m_spPixelConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        m_spPixelConstantMap = pMap;
    }
}

void BeamShader::ReloadShaders() {
    LoadShaders();
    LoadStagesAndPasses();
}

[[clang::disable_tail_calls]] void BeamShader::ClearPassStages() {
    ClearPass(spBeamPass.m_pObject);
}

void BeamShader::SetCurrentPass(uint32_t aPass, bool aFirstPass) {
    ePass = static_cast<BSShaderManager::eBSSMPassEnum>(aPass);
    bFirstPass = aFirstPass;
}

// Each macro list is initialised with its first macro, then the other eight cleared by memset.
void BeamShader::LoadShaders() {
    BSShaderMacro kPixelMacros;
    BSShaderMacro kVertexMacros;
    char PSOFile[260];
    char Str1[260];
    kVertexMacros.strPath = "beam\\v\\beam.v.hlsl";
    kVertexMacros.kMacros[0].Name = nullptr;
    kVertexMacros.kMacros[0].Definition = nullptr;
    std::memset(&kVertexMacros.kMacros[1], 0, sizeof(kVertexMacros.kMacros) - sizeof(D3DXMACRO));
    BSShader::MakePath(kVertexMacros.strPath, Str1);
    BSsprintf(PSOFile, 260, "BEAM.vso");
    NiD3DVertexShader*
        pVertexShader = CompileVertexShaderHLSL(Str1, kVertexMacros.kMacros, ShaderTarget_vs_1_1, PSOFile);
    spBeamVertexShader = pVertexShader;
    const char* ptarget = BSShaderManager::GetPSTarget(true);
    kPixelMacros.strPath = "beam\\p\\beam.p.hlsl";
    kPixelMacros.kMacros[0].Name = nullptr;
    kPixelMacros.kMacros[0].Definition = nullptr;
    std::memset(&kPixelMacros.kMacros[1], 0, sizeof(kPixelMacros.kMacros) - sizeof(D3DXMACRO));
    BSShader::MakePath(kPixelMacros.strPath, Str1);
    BSsprintf(PSOFile, 260, "BEAM.pso");
    NiD3DPixelShader* pPixelShader = CompilePixelShaderHLSL(Str1, kPixelMacros.kMacros, ptarget, PSOFile);
    spBeamPixelShader = pPixelShader;
}

// Creates the beam pass on first use, then sets up its stages.
bool BeamShader::LoadStagesAndPasses() {
    if (!spBeamPass.m_pObject) {
        NiD3DPass::CreateNewPass(spBeamPass);
    }
    PresetStages();
    return true;
}

void BeamShader::PresetStages() {
    NiD3DPass* v2 = nullptr;
    {
        NiPointer<NiD3DTextureStage> spStage;
        NiD3DPass* m_pObject = spBeamPass.m_pObject;
        if (m_pObject) {
            v2 = m_pObject;
            NiIncRefCount(m_pObject);
        }
        if (!v2->m_uiStageCount) {
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 0, 1u, 2u, true);
            spStage->m_pkTextureStageGroup->SetSamplerState(7u, 0, false, false);
            v2->SetStage(v2->m_uiCurrentStage, spStage.m_pObject);
        }
        NiD3DVertexShader* v4 = spBeamVertexShader.m_pObject;
        v2->m_spVertexShader = v4;
        NiD3DPixelShader* v6 = spBeamPixelShader.m_pObject;
        v2->m_spPixelShader = v6;
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(22u, 1u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(168u, 15u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(52u, 0, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(7u, 1u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(23u, 4u, true);
        if (!v2->m_pkRenderStateGroup) {
            v2->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v2->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
    }
    v2->DecRefCount();
}

void BeamShader::Free() {
    NiTriShape* v0;
    if (spBaseGeometry) {
        v0 = spBaseGeometry;
        if (!InterlockedDecrementUInt32(&spBaseGeometry->m_uiRefCount)) {
            v0->DeleteThis();
        }
        spBaseGeometry.m_pObject = nullptr;
    }
}

BSShader* BeamShader::CreateShader() {
    BeamShader* pshader = nullptr;
    NiDX9ShaderDeclaration* spShaderDecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 5u, 1u);
    spShaderDecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    spShaderDecl->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    spShaderDecl->SetEntry(2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
    spShaderDecl
        ->SetEntry(3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0);
    spShaderDecl->GetD3DDeclaration();
    pshader = new BeamShader(spShaderDecl);
    pshader->Initialize();
    pshader->LoadShaders();
    pshader->LoadStagesAndPasses();
    pshader->CreateConstantMaps();
    pshader->SetShaderDecl(spShaderDecl);
    return pshader;
}
