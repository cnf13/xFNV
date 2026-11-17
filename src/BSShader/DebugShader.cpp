#include "BSShader/DebugShader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffectState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElementsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/string.h"

DebugShader::DebugShader() : spTextureToRender{}, spScreenTriShape(nullptr), spVertexShader(nullptr), spPixelShader{} {
    iShaderType = 10;
    m_bUsesNiRenderState = 1;
}

// The shaders, the screen shape and the textures are held references, released here.
DebugShader::~DebugShader() {
    if (spVertexShader) {
        NiDecRefCount(spVertexShader);
        spVertexShader = nullptr;
    }
    for (NiD3DPixelShader*& rpPixelShader : spPixelShader) {
        if (rpPixelShader) {
            NiDecRefCount(rpPixelShader);
            rpPixelShader = nullptr;
        }
    }
    if (spScreenTriShape) {
        NiDecRefCount(spScreenTriShape);
        spScreenTriShape = nullptr;
    }
    for (NiRenderedTexture*& rpTexture : spTextureToRender) {
        if (rpTexture) {
            NiDecRefCount(rpTexture);
            rpTexture = nullptr;
        }
    }
}

bool DebugShader::Initialize() {
    bool bResult = BSShader::Initialize();
    NiUpdateData kUpdateData;
    kUpdateData.fTime = 0.0f;
    kUpdateData.bUpdateControllers = false;
    kUpdateData.bParallelUpdate = false;
    kUpdateData.bFoundParticles = false;
    kUpdateData.bFoundMorphController = false;
    kUpdateData.bSceneGraphChange = false;
    void* pMemory = NiMemObject::operator new(sizeof(NiScreenElements));
    NiScreenElements* pScreenElements;
    if (pMemory) {
        void* pDataMemory = NiMemObject::operator new(sizeof(NiScreenElementsData));
        NiScreenElementsData* pData = nullptr;
        if (pDataMemory) {
            pData = ::new (pDataMemory) NiScreenElementsData(false, false, 1, 1, 1, 4, 1, 2, 1);
        }
        pScreenElements = ::new (pMemory) NiScreenElements(pData);
    }
    else {
        pScreenElements = nullptr;
    }
    // Assigns the smart pointer member by hand: release the old object, store, take a reference.
    NiScreenElements* pOld = spScreenTriShape;
    if (pOld != pScreenElements) {
        if (pOld) {
            NiDecRefCount(pOld);
        }
        spScreenTriShape = pScreenElements;
        if (pScreenElements) {
            NiIncRefCount(pScreenElements);
        }
    }
    static_cast<NiScreenElementsData*>(spScreenTriShape->m_spModelData.m_pObject)->Insert(4, 0, nullptr);
    static_cast<NiScreenElementsData*>(spScreenTriShape->m_spModelData.m_pObject)
        ->SetRectangle(0, -1.0f, 1.0f, 2.0f, -2.0f);
    static_cast<NiScreenElementsData*>(spScreenTriShape->m_spModelData.m_pObject)
        ->SetTextures(0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
    static_cast<NiScreenElementsData*>(spScreenTriShape->m_spModelData.m_pObject)->UpdateBound();
    spScreenTriShape->UpdateProperties();
    spScreenTriShape->Update(kUpdateData);
    if (!m_spPixelConstantMap.m_pObject) {
        void* pMapMemory = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
        NiD3DShaderConstantMap* pMap;
        if (pMapMemory) {
            pMap = ::new (pMapMemory) NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        }
        else {
            pMap = nullptr;
        }
        m_spPixelConstantMap = pMap;
        m_spPixelConstantMap.m_pObject->AddEntry("ChannelMask", 0x10000007, 0, 5, 1, "", 16, 4, fChannelMask, false);
    }
    return bResult;
}

bool DebugShader::SetupGeometry(NiGeometry* apGeometry) {
    return (bool)((uint8_t)NiD3DShader::SetupGeometry(apGeometry));
}

uint32_t DebugShader::PreProcessPipeline(
    NiGeometry* apGeometry,
    const NiSkinInstance* apSkin,
    NiGeometryData::RendererData* apBuffData,
    const NiPropertyState* apState,
    const NiDynamicEffectState* apEffects,
    const NiTransform& aWorld,
    const NiBound& aWorldBound
) {
    return NiD3DShader::PreProcessPipeline(apGeometry, apSkin, apBuffData, apState, apEffects, aWorld, aWorldBound);
}

uint32_t DebugShader::UpdatePipeline(
    NiGeometry* apGeometry,
    const NiSkinInstance* apSkin,
    NiGeometryData::RendererData* apBuffData,
    const NiPropertyState* apState,
    const NiDynamicEffectState* apEffects,
    const NiTransform& aWorld,
    const NiBound& aWorldBound
) {
    ResetPasses();
    spPass.m_pObject->m_kStages.m_pBase->m_pObject->m_spTexture = spTextureToRender[curTexIdx];
    NiD3DPass* m_pObject = spPass.m_pObject;
    NiD3DPixelShader* v10 = spPixelShader[DebugShader::uiPixelShaderCount];
    NiD3DPixelShader* v11 = m_pObject->m_spPixelShader.m_pObject;
    NiPointer<NiD3DPixelShader>* p_m_spPixelShader = &m_pObject->m_spPixelShader;
    if (v11 == v10) {
        NiD3DShader::m_spCurrentPass = spPass.m_pObject;
        return 0;
    }
    else {
        if (v11) {
            NiDecRefCount(v11);
        }
        p_m_spPixelShader->m_pObject = v10;
        if (v10) {
            NiIncRefCount(v10);
        }
        NiD3DShader::m_spCurrentPass = spPass.m_pObject;
        return 0;
    }
}

void DebugShader::ReloadShaders() {
    LoadShaders();
}

// Draws each texture to render into its cell of a 4x4 grid over the destination (apDstBuffer's first texture, or the
// default render target when null), then releases it.
void DebugShader::RenderEffect(BSRenderedTexture* apDstBuffer) {
    float width_1;
    int32_t iHeight;
    if (apDstBuffer) {
        NiRenderedTexture* pTexture = apDstBuffer->spTexture[0].m_pObject;
        int32_t iWidth = pTexture ? static_cast<int32_t>(pTexture->GetWidth()) : 0;
        double dWidth = iWidth;
        if (iWidth < 0) {
            dWidth += 4294967296.0f;
        }
        width_1 = dWidth;
        pTexture = apDstBuffer->spTexture[0].m_pObject;
        iHeight = pTexture ? static_cast<int32_t>(pTexture->GetHeight()) : 0;
    }
    else {
        int32_t iWidth = static_cast<int32_t>(NiRenderer::ms_pkRenderer->GetDefaultRenderTargetGroup()->GetWidth(0));
        double dWidth = iWidth;
        if (iWidth < 0) {
            dWidth += 4294967296.0f;
        }
        width_1 = dWidth;
        iHeight = static_cast<int32_t>(NiRenderer::ms_pkRenderer->GetDefaultRenderTargetGroup()->GetHeight(0));
    }
    double v7 = iHeight;
    if (iHeight < 0) {
        v7 += 4294967296.0f;
    }
    float apTexturec = v7;
    double v11 = apTexturec / width_1;
    float apTexture = v11;
    double v12 = apTexture;
    if (apTexture < 1.0f) {
        apTexture = 1.0f;
    }
    double v13 = v11 / apTexture;
    double v14 = 1.0;
    float apTexturea = v12;
    if (!(apTexturea < 1.0f)) {
        v14 = apTexturea;
    }
    NiScreenElements* pScreenTriShape = nullptr;
    float apTextured = v14;
    float apTexturee = 1.0 / apTextured;
    NiRect<float> kPort;
    kPort.m_left = 0.05f;
    float width_1a = v13;
    kPort.m_right = width_1a;
    kPort.m_top = apTexturee - 0.05000000074505806;
    kPort.m_bottom = 0.0f;
    // EXE-INLINED none: NiPointer<NiScreenElements>::NiPointer
    NiScreenElements* pShape = this->spScreenTriShape;
    if (pShape) {
        pScreenTriShape = pShape;
        pShape->IncRefCount();
    }
    this->curTexIdx = 0;
    do {
        NiRenderedTexture* pTexture = spTextureToRender[this->curTexIdx];
        if (pTexture) {
            double v21;
            double v22;
            int32_t iTexWidth = static_cast<int32_t>(pTexture->GetWidth());
            double dTexWidth = iTexWidth;
            if (iTexWidth < 0) {
                dTexWidth += 4294967296.0f;
            }
            float apTexturef = dTexWidth;
            pTexture = spTextureToRender[this->curTexIdx];
            int32_t iTexHeight = static_cast<int32_t>(pTexture->GetHeight());
            double dTexHeight = iTexHeight;
            if (iTexHeight < 0) {
                dTexHeight += 4294967296.0f;
            }
            float v34 = dTexHeight;
            double v19 = apTexturef / v34;
            float apTextureb = v19;
            double v20 = apTextureb;
            if (!(apTextureb < 1.0f)) {
                v22 = apTextureb;
                v21 = 1.0;
            }
            else {
                v21 = 1.0;
                v22 = 1.0;
            }
            float apTextureg = v22;
            double v23 = v21;
            double v24 = v19 / apTextureg;
            double v25 = v23;
            float apTextureh = v20;
            double v26 = apTextureh;
            if (apTextureh < v23) {
                v26 = 1.0;
            }
            float apTexturei = v26;
            float v39 = v25 / apTexturei;
            uint32_t uiTexIdx = this->curTexIdx;
            int32_t iRow = static_cast<int32_t>(uiTexIdx >> 2);
            double dRow = iRow;
            if (iRow < 0) {
                dRow += 4294967296.0f;
            }
            float v35 = dRow;
            int32_t iColumn = static_cast<int32_t>(uiTexIdx & 3);
            double dColumn = iColumn;
            if (iColumn < 0) {
                dColumn += 4294967296.0f;
            }
            float apTexturej = dColumn;
            float fLeft = apTexturej * 0.5 - v25;
            float fTop = v25 + v35 * -0.5;
            float v37 = v24;
            float fWidth = v37 * 0.49000000953674316;
            float apTexturek = v39 * -0.49000000953674316;
            static_cast<NiScreenElementsData*>(pScreenTriShape->m_spModelData.m_pObject)
                ->NiScreenElementsData::SetRectangle(0, fLeft, fTop, fWidth, apTexturek);
            static_cast<NiScreenElementsData*>(pScreenTriShape->m_spModelData.m_pObject)
                ->NiScreenElementsData::UpdateBound();
            pScreenTriShape->m_spShader = this;
            SetupGeometry(pScreenTriShape);
            NiRenderer* pRenderer = NiRenderer::ms_pkRenderer;
            // EXE-INLINED 0x4A0DD0
            if ((pRenderer->m_eFrameState == NiRenderer::FRAMESTATE_INSIDE_FRAME ||
                 pRenderer->m_eFrameState == NiRenderer::FRAMESTATE_INSIDE_OFFSCREEN_FRAME) &&
                pRenderer->m_bRenderTargetGroupActive) {
                pRenderer->Do_SetScreenSpaceCameraData(&kPort);
                pRenderer = NiRenderer::ms_pkRenderer;
            }
            pScreenTriShape->Draw(pRenderer);
            NiRenderedTexture** ppTexture = &spTextureToRender[this->curTexIdx];
            NiRenderedTexture* pOld = *ppTexture;
            if (pOld) {
                pOld->DecRefCount();
                *ppTexture = nullptr;
            }
            pScreenTriShape->m_spShader = nullptr;
        }
        ++this->curTexIdx;
    }
    while (this->curTexIdx < 16);
    if (pScreenTriShape) {
        pScreenTriShape->DecRefCount();
    }
}

// Picks the first free slot of the 16 texture-to-render slots (-1 when none is free), then holds apTexture there when
// it is a rendered or a source texture. The loop's bound is compared unsigned.
void DebugShader::SetTextureToRender(NiTexture* apTexture) {
    int32_t v3 = -1;
    int32_t v4 = 2;
    NiRenderedTexture** v5 = &spTextureToRender[1];
    while (v3 == -1) {
        if (!*(v5 - 1)) {
            v3 = v4 - 2;
            break;
        }
        if (!*v5) {
            v3 = v4 - 1;
            break;
        }
        if (!v5[1]) {
            v3 = v4;
            break;
        }
        if (!v5[2]) {
            v3 = v4 + 1;
        }
        v4 += 4;
        v5 += 4;
        if (static_cast<uint32_t>(v4 - 2) >= 16) {
            if (v3 == -1) {
                return;
            }
            break;
        }
    }
    if (apTexture) {
        NiRenderedTexture* v8;
        const NiRTTI* v6 = apTexture->GetRTTI();
        if (v6) {
            while (v6 != &NiSourceTexture::ms_RTTI) {
                v6 = v6->GetBaseRTTI();
                if (!v6) {
                    goto LABEL_17;
                }
            }
            v8 = spTextureToRender[v3];
            if (v8 != apTexture) {
                if (v8) {
                LABEL_26:;
                    NiDecRefCount(v8);
                }
            LABEL_28:;
                spTextureToRender[v3] = static_cast<NiRenderedTexture*>(apTexture);
                NiIncRefCount(apTexture);
            }
        }
        else {
        LABEL_17:;
            const NiRTTI* v7 = apTexture->GetRTTI();
            if (v7) {
                while (v7 != &NiRenderedTexture::ms_RTTI) {
                    v7 = v7->GetBaseRTTI();
                    if (!v7) {
                        return;
                    }
                }
                v8 = spTextureToRender[v3];
                if (v8 != apTexture) {
                    if (v8) {
                        goto LABEL_26;
                    }
                    goto LABEL_28;
                }
            }
        }
    }
}

void DebugShader::LoadShaders() {
    _D3DXMACRO v12;
    char v14[60];
    _D3DXMACRO v16;
    char v17[64];
    _D3DXMACRO Size;
    char v20[64];
    char PSOFile[260];
    char HLSLFile[260];
    const char* ptarget = BSShaderManager::GetPSTarget(1);
    Size.Name = 0;
    Size.Definition = 0;
    const char* apFile = "imagespace\\1x\\v\\base_old.v.hlsl";
    std::memset(v20, 0, sizeof(v20));
    BSShader::MakePath("imagespace\\1x\\v\\base_old.v.hlsl", HLSLFile);
    BSsprintf(PSOFile, 260, "DEBUG.vso");
    NiD3DVertexShader* v3 = CompileVertexShaderHLSL(HLSLFile, &Size, ShaderTarget_vs_1_1, PSOFile);
    NiD3DVertexShader* spVertexShader = this->spVertexShader;
    if (spVertexShader != v3) {
        if (spVertexShader) {
            NiDecRefCount(spVertexShader);
        }
        this->spVertexShader = v3;
        if (v3) {
            NiIncRefCount(v3);
        }
    }
    const char* v15 = "imagespace\\1x\\p\\copy.p.hlsl";
    v16.Name = 0;
    v16.Definition = 0;
    std::memset(v17, 0, sizeof(v17));
    BSShader::MakePath("imagespace\\1x\\p\\copy.p.hlsl", HLSLFile);
    BSsprintf(PSOFile, 260, "DEBUG00.pso");
    NiD3DPixelShader* v6 = CompilePixelShaderHLSL(HLSLFile, &v16, ptarget, PSOFile);
    NiD3DPixelShader* v7 = spPixelShader[0];
    if (v7 != v6) {
        if (v7) {
            NiDecRefCount(v7);
        }
        spPixelShader[0] = v6;
        if (v6) {
            NiIncRefCount(v6);
        }
    }
    v12.Name = "CHANNELMASK";
    v12.Definition = "";
    int32_t v13 = 0;
    std::memset(v14, 0, sizeof(v14));
    BSShader::MakePath("imagespace\\1x\\p\\copy.p.hlsl", HLSLFile);
    BSsprintf(PSOFile, 260, "DEBUG01.pso");
    NiD3DPixelShader* v9 = CompilePixelShaderHLSL(HLSLFile, &v12, ptarget, PSOFile);
    NiD3DPixelShader* v10 = spPixelShader[1];
    if (v10 != v9) {
        if (v10) {
            NiDecRefCount(v10);
        }
        spPixelShader[1] = v9;
        if (v9) {
            NiIncRefCount(v9);
        }
    }
}

bool DebugShader::LoadStagesAndPasses() {
    NiPointer<NiD3DPass>* p_spPass = &spPass;
    NiD3DPass::CreateNewPass(spPass);
    NiPointer<NiD3DTextureStage> aspD3DTextureStage;
    NiD3DTextureStage::CreateNewStage(aspD3DTextureStage);
    BSShader::SetupDefaultStage(aspD3DTextureStage.m_pObject, 0, 3u, 2u, 1);
    aspD3DTextureStage.m_pObject->SetFilterMode(NiTexturingProperty::FILTER_NEAREST);
    p_spPass->m_pObject->SetStage(p_spPass->m_pObject->m_uiCurrentStage, aspD3DTextureStage.m_pObject);
    p_spPass->m_pObject->m_spVertexShader = this->spVertexShader;
    NiD3DPass* v5 = p_spPass->m_pObject;
    NiD3DPixelShader* v6 = spPixelShader[0];
    NiD3DPixelShader* v7 = p_spPass->m_pObject->m_spPixelShader.m_pObject;
    if (v7 != v6) {
        if (v7) {
            NiDecRefCount(v7);
        }
        v5->m_spPixelShader.m_pObject = v6;
        if (v6) {
            NiIncRefCount(v6);
        }
    }
    NiD3DPass* v8 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v8->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v8->m_pkRenderStateGroup->SetRenderState(7u, 0, 1);
    NiD3DPass* v9 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v9->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v9->m_pkRenderStateGroup->SetRenderState(14u, 0, 1);
    NiD3DPass* v10 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v10->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v10->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
    NiD3DPass* v11 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v11->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v11->m_pkRenderStateGroup->SetRenderState(19u, 2u, 1);
    if (!v11->m_pkRenderStateGroup) {
        v11->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v11->m_pkRenderStateGroup->SetRenderState(20u, 2u, 1);
    NiD3DPass* v12 = p_spPass->m_pObject;
    if (!v12->m_pkRenderStateGroup) {
        v12->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v12->m_pkRenderStateGroup->SetRenderState(168u, 15u, 1);
    return 1;
}

BSShader* DebugShader::CreateShader() {
    DebugShader* v0 = 0;
    NiDX9ShaderDeclaration* v1 = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 2u, 1u);
    v1->SetEntry(0u, 0u, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0u);
    v1->SetEntry(1u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0u);
    v1->GetD3DDeclaration();
    DebugShader* v2 = static_cast<DebugShader*>(NiMemObject::operator new(sizeof(DebugShader)));
    if (v2) {
        v0 = ::new (v2) DebugShader;
    }
    v0->Initialize();
    v0->LoadShaders();
    v0->LoadStagesAndPasses();
    v0->SetShaderDecl(v1);
    return v0;
}
