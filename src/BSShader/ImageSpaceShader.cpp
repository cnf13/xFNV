#include "BSShader/ImageSpaceShader.h"

#include "BSCore/BSString.h"
#include "BSShader/BSBatchRenderer.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiOldScreenSpace.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"

ImageSpaceShaderParam ImageSpaceShader::DefaultParam;

ImageSpaceShader::ImageSpaceShader() : spVertexShader(nullptr), spPixelShader(nullptr) {
    iShaderType = 24;
    BSShader::Initialize();
    ImageSpaceShader::SetupVertexStream();
}

// The NiPointer members release the vertex and pixel shaders; the bases destroy themselves.
ImageSpaceShader::~ImageSpaceShader() {}

// Puts the shader on the screen geometry, binds the effect's programs, starts the effect's output texture (or the back
// buffer when it has none and the letterbox is on) and draws one pass.
void ImageSpaceShader::Render(
    NiScreenElements* apGeometry,
    NiDX9Renderer* apRenderer,
    ImageSpaceEffectParam* apParam,
    int32_t abEndFrame
) {
    ImageSpaceEffect* pEffect = static_cast<ImageSpaceEffect*>(this);
    ImageSpaceShaderParam* pParam = static_cast<ImageSpaceShaderParam*>(apParam);
    if (!pParam) {
        pParam = &ImageSpaceShader::DefaultParam;
    }
    NiPointer<BSRenderedTexture> spRenderedTexture = pEffect->pTextures.m_pBase[0]->GetRenderedTexture();
    apGeometry->m_spShader = this;
    SetupGeometry(apGeometry);
    SetConstantMap(pParam);
    SetVSConstants();
    uint32_t uiClearMode = pParam->iClearFlags;
    NiColorA kBackgroundColor(0.0f, 0.0f, 0.0f, 0.0f);
    NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
    if (!spRenderedTexture && BSShaderManager::bLetterbox) {
        uiClearMode = NiRenderer::CLEAR_BACKBUFFER;
        BSShaderManager::pRenderer->GetBackgroundColor(kBackgroundColor);
        BSShaderManager::pRenderer->SetBackgroundColorAlpha(NiColorA::BLACK);
        if (pRenderer->m_eFrameState == NiRenderer::FRAMESTATE_OUTSIDE_FRAME) {
            BSRenderedTexture::Start(uiClearMode, nullptr);
        }
    }
    else {
        if ((uiClearMode & 1) != 0) {
            BSShaderManager::pRenderer->GetBackgroundColor(kBackgroundColor);
            BSShaderManager::pRenderer->SetBackgroundColorAlpha(pParam->fClearColor);
        }
        if (spRenderedTexture) {
            if (pRenderer->m_eFrameState == NiRenderer::FRAMESTATE_OUTSIDE_FRAME) {
                BSRenderedTexture::StartOffscreen(uiClearMode, spRenderedTexture->GetGroup());
            }
        }
        else if (pRenderer->m_eFrameState == NiRenderer::FRAMESTATE_OUTSIDE_FRAME) {
            BSRenderedTexture::Start(uiClearMode, nullptr);
        }
    }
    NiRect<float>* pViewport = nullptr;
    if (spRenderedTexture.m_pObject == BSShaderManager::pImageSpaceManager->pEOFDest) {
        pViewport = &BSShaderManager::pImageSpaceManager->CustomViewPort;
    }
    if (!spRenderedTexture && BSShaderManager::bLetterbox) {
        if (pRenderer->GetDefaultRenderTargetGroup() == pRenderer->GetCurrentRenderTargetGroup()) {
            pViewport = &BSShaderManager::kFramebufferViewport;
        }
    }
    if ((pRenderer->m_eFrameState == NiRenderer::FRAMESTATE_INSIDE_FRAME ||
         pRenderer->m_eFrameState == NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME) &&
        pRenderer->m_bRenderTargetGroupActive) {
        pRenderer->Do_SetScreenSpaceCameraData(pViewport);
    }
    SetupPassShaders(0x2F6); // BSSM_IMAGESPACE
    BSShaderProperty::RenderPass kPass(apGeometry, 0x2F6, true, 0, nullptr);
    BSBatchRenderer::RenderPassImmediately(&kPass, 0x2F6, false, false, false);
    // Tests abEndFrame's low byte only.
    if (spRenderedTexture && static_cast<uint8_t>(abEndFrame) != 0) {
        if (BSRenderedTexture::GetCurrent() != pRenderer->GetDefaultRenderTargetGroup()) {
            BSRenderedTexture::StopOffscreen();
        }
    }
    if ((pParam->iClearFlags & NiRenderer::CLEAR_BACKBUFFER) != 0) {
        BSShaderManager::pRenderer->SetBackgroundColorAlpha(kBackgroundColor);
    }
}

bool ImageSpaceShader::SetupGeometry(NiGeometry* apGeometry) {
    return true;
}

void ImageSpaceShader::SetupGeometryTextures(const NiPropertyState* apState) {
    NiPointer<NiD3DTextureStage> spStage;
    for (uint32_t i = 1; i < pTextures.m_usSize; ++i) {
        spStage = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[i - 1].m_pObject;
        if (spStage.m_pObject) {
            ImageSpaceTexture* pTexture = pTextures.m_pBase[i];
            if (pTexture) {
                pTexture->SetupTexture(spStage.m_pObject);
            }
            else {
                spStage->SetTexture(nullptr);
            }
            spStage->ConfigureStage(false);
        }
    }
}

void ImageSpaceShader::SetupGeometryConstants(const NiPropertyState* apState) {
    NiTriStrips* pGeometry;
    NiBound* m_kWorldBound;
    NiGeometryBufferData* m_pkBuffData;
    NiD3DShaderConstantMap* m_pObject;
    NiD3DPixelShader* v7;
    NiD3DVertexShader* v9;
    pGeometry = static_cast<NiTriStrips*>(BSShaderManager::pCurrentRenderPass->pGeometry);
    m_kWorldBound = BSShaderManager::pCurrentRenderPass->pGeometry->m_kWorldBound;
    ImageSpaceShader* v4 = this;
    if (!m_kWorldBound) {
        m_kWorldBound = &NiAVObject::NullBoundS;
    }
    m_pkBuffData = pGeometry->m_spModelData->m_pkBuffData;
    m_pObject = m_spPixelConstantMap.m_pObject;
    NiBound* v10 = m_kWorldBound;
    if (m_pObject) {
        v7 = NiD3DShader::m_spCurrentPass->m_spPixelShader.m_pObject;
        if (v7) {
            m_pObject->SetShaderConstants(
                v7,
                pGeometry,
                nullptr,
                nullptr,
                m_pkBuffData,
                apState,
                nullptr,
                pGeometry->m_kWorld,
                *m_kWorldBound,
                0,
                true
            );
            v4 = this;
            m_kWorldBound = v10;
        }
    }
    NiD3DShaderConstantMap* v8 = v4->m_spVertexConstantMap.m_pObject;
    if (v8) {
        v9 = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
        if (v9) {
            v8->SetShaderConstants(
                v9,
                pGeometry,
                nullptr,
                nullptr,
                m_pkBuffData,
                apState,
                nullptr,
                pGeometry->m_kWorld,
                *m_kWorldBound,
                0,
                true
            );
        }
    }
}

void ImageSpaceShader::PostGeometry(const NiPropertyState* apState) {
    NiPointer<NiD3DTextureStage> spStage;
    for (uint32_t i = 1; i < pTextures.m_usSize; ++i) {
        spStage = NiD3DShader::m_spCurrentPass->m_kStages.m_pBase[i - 1].m_pObject;
        if (spStage.m_pObject) {
            spStage->SetTexture(nullptr);
            NiD3DRenderState* pRenderState = NiD3DShader::m_spCurrentPass->GetD3DRenderState();
            pRenderState->SetTexture(i - 1, nullptr);
        }
    }
}

void ImageSpaceShader::InitPass(uint32_t auiCurrentPass) {
    NiD3DShader::m_spCurrentPass = spPass.m_pObject;
}

void ImageSpaceShader::LoadShaders(
    BSShaderMacro& aVShader,
    BSShaderMacro& aPShader,
    const char* apcName,
    const char* apcVTarget,
    const char* apcPTarget
) {
    char apDest[260];
    char Str1[260];
    BSShader::MakePath(aVShader.strPath, Str1);
    BSsprintf(apDest, 260, "IS%s.vso", apcName);
    NiD3DVertexShader* v7 = CompileVertexShaderHLSL(Str1, aVShader.kMacros, apcVTarget, apDest);
    spVertexShader = v7;
    if (apcPTarget) {
        BSShader::MakePath(aPShader.strPath, Str1);
        BSsprintf(apDest, 260, "IS%s.pso", apcName);
        NiD3DPixelShader* v10 = CompilePixelShaderHLSL(Str1, aPShader.kMacros, apcPTarget, apDest);
        spPixelShader = v10;
    }
    else {
        spPixelShader = nullptr;
    }
}

void ImageSpaceShader::SetupVertexStream() {
    NiDX9ShaderDeclaration* v2 = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 2u, 1u);
    v2->SetEntry(
        static_cast<uint32_t>(0),
        static_cast<uint32_t>(0),
        NiShaderDeclaration::SHADERPARAM_NI_POSITION,
        NiShaderDeclaration::SPTYPE_FLOAT3,
        static_cast<uint32_t>(0)
    );
    v2->SetEntry(
        1u,
        7u,
        NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0,
        NiShaderDeclaration::SPTYPE_FLOAT2,
        static_cast<uint32_t>(0)
    );
    v2->GetD3DDeclaration();
    SetShaderDecl(v2);
}

void ImageSpaceShader::SetConstantMap(ImageSpaceShaderParam* apParam) {
    m_spVertexConstantMap = apParam->spVertexConstantMap;
    m_spPixelConstantMap = apParam->spPixelConstantMap;
    pParam = apParam;
}

bool ImageSpaceShader::LoadStagesAndPasses() {
    NiPointer<NiD3DTextureStage> spStage;
    NiPointer<NiD3DPass>* p_spPass = &spPass;
    NiD3DPass::CreateNewPass(spPass);
    for (uint32_t i = 0; i < pTextures.m_usMaxSize; ++i) {
        NiD3DTextureStage::CreateNewStage(spStage);
        BSShader::SetupDefaultStage(spStage.m_pObject, i, 3u, 2u, true);
        p_spPass->m_pObject->SetStage(p_spPass->m_pObject->m_uiCurrentStage, spStage.m_pObject);
    }
    NiD3DPass* m_pObject = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 0, true);
    NiD3DPass* v5 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v5->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v5->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
    NiD3DPass* v6 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v6->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v6->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
    NiD3DPass* v7 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v7->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v7->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
    NiD3DPass* v8 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v8->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v8->m_pkRenderStateGroup->SetRenderState(168u, 15u, true);
    SetState();
    p_spPass->m_pObject->m_spVertexShader = spVertexShader.m_pObject;
    p_spPass->m_pObject->m_spPixelShader = spPixelShader.m_pObject;
    NiD3DShader::m_spCurrentPass = p_spPass->m_pObject;
    return true;
}

void ImageSpaceShader::SetVSConstants() {
    for (uint32_t i = 0; i < pTextures.m_usSize; ++i) {
        ImageSpaceTexture** ppTexture = &pTextures.m_pBase[i];
        NiTexture* pTexture;
        if (*ppTexture && (pTexture = (*ppTexture)->GetTexture()) != nullptr) {
            if (pParam) {
                int32_t iHeight = pTexture->GetHeight();
                int32_t iWidth = pTexture->GetWidth();
                pParam->SetTextureOffsets(i, iWidth, iHeight);
            }
        }
        else if (pParam) {
            // Without a texture the offsets come from the default render target's first buffer.
            uint32_t uiHeight;
            NiRenderTargetGroup* pGroup = NiRenderer::ms_pkRenderer->GetDefaultRenderTargetGroup();
            if (pGroup) {
                Ni2DBuffer* pBuffer = pGroup->GetBuffer(0);
                if (pBuffer) {
                    uiHeight = pBuffer->m_uiHeight;
                }
                else {
                    uiHeight = 0;
                }
            }
            else {
                uiHeight = 0;
            }
            uint32_t uiWidth = 0;
            pGroup = NiRenderer::ms_pkRenderer->GetDefaultRenderTargetGroup();
            if (pGroup) {
                Ni2DBuffer* pBuffer = pGroup->GetBuffer(0);
                if (pBuffer) {
                    uiWidth = pBuffer->m_uiWidth;
                }
            }
            pParam->SetTextureOffsets(i, uiWidth, uiHeight);
        }
    }
}
