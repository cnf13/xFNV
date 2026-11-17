#include "BSShader/HairShader.h"

#include "BSCore/BSString.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/ehvec.h"

NiRTTI HairShader::ms_RTTI("HairShader", &ShadowLightShader::ms_RTTI);

// The hair shader draws with the skinned declaration.
HairShader::HairShader(NiDX9ShaderDeclaration* apShaderDecl, NiDX9ShaderDeclaration* apSkinnedShaderDecl)
  : ShadowLightShader(apShaderDecl, apSkinnedShaderDecl, nullptr, nullptr), spShaderDecl(apShaderDecl),
    spSkinnedShaderDecl(apSkinnedShaderDecl) {
    iShaderType = 2;
    m_spShaderDecl = apSkinnedShaderDecl;
    ePass = BSShaderManager::BSSM_EMPTY;
}

// The smart pointer members release their objects themselves.
HairShader::~HairShader() {
    ePass = BSShaderManager::BSSM_EMPTY;
}

const NiRTTI* HairShader::GetRTTI() const {
    return &HairShader::ms_RTTI;
}

bool HairShader::Initialize() {
    BSShader::Initialize();
    NiPointer<NiD3DVertexShader>* spHairVertexShader = this->spHairVertexShader;
    int32_t v3 = 7;
    do {
        *spHairVertexShader = nullptr;
        ++spHairVertexShader;
        --v3;
    }
    while (v3);
    NiPointer<NiD3DVertexShader>* spHairVertexShader2 = this->spHairVertexShader2;
    int32_t v6 = 7;
    do {
        *spHairVertexShader2 = nullptr;
        ++spHairVertexShader2;
        --v6;
    }
    while (v6);
    NiPointer<NiD3DPixelShader>* spHairPixelShader = this->spHairPixelShader;
    int32_t v9 = 3;
    do {
        *spHairPixelShader = nullptr;
        ++spHairPixelShader;
        --v9;
    }
    while (v9);
    NiPointer<NiD3DPixelShader>* spHairPixelShader2 = this->spHairPixelShader2;
    int32_t v12 = 3;
    do {
        *spHairPixelShader2 = nullptr;
        ++spHairPixelShader2;
        --v12;
    }
    while (v12);
    int32_t v14 = 2;
    NiPointer<NiD3DPass>* spHairPass = this->spHairPass;
    do {
        NiD3DPass* v16 = spHairPass->m_pObject;
        if (spHairPass->m_pObject) {
            if (v16->m_uiRefCount-- == 1) {
                v16->ReturnPassToPool();
            }
            spHairPass->m_pObject = nullptr;
        }
        ++spHairPass;
        --v14;
    }
    while (v14);
    return true;
}

void HairShader::RecreateRendererData() {
    if (m_spShaderDecl.m_pObject) {
        NiPointer<NiD3DVertexShader>* spHairVertexShader = this->spHairVertexShader;
        int32_t v3 = 7;
        do {
            if (spHairVertexShader->m_pObject) {
                spHairVertexShader->m_pObject->RecreateRendererData();
            }
            ++spHairVertexShader;
            --v3;
        }
        while (v3);
        NiPointer<NiD3DVertexShader>* spHairVertexShader2 = this->spHairVertexShader2;
        int32_t v5 = 7;
        do {
            if (spHairVertexShader2->m_pObject) {
                spHairVertexShader2->m_pObject->RecreateRendererData();
            }
            ++spHairVertexShader2;
            --v5;
        }
        while (v5);
    }
    NiPointer<NiD3DPixelShader>* spHairPixelShader2 = this->spHairPixelShader2;
    int32_t v7 = 3;
    do {
        NiD3DPixelShader* m_pObject = spHairPixelShader2[-10].m_pObject;
        if (m_pObject) {
            m_pObject->RecreateRendererData();
        }
        if (spHairPixelShader2->m_pObject) {
            spHairPixelShader2->m_pObject->RecreateRendererData();
        }
        ++spHairPixelShader2;
        --v7;
    }
    while (v7);
    ShadowLightShader::RecreateRendererData();
}

void HairShader::CreateConstantMaps() {
    ShadowLightShader* pShader = static_cast<ShadowLightShader*>(BSShaderManager::GetShader(1));
    NiD3DShaderConstantMap* pPixelConstantMap = pShader->m_spPixelConstantMap.m_pObject;
    if (pPixelConstantMap) {
        NiIncRefCount(pPixelConstantMap);
    }
    NiD3DShaderConstantMap* pVertexConstantMap = pShader->m_spVertexConstantMap.m_pObject;
    if (pVertexConstantMap) {
        NiIncRefCount(pVertexConstantMap);
    }
    m_spVertexConstantMap = pVertexConstantMap;
    m_spPixelConstantMap = pPixelConstantMap;
    spTexEffectPConstantMap = pShader->spTexEffectPConstantMap.m_pObject;
    spTexEffectVConstantMap = pShader->spTexEffectVConstantMap.m_pObject;
    spDefaultPixelConstantMap = pShader->spDefaultPixelConstantMap.m_pObject;
    spDefaultVertexConstantMap = pShader->spDefaultVertexConstantMap.m_pObject;
    if (pVertexConstantMap) {
        NiDecRefCount(pVertexConstantMap);
    }
    if (pPixelConstantMap) {
        NiDecRefCount(pPixelConstantMap);
    }
}

void HairShader::ReloadShaders() {
    LoadShaders();
}

// Clears the first two entries of ShadowLightShader's pass table, not spHairPass, as the game does.
void HairShader::ClearPassStages() {
    for (uint32_t i = 0; i < 2; ++i) {
        ClearPass(ShadowLightShader::spPass[i].m_pObject);
    }
}

// Each table entry is a shader file and its macro list; the rest of each entry is zeroed, from the first unused macro's
// Definition to the entry's end.
void HairShader::LoadVertexShaders() {
    BSShaderMacro akVertexShaders2x[7];
    BSShaderMacro akVertexShaders[7];
    char acCompiledShaderName[260];
    char acShaderPath[260];
    akVertexShaders[0].strPath = "hair\\1x\\hair.v.hlsl";
    akVertexShaders[0].kMacros[0].Name = "DIRP";
    akVertexShaders[0].kMacros[0].Definition = "";
    akVertexShaders[0].kMacros[1].Name = nullptr;
    std::memset(
        &akVertexShaders[0].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    akVertexShaders[1].strPath = "hair\\1x\\hair.v.hlsl";
    akVertexShaders[1].kMacros[0].Name = "DIRP";
    akVertexShaders[1].kMacros[0].Definition = "";
    akVertexShaders[1].kMacros[1].Name = "PTS";
    akVertexShaders[1].kMacros[1].Definition = "";
    akVertexShaders[1].kMacros[2].Name = nullptr;
    std::memset(
        &akVertexShaders[1].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    akVertexShaders[2].strPath = "hair\\1x\\hair.v.hlsl";
    akVertexShaders[2].kMacros[0].Name = "PTP";
    akVertexShaders[2].kMacros[0].Definition = "";
    akVertexShaders[2].kMacros[1].Name = "DIRS";
    akVertexShaders[2].kMacros[1].Definition = "";
    akVertexShaders[2].kMacros[2].Name = nullptr;
    std::memset(
        &akVertexShaders[2].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    akVertexShaders[3].strPath = "hair\\1x\\hair.v.hlsl";
    akVertexShaders[3].kMacros[0].Name = "DIRP";
    akVertexShaders[3].kMacros[0].Definition = "";
    akVertexShaders[3].kMacros[1].Name = "PTS";
    akVertexShaders[3].kMacros[1].Definition = "";
    akVertexShaders[3].kMacros[2].Name = "PTT";
    akVertexShaders[3].kMacros[2].Definition = "";
    akVertexShaders[3].kMacros[3].Name = nullptr;
    std::memset(
        &akVertexShaders[3].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    akVertexShaders[4].strPath = "hair\\1x\\hair.v.hlsl";
    akVertexShaders[4].kMacros[0].Name = "PTP";
    akVertexShaders[4].kMacros[0].Definition = "";
    akVertexShaders[4].kMacros[1].Name = "DIRS";
    akVertexShaders[4].kMacros[1].Definition = "";
    akVertexShaders[4].kMacros[2].Name = "PTT";
    akVertexShaders[4].kMacros[2].Definition = "";
    akVertexShaders[4].kMacros[3].Name = nullptr;
    std::memset(
        &akVertexShaders[4].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    akVertexShaders[5].strPath = "hair\\1x\\hair.v.hlsl";
    akVertexShaders[5].kMacros[0].Name = "PTP";
    akVertexShaders[5].kMacros[0].Definition = "";
    akVertexShaders[5].kMacros[1].Name = "PTS";
    akVertexShaders[5].kMacros[1].Definition = "";
    akVertexShaders[5].kMacros[2].Name = "PTT";
    akVertexShaders[5].kMacros[2].Definition = "";
    akVertexShaders[5].kMacros[3].Name = nullptr;
    std::memset(
        &akVertexShaders[5].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    akVertexShaders[6].strPath = "hair\\1x\\hair.v.hlsl";
    akVertexShaders[6].kMacros[0].Name = "PTP";
    akVertexShaders[6].kMacros[0].Definition = "";
    akVertexShaders[6].kMacros[1].Name = "PTS";
    akVertexShaders[6].kMacros[1].Definition = "";
    akVertexShaders[6].kMacros[2].Name = nullptr;
    std::memset(
        &akVertexShaders[6].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    akVertexShaders2x[0].strPath = "hair\\2x\\hair.v.hlsl";
    akVertexShaders2x[0].kMacros[0].Name = "DIRP";
    akVertexShaders2x[0].kMacros[0].Definition = "";
    akVertexShaders2x[0].kMacros[1].Name = nullptr;
    std::memset(
        &akVertexShaders2x[0].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    akVertexShaders2x[1].strPath = "hair\\2x\\hair.v.hlsl";
    akVertexShaders2x[1].kMacros[0].Name = "DIRP";
    akVertexShaders2x[1].kMacros[0].Definition = "";
    akVertexShaders2x[1].kMacros[1].Name = "PTS";
    akVertexShaders2x[1].kMacros[1].Definition = "";
    akVertexShaders2x[1].kMacros[2].Name = nullptr;
    std::memset(
        &akVertexShaders2x[1].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    akVertexShaders2x[2].strPath = "hair\\2x\\hair.v.hlsl";
    akVertexShaders2x[2].kMacros[0].Name = "PTP";
    akVertexShaders2x[2].kMacros[0].Definition = "";
    akVertexShaders2x[2].kMacros[1].Name = "DIRS";
    akVertexShaders2x[2].kMacros[1].Definition = "";
    akVertexShaders2x[2].kMacros[2].Name = nullptr;
    std::memset(
        &akVertexShaders2x[2].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    akVertexShaders2x[3].strPath = "hair\\2x\\hair.v.hlsl";
    akVertexShaders2x[3].kMacros[0].Name = "DIRP";
    akVertexShaders2x[3].kMacros[0].Definition = "";
    akVertexShaders2x[3].kMacros[1].Name = "PTS";
    akVertexShaders2x[3].kMacros[1].Definition = "";
    akVertexShaders2x[3].kMacros[2].Name = "PTT";
    akVertexShaders2x[3].kMacros[2].Definition = "";
    akVertexShaders2x[3].kMacros[3].Name = nullptr;
    std::memset(
        &akVertexShaders2x[3].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    akVertexShaders2x[4].strPath = "hair\\2x\\hair.v.hlsl";
    akVertexShaders2x[4].kMacros[0].Name = "PTP";
    akVertexShaders2x[4].kMacros[0].Definition = "";
    akVertexShaders2x[4].kMacros[1].Name = "DIRS";
    akVertexShaders2x[4].kMacros[1].Definition = "";
    akVertexShaders2x[4].kMacros[2].Name = "PTT";
    akVertexShaders2x[4].kMacros[2].Definition = "";
    akVertexShaders2x[4].kMacros[3].Name = nullptr;
    std::memset(
        &akVertexShaders2x[4].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    akVertexShaders2x[5].strPath = "hair\\2x\\hair.v.hlsl";
    akVertexShaders2x[5].kMacros[0].Name = "PTP";
    akVertexShaders2x[5].kMacros[0].Definition = "";
    akVertexShaders2x[5].kMacros[1].Name = "PTS";
    akVertexShaders2x[5].kMacros[1].Definition = "";
    akVertexShaders2x[5].kMacros[2].Name = "PTT";
    akVertexShaders2x[5].kMacros[2].Definition = "";
    akVertexShaders2x[5].kMacros[3].Name = nullptr;
    std::memset(
        &akVertexShaders2x[5].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    akVertexShaders2x[6].strPath = "hair\\2x\\hair.v.hlsl";
    akVertexShaders2x[6].kMacros[0].Name = "PTP";
    akVertexShaders2x[6].kMacros[0].Definition = "";
    akVertexShaders2x[6].kMacros[1].Name = "PTS";
    akVertexShaders2x[6].kMacros[1].Definition = "";
    akVertexShaders2x[6].kMacros[2].Name = nullptr;
    std::memset(
        &akVertexShaders2x[6].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    for (int32_t i = 0; i < 7; ++i) {
        if (akVertexShaders[i].strPath) {
            BSShader::MakePath(akVertexShaders[i].strPath, acShaderPath);
            BSsprintf(acCompiledShaderName, 260, "HAIR1%03i.vso", i);
            spHairVertexShader[i] = CompileVertexShaderHLSL(
                acShaderPath, akVertexShaders[i].kMacros, ShaderTarget_vs_1_1, acCompiledShaderName
            );
        }
    }
    if (BSShaderManager::eShaderLevel >= 2) {
        for (int32_t i = 0; i < 7; ++i) {
            BSShader::MakePath(akVertexShaders2x[i].strPath, acShaderPath);
            BSsprintf(acCompiledShaderName, 260, "HAIR2%03i.vso", i);
            spHairVertexShader2[i] = CompileVertexShaderHLSL(
                acShaderPath, akVertexShaders2x[i].kMacros, ShaderTarget_vs_2_0, acCompiledShaderName
            );
        }
    }
}

bool HairShader::LoadStagesAndPasses() {
    for (uint32_t i = 0; i < 2; ++i) {
        NiD3DPass::CreateNewPass(spHairPass[i]);
        NiPointer<NiD3DTextureStage> spStage;
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 0, NiTexturingProperty::CLAMP_S_WRAP_T, D3DTEXF_LINEAR, true);
        spHairPass[i].m_pObject->SetStage(spHairPass[i].m_pObject->m_uiCurrentStage, spStage.m_pObject);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 1, NiTexturingProperty::CLAMP_S_WRAP_T, D3DTEXF_LINEAR, true);
        spHairPass[i].m_pObject->SetStage(spHairPass[i].m_pObject->m_uiCurrentStage, spStage.m_pObject);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 2, NiTexturingProperty::CLAMP_S_WRAP_T, D3DTEXF_LINEAR, true);
        spHairPass[i].m_pObject->SetStage(spHairPass[i].m_pObject->m_uiCurrentStage, spStage.m_pObject);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 3, NiTexturingProperty::WRAP_S_WRAP_T, D3DTEXF_LINEAR, true);
        spHairPass[i].m_pObject->SetStage(spHairPass[i].m_pObject->m_uiCurrentStage, spStage.m_pObject);
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, 4, NiTexturingProperty::CLAMP_S_WRAP_T, D3DTEXF_LINEAR, true);
        spHairPass[i].m_pObject->SetStage(spHairPass[i].m_pObject->m_uiCurrentStage, spStage.m_pObject);
        if (BSShaderManager::GetShaderLevel(BSShaderManager::VertexShader) >= 2) {
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, 5, NiTexturingProperty::WRAP_S_WRAP_T, D3DTEXF_LINEAR, true);
            spHairPass[i].m_pObject->SetStage(spHairPass[i].m_pObject->m_uiCurrentStage, spStage.m_pObject);
        }
        // spStage's release (the NiPointer destructor) ends the iteration, as the game does.
    }
    ShadowLightShader::ulVertexConstantFlags[193] = 0x482;
    ShadowLightShader::ulVertexConstantFlags[194] = 0x482;
    ShadowLightShader::ulPixelConstantFlags[193] = 0x2C;
    ShadowLightShader::ulPixelConstantFlags[194] = 0x3C;
    return true;
}

int32_t HairShader::GetShaderType() {
    return BSShaderManager::BSSM_SHADERTYPE_HAIR;
}

BSShader* HairShader::CreateShader() {
    HairShader* pshader;
    NiDX9ShaderDeclaration* spShaderDecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 6u, 1u);
    NiDX9ShaderDeclaration* pSkinnedShaderDecl =
        NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 8u, 1u);
    spShaderDecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    spShaderDecl->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    spShaderDecl->SetEntry(2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
    spShaderDecl->SetEntry(
        3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    spShaderDecl->SetEntry(4u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    spShaderDecl->SetEntry(
        5u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    spShaderDecl->GetD3DDeclaration();
    pSkinnedShaderDecl->SetEntry(
        0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pSkinnedShaderDecl->SetEntry(
        1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_BLENDWEIGHT, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pSkinnedShaderDecl->SetEntry(
        2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_BLENDINDICES, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pSkinnedShaderDecl->SetEntry(
        3u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pSkinnedShaderDecl->SetEntry(
        4u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pSkinnedShaderDecl->SetEntry(
        5u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    pSkinnedShaderDecl->SetEntry(
        6u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pSkinnedShaderDecl->SetEntry(
        7u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pSkinnedShaderDecl->GetD3DDeclaration();
    HairShader* v2 = static_cast<HairShader*>(NiMemObject::operator new(sizeof(HairShader)));
    if (v2) {
        pshader = ::new (v2) HairShader(spShaderDecl, pSkinnedShaderDecl);
    }
    else {
        pshader = nullptr;
    }
    pshader->Initialize();
    pshader->LoadShaders();
    pshader->LoadStagesAndPasses();
    pshader->CreateConstantMaps();
    pshader->SetShaderDecl(spShaderDecl);
    return pshader;
}
