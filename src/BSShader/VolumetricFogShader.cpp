#include "BSShader/VolumetricFogShader.h"

#include "BSCore/BSString.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/ehvec.h"

NiPointer<BSRenderedTexture> VolumetricFogShader::pFogThicknessTexture;
bool VolumetricFogShader::bUseSinglePass = BSShaderManager::b30VFog;
NiRTTI VolumetricFogShader::ms_RTTI("VolumetricFogShader", &NiD3DDefaultShader::ms_RTTI);

VolumetricFogShader::VolumetricFogShader() : fVars(0.0f, 0.0f, 0.0f, 0.0f), fFogProps(0.0f, 0.0f, 0.0f, 0.0f) {
    iShaderType = 19;
}

// The passes and shaders release themselves.
VolumetricFogShader::~VolumetricFogShader() {}

const NiRTTI* VolumetricFogShader::GetRTTI() const {
    return &VolumetricFogShader::ms_RTTI;
}

bool VolumetricFogShader::Initialize() {
    bool v13 = BSShader::Initialize();
    NiPointer<NiD3DPass>* spPass = this->VolumetricFogShader::spPass;
    int32_t v3 = 3;
    do {
        NiD3DPass* m_pObject = spPass->m_pObject;
        if (spPass->m_pObject) {
            if (m_pObject->m_uiRefCount-- == 1) {
                m_pObject->ReturnPassToPool();
            }
            spPass->m_pObject = nullptr;
        }
        ++spPass;
        --v3;
    }
    while (v3);
    NiPointer<NiD3DVertexShader>* spVertexShader = this->spVertexShader;
    int32_t v7 = 2;
    do {
        *spVertexShader = nullptr;
        ++spVertexShader;
        --v7;
    }
    while (v7);
    NiPointer<NiD3DPixelShader>* spPixelShader = this->spPixelShader;
    int32_t v10 = 2;
    do {
        *spPixelShader = nullptr;
        ++spPixelShader;
        --v10;
    }
    while (v10);
    return v13;
}

// Creates the pixel map (fog properties) and the vertex map (the predefined transform and the vars) when they are
// missing. The variable names are empty strings; the entries do not compare the address.
void VolumetricFogShader::CreateConstantMaps() {
    if (!m_spPixelConstantMap.m_pObject) {
        void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
        NiD3DShaderConstantMap* pMap = nullptr;
        if (pMemory) {
            pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        }
        m_spPixelConstantMap = pMap;
        m_spPixelConstantMap.m_pObject->AddEntry("FogProps", 0x10000007, 0, 0, 1, "", 0x10, 4, &fFogProps, false);
    }
    if (!m_spVertexConstantMap.m_pObject) {
        void* pMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
        NiD3DShaderConstantMap* pMap = nullptr;
        if (pMemory) {
            pMap = ::new (pMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        }
        m_spVertexConstantMap = pMap;
        m_spVertexConstantMap.m_pObject->AddPredefinedEntry(
            "WorldViewProjTranspose", 0x20000009, 0, "", 0, 0, nullptr, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry("fvars0", 0x10000007, 0, 4, 1, "", 0x10, 4, &fVars, false);
    }
}

// Reloads the shader programs, then rebuilds the passes.
void VolumetricFogShader::ReloadShaders() {
    LoadShaders();
    LoadStagesAndPasses();
}

void VolumetricFogShader::ClearPassStages() {
    for (uint32_t i = 0; i < 3; ++i) {
        ClearPass(spPass[i].m_pObject);
    }
}

void VolumetricFogShader::LoadShaders() {
    LoadVertexShaders();
    LoadPixelShaders();
}

// The two macro sets are kept as one array and indexed by the shader model.
void VolumetricFogShader::LoadVertexShaders() {
    const char* pvtarget[2];
    BSShaderMacro akMacros[2];
    int32_t i = 0;
    pvtarget[0] = ShaderTarget_vs_2_0;
    pvtarget[1] = "vs_3_0";
    akMacros[0].strPath = "vfog\\VolumetricFog.v.hlsl";
    std::memset(akMacros[0].kMacros, 0, sizeof(akMacros[0].kMacros));
    akMacros[1].strPath = "vfog\\VolumetricFog.v.hlsl";
    std::memset(akMacros[1].kMacros, 0, sizeof(akMacros[1].kMacros));
    do {
        if (i != 1 || BSShaderManager::b30VFog) {
            char compiledShaderName[260];
            char Str1[260];
            BSShader::MakePath(akMacros[i].strPath, Str1);
            BSsprintf(compiledShaderName, 260, "VolumetricFog%03d.vso", i);
            NiD3DVertexShader* v3 = CompileVertexShaderHLSL(Str1, akMacros[i].kMacros, pvtarget[i], compiledShaderName);
            NiD3DVertexShader* m_pObject = spVertexShader[i].m_pObject;
            NiD3DVertexShader* v5 = v3;
            if (m_pObject != v3) {
                if (m_pObject) {
                    NiDecRefCount(m_pObject);
                }
                spVertexShader[i].m_pObject = v5;
                if (v5) {
                    NiIncRefCount(v5);
                }
            }
        }
        ++i;
    }
    while (i < 2);
}

void VolumetricFogShader::LoadPixelShaders() {
    const char* PSVersion[2];
    BSShaderMacro akMacros[2];
    int32_t i = 0;
    PSVersion[0] = "ps_2_a";
    PSVersion[1] = "ps_3_0";
    akMacros[0].strPath = "vfog\\VolumetricFog.p.hlsl";
    akMacros[0].kMacros[0].Name = nullptr;
    std::memset(
        &akMacros[0].kMacros[0].Definition, 0, sizeof(akMacros[0].kMacros) - sizeof(akMacros[0].kMacros[0].Name)
    );
    akMacros[1].strPath = "vfog\\VolumetricFog.p.hlsl";
    akMacros[1].kMacros[0].Name = "SINGLE_PASS";
    akMacros[1].kMacros[0].Definition = "";
    akMacros[1].kMacros[1].Name = nullptr;
    std::memset(
        &akMacros[1].kMacros[1].Definition, 0,
        sizeof(akMacros[1].kMacros) - sizeof(D3DXMACRO) - sizeof(akMacros[1].kMacros[1].Name)
    );
    do {
        if (i != 1 || BSShaderManager::b30VFog) {
            char apDest[260];
            char apPath[260];
            BSShader::MakePath(akMacros[i].strPath, apPath);
            BSsprintf(apDest, 260, "VolumetricFog%03d.pso", i);
            NiD3DPixelShader* v3 = CompilePixelShaderHLSL(apPath, akMacros[i].kMacros, PSVersion[i], apDest);
            NiD3DPixelShader* m_pObject = spPixelShader[i].m_pObject;
            NiD3DPixelShader* v5 = v3;
            if (m_pObject != v3) {
                if (m_pObject) {
                    NiDecRefCount(m_pObject);
                }
                spPixelShader[i].m_pObject = v5;
                if (v5) {
                    NiIncRefCount(v5);
                }
            }
        }
        ++i;
    }
    while (i < 2);
}

bool VolumetricFogShader::LoadStagesAndPasses() {
    VolumetricFogShader* v1 = this;
    int32_t v2 = 0;
    int32_t v31 = 0;
    do {
        if (v2 != 2 || BSShaderManager::b30VFog) {
            NiD3DPass** p_m_pObject = &v1->VolumetricFogShader::spPass[v2].m_pObject;
            if (!*p_m_pObject) {
                NiD3DPass::CreateNewPass(v1->VolumetricFogShader::spPass[v2]);
                NiPointer<NiD3DTextureStage> aspD3DTextureStage;
                NiD3DTextureStage::CreateNewStage(aspD3DTextureStage);
                BSShader::SetupDefaultStage(aspD3DTextureStage.m_pObject, 0, 3u, 2u, true);
                aspD3DTextureStage.m_pObject->SetFilterMode(NiTexturingProperty::FILTER_NEAREST);
                (*p_m_pObject)->SetStage((*p_m_pObject)->m_uiCurrentStage, aspD3DTextureStage.m_pObject);
            }
            if (v2 >= 0) {
                NiD3DVertexShader* v8;
                if (v2 <= 1) {
                    NiD3DPixelShader* v9 = v1->spPixelShader[0].m_pObject;
                    (*p_m_pObject)->m_spPixelShader = v9;
                    v8 = spVertexShader[0].m_pObject;
                }
                else {
                    NiD3DPixelShader* v5 = v1->spPixelShader[1].m_pObject;
                    (*p_m_pObject)->m_spPixelShader = v5;
                    v8 = spVertexShader[1].m_pObject;
                }
                (*p_m_pObject)->m_spVertexShader = v8;
                v1 = this;
                v2 = v31;
            }
            NiD3DPass* v14 = *p_m_pObject;
            if (!(*p_m_pObject)->m_pkRenderStateGroup) {
                v14->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v14->m_pkRenderStateGroup->SetRenderState(168u, 15u, true);
            NiD3DPass* v15 = *p_m_pObject;
            if (!(*p_m_pObject)->m_pkRenderStateGroup) {
                v15->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v15->m_pkRenderStateGroup->SetRenderState(7u, 0, true);
            NiD3DPass* v16 = *p_m_pObject;
            if (!(*p_m_pObject)->m_pkRenderStateGroup) {
                v16->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v16->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
            NiD3DPass* v17 = *p_m_pObject;
            if (!(*p_m_pObject)->m_pkRenderStateGroup) {
                v17->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v17->m_pkRenderStateGroup->SetRenderState(23u, 8u, true);
            NiD3DPass* v18 = *p_m_pObject;
            if (!(*p_m_pObject)->m_pkRenderStateGroup) {
                v18->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v18->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
            NiD3DPass* v19 = *p_m_pObject;
            if (!(*p_m_pObject)->m_pkRenderStateGroup) {
                v19->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v19->m_pkRenderStateGroup->SetRenderState(19u, 2u, true);
            if (!v19->m_pkRenderStateGroup) {
                v19->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v19->m_pkRenderStateGroup->SetRenderState(20u, 2u, true);
            if (v2 == 1) {
                NiD3DPass* v20 = v1->VolumetricFogShader::spPass[1].m_pObject;
                if (!v20->m_pkRenderStateGroup) {
                    v20->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                }
                v20->m_pkRenderStateGroup->SetRenderState(22u, 3u, true);
            }
            else {
                if (v2) {
                    if (v2 == 2) {
                        NiD3DPass* v22 = v1->VolumetricFogShader::spPass[2].m_pObject;
                        if (!v22->m_pkRenderStateGroup) {
                            v22->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                        }
                        v22->m_pkRenderStateGroup->SetRenderState(22u, 1u, true);
                    }
                }
                else {
                    NiD3DPass* v21 = v1->VolumetricFogShader::spPass[0].m_pObject;
                    if (!v21->m_pkRenderStateGroup) {
                        v21->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
                    }
                    v21->m_pkRenderStateGroup->SetRenderState(22u, 2u, true);
                }
            }
            NiD3DPass* v23 = *p_m_pObject;
            if (!(*p_m_pObject)->m_pkRenderStateGroup) {
                v23->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v23->m_pkRenderStateGroup->SetRenderState(52u, 1u, true);
            NiD3DPass* v24 = *p_m_pObject;
            if (!(*p_m_pObject)->m_pkRenderStateGroup) {
                v24->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v24->m_pkRenderStateGroup->SetRenderState(56u, 8u, true);
            NiD3DPass* v25 = *p_m_pObject;
            if (!(*p_m_pObject)->m_pkRenderStateGroup) {
                v25->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v25->m_pkRenderStateGroup->SetRenderState(57u, 1u, true);
            NiD3DPass* v26 = *p_m_pObject;
            if (!(*p_m_pObject)->m_pkRenderStateGroup) {
                v26->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v26->m_pkRenderStateGroup->SetRenderState(55u, 3u, true);
            NiD3DPass* v27 = *p_m_pObject;
            if (!v27->m_pkRenderStateGroup) {
                v27->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
            }
            v27->m_pkRenderStateGroup->SetRenderState(54u, 3u, true);
        }
        v31 = ++v2;
    }
    while (v2 < 3);
    return true;
}

BSShader* VolumetricFogShader::CreateShader() {
    VolumetricFogShader* v0 = nullptr;
    NiDX9ShaderDeclaration* v1 = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 2u, 1u);
    v1->SetEntry(0u, 0u, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0u);
    v1->GetD3DDeclaration();
    VolumetricFogShader* v2 = static_cast<VolumetricFogShader*>(NiMemObject::operator new(sizeof(VolumetricFogShader)));
    if (v2) {
        v0 = ::new (v2) VolumetricFogShader;
    }
    v0->Initialize();
    v0->LoadShaders();
    v0->LoadStagesAndPasses();
    v0->CreateConstantMaps();
    v0->SetShaderDecl(v1);
    return v0;
}
