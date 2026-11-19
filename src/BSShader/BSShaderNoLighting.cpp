#include "BSShader/BSShaderNoLighting.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffectState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

NiRTTI BSShaderNoLighting::ms_RTTI("BSShaderNoLighting", &NiD3DDefaultShader::ms_RTTI);
BSShaderNoLighting::SubTexOffsetArray BSShaderNoLighting::aSubTexOffsets;

// Shader type 22 holding the twelve declarations; the shared material colour goes back to white.
BSShaderNoLighting::BSShaderNoLighting(
    NiDX9ShaderDeclaration* apDecl,
    NiDX9ShaderDeclaration* apDeclVC,
    NiDX9ShaderDeclaration* apDeclTexVC,
    NiDX9ShaderDeclaration* apDeclN,
    NiDX9ShaderDeclaration* apDeclVCN,
    NiDX9ShaderDeclaration* apDeclTexVCN,
    NiDX9ShaderDeclaration* apSkinDecl,
    NiDX9ShaderDeclaration* apSkinDeclVC,
    NiDX9ShaderDeclaration* apSkinDeclTexVC,
    NiDX9ShaderDeclaration* apSkinDeclN,
    NiDX9ShaderDeclaration* apSkinDeclVCN,
    NiDX9ShaderDeclaration* apSkinDeclTexVCN
)
  : spVertexShader{}, spPixelShader{}, pParticleSubTextureOffsetEntry(nullptr), spDecl(apDecl), spDeclVc(apDeclVC),
    spDeclTexVc(apDeclTexVC), spDeclN(apDeclN), spDeclVcN(apDeclVCN), spDeclTexVcN(apDeclTexVCN),
    spSkinDecl(apSkinDecl), spSkinDeclVc(apSkinDeclVC), spSkinDeclTexVc(apSkinDeclTexVC), spSkinDeclN(apSkinDeclN),
    spSkinDeclVcN(apSkinDeclVCN), spSkinDeclTexVcN(apSkinDeclTexVCN) {
    iShaderType = 22;
    for (NiDX9ShaderDeclaration* pDecl :
         {spDecl, spDeclVc, spDeclTexVc, spDeclN, spDeclVcN, spDeclTexVcN, spSkinDecl, spSkinDeclVc, spSkinDeclTexVc,
          spSkinDeclN, spSkinDeclVcN, spSkinDeclTexVcN}) {
        if (pDecl) {
            NiIncRefCount(pDecl);
        }
    }
    BSShaderNoLighting::kMaterialColor = NiColorA_WHITE;
}

// The passes go back to the pass pool at their last reference; the shaders and the declarations are released.
BSShaderNoLighting::~BSShaderNoLighting() {
    for (NiPointer<NiD3DPass>& rspPass : BSShaderNoLighting::spPass) {
        NiD3DPass* pPass = rspPass.m_pObject;
        if (pPass) {
            if (pPass->m_uiRefCount-- == 1) {
                pPass->ReturnPassToPool();
            }
            rspPass.m_pObject = nullptr;
        }
    }
    for (NiD3DVertexShader*& rpShader : spVertexShader) {
        if (rpShader) {
            NiDecRefCount(rpShader);
            rpShader = nullptr;
        }
    }
    for (NiD3DPixelShader*& rpShader : spPixelShader) {
        if (rpShader) {
            NiDecRefCount(rpShader);
            rpShader = nullptr;
        }
    }
    NiDX9ShaderDeclaration** apDecls[] = {&spDecl,          &spDeclVc,     &spDeclTexVc,   &spDeclN,
                                          &spDeclVcN,       &spDeclTexVcN, &spSkinDecl,    &spSkinDeclVc,
                                          &spSkinDeclTexVc, &spSkinDeclN,  &spSkinDeclVcN, &spSkinDeclTexVcN};
    for (NiDX9ShaderDeclaration** ppDecl : apDecls) {
        if (*ppDecl) {
            NiDecRefCount(*ppDecl);
            *ppDecl = nullptr;
        }
    }
    pParticleSubTextureOffsetEntry = nullptr;
}

const NiRTTI* BSShaderNoLighting::GetRTTI() const {
    return &BSShaderNoLighting::ms_RTTI;
}

bool BSShaderNoLighting::Initialize() {
    return BSShader::Initialize();
}

uint32_t BSShaderNoLighting::SetupBoneMatrices(
    NiGeometry* a1,
    const NiSkinInstance* a2,
    const NiSkinPartition::Partition* a3,
    NiGeometryData::RendererData* a4,
    const NiPropertyState* a5,
    const NiDynamicEffectState* a6,
    const NiTransform& a7,
    const NiBound& a8
) {
    if (a3) {
        BSShaderManager::pRenderer.m_pObject->CalculateBoneMatrices(a2, a7, false, 3u, true);
        NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject->SetShaderConstantArray(
            pBonesEntry, a2->m_pvBoneMatrices, a3->m_usBones, a2->m_uiNumRegisters, a3->m_pusBones
        );
    }
    return 0;
}

void BSShaderNoLighting::SetupGeometryTextures(const NiPropertyState* apState) {
    NiD3DTextureStage* pstage = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase->m_pObject;
    if (pstage) {
        BSShaderNoLightingProperty* pprop = static_cast<BSShaderNoLightingProperty*>(apState->m_aspProps[3].m_pObject);
        NiTexture* ptex = pprop->GetBaseTexture();
        if (ptex) {
            pstage->m_spTexture = ptex;
            NiTexturingProperty::ClampMode v5 = pprop->GetClampNI();
            pstage->SetClampMode(v5);
            pstage->ConfigureStage(false);
        }
    }
}

// Sets the model transform (identity at PosAdjust for skinned geometry), the particle, texture transform, water fog
// and material colour constants, the falloff and blend toggles, applies both constant maps, then the z-buffer flags
// and the stencil property.
void BSShaderNoLighting::SetupGeometryConstants(const NiPropertyState* apState) {
    NiTransform xform;
    NiGeometry* pgeometry = BSShaderManager::pCurrentRenderPass->pGeometry;
    BSShaderManager::eBSSMPassEnum uiPassType = BSShaderManager::eCurrentPass;
    const NiTransform* worldTransform = &BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorld;
    BSNiBound* pBound = BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorldBound;
    if (!pBound) {
        pBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
    }
    const NiSkinInstance* pskin = pgeometry->m_spSkinInstance.m_pObject;
    NiGeometryBufferData* pbufferData = pgeometry->m_spModelData.m_pObject->m_pkBuffData;
    NiMaterialProperty* pmaterialProperty = static_cast<NiMaterialProperty*>(apState->m_aspProps[2].m_pObject);
    BSShaderNoLightingProperty* pprop = static_cast<BSShaderNoLightingProperty*>(apState->m_aspProps[3].m_pObject);
    if (pskin) {
        xform.MakeIdentity();
        xform.m_Translate = NiRenderer::PosAdjust;
        worldTransform = &xform;
    }
    m_pkD3DRenderer->SetModelTransform(*worldTransform, false);
    if (uiPassType >= BSShaderManager::BSSM_NOLIGHTING_PSYS &&
        uiPassType <= BSShaderManager::BSSM_NOLIGHTING_STRIP_PSYS_SUBTEX) {
        SetupGeometryConstants_Particles(pgeometry, pgeometry->m_kWorld);
    }
    SetupGeometryConstants_TextureTransform(apState);
    SetupGeometryConstants_WaterFog();
    SetupGeometryConstants_MaterialColor(pprop, pmaterialProperty);
    // The strip particle passes take the property's falloff only when its falloff flag is set; every other pass takes
    // it always.
    if ((uiPassType != BSShaderManager::BSSM_NOLIGHTING_STRIP_PSYS &&
         uiPassType != BSShaderManager::BSSM_NOLIGHTING_STRIP_PSYS_SUBTEX) ||
        (pprop->ulFlags[0] & BSS1_Falloff) != 0) {
        BSShaderNoLighting::FalloffData.x = pprop->FalloffData.fFalloffStartAngle;
        BSShaderNoLighting::FalloffData.y = pprop->FalloffData.fFalloffStopAngle;
        BSShaderNoLighting::FalloffData.z = pprop->FalloffData.fFalloffStartOpacity;
        BSShaderNoLighting::FalloffData.w = pprop->FalloffData.fFalloffStopOpacity;
    }
    else {
        BSShaderNoLighting::FalloffData.x = 0.0f;
        BSShaderNoLighting::FalloffData.y = 0.0f;
        BSShaderNoLighting::FalloffData.z = 0.0f;
        BSShaderNoLighting::FalloffData.w = 0.0f;
    }
    NiD3DShaderConstantMap* pVertexMap = m_spVertexConstantMap.m_pObject;
    if (pVertexMap) {
        NiD3DVertexShader* pVertexShader = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
        if (pVertexShader) {
            pVertexMap->SetShaderConstants(
                pVertexShader, pgeometry, pskin, nullptr, pbufferData, apState, nullptr, pgeometry->m_kWorld, *pBound,
                0, true
            );
        }
    }
    BSShaderNoLighting::Toggles.x = 0.0f;
    BSShaderNoLighting::Toggles.y = 0.0f;
    NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apState->m_aspProps[0].m_pObject);
    if (pAlpha) {
        uint32_t uiDestBlend = (pAlpha->m_uFlags & NiAlphaProperty::DEST_BLEND_MASK) >> NiAlphaProperty::DEST_BLEND_POS;
        if (uiDestBlend == NiAlphaProperty::ALPHA_ONE) {
            BSShaderNoLighting::Toggles.x = 1.0f;
        }
        else if (uiDestBlend == NiAlphaProperty::ALPHA_SRCCOLOR) {
            BSShaderNoLighting::Toggles.y = 1.0f;
        }
    }
    NiD3DShaderConstantMap* pPixelMap = m_spPixelConstantMap.m_pObject;
    if (pPixelMap) {
        NiD3DPixelShader* pPixelShader = NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject;
        if (pPixelShader) {
            pPixelMap->SetShaderConstants(
                pPixelShader, pgeometry, pskin, nullptr, pbufferData, apState, nullptr, pgeometry->m_kWorld, *pBound, 0,
                true
            );
        }
    }
    if ((pprop->ulFlags[1] & BSS2_ZBuffer_Write) == 0) {
        BSRenderState::SetZWriteEnable(false, BSRS_NOLOCK);
    }
    if ((pprop->ulFlags[0] & BSS1_ZBuffer_Test) == 0) {
        BSRenderState::SetZEnable(false, BSRS_NOLOCK);
    }
    NiStencilProperty* pStencil = static_cast<NiStencilProperty*>(apState->m_aspProps[4].m_pObject);
    if ((pStencil) && ((pStencil->m_uFlags & 1) != 0)) {
        m_pkD3DRenderState->ApplyStencil(pStencil);
    }
}

void BSShaderNoLighting::SetupGeometryAlphaBlending(const NiPropertyState* apState) {
    // The alpha property's blend flag is bit 0 of its flags word (0x18).
    NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apState->m_aspProps[0].m_pObject);
    if ((pAlpha && (pAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK) != 0) ||
        static_cast<uint32_t>(BSShaderManager::eCurrentPass - BSShaderManager::BSSM_NOLIGHTING_FALLOFF) >
            BSShaderManager::BSSM_NOLIGHTING_MAT_FALLOFF_S - BSShaderManager::BSSM_NOLIGHTING_FALLOFF) {
        BSShader::SetupGeometryAlphaBlending(apState);
    }
    else {
        BSRenderState::SetAlphaBlendEnable(true, BSRS_NOLOCK);
        BSRenderState::SetAlphaBlendFunc(
            NiAlphaProperty::ALPHA_SRCALPHA, NiAlphaProperty::ALPHA_INVSRCALPHA, BSRS_NOLOCK
        );
    }
}

void BSShaderNoLighting::PostGeometry(const NiPropertyState* apState) {
    BSShaderProperty* pShaderProp = static_cast<BSShaderProperty*>(apState->m_aspProps[3].m_pObject);
    if ((pShaderProp) && ((pShaderProp->ulFlags[1] & BSS2_Wireframe) != 0)) {
        BSRenderState::SetWireframeEnable(false, BSRS_NOLOCK);
    }

    // The property is tested for null only around the wireframe flag, and its flags below are read unguarded, as the
    // game does.
    if ((pShaderProp->ulFlags[1] & BSS2_ZBuffer_Write) == 0) {
        BSRenderState::SetZWriteEnable(true, BSRS_NOLOCK);
    }
    if ((pShaderProp->ulFlags[0] & BSS1_ZBuffer_Test) == 0) {
        BSRenderState::SetZEnable(true, BSRS_NOLOCK);
    }
    NiStencilProperty* pStencil = static_cast<NiStencilProperty*>(apState->m_aspProps[4].m_pObject);
    if ((pStencil) && ((pStencil->m_uFlags & NiStencilProperty::ENABLE_MASK) != 0)) {
        BSRenderState::SetStencilEnable(false, BSRS_NOLOCK);
    }
}

void BSShaderNoLighting::InitPass(uint32_t auiCurrentPass) {
    if (auiCurrentPass > 732) {
        switch (auiCurrentPass) {
            case 733u: {
                NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[26];
                m_spShaderDecl.SetNew(spSkinDecl);
                break;
            }
            case 738u: {
                NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[23];
                m_spShaderDecl.SetNew(spDecl);
                break;
            }
            case 739u: {
                NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[27];
                m_spShaderDecl.SetNew(spSkinDecl);
                break;
            }
            case 744u: {
                NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[24];
                m_spShaderDecl.SetNew(spDeclVc);
                break;
            }
            case 745u: {
                NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[28];
                m_spShaderDecl.SetNew(spSkinDeclVc);
                break;
            }
            case 750u: {
                NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[25];
                m_spShaderDecl.SetNew(spDeclTexVc);
                break;
            }
            case 751u: {
                NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[29];
                m_spShaderDecl.SetNew(spSkinDeclTexVc);
                break;
            }
            default: {
                return;
            }
        }
    }
    else {
        if (auiCurrentPass == 732) {
            NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[22];
            m_spShaderDecl.SetNew(spDecl);
        }
        else {
            NiD3DPass* v3;
            NiD3DShaderDeclaration* m_pObject;
            NiD3DPass* v5;
            switch (auiCurrentPass) {
                case 66u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[0];
                    m_spShaderDecl.SetNew(spDecl);
                    return;
                }
                case 67u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[2];
                    m_spShaderDecl.SetNew(spDeclVc);
                    return;
                }
                case 68u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[1];
                    m_spShaderDecl.SetNew(spDeclTexVc);
                    return;
                }
                case 69u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[3];
                    m_spShaderDecl.SetNew(spDecl);
                    return;
                }
                case 70u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[8];
                    m_spShaderDecl.SetNew(spSkinDecl);
                    return;
                }
                case 71u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[10];
                    m_spShaderDecl.SetNew(spSkinDeclVc);
                    return;
                }
                case 72u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[9];
                    m_spShaderDecl.SetNew(spSkinDeclTexVc);
                    return;
                }
                case 73u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[11];
                    m_spShaderDecl.SetNew(spSkinDecl);
                    return;
                }
                case 74u: {
                    v3 = BSShaderNoLighting::spPass[4];
                    goto LABEL_9;
                }
                case 75u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[6];
                    m_spShaderDecl.SetNew(spDeclVcN);
                    return;
                }
                case 76u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[5];
                    m_spShaderDecl.SetNew(spDeclTexVcN);
                    return;
                }
                case 77u: {
                    v3 = BSShaderNoLighting::spPass[7];
                LABEL_9:;
                    NiD3DShader::m_spCurrentPass = v3;
                    m_spShaderDecl.SetNew(spDeclN);
                    return;
                }
                case 78u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[12];
                    m_spShaderDecl.SetNew(spSkinDeclN);
                    return;
                }
                case 79u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[14];
                    m_spShaderDecl.SetNew(spSkinDeclVcN);
                    return;
                }
                case 80u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[13];
                    m_spShaderDecl.SetNew(spSkinDeclTexVcN);
                    return;
                }
                case 81u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[15];
                    m_spShaderDecl.SetNew(spSkinDeclN);
                    return;
                }
                case 82u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[16];
                    m_pObject = m_spShaderDecl.m_pObject;
                    if (m_pObject) {
                        goto LABEL_34;
                    }
                    return;
                }
                case 83u: {
                    v5 = BSShaderNoLighting::spPass[17];
                    goto LABEL_24;
                }
                case 84u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[18];
                    m_pObject = m_spShaderDecl.m_pObject;
                    if (!m_pObject) {
                        return;
                    }
                    if (!InterlockedDecrementUInt32(&m_pObject->m_uiRefCount)) {
                        goto LABEL_31;
                    }
                    goto LABEL_27;
                }
                case 85u: {
                    v5 = BSShaderNoLighting::spPass[19];
                    goto LABEL_24;
                }
                case 86u: {
                    NiD3DShader::m_spCurrentPass = BSShaderNoLighting::spPass[20];
                    m_pObject = m_spShaderDecl.m_pObject;
                    if (!m_pObject) {
                        return;
                    }
                LABEL_34:;
                    if (InterlockedDecrementUInt32(&m_pObject->m_uiRefCount)) {
                        goto LABEL_27;
                    }
                LABEL_31:;
                    m_pObject->DeleteThis();
                    m_spShaderDecl.m_pObject = nullptr;
                    break;
                }
                case 87u: {
                    NiD3DShaderDeclaration* v6;
                    v5 = BSShaderNoLighting::spPass[21];
                LABEL_24:;
                    NiD3DShader::m_spCurrentPass = v5;
                    v6 = m_spShaderDecl.m_pObject;
                    if (!v6) {
                        return;
                    }
                    NiDecRefCount(v6);
                LABEL_27:;
                    m_spShaderDecl.m_pObject = nullptr;
                    break;
                }
                default: {
                    return;
                }
            }
        }
    }
}

void BSShaderNoLighting::SetupPassConstants(uint32_t auiCurrentPass) {
    NiShaderConstantMapEntry* pParticleSubTextureOffsetEntry = this->pParticleSubTextureOffsetEntry;
    pParticleSubTextureOffsetEntry->m_bEnabled = auiCurrentPass - 82 <= 5;
    pWorldViewEntry->m_bEnabled =
        (auiCurrentPass >= 74 && auiCurrentPass <= 81) || (auiCurrentPass == 86 || auiCurrentPass == 87);
    BSFogProperty* pbsfog = BSShaderManager::GetFogProperty(BSShaderManager::cSceneGraph);
    if (pbsfog) {
        float distNear = pbsfog->fStartDistance;
        float r = pbsfog->m_kColor.r;
        float distFar = pbsfog->fEndDistance;
        float g = pbsfog->m_kColor.g;
        float a2 = pbsfog->fPower;
        float b = pbsfog->m_kColor.b;
        if (distFar == 0.0f && 0.0f == distNear) {
            BSShaderNoLighting::pFogPropA[0].r = 500000.0f;
            BSShaderNoLighting::pFogPropA[0].g = 0.0f;
            BSShaderNoLighting::pFogPropA[0].b = a2;
            BSShaderNoLighting::pFogPropA[0].a = 0.0f;
            BSShaderNoLighting::pFogPropA[1] = NiColorA_BLACK;
        }
        else {
            BSShaderNoLighting::pFogPropA[0].r = pbsfog->fEndDistance;
            float v9 = distFar - distNear;
            BSShaderNoLighting::pFogPropA[0].g = v9;
            BSShaderNoLighting::pFogPropA[0].b = a2;
            BSShaderNoLighting::pFogPropA[0].a = 0.0f;
            BSShaderNoLighting::pFogPropA[1].r = r;
            BSShaderNoLighting::pFogPropA[1].g = g;
            BSShaderNoLighting::pFogPropA[1].b = b;
            BSShaderNoLighting::pFogPropA[1].a = 1.0f;
        }
    }
}

// The property is a BSShaderNoLightingProperty: its base map is tested for the texture variants.
NiD3DShaderDeclaration* BSShaderNoLighting::GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) {
    uint32_t v3 = apProperty->ulFlags[0];
    BSShaderNoLightingProperty* pNoLightingProp = static_cast<BSShaderNoLightingProperty*>(apProperty);
    if ((v3 & 64) != 0) {
        if ((apProperty->ulFlags[1] & 32) != 0) {
            if (pNoLightingProp->spBaseMap.m_pObject) {
                if ((v3 & 2) != 0) {
                    return spSkinDeclTexVcN;
                }

                return spDeclTexVcN;
            }

            if ((v3 & 2) != 0) {
                return spSkinDeclVcN;
            }

            return spDeclVcN;
        }

        if ((v3 & 2) != 0) {
            return spSkinDeclN;
        }

        return spDeclN;
    }

    if ((apProperty->ulFlags[1] & 32) != 0) {
        if (pNoLightingProp->spBaseMap.m_pObject) {
            if ((v3 & 2) != 0) {
                return spSkinDeclTexVc;
            }

            return spDeclTexVc;
        }

        if ((v3 & 2) != 0) {
            return spSkinDeclVc;
        }

        return spDeclVc;
    }

    if ((v3 & 2) != 0) {
        return spSkinDecl;
    }

    return spDecl;
}

// Creates the pixel map (material colour and alpha, fog colour, water fog) and the vertex map (transforms, texture
// transform, fog, falloff, sub-texture offsets, bones) when they are missing, and keeps the entries it looks up. The
// variable names are the empty string.
void BSShaderNoLighting::CreateConstantMaps() {
    if (!m_spPixelConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        m_spPixelConstantMap = pMap;
        m_spPixelConstantMap.m_pObject->AddEntry("matcolor", 0x10000007, 0, 0, 1, "", 0x10, 4, &kMaterialColor, false);
        m_spPixelConstantMap.m_pObject->AddEntry("matalpha", 0x10000007, 0, 1, 1, "", 0x10, 4, &Toggles, false);
        m_spPixelConstantMap.m_pObject->AddEntry("fogcolor", 0x10000007, 0, 2, 1, "", 0x10, 4, &pFogPropA[1], false);
        m_spPixelConstantMap.m_pObject->AddEntry("waterfog", 0x10000009, 0, 5, 2, "", 0x20, 4, pWaterFogA, false);
    }
    if (!m_spVertexConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        m_spVertexConstantMap = pMap;
        m_spVertexConstantMap.m_pObject->AddEntry(
            "WorldViewProjTranspose", 0x20000009, 0, 0, 4, nullptr, 0, 0, nullptr, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "WorldViewTranspose", 0x20000009, 0, 0x24, 4, nullptr, 0, 0, nullptr, false
        );
        pWorldViewEntry = m_spVertexConstantMap.m_pObject->GetEntry("WorldViewTranspose");
        m_spVertexConstantMap.m_pObject->AddEntry("WorldTranspose", 0x20000009, 0, 8, 4, nullptr, 0, 0, nullptr, false);
        pWorldEntry = m_spVertexConstantMap.m_pObject->GetEntry("WorldTranspose");
        m_spVertexConstantMap.m_pObject->AddEntry("EyeDir", 0x20000009, 0, 0x29, 4, nullptr, 0, 0, nullptr, false);
        pWorldEyeDirEntry = m_spVertexConstantMap.m_pObject->GetEntry("EyeDir");
        m_spVertexConstantMap.m_pObject->AddEntry(
            "TextureTransform", 0x10000008, 0, 4, 3, "", 0x30, 4, pTextureTransform, false
        );
        m_spVertexConstantMap.m_pObject->AddEntry("fog", 0x10000009, 0, 0xD, 2, "", 0x20, 4, pFogPropA, false);
        m_spVertexConstantMap.m_pObject->AddEntry("falloff", 0x10000007, 0, 0x28, 1, "", 0x10, 4, &FalloffData, false);
        m_spVertexConstantMap.m_pObject->AddEntry(
            "subtex", 0x10000009, 0, 0xF, 0x10, "", 0x100, 4, aSubTexOffsets, false
        );
        pParticleSubTextureOffsetEntry = m_spVertexConstantMap.m_pObject->GetEntry("subtex");
        m_spVertexConstantMap.m_pObject->AddEntry(
            "BoneMatrix3", 0x20000009, 0x120000, 0x40, 0x36, nullptr, 0, 0, nullptr, false
        );
        pBonesEntry = m_spVertexConstantMap.m_pObject->GetEntry("BoneMatrix3");
    }
}

void BSShaderNoLighting::ReloadShaders() {
    LoadShaders();
    LoadStagesAndPasses();
}

void BSShaderNoLighting::ClearPassStages() {
    for (uint32_t i = 0; i < 30; ++i) {
        ClearPass(spPass[i]);
    }
}

// 3 is D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN.
bool BSShaderNoLighting::LoadStagesAndPasses() {
    NiD3DPass::CreateNewPass(spPass[0]);
    NiPointer<NiD3DTextureStage> spStage;
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[0]->SetStage(spPass[0]->m_uiCurrentStage, spStage.m_pObject);
    spPass[0]->m_spVertexShader = spVertexShader[0];
    spPass[0]->m_spPixelShader = spPixelShader[0];

    NiD3DPass::CreateNewPass(spPass[1]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[1]->SetStage(spPass[1]->m_uiCurrentStage, spStage.m_pObject);
    spPass[1]->m_spVertexShader = spVertexShader[2];
    spPass[1]->m_spPixelShader = spPixelShader[2];

    NiD3DPass::CreateNewPass(spPass[2]);
    spPass[2]->m_spVertexShader = spVertexShader[1];
    spPass[2]->m_spPixelShader = spPixelShader[1];

    NiD3DPass::CreateNewPass(spPass[3]);
    spPass[3]->m_spVertexShader = spVertexShader[3];
    spPass[3]->m_spPixelShader = spPixelShader[4];

    NiD3DPass::CreateNewPass(spPass[4]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[4]->SetStage(spPass[4]->m_uiCurrentStage, spStage.m_pObject);
    spPass[4]->m_spVertexShader = spVertexShader[4];
    spPass[4]->m_spPixelShader = spPixelShader[0];

    NiD3DPass::CreateNewPass(spPass[5]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[5]->SetStage(spPass[5]->m_uiCurrentStage, spStage.m_pObject);
    spPass[5]->m_spVertexShader = spVertexShader[6];
    spPass[5]->m_spPixelShader = spPixelShader[2];

    NiD3DPass::CreateNewPass(spPass[6]);
    spPass[6]->m_spVertexShader = spVertexShader[5];
    spPass[6]->m_spPixelShader = spPixelShader[1];

    NiD3DPass::CreateNewPass(spPass[7]);
    spPass[7]->m_spVertexShader = spVertexShader[7];
    spPass[7]->m_spPixelShader = spPixelShader[4];

    NiD3DPass::CreateNewPass(spPass[8]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[8]->SetStage(spPass[8]->m_uiCurrentStage, spStage.m_pObject);
    spPass[8]->m_spVertexShader = spVertexShader[8];
    spPass[8]->m_spPixelShader = spPixelShader[0];

    NiD3DPass::CreateNewPass(spPass[9]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[9]->SetStage(spPass[9]->m_uiCurrentStage, spStage.m_pObject);
    spPass[9]->m_spVertexShader = spVertexShader[10];
    spPass[9]->m_spPixelShader = spPixelShader[2];

    NiD3DPass::CreateNewPass(spPass[10]);
    spPass[10]->m_spVertexShader = spVertexShader[9];
    spPass[10]->m_spPixelShader = spPixelShader[1];

    NiD3DPass::CreateNewPass(spPass[11]);
    spPass[11]->m_spVertexShader = spVertexShader[11];
    spPass[11]->m_spPixelShader = spPixelShader[4];

    NiD3DPass::CreateNewPass(spPass[12]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[12]->SetStage(spPass[12]->m_uiCurrentStage, spStage.m_pObject);
    spPass[12]->m_spVertexShader = spVertexShader[12];
    spPass[12]->m_spPixelShader = spPixelShader[0];

    NiD3DPass::CreateNewPass(spPass[13]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[13]->SetStage(spPass[13]->m_uiCurrentStage, spStage.m_pObject);
    spPass[13]->m_spVertexShader = spVertexShader[14];
    spPass[13]->m_spPixelShader = spPixelShader[2];

    NiD3DPass::CreateNewPass(spPass[14]);
    spPass[14]->m_spVertexShader = spVertexShader[13];
    spPass[14]->m_spPixelShader = spPixelShader[1];

    NiD3DPass::CreateNewPass(spPass[15]);
    spPass[15]->m_spVertexShader = spVertexShader[15];
    spPass[15]->m_spPixelShader = spPixelShader[4];

    NiD3DPass::CreateNewPass(spPass[16]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[16]->SetStage(spPass[16]->m_uiCurrentStage, spStage.m_pObject);
    spPass[16]->m_spVertexShader = spVertexShader[16];
    spPass[16]->m_spPixelShader = spPixelShader[2];

    NiD3DPass::CreateNewPass(spPass[17]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[17]->SetStage(spPass[17]->m_uiCurrentStage, spStage.m_pObject);
    spPass[17]->m_spVertexShader = spVertexShader[17];
    spPass[17]->m_spPixelShader = spPixelShader[2];

    NiD3DPass::CreateNewPass(spPass[18]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[18]->SetStage(spPass[18]->m_uiCurrentStage, spStage.m_pObject);
    spPass[18]->m_spVertexShader = spVertexShader[16];
    spPass[18]->m_spPixelShader = spPixelShader[3];

    NiD3DPass::CreateNewPass(spPass[19]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[19]->SetStage(spPass[19]->m_uiCurrentStage, spStage.m_pObject);
    spPass[19]->m_spVertexShader = spVertexShader[17];
    spPass[19]->m_spPixelShader = spPixelShader[3];

    NiD3DPass::CreateNewPass(spPass[20]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[20]->SetStage(spPass[20]->m_uiCurrentStage, spStage.m_pObject);
    spPass[20]->m_spVertexShader = spVertexShader[18];
    spPass[20]->m_spPixelShader = spPixelShader[2];

    NiD3DPass::CreateNewPass(spPass[21]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[21]->SetStage(spPass[21]->m_uiCurrentStage, spStage.m_pObject);
    spPass[21]->m_spVertexShader = spVertexShader[19];
    spPass[21]->m_spPixelShader = spPixelShader[2];

    NiD3DPass::CreateNewPass(spPass[22]);
    spPass[22]->m_spVertexShader = spVertexShader[20];
    spPass[22]->m_spPixelShader = spPixelShader[5];
    spPass[22]->SetRenderState(D3DRS_COLORWRITEENABLE, 3u, true);

    NiD3DPass::CreateNewPass(spPass[23]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[23]->SetStage(spPass[23]->m_uiCurrentStage, spStage.m_pObject);
    spPass[23]->m_spVertexShader = spVertexShader[21];
    spPass[23]->m_spPixelShader = spPixelShader[6];
    spPass[23]->SetRenderState(D3DRS_COLORWRITEENABLE, 3u, true);

    NiD3DPass::CreateNewPass(spPass[24]);
    spPass[24]->m_spVertexShader = spVertexShader[22];
    spPass[24]->m_spPixelShader = spPixelShader[5];
    spPass[24]->SetRenderState(D3DRS_COLORWRITEENABLE, 3u, true);

    NiD3DPass::CreateNewPass(spPass[25]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[25]->SetStage(spPass[25]->m_uiCurrentStage, spStage.m_pObject);
    spPass[25]->m_spVertexShader = spVertexShader[23];
    spPass[25]->m_spPixelShader = spPixelShader[6];
    spPass[25]->SetRenderState(D3DRS_COLORWRITEENABLE, 3u, true);

    NiD3DPass::CreateNewPass(spPass[26]);
    spPass[26]->m_spVertexShader = spVertexShader[24];
    spPass[26]->m_spPixelShader = spPixelShader[5];
    spPass[26]->SetRenderState(D3DRS_COLORWRITEENABLE, 3u, true);

    NiD3DPass::CreateNewPass(spPass[27]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[27]->SetStage(spPass[27]->m_uiCurrentStage, spStage.m_pObject);
    spPass[27]->m_spVertexShader = spVertexShader[25];
    spPass[27]->m_spPixelShader = spPixelShader[6];
    spPass[27]->SetRenderState(D3DRS_COLORWRITEENABLE, 3u, true);

    NiD3DPass::CreateNewPass(spPass[28]);
    spPass[28]->m_spVertexShader = spVertexShader[26];
    spPass[28]->m_spPixelShader = spPixelShader[5];
    spPass[28]->SetRenderState(D3DRS_COLORWRITEENABLE, 3u, true);

    NiD3DPass::CreateNewPass(spPass[29]);
    NiD3DTextureStage::CreateNewStage(spStage);
    BSShader::SetupDefaultStage(spStage.m_pObject, 0, 3u, 2u, true);
    spPass[29]->SetStage(spPass[29]->m_uiCurrentStage, spStage.m_pObject);
    spPass[29]->m_spVertexShader = spVertexShader[27];
    spPass[29]->m_spPixelShader = spPixelShader[6];
    spPass[29]->SetRenderState(D3DRS_COLORWRITEENABLE, 3u, true);
    return true;
}

void BSShaderNoLighting::LoadShaders() {
    BSShaderMacro v31;
    BSShaderMacro pixelShaderParamsTexVC;
    BSShaderMacro v33;
    BSShaderMacro pixelShaderParamsWaterFogAt;
    BSShaderMacro pixelShaderParamsVCOnly;
    // The 28 vertex shader macro sets are one frame array the compile loop walks: 16 lighting, 4 particle system,
    // then 8 water fog sets.
    BSShaderMacro v36[28];
    BSShaderMacro* v37 = &v36[16];
    BSShaderMacro* v38 = &v36[20];
    BSShaderMacro pixelShaderParamsWaterFog;
    BSShaderMacro pixelShaderParamsMat;
    char Str1[260];
    char apDest[260];
    v36[0].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[0].kMacros[0].Name = "TEX";
    v36[0].kMacros[0].Definition = "";
    v36[0].kMacros[1].Name = nullptr;
    std::memset(
        &v36[0].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    v36[1].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[1].kMacros[0].Name = "VC";
    v36[1].kMacros[0].Definition = "";
    v36[1].kMacros[1].Name = nullptr;
    std::memset(
        &v36[1].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    v36[2].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[2].kMacros[0].Name = "TEX";
    v36[2].kMacros[0].Definition = "";
    v36[2].kMacros[1].Name = "VC";
    v36[2].kMacros[1].Definition = "";
    v36[2].kMacros[2].Name = nullptr;
    std::memset(
        &v36[2].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    v36[3].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[3].kMacros[0].Name = nullptr;
    std::memset(
        &v36[3].kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    v36[4].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[4].kMacros[0].Name = "TEX";
    v36[4].kMacros[0].Definition = "";
    v36[4].kMacros[1].Name = "FALLOFF";
    v36[4].kMacros[1].Definition = "";
    v36[4].kMacros[2].Name = nullptr;
    std::memset(
        &v36[4].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    v36[5].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[5].kMacros[0].Name = "VC";
    v36[5].kMacros[0].Definition = "";
    v36[5].kMacros[1].Name = "FALLOFF";
    v36[5].kMacros[1].Definition = "";
    v36[5].kMacros[2].Name = nullptr;
    std::memset(
        &v36[5].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    v36[6].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[6].kMacros[0].Name = "TEX";
    v36[6].kMacros[0].Definition = "";
    v36[6].kMacros[1].Name = "VC";
    v36[6].kMacros[1].Definition = "";
    v36[6].kMacros[2].Name = "FALLOFF";
    v36[6].kMacros[2].Definition = "";
    v36[6].kMacros[3].Name = nullptr;
    std::memset(
        &v36[6].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    v36[7].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[7].kMacros[0].Name = "FALLOFF";
    v36[7].kMacros[0].Definition = "";
    v36[7].kMacros[1].Name = nullptr;
    std::memset(
        &v36[7].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    v36[8].kMacros[0].Name = "TEX";
    v36[8].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[8].kMacros[0].Definition = "";
    v36[8].kMacros[1].Name = "SKIN";
    v36[8].kMacros[1].Definition = "";
    v36[8].kMacros[2].Name = nullptr;
    std::memset(
        &v36[8].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    v36[9].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[9].kMacros[0].Name = "VC";
    v36[9].kMacros[0].Definition = "";
    v36[9].kMacros[1].Name = "SKIN";
    v36[9].kMacros[1].Definition = "";
    v36[9].kMacros[2].Name = nullptr;
    std::memset(
        &v36[9].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    v36[10].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[10].kMacros[0].Name = "TEX";
    v36[10].kMacros[0].Definition = "";
    v36[10].kMacros[1].Name = "VC";
    v36[10].kMacros[1].Definition = "";
    v36[10].kMacros[2].Name = "SKIN";
    v36[10].kMacros[2].Definition = "";
    v36[10].kMacros[3].Name = nullptr;
    std::memset(
        &v36[10].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    v36[11].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[11].kMacros[0].Name = "SKIN";
    v36[11].kMacros[0].Definition = "";
    v36[11].kMacros[1].Name = nullptr;
    std::memset(
        &v36[11].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    v36[12].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[12].kMacros[0].Name = "TEX";
    v36[12].kMacros[0].Definition = "";
    v36[12].kMacros[1].Name = "FALLOFF";
    v36[12].kMacros[1].Definition = "";
    v36[12].kMacros[2].Name = "SKIN";
    v36[12].kMacros[2].Definition = "";
    v36[12].kMacros[3].Name = nullptr;
    std::memset(
        &v36[12].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    v36[13].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[13].kMacros[0].Name = "VC";
    v36[13].kMacros[0].Definition = "";
    v36[13].kMacros[1].Name = "FALLOFF";
    v36[13].kMacros[1].Definition = "";
    v36[13].kMacros[2].Name = "SKIN";
    v36[13].kMacros[2].Definition = "";
    v36[13].kMacros[3].Name = nullptr;
    std::memset(
        &v36[13].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    v36[14].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[14].kMacros[0].Name = "TEX";
    v36[14].kMacros[0].Definition = "";
    v36[14].kMacros[1].Name = "VC";
    v36[14].kMacros[1].Definition = "";
    v36[14].kMacros[2].Name = "FALLOFF";
    v36[14].kMacros[2].Definition = "";
    v36[14].kMacros[3].Name = "SKIN";
    v36[14].kMacros[3].Definition = "";
    v36[14].kMacros[4].Name = nullptr;
    v36[14].kMacros[4].Definition = nullptr;
    v36[14].kMacros[5].Name = nullptr;
    v36[14].kMacros[5].Definition = nullptr;
    v36[14].kMacros[6].Name = nullptr;
    v36[14].kMacros[6].Definition = nullptr;
    v36[14].kMacros[7].Name = nullptr;
    v36[14].kMacros[7].Definition = nullptr;
    v36[14].kMacros[8].Name = nullptr;
    v36[14].kMacros[8].Definition = nullptr;
    v36[15].strPath = "lighting\\1x\\v\\NoLighting.v.hlsl";
    v36[15].kMacros[0].Name = "FALLOFF";
    v36[15].kMacros[0].Definition = "";
    v36[15].kMacros[1].Name = "SKIN";
    v36[15].kMacros[1].Definition = "";
    v36[15].kMacros[2].Name = nullptr;
    std::memset(
        &v36[15].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    v37[0].strPath = "lighting\\1x\\v\\particlesystem.v.hlsl";
    v37[0].kMacros[0].Name = nullptr;
    std::memset(
        &v37[0].kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    v37[1].strPath = "lighting\\1x\\v\\particlesystem.v.hlsl";
    v37[1].kMacros[0].Name = "PSYS_SUBTEX";
    v37[1].kMacros[0].Definition = "";
    v37[1].kMacros[1].Name = nullptr;
    std::memset(
        &v37[1].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    v37[2].strPath = "lighting\\1x\\v\\particlesystem.v.hlsl";
    v37[2].kMacros[0].Name = "PSYS_STRIP";
    v37[2].kMacros[0].Definition = "";
    v37[2].kMacros[1].Name = nullptr;
    std::memset(
        &v37[2].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    v37[3].strPath = "lighting\\1x\\v\\particlesystem.v.hlsl";
    v37[3].kMacros[0].Name = "PSYS_STRIP";
    v37[3].kMacros[0].Definition = "";
    v37[3].kMacros[1].Name = "PSYS_SUBTEX";
    v37[3].kMacros[1].Definition = "";
    v37[3].kMacros[2].Name = nullptr;
    std::memset(
        &v37[3].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    v38[0].strPath = "lighting\\2x\\v\\waterfog.v.hlsl";
    v38[0].kMacros[0].Name = nullptr;
    std::memset(
        &v38[0].kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    v38[1].strPath = "lighting\\2x\\v\\waterfog.v.hlsl";
    v38[1].kMacros[0].Name = "TEXCOORD";
    v38[1].kMacros[0].Definition = "";
    v38[1].kMacros[1].Name = nullptr;
    std::memset(
        &v38[1].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    v38[2].strPath = "lighting\\2x\\v\\waterfog.v.hlsl";
    v38[2].kMacros[0].Name = "VC";
    v38[2].kMacros[0].Definition = "";
    v38[2].kMacros[1].Name = nullptr;
    std::memset(
        &v38[2].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    v38[3].strPath = "lighting\\2x\\v\\waterfog.v.hlsl";
    v38[3].kMacros[0].Name = "VC";
    v38[3].kMacros[0].Definition = "";
    v38[3].kMacros[1].Name = "TEXCOORD";
    v38[3].kMacros[1].Definition = "";
    v38[3].kMacros[2].Name = nullptr;
    std::memset(
        &v38[3].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    v38[4].strPath = "lighting\\2x\\v\\waterfog.v.hlsl";
    v38[4].kMacros[0].Name = "SKIN";
    v38[4].kMacros[0].Definition = "";
    v38[4].kMacros[1].Name = nullptr;
    std::memset(
        &v38[4].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    v38[5].strPath = "lighting\\2x\\v\\waterfog.v.hlsl";
    v38[5].kMacros[0].Name = "TEXCOORD";
    v38[5].kMacros[0].Definition = "";
    v38[5].kMacros[1].Name = "SKIN";
    v38[5].kMacros[1].Definition = "";
    v38[5].kMacros[2].Name = nullptr;
    std::memset(
        &v38[5].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    v38[6].strPath = "lighting\\2x\\v\\waterfog.v.hlsl";
    v38[6].kMacros[0].Name = "VC";
    v38[6].kMacros[0].Definition = "";
    v38[6].kMacros[1].Name = "SKIN";
    v38[6].kMacros[1].Definition = "";
    v38[6].kMacros[2].Name = nullptr;
    std::memset(
        &v38[6].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    v38[7].strPath = "lighting\\2x\\v\\waterfog.v.hlsl";
    v38[7].kMacros[0].Name = "VC";
    v38[7].kMacros[0].Definition = "";
    v38[7].kMacros[1].Name = "TEXCOORD";
    v38[7].kMacros[1].Definition = "";
    v38[7].kMacros[2].Name = "SKIN";
    v38[7].kMacros[2].Definition = "";
    v38[7].kMacros[3].Name = nullptr;
    std::memset(
        &v38[7].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    int32_t i = 0;
    BSShaderMacro* v28 = v36;
    NiD3DVertexShader** spVertexShader = this->spVertexShader;
    do {
        if (v28->strPath) {
            BSShader::MakePath(v28->strPath, Str1);
            BSsprintf(apDest, 260, "NOLIGHT%03i.vso", i);
            NiD3DVertexShader* v3 = this->CompileVertexShaderHLSL(Str1, v28->kMacros, ShaderTarget_vs_2_0, apDest);
            NiD3DVertexShader* v4 = *spVertexShader;
            NiD3DVertexShader* v27 = v3;
            NiRefObject* v30 = (NiRefObject*)*spVertexShader;
            if (*spVertexShader != v3) {
                if (v4) {
                    if (!InterlockedDecrementInt32(reinterpret_cast<int32_t*>(&v4->m_uiRefCount))) {
                        v30->DeleteThis();
                    }
                    v3 = v27;
                }
                *spVertexShader = v3;
                if (v3) {
                    NiIncRefCount(v3);
                }
            }
        }
        ++spVertexShader;
        ++v28;
        ++i;
    }
    while (i < 28);
    v33.strPath = "lighting\\1x\\p\\NoLighting.p.hlsl";
    v33.kMacros[0].Name = "TEX";
    v33.kMacros[0].Definition = "";
    v33.kMacros[1].Name = nullptr;
    std::memset(&v33.kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition));
    pixelShaderParamsVCOnly.strPath = "lighting\\1x\\p\\NoLighting.p.hlsl";
    pixelShaderParamsVCOnly.kMacros[0].Name = "VC";
    pixelShaderParamsVCOnly.kMacros[0].Definition = "";
    pixelShaderParamsVCOnly.kMacros[1].Name = nullptr;
    std::memset(
        &pixelShaderParamsVCOnly.kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    pixelShaderParamsTexVC.strPath = "lighting\\1x\\p\\NoLighting.p.hlsl";
    pixelShaderParamsTexVC.kMacros[0].Name = "TEX";
    pixelShaderParamsTexVC.kMacros[0].Definition = "";
    pixelShaderParamsTexVC.kMacros[1].Name = "VC";
    pixelShaderParamsTexVC.kMacros[1].Definition = "";
    pixelShaderParamsTexVC.kMacros[2].Name = nullptr;
    std::memset(
        &pixelShaderParamsTexVC.kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    v31.strPath = "lighting\\1x\\p\\NoLighting.p.hlsl";
    v31.kMacros[0].Name = "PREMULT_ALPHA";
    v31.kMacros[0].Definition = "";
    v31.kMacros[1].Name = "TEX";
    v31.kMacros[1].Definition = "";
    v31.kMacros[2].Name = "VC";
    v31.kMacros[2].Definition = "";
    v31.kMacros[3].Name = nullptr;
    std::memset(&v31.kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition));
    pixelShaderParamsMat.strPath = "lighting\\1x\\p\\NoLighting.p.hlsl";
    pixelShaderParamsMat.kMacros[0].Name = nullptr;
    std::memset(
        &pixelShaderParamsMat.kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    pixelShaderParamsWaterFog.strPath = "lighting\\2x\\p\\waterfog.p.hlsl";
    pixelShaderParamsWaterFog.kMacros[0].Name = nullptr;
    std::memset(
        &pixelShaderParamsWaterFog.kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    pixelShaderParamsWaterFogAt.strPath = "lighting\\2x\\p\\waterfog.p.hlsl";
    pixelShaderParamsWaterFogAt.kMacros[0].Name = "ALPHATESTING";
    pixelShaderParamsWaterFogAt.kMacros[0].Definition = "";
    pixelShaderParamsWaterFogAt.kMacros[1].Name = nullptr;
    std::memset(
        &pixelShaderParamsWaterFogAt.kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    BSShader::MakePath("lighting\\1x\\p\\NoLighting.p.hlsl", Str1);
    BSsprintf(apDest, 260, "NOLIGHTTEX.pso");
    NiD3DPixelShader* v5 = this->CompilePixelShaderHLSL(Str1, v33.kMacros, "ps_2_a", apDest);
    NiD3DPixelShader* v6 = this->spPixelShader[0];
    if (v6 != v5) {
        if (v6) {
            NiDecRefCount(v6);
        }
        this->spPixelShader[0] = v5;
        if (v5) {
            NiIncRefCount(v5);
        }
    }
    BSShader::MakePath(pixelShaderParamsVCOnly.strPath, Str1);
    BSsprintf(apDest, 260, "NOLIGHTVCONLY.pso");
    NiD3DPixelShader* v8 = this->CompilePixelShaderHLSL(Str1, pixelShaderParamsVCOnly.kMacros, "ps_2_a", apDest);
    NiD3DPixelShader* v9 = this->spPixelShader[1];
    if (v9 != v8) {
        if (v9) {
            NiDecRefCount(v9);
        }
        this->spPixelShader[1] = v8;
        if (v8) {
            NiIncRefCount(v8);
        }
    }
    BSShader::MakePath(pixelShaderParamsTexVC.strPath, Str1);
    BSsprintf(apDest, 260, "NOLIGHTTEXVC.pso");
    NiD3DPixelShader* v11 = this->CompilePixelShaderHLSL(Str1, pixelShaderParamsTexVC.kMacros, "ps_2_a", apDest);
    NiD3DPixelShader* v12 = this->spPixelShader[2];
    if (v12 != v11) {
        if (v12) {
            NiDecRefCount(v12);
        }
        this->spPixelShader[2] = v11;
        if (v11) {
            NiIncRefCount(v11);
        }
    }
    BSShader::MakePath("lighting\\1x\\p\\NoLighting.p.hlsl", Str1);
    BSsprintf(apDest, 260, "NOLIGHTTEXVCPMA.pso");
    NiD3DPixelShader* v14 = this->CompilePixelShaderHLSL(Str1, v31.kMacros, "ps_2_a", apDest);
    NiD3DPixelShader* v15 = this->spPixelShader[3];
    if (v15 != v14) {
        if (v15) {
            NiDecRefCount(v15);
        }
        this->spPixelShader[3] = v14;
        if (v14) {
            NiIncRefCount(v14);
        }
    }
    BSShader::MakePath(pixelShaderParamsMat.strPath, Str1);
    BSsprintf(apDest, 260, "NOLIGHTMAT.pso");
    NiD3DPixelShader* v17 = this->CompilePixelShaderHLSL(Str1, pixelShaderParamsMat.kMacros, "ps_2_a", apDest);
    NiD3DPixelShader* v18 = this->spPixelShader[4];
    if (v18 != v17) {
        if (v18) {
            NiDecRefCount(v18);
        }
        this->spPixelShader[4] = v17;
        if (v17) {
            NiIncRefCount(v17);
        }
    }
    BSShader::MakePath(pixelShaderParamsWaterFog.strPath, Str1);
    BSsprintf(apDest, 260, "NOLIGHTWF.pso");
    NiD3DPixelShader* v20 = this->CompilePixelShaderHLSL(Str1, pixelShaderParamsWaterFog.kMacros, "ps_2_a", apDest);
    NiD3DPixelShader* v21 = this->spPixelShader[5];
    if (v21 != v20) {
        if (v21) {
            NiDecRefCount(v21);
        }
        this->spPixelShader[5] = v20;
        if (v20) {
            NiIncRefCount(v20);
        }
    }
    BSShader::MakePath(pixelShaderParamsWaterFogAt.strPath, Str1);
    BSsprintf(apDest, 260, "NOLIGHTWFAt.pso");
    NiD3DPixelShader* v23 = this->CompilePixelShaderHLSL(Str1, pixelShaderParamsWaterFogAt.kMacros, "ps_2_a", apDest);
    NiD3DPixelShader* v24 = this->spPixelShader[6];
    if (v24 != v23) {
        if (v24) {
            NiDecRefCount(v24);
        }
        this->spPixelShader[6] = v23;
        if (v23) {
            NiIncRefCount(v23);
        }
    }
}

void BSShaderNoLighting::SetupGeometryConstants_MaterialColor(
    BSShaderNoLightingProperty* pprop,
    NiMaterialProperty* pmaterialProperty
) {
    double b;
    double g;
    double r;
    double m_fEmitMult;
    float v9;
    float v10;
    float v11;
    double fAlpha = 1.0;
    if (pprop->fAlpha < 1.0) {
        fAlpha = (double)pprop->fAlpha;
    }
    BSShaderNoLighting::kMaterialColor.a = (float)fAlpha;
    NiColor* m_pExternalEmittance = pmaterialProperty->m_external_emit;
    if (!m_pExternalEmittance) {
        m_pExternalEmittance = &pmaterialProperty->m_emit;
    }
    b = (double)m_pExternalEmittance->b;
    g = (double)m_pExternalEmittance->g;
    r = (double)m_pExternalEmittance->r;
    if (BSShaderManager::bHDR ||
        ((r < 0.10000000149011612) && ((g < 0.10000000149011612) && (b < 0.10000000149011612)))) {
        m_fEmitMult = (double)pmaterialProperty->m_fEmitMult;
        v9 = (float)(r * m_fEmitMult);
        v10 = (float)(g * m_fEmitMult);
        v11 = (float)(b * m_fEmitMult);
        b = (double)v11;
        g = (double)v10;
        r = (double)v9;
    }
    if (((NiColor_BLACK.r == r) && (NiColor_BLACK.g == g)) && (NiColor_BLACK.b == b)) {
        b = (double)NiColor_WHITE.b;
        g = (double)NiColor_WHITE.g;
        r = (double)NiColor_WHITE.r;
    }
    BSShaderNoLighting::kMaterialColor.r = (float)r;
    BSShaderNoLighting::kMaterialColor.g = (float)g;
    BSShaderNoLighting::kMaterialColor.b = (float)b;
}

void BSShaderNoLighting::SetupGeometryConstants_Particles(NiGeometry* pgeometry, const NiTransform& worldTransform) {
    NiPSysData* m_pObject = static_cast<NiPSysData*>(pgeometry->m_spModelData.m_pObject);
    uint8_t m_ucSubTextureOffsetCount = m_pObject->m_ucSubTextureOffsetCount;
    if (m_ucSubTextureOffsetCount) {
        uint32_t v6 = m_ucSubTextureOffsetCount;
        rt::memcpy(
            BSShaderNoLighting::aSubTexOffsets, m_pObject->m_pkSubTextureOffsets,
            sizeof(NiVector4) * m_ucSubTextureOffsetCount
        );
        pParticleSubTextureOffsetEntry->m_uiRegisterCount = v6;
    }
}

void BSShaderNoLighting::SetupGeometryConstants_WaterFog() {
    if (static_cast<uint32_t>(BSShaderManager::eCurrentPass - BSShaderManager::BSSM_WATER_DEPTH) <=
        BSShaderManager::BSSM_NLWATER_DEPTH_AtVc_S - BSShaderManager::BSSM_WATER_DEPTH) {
        pWorldEntry->m_bEnabled = true;
        BSShaderNoLighting::pFogPropA[0].r = BSShaderManager::kWaterFogDistances.y;
        BSShaderNoLighting::pFogPropA[0].g = 0.0f;
        BSShaderNoLighting::pFogPropA[0].b = 0.0f;
        BSShaderNoLighting::pFogPropA[0].a = 0.0f;
        BSShaderNoLighting::pFogPropA[1].r = BSShaderManager::kWaterPlane.m_kNormal.x;
        BSShaderNoLighting::pFogPropA[1].g = BSShaderManager::kWaterPlane.m_kNormal.y;
        BSShaderNoLighting::pFogPropA[1].b = BSShaderManager::kWaterPlane.m_kNormal.z;
        float v1 = BSShaderManager::kWaterPlane.m_kNormal.x * BSShaderLightingProperty::pShaderLightConstants[30].X() +
            BSShaderManager::kWaterPlane.m_kNormal.y * BSShaderLightingProperty::pShaderLightConstants[30].Y() +
            BSShaderManager::kWaterPlane.m_kNormal.z * BSShaderLightingProperty::pShaderLightConstants[30].Z();
        float v2 = v1 - BSShaderManager::kWaterPlane.m_fConstant;
        BSShaderNoLighting::pFogPropA[1].a = v2;
    }
}

void BSShaderNoLighting::SetupGeometryConstants_TextureTransform(const NiPropertyState* apState) {
    NiMatrix3 kTransformMatrix;
    NiTexturingProperty* m_pObject = static_cast<NiTexturingProperty*>(apState->m_aspProps[5].m_pObject);
    kTransformMatrix.m_pEntry[0][0] = 1.0;
    kTransformMatrix.m_pEntry[0][1] = 0.0;
    kTransformMatrix.m_pEntry[0][2] = 0.0;
    kTransformMatrix.m_pEntry[1][0] = 0.0;
    kTransformMatrix.m_pEntry[1][2] = 0.0;
    kTransformMatrix.m_pEntry[2][0] = 0.0;
    kTransformMatrix.m_pEntry[2][1] = 0.0;
    kTransformMatrix.m_pEntry[1][1] = 1.0;
    kTransformMatrix.m_pEntry[2][2] = 1.0;
    if (m_pObject) {
        NiTexturingProperty::Map* v3 = *m_pObject->m_kMaps.m_pBase;
        if (v3) {
            NiTextureTransform* m_pkTextureTransform = v3->m_pkTextureTransform;
            if (m_pkTextureTransform) {
                if (m_pkTextureTransform->m_bMatrixDirty) {
                    v3->m_pkTextureTransform->UpdateMatrix();
                }
                kTransformMatrix = m_pkTextureTransform->m_kMatrix;
            }
        }
    }
    BSShaderNoLighting::pTextureTransform[0].x = kTransformMatrix.m_pEntry[0][0];
    BSShaderNoLighting::pTextureTransform[0].y = kTransformMatrix.m_pEntry[0][1];
    BSShaderNoLighting::pTextureTransform[0].z = kTransformMatrix.m_pEntry[0][2];
    BSShaderNoLighting::pTextureTransform[1].x = kTransformMatrix.m_pEntry[1][0];
    BSShaderNoLighting::pTextureTransform[1].y = kTransformMatrix.m_pEntry[1][1];
    BSShaderNoLighting::pTextureTransform[1].z = kTransformMatrix.m_pEntry[1][2];
    BSShaderNoLighting::pTextureTransform[2].x = kTransformMatrix.m_pEntry[2][0];
    BSShaderNoLighting::pTextureTransform[2].y = kTransformMatrix.m_pEntry[2][1];
    BSShaderNoLighting::pTextureTransform[2].z = kTransformMatrix.m_pEntry[2][2];
}

BSShader* BSShaderNoLighting::CreateShader() {
    BSShaderNoLighting* pshader;
    NiDX9ShaderDeclaration* pdecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 2u, 1u);
    NiDX9ShaderDeclaration* pdeclVC = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 2u, 1u);
    NiDX9ShaderDeclaration* pdeclTexVC = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 3u, 1u);
    NiDX9ShaderDeclaration* pdeclN = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 3u, 1u);
    NiDX9ShaderDeclaration* pdeclVCN = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 3u, 1u);
    NiDX9ShaderDeclaration* pdeclTexVCN = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 4u, 1u);
    NiDX9ShaderDeclaration* pskindecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 4u, 1u);
    NiDX9ShaderDeclaration* pskindeclVC = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 4u, 1u);
    NiDX9ShaderDeclaration* pskindeclTexVC =
        NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 5u, 1u);
    NiDX9ShaderDeclaration* pskindeclN = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 5u, 1u);
    NiDX9ShaderDeclaration* pskindeclVCN = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 5u, 1u);
    NiDX9ShaderDeclaration* pskindeclTexVCN =
        NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 6u, 1u);
    pdecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pdecl->SetEntry(1u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0);
    pdecl->GetD3DDeclaration();
    pdeclVC->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pdeclVC->SetEntry(1u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0);
    pdeclVC->GetD3DDeclaration();
    pdeclTexVC->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pdeclTexVC->SetEntry(1u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0);
    pdeclTexVC->SetEntry(2u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0);
    pdeclTexVC->GetD3DDeclaration();
    pdeclN->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pdeclN->SetEntry(1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pdeclN->SetEntry(2u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0);
    pdeclN->GetD3DDeclaration();
    pdeclVCN->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pdeclVCN->SetEntry(1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pdeclVCN->SetEntry(2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0);
    pdeclVCN->GetD3DDeclaration();
    pdeclTexVCN->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pdeclTexVCN->SetEntry(1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pdeclTexVCN->SetEntry(2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0);
    pdeclTexVCN->SetEntry(3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0);
    pdeclTexVCN->GetD3DDeclaration();
    pskindecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pskindecl->SetEntry(1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_BLENDWEIGHT, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
    pskindecl->SetEntry(
        2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_BLENDINDICES, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pskindecl->SetEntry(3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0);
    pskindecl->GetD3DDeclaration();
    pskindeclVC->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pskindeclVC->SetEntry(
        1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_BLENDWEIGHT, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pskindeclVC->SetEntry(
        2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_BLENDINDICES, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pskindeclVC->SetEntry(3u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0);
    pskindeclVC->GetD3DDeclaration();
    pskindeclTexVC->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pskindeclTexVC->SetEntry(
        1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_BLENDWEIGHT, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pskindeclTexVC->SetEntry(
        2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_BLENDINDICES, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pskindeclTexVC->SetEntry(
        3u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pskindeclTexVC->SetEntry(
        4u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    pskindeclTexVC->GetD3DDeclaration();
    pskindeclN->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pskindeclN->SetEntry(
        1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_BLENDWEIGHT, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pskindeclN->SetEntry(
        2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_BLENDINDICES, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pskindeclN->SetEntry(3u, 1u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pskindeclN->SetEntry(4u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0);
    pskindeclN->GetD3DDeclaration();
    pskindeclVCN->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pskindeclVCN->SetEntry(
        1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_BLENDWEIGHT, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pskindeclVCN->SetEntry(
        2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_BLENDINDICES, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pskindeclVCN->SetEntry(3u, 1u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pskindeclVCN->SetEntry(
        4u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pskindeclVCN->GetD3DDeclaration();
    pskindeclTexVCN->SetEntry(
        0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskindeclTexVCN->SetEntry(
        1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_BLENDWEIGHT, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pskindeclTexVCN->SetEntry(
        2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_BLENDINDICES, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pskindeclTexVCN->SetEntry(
        3u, 1u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskindeclTexVCN->SetEntry(
        4u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pskindeclTexVCN->SetEntry(
        5u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    pskindeclTexVCN->GetD3DDeclaration();
    pshader = new BSShaderNoLighting(
        pdecl, pdeclVC, pdeclTexVC, pdeclN, pdeclVCN, pdeclTexVCN, pskindecl, pskindeclVC, pskindeclTexVC, pskindeclN,
        pskindeclVCN, pskindeclTexVCN
    );
    pshader->Initialize();
    pshader->LoadShaders();
    pshader->LoadStagesAndPasses();
    pshader->CreateConstantMaps();
    pshader->SetShaderDecl(pdecl);
    return pshader;
}
