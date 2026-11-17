#include "BSShader/SkinShader.h"

#include "BSCore/BSString.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
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
#include "SDK/MSVC/stdlib.h"

SkinShader::kPasses_t SkinShader::kPasses;

NiRTTI SkinShader::ms_RTTI("SkinShader", &ShadowLightShader::ms_RTTI);

SkinShader::SkinShader(NiDX9ShaderDeclaration* apShaderDecl, NiDX9ShaderDeclaration* apSkinnedShaderDecl)
  : ShadowLightShader(apShaderDecl, apSkinnedShaderDecl, nullptr, nullptr), pVertexShader(), pPixelShader() {
    iShaderType = 4;
    bFirstPass = 0;
    ePass = BSShaderManager::BSSM_EMPTY;
}

SkinShader::~SkinShader() {
    for (NiD3DVertexShader*& pShader : pVertexShader) {
        if (pShader) {
            pShader->DecRefCount();
            pShader = nullptr;
        }
    }
    for (NiD3DPixelShader*& pShader : pPixelShader) {
        if (pShader) {
            pShader->DecRefCount();
            pShader = nullptr;
        }
    }
    for (NiPointer<NiD3DPass>& spPass : kPasses) {
        spPass = nullptr;
    }
}

const NiRTTI* SkinShader::GetRTTI() const {
    return &SkinShader::ms_RTTI;
}

bool SkinShader::Initialize() {
    BSShader::Initialize();
    // The arrays hold NiPointers, released here to null.
    for (NiD3DVertexShader*& pShader : pVertexShader) {
        if (pShader) {
            NiDecRefCount(pShader);
            pShader = 0;
        }
    }
    for (NiD3DPixelShader*& pShader : pPixelShader) {
        if (pShader) {
            NiDecRefCount(pShader);
            pShader = 0;
        }
    }
    for (NiPointer<NiD3DPass>& spPass : SkinShader::kPasses) {
        NiD3DPass* pPass = spPass.m_pObject;
        if (pPass) {
            if (pPass->m_uiRefCount-- == 1) {
                pPass->ReturnPassToPool();
            }
            spPass.m_pObject = 0;
        }
    }
    return true;
}

void SkinShader::SetupGeometryTextures(const NiPropertyState* apState) {
    BSShaderPPLightingProperty* pprop = static_cast<BSShaderPPLightingProperty*>(apState->m_aspProps[3].m_pObject);
    switch (BSShaderManager::eCurrentPass) {
        case BSShaderManager::BSSM_ADT10_Fg:
        case BSShaderManager::BSSM_ADT10_SFg:
        case BSShaderManager::BSSM_ADT4_Fg:
        case BSShaderManager::BSSM_ADT4_SFg:
        case BSShaderManager::BSSM_ADTS10_Fg:
        case BSShaderManager::BSSM_ADTS10_SFg:
        case BSShaderManager::BSSM_ADT_Fg:
        case BSShaderManager::BSSM_ADT_SFg:
        case BSShaderManager::BSSM_ADT_FgShp:
        case BSShaderManager::BSSM_ADT_SFgShp:
        case BSShaderManager::BSSM_ADT2_Fg:
        case BSShaderManager::BSSM_ADT2_SFg:
        case BSShaderManager::BSSM_ADT2_FgShp:
        case BSShaderManager::BSSM_ADT2_SFgShp: {
            SetDiffuseMap(pprop, 0, 0);
            SetNormalMap(pprop, 1u, 0);
            SetDiffuseMap(pprop, 2u, 1u);
            SetNormalMap(pprop, 3u, 1u);
            goto LABEL_5;
        }
        case BSShaderManager::BSSM_AD2_Fg:
        case BSShaderManager::BSSM_AD2_SFg:
        case BSShaderManager::BSSM_AD2_FgShp:
        case BSShaderManager::BSSM_AD2_SFgShp:
        case BSShaderManager::BSSM_AD3_Fg:
        case BSShaderManager::BSSM_AD3_SFg:
        case BSShaderManager::BSSM_AD3_FgShp:
        case BSShaderManager::BSSM_AD3_SFgShp: {
            SetDiffuseMap(pprop, 0, 0);
            SetNormalMap(pprop, 1u, 0);
            SetGlowMap(pprop, 3u, 0, 1);
            goto LABEL_6;
        }
        case BSShaderManager::BSSM_DIFFUSEPT2_Fg:
        case BSShaderManager::BSSM_DIFFUSEPT2_SFg:
        case BSShaderManager::BSSM_DIFFUSEPT3_Fg:
        case BSShaderManager::BSSM_DIFFUSEPT3_SFg: {
            SetNormalMap(pprop, 0, 0);
        LABEL_5:;
            SetGlowMap(pprop, 4u, 0, 1);
        LABEL_6:;
            ApplyTextureStages();
            break;
        }
        default: {
            ShadowLightShader::SetupGeometryTextures(apState);
            break;
        }
    }
}

void SkinShader::RecreateRendererData() {
    NiD3DVertexShader** pVertexShader;
    NiD3DPixelShader** pPixelShader;
    pVertexShader = this->pVertexShader;
    int32_t v3 = 26;
    do {
        if (*pVertexShader) {
            (*pVertexShader)->RecreateRendererData();
        }
        ++pVertexShader;
        --v3;
    }
    while (v3);
    pPixelShader = this->pPixelShader;
    int32_t v5 = 13;
    do {
        if (*pPixelShader) {
            (*pPixelShader)->RecreateRendererData();
        }
        ++pPixelShader;
        --v5;
    }
    while (v5);
    ShadowLightShader::RecreateRendererData();
}

void SkinShader::InitPass(uint32_t auiCurrentPass) {
    NiD3DPass* m_pObject;
    if (auiCurrentPass > 256) {
        switch (auiCurrentPass) {
            case 263u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[9].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                break;
            }
            case 269u: {
                m_pObject = SkinShader::kPasses[10].m_pObject;
                goto LABEL_5;
            }
            case 275u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[11].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                break;
            }
            case 280u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[12].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                break;
            }
            case 286u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[13].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                break;
            }
            case 291u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[14].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                break;
            }
            case 297u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[15].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                break;
            }
            case 307u: {
                m_pObject = SkinShader::kPasses[16].m_pObject;
            LABEL_5:;
                NiD3DShader::m_spCurrentPass = m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                break;
            }
            case 310u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[17].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                break;
            }
            case 319u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[18].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                break;
            }
            case 322u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[19].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                break;
            }
            default: {
            LABEL_34:;
                ShadowLightShader::InitPass(auiCurrentPass);
                BSShader* Shader = BSShaderManager::GetShader(1);
                Shader->InitPass(auiCurrentPass);
                break;
            }
        }
    }
    else {
        if (auiCurrentPass == 256) {
            NiD3DShader::m_spCurrentPass = SkinShader::kPasses[8].m_pObject;
            m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
        LABEL_37:;
            m_spPixelConstantMap.operator=(spDefaultPixelConstantMap);
            m_spVertexConstantMap.operator=(spDefaultVertexConstantMap);
            return;
        }
        switch (auiCurrentPass) {
            case 1u:
            case 2u: {
                ShadowLightShader::InitPass(auiCurrentPass);
                BSShader* v4 = BSShaderManager::GetShader(1);
                v4->InitPass(auiCurrentPass);
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                break;
            }
            case 3u: {
                ShadowLightShader::InitPass(auiCurrentPass);
                BSShader* v5 = BSShaderManager::GetShader(1);
                v5->InitPass(auiCurrentPass);
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                break;
            }
            case 197u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[20].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                break;
            }
            case 200u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[21].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                break;
            }
            case 204u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[22].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                break;
            }
            case 207u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[23].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                break;
            }
            case 211u: {
                m_pObject = SkinShader::kPasses[24].m_pObject;
                goto LABEL_5;
            }
            case 214u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[25].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                break;
            }
            case 219u: {
                m_pObject = SkinShader::kPasses[0].m_pObject;
                goto LABEL_5;
            }
            case 222u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[1].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                break;
            }
            case 228u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[2].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                break;
            }
            case 231u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[3].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                break;
            }
            case 237u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[4].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                break;
            }
            case 240u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[5].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                break;
            }
            case 246u: {
                m_pObject = SkinShader::kPasses[6].m_pObject;
                goto LABEL_5;
            }
            case 249u: {
                NiD3DShader::m_spCurrentPass = SkinShader::kPasses[7].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                break;
            }
            default: {
                goto LABEL_34;
            }
        }
    }
    if (auiCurrentPass != 592 && auiCurrentPass != 593) {
        goto LABEL_37;
    }
    m_spPixelConstantMap = spTexEffectPConstantMap;
    m_spVertexConstantMap = spTexEffectVConstantMap;
}

void SkinShader::SetupPassTextures(uint32_t auiCurrentPass) {
    switch (auiCurrentPass) {
        case 197u:
        case 200u:
        case 204u:
        case 207u:
        case 211u:
        case 214u:
        case 219u:
        case 222u:
        case 237u:
        case 240u:
        case 256u:
        case 263u:
        case 280u:
        case 286u:
        case 307u:
        case 310u:
        case 319u:
        case 322u: {
            return;
        }
        case 228u:
        case 231u:
        case 246u:
        case 249u: {
            SetProjectedShadowMap(5u);
            SetProjectedShadowMaskMap(6u);
            break;
        }
        case 269u:
        case 275u:
        case 291u:
        case 297u: {
            SetProjectedShadowMap(6u);
            SetProjectedShadowMaskMap(7u);
            break;
        }
        default: {
            ShadowLightShader::SetupPassTextures(auiCurrentPass);
            break;
        }
    }
}

NiD3DShaderDeclaration* SkinShader::GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) {
    if (apGeometry->m_spSkinInstance.m_pObject) {
        return spSkinnedShaderDecl.m_pObject;
    }
    else {
        return spShaderDecl.m_pObject;
    }
}

// Takes the shared constant maps of the lighting shader (shader 1).
void SkinShader::CreateConstantMaps() {
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

// Reloads the shader programs, then rebuilds the passes.
void SkinShader::ReloadShaders() {
    LoadShaders();
    PresetPasses();
}

void SkinShader::ClearPassStages() {
    for (NiPointer<NiD3DPass>& spPass : SkinShader::kPasses) {
        ClearPass(spPass.m_pObject);
    }
}

void SkinShader::LoadVertexShaders() {
    BSShaderMacro kMacroArray[26];
    kMacroArray[0].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray[0].kMacros[0].Name = "FACEGENBLEND";
    kMacroArray[0].kMacros[0].Definition = "";
    kMacroArray[0].kMacros[1].Name = 0;
    std::memset(
        &kMacroArray[0].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[1].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray[1].kMacros[0].Name = "SKIN";
    kMacroArray[1].kMacros[0].Definition = "";
    kMacroArray[1].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[1].kMacros[1].Definition = "";
    kMacroArray[1].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[1].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[2].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray[2].kMacros[0].Name = "FACEGENBLEND";
    kMacroArray[2].kMacros[0].Definition = "";
    kMacroArray[2].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray[2].kMacros[1].Definition = "";
    kMacroArray[2].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[2].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[3].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray[3].kMacros[0].Name = "SKIN";
    kMacroArray[3].kMacros[0].Definition = "";
    kMacroArray[3].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[3].kMacros[1].Definition = "";
    kMacroArray[3].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray[3].kMacros[2].Definition = "";
    kMacroArray[3].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[3].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[4].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray[4].kMacros[0].Name = "LIGHTS";
    kMacroArray[4].kMacros[0].Definition = "2";
    kMacroArray[4].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[4].kMacros[1].Definition = "";
    kMacroArray[4].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[4].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[5].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray[5].kMacros[0].Name = "SKIN";
    kMacroArray[5].kMacros[0].Definition = "";
    kMacroArray[5].kMacros[1].Name = "LIGHTS";
    kMacroArray[5].kMacros[1].Definition = "2";
    kMacroArray[5].kMacros[2].Name = "FACEGENBLEND";
    kMacroArray[5].kMacros[2].Definition = "";
    kMacroArray[5].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[5].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[6].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray[6].kMacros[0].Name = "LIGHTS";
    kMacroArray[6].kMacros[0].Definition = "2";
    kMacroArray[6].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[6].kMacros[1].Definition = "";
    kMacroArray[6].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray[6].kMacros[2].Definition = "";
    kMacroArray[6].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[6].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[7].strPath = "lighting\\2x\\v\\ADTS.v.hlsl";
    kMacroArray[7].kMacros[0].Name = "SKIN";
    kMacroArray[7].kMacros[0].Definition = "";
    kMacroArray[7].kMacros[1].Name = "LIGHTS";
    kMacroArray[7].kMacros[1].Definition = "2";
    kMacroArray[7].kMacros[2].Name = "FACEGENBLEND";
    kMacroArray[7].kMacros[2].Definition = "";
    kMacroArray[7].kMacros[3].Name = "PROJ_SHADOW";
    kMacroArray[7].kMacros[3].Definition = "";
    for (int32_t j = 4; j < 9; ++j) {
        kMacroArray[7].kMacros[j].Name = 0;
        kMacroArray[7].kMacros[j].Definition = 0;
    }
    kMacroArray[8].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray[8].kMacros[0].Name = "LIGHTS";
    kMacroArray[8].kMacros[0].Definition = "2";
    kMacroArray[8].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[8].kMacros[1].Definition = "";
    kMacroArray[8].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[8].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[9].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray[9].kMacros[0].Name = "SKIN";
    kMacroArray[9].kMacros[0].Definition = "";
    kMacroArray[9].kMacros[1].Name = "LIGHTS";
    kMacroArray[9].kMacros[1].Definition = "2";
    kMacroArray[9].kMacros[2].Name = "FACEGENBLEND";
    kMacroArray[9].kMacros[2].Definition = "";
    kMacroArray[9].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[9].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[10].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray[10].kMacros[0].Name = "LIGHTS";
    kMacroArray[10].kMacros[0].Definition = "2";
    kMacroArray[10].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[10].kMacros[1].Definition = "";
    kMacroArray[10].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray[10].kMacros[2].Definition = "";
    kMacroArray[10].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[10].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[11].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray[11].kMacros[0].Name = "SKIN";
    kMacroArray[11].kMacros[0].Definition = "";
    kMacroArray[11].kMacros[1].Name = "LIGHTS";
    kMacroArray[11].kMacros[1].Definition = "2";
    kMacroArray[11].kMacros[2].Name = "FACEGENBLEND";
    kMacroArray[11].kMacros[2].Definition = "";
    kMacroArray[11].kMacros[3].Name = "PROJ_SHADOW";
    kMacroArray[11].kMacros[3].Definition = "";
    for (int32_t j = 4; j < 9; ++j) {
        kMacroArray[11].kMacros[j].Name = 0;
        kMacroArray[11].kMacros[j].Definition = 0;
    }
    kMacroArray[12].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray[12].kMacros[0].Name = "LIGHTS";
    kMacroArray[12].kMacros[0].Definition = "3";
    kMacroArray[12].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[12].kMacros[1].Definition = "";
    kMacroArray[12].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[12].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[13].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray[13].kMacros[0].Name = "SKIN";
    kMacroArray[13].kMacros[0].Definition = "";
    kMacroArray[13].kMacros[1].Name = "LIGHTS";
    kMacroArray[13].kMacros[1].Definition = "3";
    kMacroArray[13].kMacros[2].Name = "FACEGENBLEND";
    kMacroArray[13].kMacros[2].Definition = "";
    kMacroArray[13].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[13].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[14].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray[14].kMacros[0].Name = "LIGHTS";
    kMacroArray[14].kMacros[0].Definition = "3";
    kMacroArray[14].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[14].kMacros[1].Definition = "";
    kMacroArray[14].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray[14].kMacros[2].Definition = "";
    kMacroArray[14].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[14].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[15].strPath = "lighting\\2x\\v\\AD.v.hlsl";
    kMacroArray[15].kMacros[0].Name = "SKIN";
    kMacroArray[15].kMacros[0].Definition = "";
    kMacroArray[15].kMacros[1].Name = "LIGHTS";
    kMacroArray[15].kMacros[1].Definition = "3";
    kMacroArray[15].kMacros[2].Name = "FACEGENBLEND";
    kMacroArray[15].kMacros[2].Definition = "";
    kMacroArray[15].kMacros[3].Name = "PROJ_SHADOW";
    kMacroArray[15].kMacros[3].Definition = "";
    for (int32_t j = 4; j < 9; ++j) {
        kMacroArray[15].kMacros[j].Name = 0;
        kMacroArray[15].kMacros[j].Definition = 0;
    }
    kMacroArray[16].strPath = "lighting\\2x\\v\\DiffusePt.v.hlsl";
    kMacroArray[16].kMacros[0].Name = "LIGHTS";
    kMacroArray[16].kMacros[0].Definition = "2";
    kMacroArray[16].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[16].kMacros[1].Definition = "";
    kMacroArray[16].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[16].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[17].strPath = "lighting\\2x\\v\\DiffusePt.v.hlsl";
    kMacroArray[17].kMacros[0].Name = "LIGHTS";
    kMacroArray[17].kMacros[0].Definition = "2";
    kMacroArray[17].kMacros[1].Name = "SKIN";
    kMacroArray[17].kMacros[1].Definition = "";
    kMacroArray[17].kMacros[2].Name = "FACEGENBLEND";
    kMacroArray[17].kMacros[2].Definition = "";
    kMacroArray[17].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[17].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[18].strPath = "lighting\\2x\\v\\DiffusePt.v.hlsl";
    kMacroArray[18].kMacros[0].Name = "LIGHTS";
    kMacroArray[18].kMacros[0].Definition = "3";
    kMacroArray[18].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[18].kMacros[1].Definition = "";
    kMacroArray[18].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[18].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[19].strPath = "lighting\\2x\\v\\DiffusePt.v.hlsl";
    kMacroArray[19].kMacros[0].Name = "LIGHTS";
    kMacroArray[19].kMacros[0].Definition = "3";
    kMacroArray[19].kMacros[1].Name = "SKIN";
    kMacroArray[19].kMacros[1].Definition = "";
    kMacroArray[19].kMacros[2].Name = "FACEGENBLEND";
    kMacroArray[19].kMacros[2].Definition = "";
    kMacroArray[19].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[19].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[20].strPath = "lighting\\2x\\v\\ADTS10.v.hlsl";
    kMacroArray[20].kMacros[0].Name = "LIGHTS";
    kMacroArray[20].kMacros[0].Definition = "9";
    kMacroArray[20].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[20].kMacros[1].Definition = "";
    kMacroArray[20].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[20].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    // FACEGENBLEND's definition is deliberately left null in entries 21 and 23 to 25, as the game does.
    kMacroArray[21].strPath = "lighting\\2x\\v\\ADTS10.v.hlsl";
    kMacroArray[21].kMacros[0].Name = "LIGHTS";
    kMacroArray[21].kMacros[0].Definition = "9";
    kMacroArray[21].kMacros[1].Name = "SKIN";
    kMacroArray[21].kMacros[1].Definition = "";
    kMacroArray[21].kMacros[2].Name = "FACEGENBLEND";
    kMacroArray[21].kMacros[2].Definition = 0;
    kMacroArray[21].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[21].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[22].strPath = "lighting\\2x\\v\\ADTS10.v.hlsl";
    kMacroArray[22].kMacros[0].Name = "LIGHTS";
    kMacroArray[22].kMacros[0].Definition = "4";
    kMacroArray[22].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[22].kMacros[1].Definition = "";
    kMacroArray[22].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[22].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[23].strPath = "lighting\\2x\\v\\ADTS10.v.hlsl";
    kMacroArray[23].kMacros[0].Name = "LIGHTS";
    kMacroArray[23].kMacros[0].Definition = "4";
    kMacroArray[23].kMacros[1].Name = "SKIN";
    kMacroArray[23].kMacros[1].Definition = "";
    kMacroArray[23].kMacros[2].Name = "FACEGENBLEND";
    kMacroArray[23].kMacros[2].Definition = 0;
    kMacroArray[23].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[23].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[24].strPath = "lighting\\2x\\v\\ADTS10.v.hlsl";
    kMacroArray[24].kMacros[0].Name = "SPECULAR";
    kMacroArray[24].kMacros[0].Definition = "";
    kMacroArray[24].kMacros[1].Name = "LIGHTS";
    kMacroArray[24].kMacros[1].Definition = "4";
    kMacroArray[24].kMacros[2].Name = "FACEGENBLEND";
    kMacroArray[24].kMacros[2].Definition = 0;
    kMacroArray[24].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[24].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[25].strPath = "lighting\\2x\\v\\ADTS10.v.hlsl";
    kMacroArray[25].kMacros[0].Name = "SPECULAR";
    kMacroArray[25].kMacros[0].Definition = "";
    kMacroArray[25].kMacros[1].Name = "LIGHTS";
    kMacroArray[25].kMacros[1].Definition = "4";
    kMacroArray[25].kMacros[2].Name = "SKIN";
    kMacroArray[25].kMacros[2].Definition = "";
    kMacroArray[25].kMacros[3].Name = "FACEGENBLEND";
    kMacroArray[25].kMacros[3].Definition = 0;
    for (int32_t j = 4; j < 9; ++j) {
        kMacroArray[25].kMacros[j].Name = 0;
        kMacroArray[25].kMacros[j].Definition = 0;
    }
    if (BSShaderManager::eShaderLevel >= 2) {
        for (int32_t i = 0; i < 26; ++i) {
            BSShaderMacro& rMacro = kMacroArray[i];
            if (rMacro.strPath) {
                char acShaderName[260];
                char acPath[260];
                BSShader::MakePath(rMacro.strPath, acPath);
                BSsprintf(acShaderName, 260, "SKIN2%03i.vso", i);
                NiD3DVertexShader* pShader =
                    CompileVertexShaderHLSL(acPath, rMacro.kMacros, ShaderTarget_vs_2_0, acShaderName);
                // The array holds NiPointer<NiD3DVertexShader>s; their assignment is written out by hand here.
                NiD3DVertexShader* pOld = pVertexShader[i];
                if (pOld != pShader) {
                    if (pOld) {
                        NiDecRefCount(pOld);
                    }
                    pVertexShader[i] = pShader;
                    if (pShader) {
                        NiIncRefCount(pShader);
                    }
                }
            }
        }
    }
}

void SkinShader::LoadPixelShaders() {
    BSShaderMacro kMacroArray[13];
    kMacroArray[0].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray[0].kMacros[0].Name = "FACEGENBLEND";
    kMacroArray[0].kMacros[0].Definition = "";
    kMacroArray[0].kMacros[1].Name = 0;
    std::memset(
        &kMacroArray[0].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[1].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray[1].kMacros[0].Name = "FACEGENBLEND";
    kMacroArray[1].kMacros[0].Definition = "";
    kMacroArray[1].kMacros[1].Name = "PROJ_SHADOW";
    kMacroArray[1].kMacros[1].Definition = "";
    kMacroArray[1].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[1].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[2].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray[2].kMacros[0].Name = "LIGHTS";
    kMacroArray[2].kMacros[0].Definition = "2";
    kMacroArray[2].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[2].kMacros[1].Definition = "";
    kMacroArray[2].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[2].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[3].strPath = "lighting\\2x\\p\\ADTS.p.hlsl";
    kMacroArray[3].kMacros[0].Name = "LIGHTS";
    kMacroArray[3].kMacros[0].Definition = "2";
    kMacroArray[3].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[3].kMacros[1].Definition = "";
    kMacroArray[3].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray[3].kMacros[2].Definition = "";
    kMacroArray[3].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[3].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[4].strPath = "lighting\\2x\\p\\AD.p.hlsl";
    kMacroArray[4].kMacros[0].Name = "LIGHTS";
    kMacroArray[4].kMacros[0].Definition = "2";
    kMacroArray[4].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[4].kMacros[1].Definition = "";
    kMacroArray[4].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[4].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[5].strPath = "lighting\\2x\\p\\AD.p.hlsl";
    kMacroArray[5].kMacros[0].Name = "LIGHTS";
    kMacroArray[5].kMacros[0].Definition = "2";
    kMacroArray[5].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[5].kMacros[1].Definition = "";
    kMacroArray[5].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray[5].kMacros[2].Definition = "";
    kMacroArray[5].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[5].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[6].strPath = "lighting\\2x\\p\\AD.p.hlsl";
    kMacroArray[6].kMacros[0].Name = "LIGHTS";
    kMacroArray[6].kMacros[0].Definition = "3";
    kMacroArray[6].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[6].kMacros[1].Definition = "";
    kMacroArray[6].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[6].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[7].strPath = "lighting\\2x\\p\\AD.p.hlsl";
    kMacroArray[7].kMacros[0].Name = "LIGHTS";
    kMacroArray[7].kMacros[0].Definition = "3";
    kMacroArray[7].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[7].kMacros[1].Definition = "";
    kMacroArray[7].kMacros[2].Name = "PROJ_SHADOW";
    kMacroArray[7].kMacros[2].Definition = "";
    kMacroArray[7].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[7].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[8].strPath = "lighting\\2x\\p\\DiffusePt.p.hlsl";
    kMacroArray[8].kMacros[0].Name = "LIGHTS";
    kMacroArray[8].kMacros[0].Definition = "2";
    kMacroArray[8].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[8].kMacros[1].Definition = "";
    kMacroArray[8].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[8].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[9].strPath = "lighting\\2x\\p\\DiffusePt.p.hlsl";
    kMacroArray[9].kMacros[0].Name = "LIGHTS";
    kMacroArray[9].kMacros[0].Definition = "3";
    kMacroArray[9].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[9].kMacros[1].Definition = "";
    kMacroArray[9].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[9].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[10].strPath = "lighting\\2x\\p\\ADTS10.p.hlsl";
    kMacroArray[10].kMacros[0].Name = "LIGHTS";
    kMacroArray[10].kMacros[0].Definition = "9";
    kMacroArray[10].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[10].kMacros[1].Definition = 0;
    kMacroArray[10].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[10].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[11].strPath = "lighting\\2x\\p\\ADTS10.p.hlsl";
    kMacroArray[11].kMacros[0].Name = "LIGHTS";
    kMacroArray[11].kMacros[0].Definition = "4";
    kMacroArray[11].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[11].kMacros[1].Definition = 0;
    kMacroArray[11].kMacros[2].Name = 0;
    std::memset(
        &kMacroArray[11].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[12].strPath = "lighting\\2x\\p\\ADTS10.p.hlsl";
    kMacroArray[12].kMacros[0].Name = "SPECULAR";
    kMacroArray[12].kMacros[0].Definition = "";
    kMacroArray[12].kMacros[1].Name = "LIGHTS";
    kMacroArray[12].kMacros[1].Definition = "4";
    kMacroArray[12].kMacros[2].Name = "FACEGENBLEND";
    kMacroArray[12].kMacros[2].Definition = 0;
    kMacroArray[12].kMacros[3].Name = 0;
    std::memset(
        &kMacroArray[12].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    if (BSShaderManager::eShaderLevel >= 2) {
        for (int32_t i = 0; i < 13; ++i) {
            BSShaderMacro& rMacro = kMacroArray[i];
            if (rMacro.strPath) {
                char acShaderName[260];
                char acPath[260];
                BSShader::MakePath(rMacro.strPath, acPath);
                BSsprintf(acShaderName, 260, "SKIN2%03i.pso", i);
                NiD3DPixelShader* pShader = CompilePixelShaderHLSL(acPath, rMacro.kMacros, "ps_2_0", acShaderName);
                // The array holds NiPointer<NiD3DPixelShader>s; their assignment is written out here.
                NiD3DPixelShader* pOld = pPixelShader[i];
                if (pOld != pShader) {
                    if (pOld) {
                        NiDecRefCount(pOld);
                    }
                    pPixelShader[i] = pShader;
                    if (pShader) {
                        NiIncRefCount(pShader);
                    }
                }
            }
        }
    }
}

bool SkinShader::LoadStagesAndPasses() {
    NiPointer<NiD3DPass>* v2 = SkinShader::kPasses;
    int32_t v3 = 26;
    do {
        NiD3DPass::CreateNewPass(*v2++);
        --v3;
    }
    while (v3);
    PresetPasses();
    return 1;
}

void SkinShader::PresetPasses() {
    NiD3DPass* m_pObject = 0;
    NiD3DPass* v52 = 0;
    {
        NiPointer<NiD3DTextureStage> spstage;
        if (SkinShader::kPasses[0].m_pObject) {
            m_pObject = SkinShader::kPasses[0].m_pObject;
            v52 = SkinShader::kPasses[0].m_pObject;
            ++SkinShader::kPasses[0].m_pObject->m_uiRefCount;
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v3 = pVertexShader[8];
        m_pObject->m_spVertexShader = v3;
        NiD3DPixelShader* v5 = pPixelShader[4];
        m_pObject->m_spPixelShader = v5;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[219] = 1152;
        ShadowLightShader::ulPixelConstantFlags[219] = 12;
        if (m_pObject != SkinShader::kPasses[1].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[1].m_pObject;
            v52 = SkinShader::kPasses[1].m_pObject;
            if (SkinShader::kPasses[1].m_pObject) {
                ++SkinShader::kPasses[1].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v8 = this->pVertexShader[9];
        m_pObject->m_spVertexShader = v8;
        NiD3DPixelShader* v10 = this->pPixelShader[4];
        m_pObject->m_spPixelShader = v10;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[222] = 3200;
        ShadowLightShader::ulPixelConstantFlags[222] = 12;
        if (m_pObject != SkinShader::kPasses[2].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[2].m_pObject;
            v52 = SkinShader::kPasses[2].m_pObject;
            if (SkinShader::kPasses[2].m_pObject) {
                ++SkinShader::kPasses[2].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v12 = this->pVertexShader[10];
        m_pObject->m_spVertexShader = v12;
        NiD3DPixelShader* v14 = this->pPixelShader[5];
        m_pObject->m_spPixelShader = v14;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[228] = 1264;
        ShadowLightShader::ulPixelConstantFlags[228] = 12;
        if (m_pObject != SkinShader::kPasses[3].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[3].m_pObject;
            v52 = SkinShader::kPasses[3].m_pObject;
            if (SkinShader::kPasses[3].m_pObject) {
                ++SkinShader::kPasses[3].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v16 = this->pVertexShader[11];
        m_pObject->m_spVertexShader = v16;
        NiD3DPixelShader* v18 = this->pPixelShader[5];
        m_pObject->m_spPixelShader = v18;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[231] = 3312;
        ShadowLightShader::ulPixelConstantFlags[231] = 12;
        if (m_pObject != SkinShader::kPasses[4].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[4].m_pObject;
            v52 = SkinShader::kPasses[4].m_pObject;
            if (SkinShader::kPasses[4].m_pObject) {
                ++SkinShader::kPasses[4u].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v20 = this->pVertexShader[12];
        m_pObject->m_spVertexShader = v20;
        NiD3DPixelShader* v22 = this->pPixelShader[6];
        m_pObject->m_spPixelShader = v22;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[237] = 1152;
        ShadowLightShader::ulPixelConstantFlags[237] = 12;
        if (m_pObject != SkinShader::kPasses[5].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[5].m_pObject;
            v52 = SkinShader::kPasses[5].m_pObject;
            if (SkinShader::kPasses[5].m_pObject) {
                ++SkinShader::kPasses[5].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v25 = this->pVertexShader[13];
        m_pObject->m_spVertexShader = v25;
        NiD3DPixelShader* v27 = this->pPixelShader[6];
        m_pObject->m_spPixelShader = v27;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[240] = 3200;
        ShadowLightShader::ulPixelConstantFlags[240] = 12;
        if (m_pObject != SkinShader::kPasses[6].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[6].m_pObject;
            v52 = SkinShader::kPasses[6].m_pObject;
            if (SkinShader::kPasses[6].m_pObject) {
                ++SkinShader::kPasses[6].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v29 = this->pVertexShader[14];
        m_pObject->m_spVertexShader = v29;
        NiD3DPixelShader* v31 = this->pPixelShader[7];
        m_pObject->m_spPixelShader = v31;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[246] = 1264;
        ShadowLightShader::ulPixelConstantFlags[246] = 12;
        if (m_pObject != SkinShader::kPasses[7].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[7].m_pObject;
            v52 = SkinShader::kPasses[7].m_pObject;
            if (SkinShader::kPasses[7].m_pObject) {
                ++SkinShader::kPasses[7].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 7) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 3u, 0, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
        }
        NiD3DVertexShader* v33 = this->pVertexShader[15];
        m_pObject->m_spVertexShader = v33;
        NiD3DPixelShader* v35 = this->pPixelShader[7];
        m_pObject->m_spPixelShader = v35;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[249] = 3312;
        ShadowLightShader::ulPixelConstantFlags[249] = 12;
        if (m_pObject != SkinShader::kPasses[8].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[8].m_pObject;
            v52 = SkinShader::kPasses[8].m_pObject;
            if (SkinShader::kPasses[8].m_pObject) {
                ++SkinShader::kPasses[8].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v37 = this->pVertexShader[0];
        m_pObject->m_spVertexShader = v37;
        NiD3DPixelShader* v39 = this->pPixelShader[0];
        m_pObject->m_spPixelShader = v39;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[256] = 1920;
        ShadowLightShader::ulPixelConstantFlags[256] = 44;
        if (m_pObject != SkinShader::kPasses[9].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[9].m_pObject;
            v52 = SkinShader::kPasses[9].m_pObject;
            if (SkinShader::kPasses[9].m_pObject) {
                ++SkinShader::kPasses[9].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        NiD3DVertexShader* v41 = this->pVertexShader[1];
        m_pObject->m_spVertexShader = v41;
        NiD3DPixelShader* v43 = this->pPixelShader[0];
        m_pObject->m_spPixelShader = v43;
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[263] = 3968;
        ShadowLightShader::ulPixelConstantFlags[263] = 44;
        if (m_pObject != SkinShader::kPasses[10].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[10].m_pObject;
            v52 = SkinShader::kPasses[10].m_pObject;
            if (SkinShader::kPasses[10].m_pObject) {
                ++SkinShader::kPasses[10].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[2]);
        m_pObject->SetPixelShader(this->pPixelShader[1]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[269] = 2032;
        ShadowLightShader::ulPixelConstantFlags[269] = 44;
        if (m_pObject != SkinShader::kPasses[11].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[11].m_pObject;
            v52 = SkinShader::kPasses[11].m_pObject;
            if (SkinShader::kPasses[11].m_pObject) {
                ++SkinShader::kPasses[11].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[3]);
        m_pObject->SetPixelShader(this->pPixelShader[1]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[275] = 4080;
        ShadowLightShader::ulPixelConstantFlags[275] = 44;
        if (m_pObject != SkinShader::kPasses[12].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[12].m_pObject;
            v52 = SkinShader::kPasses[12].m_pObject;
            if (SkinShader::kPasses[12].m_pObject) {
                ++SkinShader::kPasses[12].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[4]);
        m_pObject->SetPixelShader(this->pPixelShader[2]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[280] = 1920;
        ShadowLightShader::ulPixelConstantFlags[280] = 44;
        if (m_pObject != SkinShader::kPasses[13].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[13].m_pObject;
            v52 = SkinShader::kPasses[13].m_pObject;
            if (SkinShader::kPasses[13].m_pObject) {
                ++SkinShader::kPasses[13].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[5]);
        m_pObject->SetPixelShader(this->pPixelShader[2]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[286] = 3968;
        ShadowLightShader::ulPixelConstantFlags[286] = 44;
        if (m_pObject != SkinShader::kPasses[14].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[14].m_pObject;
            v52 = SkinShader::kPasses[14].m_pObject;
            if (SkinShader::kPasses[14].m_pObject) {
                ++SkinShader::kPasses[14].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[6]);
        m_pObject->SetPixelShader(this->pPixelShader[3]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[291] = 2032;
        ShadowLightShader::ulPixelConstantFlags[291] = 44;
        if (m_pObject != SkinShader::kPasses[15].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[15].m_pObject;
            v52 = SkinShader::kPasses[15].m_pObject;
            if (SkinShader::kPasses[15].m_pObject) {
                ++SkinShader::kPasses[15].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 3u, 0, 1);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[7]);
        m_pObject->SetPixelShader(this->pPixelShader[3]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[297] = 4080;
        ShadowLightShader::ulPixelConstantFlags[297] = 44;
        if (m_pObject != SkinShader::kPasses[16].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[16].m_pObject;
            v52 = SkinShader::kPasses[16].m_pObject;
            if (SkinShader::kPasses[16].m_pObject) {
                ++SkinShader::kPasses[16].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[16]);
        m_pObject->SetPixelShader(this->pPixelShader[8]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        m_pObject->SetSrcDestBlendRenderState(2u, 2u, 0);
        ShadowLightShader::ulVertexConstantFlags[307] = 1920;
        ShadowLightShader::ulPixelConstantFlags[307] = 12;
        if (m_pObject != SkinShader::kPasses[17].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[17].m_pObject;
            v52 = SkinShader::kPasses[17].m_pObject;
            if (SkinShader::kPasses[17].m_pObject) {
                ++SkinShader::kPasses[17].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[17]);
        m_pObject->SetPixelShader(this->pPixelShader[8]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        m_pObject->SetSrcDestBlendRenderState(2u, 2u, 0);
        ShadowLightShader::ulVertexConstantFlags[310] = 3968;
        ShadowLightShader::ulPixelConstantFlags[310] = 12;
        if (m_pObject != SkinShader::kPasses[18].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[18].m_pObject;
            v52 = SkinShader::kPasses[18].m_pObject;
            if (SkinShader::kPasses[18].m_pObject) {
                ++SkinShader::kPasses[18].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[18]);
        m_pObject->SetPixelShader(this->pPixelShader[9]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        m_pObject->SetSrcDestBlendRenderState(2u, 2u, 0);
        ShadowLightShader::ulVertexConstantFlags[319] = 1920;
        ShadowLightShader::ulPixelConstantFlags[319] = 12;
        if (m_pObject != SkinShader::kPasses[19].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[19].m_pObject;
            v52 = SkinShader::kPasses[19].m_pObject;
            if (SkinShader::kPasses[19].m_pObject) {
                ++SkinShader::kPasses[19].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 4) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            spstage.m_pObject->m_spTexture = BSShaderManager::pAttenuationXY;
            spstage.m_pObject->m_uiTextureFlags = 1;
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 3u, 0, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[19]);
        m_pObject->SetPixelShader(this->pPixelShader[9]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        m_pObject->SetSrcDestBlendRenderState(2u, 2u, 0);
        ShadowLightShader::ulVertexConstantFlags[322] = 3968;
        ShadowLightShader::ulPixelConstantFlags[322] = 12;
        if (m_pObject != SkinShader::kPasses[20].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[20].m_pObject;
            v52 = SkinShader::kPasses[20].m_pObject;
            if (SkinShader::kPasses[20].m_pObject) {
                ++SkinShader::kPasses[20u].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[20]);
        m_pObject->SetPixelShader(this->pPixelShader[10]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[197] = 0x80780;
        ShadowLightShader::ulPixelConstantFlags[197] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[197] = 768;
        if (m_pObject != SkinShader::kPasses[21].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[21].m_pObject;
            v52 = SkinShader::kPasses[21].m_pObject;
            if (SkinShader::kPasses[21].m_pObject) {
                ++SkinShader::kPasses[21].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[21]);
        m_pObject->SetPixelShader(this->pPixelShader[10]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[200] = 0x80F80;
        ShadowLightShader::ulPixelConstantFlags[200] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[200] = 768;
        if (m_pObject != SkinShader::kPasses[22].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[22].m_pObject;
            v52 = SkinShader::kPasses[22].m_pObject;
            if (SkinShader::kPasses[22].m_pObject) {
                ++SkinShader::kPasses[22].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[22]);
        m_pObject->SetPixelShader(this->pPixelShader[11]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[204] = 0x80780;
        ShadowLightShader::ulPixelConstantFlags[204] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[204] = 768;
        if (m_pObject != SkinShader::kPasses[23].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[23].m_pObject;
            v52 = SkinShader::kPasses[23].m_pObject;
            if (SkinShader::kPasses[23].m_pObject) {
                ++SkinShader::kPasses[23].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[23]);
        m_pObject->SetPixelShader(this->pPixelShader[11]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[207] = 0x80F80;
        ShadowLightShader::ulPixelConstantFlags[207] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[207] = 768;
        if (m_pObject != SkinShader::kPasses[24].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[24].m_pObject;
            v52 = SkinShader::kPasses[24].m_pObject;
            if (SkinShader::kPasses[24].m_pObject) {
                ++SkinShader::kPasses[24u].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[24]);
        m_pObject->SetPixelShader(this->pPixelShader[12]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[211] = 0x80780;
        ShadowLightShader::ulPixelConstantFlags[211] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[211] = 768;
        if (m_pObject != SkinShader::kPasses[25].m_pObject) {
            m_pObject->DecRefCount();
            m_pObject = SkinShader::kPasses[25].m_pObject;
            v52 = SkinShader::kPasses[25].m_pObject;
            if (SkinShader::kPasses[25].m_pObject) {
                ++SkinShader::kPasses[25].m_pObject->m_uiRefCount;
            }
        }
        if (m_pObject->m_uiStageCount < 8) {
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 0, 1u, 2u, 1);
            m_pObject->SetStage(0, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 1u, 1u, 2u, 1);
            m_pObject->SetStage(1u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 2u, 1u, 2u, 1);
            m_pObject->SetStage(2u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 3u, 1u, 2u, 1);
            m_pObject->SetStage(3u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 4u, 1u, 2u, 1);
            m_pObject->SetStage(4u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 5u, 1u, 2u, 1);
            m_pObject->SetStage(5u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 6u, 1u, 2u, 1);
            m_pObject->SetStage(6u, spstage.m_pObject);
            NiD3DTextureStage::CreateNewStage(spstage);
            BSShader::SetupDefaultStage(spstage.m_pObject, 7u, 3u, 0, 1);
            m_pObject->SetStage(7u, spstage.m_pObject);
        }
        m_pObject->SetVertexShader(this->pVertexShader[25]);
        m_pObject->SetPixelShader(this->pPixelShader[12]);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(15u, 0, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 1u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(23u, 4u, 1);
        if (!m_pObject->m_pkRenderStateGroup) {
            m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        m_pObject->m_pkRenderStateGroup->SetRenderState(14u, 1u, 1);
        ShadowLightShader::ulVertexConstantFlags[214] = 0x80F80;
        ShadowLightShader::ulPixelConstantFlags[214] = 316;
        ShadowLightShader::uiVertexNonVolatileConstantFlags[214] = 768;
    }
    m_pObject->DecRefCount();
}

BSShader* SkinShader::CreateShader() {
    SkinShader* pshader;
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
    SkinShader* v2 = static_cast<SkinShader*>(NiMemObject::operator new(sizeof(SkinShader)));
    if (v2) {
        pshader = ::new (v2) SkinShader(spShaderDecl, pSkinnedShaderDecl);
    }
    else {
        pshader = 0;
    }
    pshader->Initialize();
    pshader->LoadShaders();
    pshader->LoadStagesAndPasses();
    pshader->CreateConstantMaps();
    pshader->SetShaderDecl(spShaderDecl);
    return pshader;
}
