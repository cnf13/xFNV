#include "BSShader/Lighting30Shader.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ImageSpaceEffectHDR.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/Lighting30ShaderProperty.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SpeedTreeLeafShader.h"
#include "Fallout Shared/TempEffects/BSTempEffectSimpleDecal.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiZBufferProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DUtility.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "SDK/MSVC/ehvec.h"

#include <cstdint>

// Belongs to CreateConstantMaps: set once the vertex entries are built.
static bool bvscminitialized;
Lighting30ShaderPixelShaderArray Lighting30Shader::spPixelShaders;
Lighting30ShaderPixelShaderArray Lighting30Shader::spPixelShadersLowLight;
Lighting30ShaderVertexShaderArray Lighting30Shader::spVertexShaders;
Lighting30ShaderPassArray Lighting30Shader::spPass;
Lighting30ShaderVSConstantArray Lighting30Shader::kVSConstants;
Lighting30ShaderConstantArray Lighting30Shader::kPSConstants;
NiRTTI Lighting30Shader::ms_RTTI("Lighting30Shader", &NiD3DDefaultShader::ms_RTTI);

Lighting30Shader::Lighting30Shader(
    NiDX9ShaderDeclaration* apShaderDecl,
    NiDX9ShaderDeclaration* apSkinnedShaderDecl,
    NiDX9ShaderDeclaration* apShaderDeclVC,
    NiDX9ShaderDeclaration* apSkinnedShaderDeclVC
)
  : spTexEffectPConstantMap(nullptr), spTexEffectVConstantMap(nullptr), spDefaultPixelConstantMap(nullptr),
    spDefaultVertexConstantMap(nullptr), spShaderDecl(apShaderDecl), spShaderDeclNC(apShaderDecl),
    spShaderDeclVC(apShaderDeclVC), spShaderDeclVCNC(apShaderDeclVC), spSkinnedShaderDecl(apSkinnedShaderDecl),
    spSkinnedShaderDeclNC(apSkinnedShaderDecl), spSkinnedShaderDeclVC(apSkinnedShaderDeclVC),
    spSkinnedShaderDeclVCNC(apSkinnedShaderDeclVC), pLastGeometryBufferData(nullptr), iZWriteEnable(1) {
    iShaderType = 17;
    m_spShaderDecl = apShaderDecl;
    for (uint8_t& rbEnable : bVSConstantEnable) {
        rbEnable = 0;
    }
    for (bool& rbEnable : bPSConstantEnable) {
        rbEnable = false;
    }
    if (BSShaderManager::eShaderLevel < 7) {
        iMaxLights = 5;
    }
}

// Releases the shared (static) shaders, passes and constant map entries and refills both constant tables with black;
// the declaration and constant map members, and the base's shader declaration, are released by their destructors.
Lighting30Shader::~Lighting30Shader() {
    for (NiPointer<NiD3DVertexShader>& spShader : spVertexShaders) {
        spShader = nullptr;
    }
    for (int32_t i = 0; i < 50; ++i) {
        spPixelShaders[i] = nullptr;
        spPixelShadersLowLight[i] = nullptr;
    }
    for (NiPointer<NiD3DPass>& spEntry : spPass) {
        spEntry = nullptr;
    }
    for (NiShaderConstantMapEntry*& rpEntry : pVertexConstantMapEntry) {
        if (rpEntry) {
            rpEntry->DecRefCount();
            rpEntry = nullptr;
        }
    }
    // Walks only the first 10 of the 11 pixel entries, deliberately, as the game does.
    for (uint32_t i = 0; i < 10; ++i) {
        if (pPixelConstantMapEntry[i]) {
            pPixelConstantMapEntry[i]->DecRefCount();
            pPixelConstantMapEntry[i] = nullptr;
        }
    }
    const NiColorA kBlack = NiColorA_BLACK;
    for (NiVector4& rConstant : kVSConstants) {
        rConstant.x = kBlack.r;
        rConstant.y = kBlack.g;
        rConstant.z = kBlack.b;
        rConstant.w = kBlack.a;
    }
    for (uint32_t i = 0; i < 63; ++i) {
        kPSConstants[i].x = kBlack.r;
        kPSConstants[i].y = kBlack.g;
        kPSConstants[i].z = kBlack.b;
        kPSConstants[i].w = kBlack.a;
    }
}

const NiRTTI* Lighting30Shader::GetRTTI() const {
    return &Lighting30Shader::ms_RTTI;
}

// Initializes the base shader, then releases the shared shaders, passes and constant map entries and refills both
// constant tables with black, as the destructor does.
bool Lighting30Shader::Initialize() {
    BSShader::Initialize();
    for (NiPointer<NiD3DVertexShader>& spShader : spVertexShaders) {
        spShader = nullptr;
    }
    for (int32_t i = 0; i < 50; ++i) {
        NiD3DPixelShader* pShader = spPixelShaders[i].m_pObject;
        if (pShader) {
            pShader->DecRefCount();
            spPixelShaders[i].m_pObject = nullptr;
        }
        pShader = spPixelShadersLowLight[i].m_pObject;
        if (pShader) {
            pShader->DecRefCount();
            spPixelShadersLowLight[i].m_pObject = nullptr;
        }
    }
    for (NiPointer<NiD3DPass>& spEntry : spPass) {
        spEntry = nullptr;
    }
    for (NiShaderConstantMapEntry*& rpEntry : pVertexConstantMapEntry) {
        NiShaderConstantMapEntry* pEntry = rpEntry;
        if (pEntry) {
            pEntry->DecRefCount();
            rpEntry = nullptr;
        }
    }
    // Walks only the first 10 of the 11 pixel entries, deliberately, as the game does.
    for (uint32_t i = 0; i < 10; ++i) {
        NiShaderConstantMapEntry* pEntry = pPixelConstantMapEntry[i];
        if (pEntry) {
            pEntry->DecRefCount();
            pPixelConstantMapEntry[i] = nullptr;
        }
    }
    const NiColorA kBlack = NiColorA_BLACK;
    kVSConstants[0].x = kBlack.r;
    kVSConstants[0].y = kBlack.g;
    kVSConstants[0].z = kBlack.b;
    kVSConstants[0].w = kBlack.a;
    // EXE-INLINED none: memcpy (an overlapping rep movsd that repeats entry 0 over the table)
    for (uint32_t i = 1; i < 164; ++i) {
        kVSConstants[i] = kVSConstants[0];
    }
    kPSConstants[0].x = kBlack.r;
    kPSConstants[0].y = kBlack.g;
    kPSConstants[0].z = kBlack.b;
    kPSConstants[0].w = kBlack.a;
    // EXE-INLINED none: memcpy (an overlapping rep movsd that repeats entry 0 over the table)
    for (uint32_t i = 1; i < 63; ++i) {
        kPSConstants[i] = kPSConstants[0];
    }
    return true;
}

uint32_t Lighting30Shader::SetupBoneMatrices(
    NiGeometry* a1,
    const NiSkinInstance* a2,
    const NiSkinPartition::Partition* a3,
    NiGeometryData::RendererData* a4,
    const NiPropertyState* a5,
    const NiDynamicEffectState* a6,
    const NiTransform& a7,
    const NiBound& a8
) {
    if (!a3 ||
        (m_uiCurrentPass ||
         ((!Lighting30Shader::pVertexConstantMapEntry[4] ||
           !Lighting30Shader::pVertexConstantMapEntry[4]->m_bEnabled) &&
          m_spVertexConstantMap.m_pObject != spTexEffectVConstantMap.m_pObject))) {
        return 0;
    }
    BSShaderManager::pRenderer.m_pObject->CalculateBoneMatrices(a2, a7, false, 3u, true);
    NiD3DShaderConstantMap* pTexEffectMap = spTexEffectVConstantMap.m_pObject;
    if (m_spVertexConstantMap.m_pObject == pTexEffectMap) {
        // The texture effect map's own bone matrix entry, looked up by name.
        NiD3DVertexShader* pVertexShader = NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject;
        pVertexShader->SetShaderConstantArray(
            pTexEffectMap->GetEntry("BoneMatrix3"), a2->m_pvBoneMatrices, a3->m_usBones, a2->m_uiNumRegisters,
            a3->m_pusBones
        );
    }
    else {
        NiD3DShader::m_spCurrentPass->m_spVertexShader.m_pObject->SetShaderConstantArray(
            Lighting30Shader::pVertexConstantMapEntry[4], a2->m_pvBoneMatrices, a3->m_usBones, a2->m_uiNumRegisters,
            a3->m_pusBones
        );
    }
    return 0;
}

// Outside the 3x tex-effect passes (0x1E0, 0x1E1) and their z-only forms (0xE, 0xF), sets up the shadow and
// effect textures for the current pass, then applies the stage flags.
void Lighting30Shader::SetupGeometryTextures(const NiPropertyState* apState) {
    Lighting30ShaderProperty* pProp = static_cast<Lighting30ShaderProperty*>(apState->m_aspProps[3].m_pObject);
    uint32_t uiPass = static_cast<uint32_t>(BSShaderManager::eCurrentPass);
    if (uiPass != 0x1E0 && uiPass != 0x1E1 && uiPass != 0xE && uiPass != 0xF) {
        UpdateDiffuseNormalStages(uiPass, pProp);
        UpdatePassTextureStages(uiPass, pProp);
        ApplyTextureStagesFlags();
    }
}

void Lighting30Shader::SetupGeometryConstants(const NiPropertyState* apState) {
    alignas(16) D3DXMATRIX d3dInvWorld;
    BSShaderProperty::RenderPass* pRenderPass = BSShaderManager::pCurrentRenderPass;
    NiGeometry* pgeometry = pRenderPass->pGeometry;
    BSShaderManager::eBSSMPassEnum eCurrentPass = BSShaderManager::eCurrentPass;
    const NiTransform& worldTransform = pgeometry->m_kWorld;
    const NiBound* pWorldBound = pgeometry->m_kWorldBound;
    if (!pWorldBound) {
        pWorldBound = &NiAVObject::NullBoundS;
    }
    NiGeometryBufferData* pbufferData = pgeometry->m_spModelData.m_pObject->m_pkBuffData;
    const NiSkinInstance* pskin = pgeometry->m_spSkinInstance.m_pObject;
    Lighting30ShaderProperty* pprop = static_cast<Lighting30ShaderProperty*>(apState->m_aspProps[3].m_pObject);
    NiMaterialProperty* pmaterialProperty = static_cast<NiMaterialProperty*>(apState->m_aspProps[2].m_pObject);
    // The constant-alpha passes with alpha blending write no depth.
    if (eCurrentPass - BSShaderManager::BSSM_3XCONSTALPHA <= 3 &&
        (static_cast<NiAlphaProperty*>(apState->m_aspProps[0].m_pObject)->m_uFlags &
         NiAlphaProperty::ALPHA_BLEND_MASK) != 0) {
        BSRenderState::SetZWriteEnable(false, BSRS_LOCK);
    }
    BSUtilities::GetInverseWorldMatrix(worldTransform, pskin, d3dInvWorld);
    m_pkD3DRenderer->SetModelTransform(worldTransform, false);
    if (eCurrentPass == BSShaderManager::BSSM_3XTEXEFFECT || eCurrentPass == BSShaderManager::BSSM_3XTEXEFFECT_S) {
        UpdatePipelineTexEffect(pprop, NiD3DShader::m_spCurrentPass);
        UpdateTexEffectConstants(pgeometry, pprop, NiD3DShader::m_spCurrentPass);
        SetupGeometryConstants_EyePosition(d3dInvWorld, worldTransform);
    }
    else {
        if (eCurrentPass == BSShaderManager::BSSM_3XZONLY_TEXEFFECT ||
            eCurrentPass == BSShaderManager::BSSM_3XZONLY_TEXEFFECT_S) {
            UpdatePipelineZOnlyTexEffect(pprop, NiD3DShader::m_spCurrentPass);
        }
        SetupGeometryConstants_Lights(
            pprop, pRenderPass, eCurrentPass, pmaterialProperty, d3dInvWorld, pskin, worldTransform
        );
        SetupGeometryConstants_ObjToCubemap(pRenderPass, eCurrentPass, pskin, worldTransform);
        SetupGeometryConstants_ShadowProjection(pskin, worldTransform);
        if (Lighting30Shader::pVertexConstantMapEntry[11]->m_bEnabled) {
            Lighting30Shader::kVSConstants[38].x = BSShaderManager::fTimerValues[0] / 3600.0 * 6.283180236816406 *
                static_cast<double>(pprop->iRefractionFirePeriod);
        }
        SetupGeometryConstants_WorldBoundCenter(*pWorldBound);
        SetupGeometryConstants_EyePosition(d3dInvWorld, worldTransform);
        SetupGeometryConstants_HairTint(pprop);
        SetupGeometryConstants_MaterialAlpha(*pWorldBound, pprop, apState, eCurrentPass);
        SetupGeometryConstants_Emittance(pmaterialProperty, pprop);
        SetupGeometryConstants_RefractionPower(pprop);
        if (Lighting30Shader::pPixelConstantMapEntry[6]->m_bEnabled) {
            Lighting30Shader::kPSConstants[7].x = pprop->fParallaxOccMaxPasses;
            Lighting30Shader::kPSConstants[7].y = pprop->fParallaxOccScale;
        }
        SetupGeometryConstants_Envmap(pprop);
        SetupGeometryConstants_WaterFog();
        // The shadowed passes clip to the shadow camera's frustum: set up its planes in clip space once per light.
        if (eCurrentPass - BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW <= 3) {
            ShadowSceneLight* plight = *BSShaderManager::pCurrentRenderPass->ppSceneLights;
            if (!plight->bClipSpacePlanesReady && !BSShaderManager::GetAccumulator()->bWaterReflection &&
                !BSShaderManager::GetAccumulator()->bWaterRefraction) {
                alignas(16) D3DXMATRIX kD3DView;
                alignas(16) float afClipPlanes[24];
                alignas(16) D3DXMATRIX kworldtoclip;
                alignas(16) D3DXMATRIX kprojection;
                alignas(16) D3DXMATRIX kinv;
                NiPoint3 kLightingOffset;
                kD3DView = m_pkD3DRenderer->m_kD3DView;
                BSShaderManager::pShadowSceneNode[0]->GetLightingOffset(&kLightingOffset);
                // The view's translation row relative to the lighting offset.
                NiPoint3 kOffset;
                NiPoint3 kAxis;
                kAxis.x = kD3DView._11;
                kAxis.y = kD3DView._21;
                kAxis.z = kD3DView._31;
                float fOffsetX = kLightingOffset.x + NiRenderer::PosAdjust.x;
                kOffset.x = fOffsetX;
                float fOffsetY = kLightingOffset.y + NiRenderer::PosAdjust.y;
                kOffset.y = fOffsetY;
                float fOffsetZ = kLightingOffset.z + NiRenderer::PosAdjust.z;
                kOffset.z = fOffsetZ;
                kD3DView._41 = -kOffset.Dot(kAxis);
                kAxis.x = kD3DView._12;
                kAxis.y = kD3DView._22;
                kAxis.z = kD3DView._32;
                kOffset.x = fOffsetX;
                kOffset.y = fOffsetY;
                kOffset.z = fOffsetZ;
                kD3DView._42 = -kOffset.Dot(kAxis);
                kAxis.x = kD3DView._13;
                kAxis.y = kD3DView._23;
                kAxis.z = kD3DView._33;
                kOffset.x = fOffsetX;
                kOffset.y = fOffsetY;
                kOffset.z = fOffsetZ;
                kD3DView._43 = -kOffset.Dot(kAxis);
                kprojection = m_pkD3DRenderer->m_kD3DProj;
                D3DXMatrixMultiply_Thunk(&kworldtoclip, &kD3DView, &kprojection);
                D3DXMatrixInverse_Thunk(&kinv, nullptr, &kworldtoclip);
                D3DXMatrixTranspose_Thunk(&kworldtoclip, &kinv);
                // Planes transform by the inverse transpose of the point transform.
                for (int32_t i = 0; i < 6; ++i) {
                    const NiPlane& kCullingPlane = plight->kShadowCameraPlanes.m_akCullingPlanes[i];
                    D3DXPLANE kplane;
                    D3DXPLANE kClipPlane;
                    NiPoint3 kNormal = kCullingPlane.m_kNormal;
                    kplane.a = kNormal.x;
                    kplane.b = kNormal.y;
                    kplane.c = kNormal.z;
                    kplane.d = -kCullingPlane.m_fConstant;
                    D3DXPlaneNormalize_Thunk(&kplane, &kplane);
                    D3DXPlaneTransform_Thunk(&kClipPlane, &kplane, &kworldtoclip);
                    afClipPlanes[4 * i] = kClipPlane.a;
                    afClipPlanes[4 * i + 1] = kClipPlane.b;
                    afClipPlanes[4 * i + 2] = kClipPlane.c;
                    afClipPlanes[4 * i + 3] = kClipPlane.d;
                }
                NiDX9Renderer* pkRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
                for (int32_t i = 0; i < 6; ++i) {
                    pkRenderer->m_pkD3DDevice9->SetClipPlane(i, &afClipPlanes[4 * i]);
                }
                plight->SetClipSpacePlanes(afClipPlanes);
            }
            else if (
                !BSShaderManager::GetAccumulator()->bWaterReflection &&
                !BSShaderManager::GetAccumulator()->bWaterRefraction
            ) {
                float* pfClipSpacePlanes = plight->pfClipSpacePlanes;
                NiDX9Renderer* pkRenderer = static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer);
                for (int32_t i = 0; i < 6; ++i) {
                    pkRenderer->m_pkD3DDevice9->SetClipPlane(i, pfClipSpacePlanes);
                    pfClipSpacePlanes += 4;
                }
            }
        }
    }
    NiD3DPass* pPass = NiD3DShader::m_spCurrentPass;
    if (m_spPixelConstantMap.m_pObject) {
        NiD3DPixelShader* pPixelShader = pPass->m_spPixelShader.m_pObject;
        if (pPixelShader) {
            pPass->m_spPixelConstantMap.m_pObject->SetShaderConstants(
                pPixelShader, pgeometry, pskin, nullptr, pbufferData, apState, nullptr, worldTransform, *pWorldBound, 0,
                true
            );
            pPass = NiD3DShader::m_spCurrentPass;
        }
    }
    if (m_spVertexConstantMap.m_pObject) {
        NiD3DVertexShader* pVertexShader = pPass->m_spVertexShader.m_pObject;
        if (pVertexShader) {
            pPass->m_spVertexConstantMap.m_pObject->SetShaderConstants(
                pVertexShader, pgeometry, pskin, nullptr, pbufferData, apState, nullptr, worldTransform, *pWorldBound,
                0, true
            );
        }
    }
    NiStencilProperty* pStencil = static_cast<NiStencilProperty*>(apState->m_aspProps[4].m_pObject);
    if (pStencil && (pStencil->m_uFlags & NiStencilProperty::ENABLE_MASK) != 0) {
        m_pkD3DRenderState->ApplyStencil(pStencil);
    }
}

// Outside the 3x render-normals, 3x envmap, 3x tex-effect and water-depth passes (unsigned pass ranges 0x1CC-0x1DC,
// 0x1BD-0x1C3, 0x1E0-0x1E1, 0x2D8-0x2EF), the base shader's alpha blending runs.
void Lighting30Shader::SetupGeometryAlphaBlending(const NiPropertyState* apState) {
    uint32_t uiPass = static_cast<uint32_t>(BSShaderManager::eCurrentPass);
    if ((uiPass < 0x1CC || uiPass > 0x1DC) && (uiPass < 0x1BD || uiPass > 0x1C3) &&
        (uiPass < 0x1E0 || uiPass > 0x1E1) && (uiPass < 0x2D8 || uiPass > 0x2EF)) {
        BSShader::SetupGeometryAlphaBlending(apState);
    }
}

void Lighting30Shader::PostGeometry(const NiPropertyState* apState) {
    BSShaderManager::eBSSMPassEnum ePass = BSShaderManager::eCurrentPass;
    if (ePass == BSShaderManager::BSSM_3XTEXEFFECT || ePass == BSShaderManager::BSSM_3XTEXEFFECT_S) {
        BSRenderState::SetZFunc(NiZBufferProperty::TEST_LESSEQUAL, BSRS_NOLOCK);
    }
    NiStencilProperty* pStencil = static_cast<NiStencilProperty*>(apState->m_aspProps[4].m_pObject);
    if ((pStencil) && ((pStencil->m_uFlags & NiStencilProperty::ENABLE_MASK) != 0)) {
        BSRenderState::SetStencilEnable(false, BSRS_NOLOCK);
    }

    // The alpha property is read without a null test, as the game does.
    NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apState->m_aspProps[0].m_pObject);
    if (ePass - BSShaderManager::BSSM_3XCONSTALPHA <= 3 &&
        (pAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK) != 0) {
        --BSRenderState::iLock[1];
        BSRenderState::SetZWriteEnable(true, BSRS_NOLOCK);
    }
}

void Lighting30Shader::RecreateRendererData() {
    uint32_t i;
    NiD3DVertexShader* v3;
    uint32_t j;
    NiD3DPixelShader* v5;
    NiD3DPixelShader* v6;
    if (m_spShaderDecl.m_pObject) {
        for (i = 0; i < 57; ++i) {
            v3 = Lighting30Shader::spVertexShaders[i];
            if (v3) {
                v3->RecreateRendererData();
            }
        }
    }
    for (j = 0; j < 50; ++j) {
        v5 = Lighting30Shader::spPixelShaders[j];
        if (v5) {
            v5->RecreateRendererData();
        }
        v6 = Lighting30Shader::spPixelShadersLowLight[j];
        if (v6) {
            v6->RecreateRendererData();
        }
    }
    NiD3DShader::RecreateRendererData();
}

void Lighting30Shader::InitPass(uint32_t auiCurrentPass) {
    NiD3DShader::m_spCurrentPass = SelectPass(static_cast<BSShaderManager::eBSSMPassEnum>(auiCurrentPass));
    Lighting30Shader::kPSConstants[0].x = NiColorA_BLACK.r;
    Lighting30Shader::kPSConstants[0].y = NiColorA_BLACK.g;
    Lighting30Shader::kPSConstants[0].z = NiColorA_BLACK.b;
    Lighting30Shader::kPSConstants[0].w = NiColorA_BLACK.a;
    Lighting30Shader::pPixelConstantMapEntry[0]->m_bEnabled = true;
    Lighting30Shader::bAmbientEnabled = 1;
    Lighting30Shader::bMatAlphaEnabled = 1;
    if (auiCurrentPass == 480 || auiCurrentPass == 481) {
        m_spPixelConstantMap = spTexEffectPConstantMap;
        m_spVertexConstantMap = spTexEffectVConstantMap;
    }
    else {
        m_spPixelConstantMap.operator=(spDefaultPixelConstantMap);
        m_spVertexConstantMap = spDefaultVertexConstantMap;
    }
}

void Lighting30Shader::SetupPassConstants(uint32_t auiCurrentPass) {
    if (auiCurrentPass != 480 && auiCurrentPass != 481) {
        int32_t ioffset;
        if (auiCurrentPass > 467) {
            if (auiCurrentPass > 730) {
                switch (auiCurrentPass) {
                    case 731u:
                    case 737u:
                    case 743u:
                    case 749u: {
                        ioffset = 105;
                        break;
                    }
                    case 736u:
                    case 742u:
                    case 748u: {
                        goto LABEL_23;
                    }
                    default: {
                    LABEL_26:;
                        ioffset = auiCurrentPass - 376;
                        break;
                    }
                }
            }
            else {
                if (auiCurrentPass == 730) {
                LABEL_23:;
                    ioffset = 104;
                }
                else {
                    switch (auiCurrentPass) {
                        case 468u: {
                            ioffset = 90;
                            break;
                        }
                        case 469u: {
                            ioffset = 91;
                            break;
                        }
                        case 470u: {
                            ioffset = 92;
                            break;
                        }
                        case 471u: {
                            ioffset = 93;
                            break;
                        }
                        case 472u: {
                            ioffset = 94;
                            break;
                        }
                        case 473u: {
                            ioffset = 95;
                            break;
                        }
                        case 474u: {
                            ioffset = 96;
                            break;
                        }
                        default: {
                            goto LABEL_26;
                        }
                    }
                }
            }
        }
        else {
            if (auiCurrentPass == 467) {
                ioffset = 89;
            }
            else {
                switch (auiCurrentPass) {
                    case 10u: {
                        ioffset = 1;
                        break;
                    }
                    case 11u: {
                        ioffset = 2;
                        break;
                    }
                    case 12u: {
                        ioffset = 3;
                        break;
                    }
                    case 13u: {
                        ioffset = 4;
                        break;
                    }
                    case 14u: {
                        ioffset = 5;
                        break;
                    }
                    case 15u: {
                        ioffset = 6;
                        break;
                    }
                    default: {
                        goto LABEL_26;
                    }
                }
            }
        }
        // Entry i's flag is bit i + 1. Over 17 entries the mask never reaches bit 31, so a shift gives the same masks
        // as a rotate.
        int32_t i = 0;
        uint32_t uiBit = 1;
        do {
            NiShaderConstantMapEntry* pEntry = Lighting30Shader::pVertexConstantMapEntry[i];
            if (pEntry) {
                pEntry->m_bEnabled = ((uiBit << 1) & Lighting30Shader::ulVertexConstantFlags[ioffset]) != 0;
            }
            uiBit <<= 1;
            Lighting30Shader::bVSConstantEnable[i++] = (uiBit & Lighting30Shader::ulVertexConstantFlags[ioffset]) != 0;
        }
        while (i < 17);
        if (Lighting30Shader::pPixelConstantMapEntry[0]) {
            Lighting30Shader::pPixelConstantMapEntry[0]->m_bEnabled =
                (Lighting30Shader::ulPixelConstantFlags[ioffset] & 2) != 0;
        }
        uint32_t v7 = Lighting30Shader::ulPixelConstantFlags[ioffset];
        uint32_t* v8 = &Lighting30Shader::ulPixelConstantFlags[ioffset];
        Lighting30Shader::bPSConstantEnable[0] = (v7 & 2) != 0;
        if (Lighting30Shader::pPixelConstantMapEntry[1]) {
            Lighting30Shader::pPixelConstantMapEntry[1]->m_bEnabled = (v7 & 4) != 0;
        }
        uint32_t v9 = *v8;
        Lighting30Shader::bPSConstantEnable[1] = (*v8 & 4) != 0;
        if (Lighting30Shader::pPixelConstantMapEntry[2]) {
            Lighting30Shader::pPixelConstantMapEntry[2]->m_bEnabled = (v9 & 8) != 0;
        }
        uint32_t v10 = *v8;
        Lighting30Shader::bPSConstantEnable[2] = (*v8 & 8) != 0;
        if (Lighting30Shader::pPixelConstantMapEntry[3]) {
            Lighting30Shader::pPixelConstantMapEntry[3]->m_bEnabled = (v10 & 16) != 0;
        }
        uint32_t v11 = *v8;
        Lighting30Shader::bPSConstantEnable[3] = (*v8 & 16) != 0;
        if (Lighting30Shader::pPixelConstantMapEntry[4]) {
            Lighting30Shader::pPixelConstantMapEntry[4]->m_bEnabled = (v11 & 32) != 0;
        }
        uint32_t v12 = *v8;
        Lighting30Shader::bPSConstantEnable[4] = (*v8 & 32) != 0;
        if (Lighting30Shader::pPixelConstantMapEntry[5]) {
            Lighting30Shader::pPixelConstantMapEntry[5]->m_bEnabled = (v12 & 64) != 0;
        }
        uint32_t v13 = *v8;
        Lighting30Shader::bPSConstantEnable[5] = (*v8 & 64) != 0;
        if (Lighting30Shader::pPixelConstantMapEntry[6]) {
            Lighting30Shader::pPixelConstantMapEntry[6]->m_bEnabled = (v13 & 128) != 0;
        }
        uint32_t v14 = *v8;
        Lighting30Shader::bPSConstantEnable[6] = (*v8 & 128) != 0;
        if (Lighting30Shader::pPixelConstantMapEntry[7]) {
            Lighting30Shader::pPixelConstantMapEntry[7]->m_bEnabled = static_cast<uint8_t>(v14 >> 8) & 1;
        }
        uint32_t v15 = *v8;
        Lighting30Shader::bPSConstantEnable[7] = static_cast<uint8_t>((*v8) >> 8) & 1;
        if (Lighting30Shader::pPixelConstantMapEntry[8]) {
            Lighting30Shader::pPixelConstantMapEntry[8u]->m_bEnabled = (v15 & 512) != 0;
        }
        uint32_t v16 = *v8;
        Lighting30Shader::bPSConstantEnable[8] = (*v8 & 512) != 0;
        if (Lighting30Shader::pPixelConstantMapEntry[9]) {
            Lighting30Shader::pPixelConstantMapEntry[9]->m_bEnabled = (v16 & 1024) != 0;
        }
        Lighting30Shader::bPSConstantEnable[9] = (*v8 & 1024) != 0;
        m_spVertexConstantMap.m_pObject->SortByEnabled();
        m_spPixelConstantMap.m_pObject->SortByEnabled();
        if (Lighting30Shader::pVertexConstantMapEntry[15]->m_bEnabled) {
            BSFogProperty* pbsfog = BSShaderManager::GetFogProperty(BSShaderManager::cSceneGraph);
            if (pbsfog) {
                float fStartDistance = pbsfog->fStartDistance;
                float r = pbsfog->m_kColor.r;
                float fEndDistance = pbsfog->fEndDistance;
                float g = pbsfog->m_kColor.g;
                float fPower = pbsfog->fPower;
                float b = pbsfog->m_kColor.b;
                if (fEndDistance == 0.0f && 0.0f == fStartDistance) {
                    Lighting30Shader::kVSConstants[44].x = 500000.0f;
                    Lighting30Shader::kVSConstants[44].y = 0.0f;
                    Lighting30Shader::kVSConstants[44].z = fPower;
                    Lighting30Shader::kVSConstants[44].w = 0.0f;
                    Lighting30Shader::kVSConstants[45].x = NiColorA_BLACK.r;
                    Lighting30Shader::kVSConstants[45].y = NiColorA_BLACK.g;
                    Lighting30Shader::kVSConstants[45].z = NiColorA_BLACK.b;
                    Lighting30Shader::kVSConstants[45].w = NiColorA_BLACK.a;
                }
                else {
                    Lighting30Shader::kVSConstants[44].x = pbsfog->fEndDistance;
                    float v26 = fEndDistance - fStartDistance;
                    Lighting30Shader::kVSConstants[44].y = v26;
                    Lighting30Shader::kVSConstants[44].z = fPower;
                    Lighting30Shader::kVSConstants[44].w = 0.0f;
                    Lighting30Shader::kVSConstants[45].x = r;
                    Lighting30Shader::kVSConstants[45].y = g;
                    Lighting30Shader::kVSConstants[45].z = b;
                    Lighting30Shader::kVSConstants[45].w = 1.0f;
                }
            }
        }
        if (Lighting30Shader::pVertexConstantMapEntry[16u]->m_bEnabled) {
            float v21 = BSShaderLightingProperty::pShaderLightConstants[30].X() - NiRenderer::PosAdjust.x;
            float v23 = BSShaderLightingProperty::pShaderLightConstants[30].Y() - NiRenderer::PosAdjust.y;
            float v25 = BSShaderLightingProperty::pShaderLightConstants[30].Z() - NiRenderer::PosAdjust.z;
            Lighting30Shader::kVSConstants[46].x = v21;
            Lighting30Shader::kVSConstants[46].y = v23;
            Lighting30Shader::kVSConstants[46].z = v25;
            Lighting30Shader::kVSConstants[46].w = 1.0f;
        }
    }
}

NiD3DShaderDeclaration* Lighting30Shader::GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) {
    uint8_t v3 = (uint8_t)((((int32_t)(apProperty->ulFlags[1] & 32)) != 0) ||
                           (apGeometry->m_spModelData.m_pObject->m_pkColor != nullptr));
    if (((int32_t)(apProperty->ulFlags[0] & 2)) != 0) {
        if (v3) {
            return spSkinnedShaderDeclVC.m_pObject;
        }

        return spSkinnedShaderDecl.m_pObject;
    }

    if (v3) {
        return spShaderDeclVC.m_pObject;
    }

    return spShaderDecl.m_pObject;
}

// Builds the vertex map's entries once (keeping a reference on each in pVertexConstantMapEntry), then gives the shader
// a fresh, empty vertex map; builds the pixel map's entries the same way when the pixel map is missing, then a fresh
// pixel map. The default maps take the current maps, and the texture-effect maps are built when missing. As the game
// does, each map holding the entries is replaced right after they are built. An empty string is passed as the variable
// names.
void Lighting30Shader::CreateConstantMaps() {
    if (!m_spVertexConstantMap.m_pObject && !bvscminitialized) {
        bvscminitialized = true;
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        m_spVertexConstantMap.SetNew(pMap);
        m_spVertexConstantMap.m_pObject->AddPredefinedEntry(
            "WorldViewProjTranspose", 0x20000009, 0, "", 0, 0, nullptr, false
        );
        SetVertexConstantMapEntry(
            &pVertexConstantMapEntry[0], m_spVertexConstantMap.m_pObject->GetEntry("WorldViewProjTranspose")
        );
        m_spVertexConstantMap.m_pObject->AddPredefinedEntry(
            "SkinWorldViewProjTranspose", 0x20000009, 0x14, "", 0, 0, nullptr, false
        );
        SetVertexConstantMapEntry(
            &pVertexConstantMapEntry[1], m_spVertexConstantMap.m_pObject->GetEntry("SkinWorldViewProjTranspose")
        );
        m_spVertexConstantMap.m_pObject->AddPredefinedEntry(
            "WorldViewTranspose", 0x20000009, 4, "", 0, 0, nullptr, false
        );
        SetVertexConstantMapEntry(
            &pVertexConstantMapEntry[2], m_spVertexConstantMap.m_pObject->GetEntry("WorldViewTranspose")
        );
        m_spVertexConstantMap.m_pObject->AddPredefinedEntry(
            "SkinWorldViewTranspose", 0x20000009, 0x18, "", 0, 0, nullptr, false
        );
        SetVertexConstantMapEntry(
            &pVertexConstantMapEntry[3], m_spVertexConstantMap.m_pObject->GetEntry("SkinWorldViewTranspose")
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "BoneMatrix3", 0x20000009, 0x120000, 0x34, 0x36, nullptr, 0, 0, nullptr, false
        );
        SetVertexConstantMapEntry(
            &pVertexConstantMapEntry[4], m_spVertexConstantMap.m_pObject->GetEntry("BoneMatrix3")
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "PrevBoneMatrix3", 0x10000009, 0, 0x6C, 0x36, "", 0x360, 4, ShadowLightShader::kPrevBones, false
        );
        SetVertexConstantMapEntry(
            &pVertexConstantMapEntry[5], m_spVertexConstantMap.m_pObject->GetEntry("PrevBoneMatrix3")
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "ObjToCubeMap", 0x10000009, 0, 8, 4, "", 0x40, 4, &kVSConstants[8], false
        );
        SetVertexConstantMapEntry(
            &pVertexConstantMapEntry[6], m_spVertexConstantMap.m_pObject->GetEntry("ObjToCubeMap")
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "EyePos", 0x10000007, 0, 0x28, 1, "", 0x10, 4, &kVSConstants[40], false
        );
        SetVertexConstantMapEntry(&pVertexConstantMapEntry[9], m_spVertexConstantMap.m_pObject->GetEntry("EyePos"));
        m_spVertexConstantMap.m_pObject->AddEntry(
            "WorldEyePos", 0x10000007, 0, 0x2E, 1, "", 0x10, 4, &kVSConstants[46], false
        );
        SetVertexConstantMapEntry(
            &pVertexConstantMapEntry[16], m_spVertexConstantMap.m_pObject->GetEntry("WorldEyePos")
        );
        m_spVertexConstantMap.m_pObject->AddEntry("Fog", 0x1000000C, 0, 0x2C, 2, "", 0x20, 4, &kVSConstants[44], false);
        SetVertexConstantMapEntry(&pVertexConstantMapEntry[15], m_spVertexConstantMap.m_pObject->GetEntry("Fog"));
        m_spVertexConstantMap.m_pObject->AddEntry(
            "DepthData", 0x10000007, 0, 0x29, 1, "", 0x10, 4, &kVSConstants[41], false
        );
        SetVertexConstantMapEntry(&pVertexConstantMapEntry[10], m_spVertexConstantMap.m_pObject->GetEntry("DepthData"));
        m_spVertexConstantMap.m_pObject->AddEntry(
            "Refraction", 0x10000007, 0, 0x26, 1, "", 0x10, 4, &kVSConstants[38], false
        );
        SetVertexConstantMapEntry(
            &pVertexConstantMapEntry[11], m_spVertexConstantMap.m_pObject->GetEntry("Refraction")
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "WorldBoundCenter", 0x10000007, 0, 0x27, 1, "", 0x10, 4, &kVSConstants[39], false
        );
        SetVertexConstantMapEntry(
            &pVertexConstantMapEntry[12], m_spVertexConstantMap.m_pObject->GetEntry("WorldBoundCenter")
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "ShadowProjectionData", 0x10000009, 0, 0x20, 6, "", 0x60, 4, &kVSConstants[32], false
        );
        SetVertexConstantMapEntry(
            &pVertexConstantMapEntry[13], m_spVertexConstantMap.m_pObject->GetEntry("ShadowProjectionData")
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "PrevWorldViewPT", 0x10000009, 0, 0xC, 4, "", 0x40, 4, &ShadowLightShader::kPrevModelViewProj, false
        );
        SetVertexConstantMapEntry(
            &pVertexConstantMapEntry[7], m_spVertexConstantMap.m_pObject->GetEntry("PrevWorldViewPT")
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "PrevWorldViewT", 0x10000009, 0, 0x10, 4, "", 0x40, 4, &ShadowLightShader::kPrevModelView, false
        );
        SetVertexConstantMapEntry(
            &pVertexConstantMapEntry[8], m_spVertexConstantMap.m_pObject->GetEntry("PrevWorldViewT")
        );
        m_spVertexConstantMap.m_pObject->AddEntry(
            "SkinToCubeMap", 0x10000009, 0, 0x1C, 4, "", 0x40, 4, &kVSConstants[28], false
        );
        SetVertexConstantMapEntry(
            &pVertexConstantMapEntry[14], m_spVertexConstantMap.m_pObject->GetEntry("SkinToCubeMap")
        );
        pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        m_spVertexConstantMap.SetNew(pMap);
    }
    if (!m_spPixelConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        m_spPixelConstantMap = pMap;
        m_spPixelConstantMap.m_pObject->AddEntry(
            "ambientcolor", 0x10000007, 0, 0, 1, "", 0x10, 4, &kPSConstants[0], false
        );
        NiShaderConstantMapEntry* pEntry = m_spPixelConstantMap.m_pObject->GetEntry("ambientcolor");
        // EXE-INLINED 0x66B0D0
        NiShaderConstantMapEntry* pOldEntry = pPixelConstantMapEntry[0];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[0] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spPixelConstantMap.m_pObject->AddEntry("eyepos", 0x10000007, 0, 1, 1, "", 0x10, 4, &kPSConstants[1], false);
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("eyepos");
        // EXE-INLINED 0x66B0D0
        pOldEntry = pPixelConstantMapEntry[1];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[1] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spPixelConstantMap.m_pObject->AddEntry("hairtint", 0x10000007, 0, 2, 1, "", 0x10, 4, &kPSConstants[2], false);
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("hairtint");
        // EXE-INLINED 0x66B0D0
        pOldEntry = pPixelConstantMapEntry[2];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[2] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spPixelConstantMap.m_pObject->AddEntry("matalpha", 0x10000007, 0, 3, 1, "", 0x10, 4, &kPSConstants[3], false);
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("matalpha");
        // EXE-INLINED 0x66B0D0
        pOldEntry = pPixelConstantMapEntry[3];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[3] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spPixelConstantMap.m_pObject->AddEntry("matemit", 0x10000007, 0, 4, 1, "", 0x10, 4, &kPSConstants[4], false);
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("matemit");
        // EXE-INLINED 0x66B0D0
        pOldEntry = pPixelConstantMapEntry[4];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[4] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spPixelConstantMap.m_pObject->AddEntry(
            "refractpower", 0x10000007, 0, 6, 1, "", 0x10, 4, &kPSConstants[6], false
        );
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("refractpower");
        // EXE-INLINED 0x66B0D0
        pOldEntry = pPixelConstantMapEntry[5];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[5] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spPixelConstantMap.m_pObject->AddEntry(
            "parallaxdata", 0x10000007, 0, 7, 1, "", 0x10, 4, &kPSConstants[7], false
        );
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("parallaxdata");
        // EXE-INLINED 0x66B0D0
        pOldEntry = pPixelConstantMapEntry[6];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[6] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spPixelConstantMap.m_pObject->AddEntry("waterfog", 0x1000000C, 0, 9, 2, "", 0x20, 4, &kPSConstants[9], false);
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("waterfog");
        // EXE-INLINED 0x66B0D0
        pOldEntry = pPixelConstantMapEntry[8];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[8] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spPixelConstantMap.m_pObject->AddEntry(
            "envmaptoggles", 0x10000007, 0, 0xB, 1, "", 0x10, 4, &kPSConstants[11], false
        );
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("envmaptoggles");
        // EXE-INLINED 0x66B0D0
        pOldEntry = pPixelConstantMapEntry[7];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[7] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        m_spPixelConstantMap.m_pObject->AddEntry(
            "lightdata", 0x10000009, 0, 0x14, 0x29, "", 0x290, 4, &kPSConstants[20], false
        );
        pEntry = m_spPixelConstantMap.m_pObject->GetEntry("lightdata");
        // EXE-INLINED 0x66B0D0
        pOldEntry = pPixelConstantMapEntry[9];
        if (pOldEntry != pEntry) {
            if (pOldEntry) {
                pOldEntry->DecRefCount();
            }
            pPixelConstantMapEntry[9] = pEntry;
            if (pEntry) {
                pEntry->IncRefCount();
            }
        }
        pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        m_spPixelConstantMap = pMap;
    }
    spDefaultVertexConstantMap = m_spVertexConstantMap;
    spDefaultPixelConstantMap = m_spPixelConstantMap;
    if (!spTexEffectVConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_VERTEX);
        spTexEffectVConstantMap = pMap;
        spTexEffectVConstantMap.m_pObject->AddPredefinedEntry(
            "WorldViewProjTranspose", 0x20000009, 0, "", 0, 0, nullptr, false
        );
        spTexEffectVConstantMap.m_pObject->AddPredefinedEntry(
            "SkinWorldViewProjTranspose", 0x20000009, 4, "", 0, 0, nullptr, false
        );
        spTexEffectVConstantMap.m_pObject->AddEntry(
            "BoneMatrix3", 0x20000009, 0x120000, 0xE, 0x36, nullptr, 0, 0, nullptr, false
        );
        spTexEffectVConstantMap.m_pObject->AddEntry(
            "EyePosition", 0x10000007, 0, 8, 1, "", 0x10, 4, &kVSConstants[40], false
        );
        spTexEffectVConstantMap.m_pObject->AddEntry("U Offset", 0x10000004, 0, 9, 1, "", 4, 4, &fTextureUOffset, false);
        spTexEffectVConstantMap.m_pObject->AddEntry(
            "V Offset", 0x10000004, 0, 0xA, 1, "", 4, 4, &fTextureVOffset, false
        );
        spTexEffectVConstantMap.m_pObject->AddEntry(
            "InvBoundDiameter", 0x10000004, 0, 0xB, 1, "", 4, 4, &fInvBoundDiameter, false
        );
        spTexEffectVConstantMap.m_pObject->AddEntry(
            "FogParam", 0x10000007, 0, 0xC, 1, "", 0x10, 4, &TexEffectFogParam, false
        );
        spTexEffectVConstantMap.m_pObject->AddEntry(
            "FogColor", 0x10000007, 0, 0xD, 1, "", 0x10, 4, &TexEffectFogColor, false
        );
    }
    if (!spTexEffectPConstantMap.m_pObject) {
        NiD3DShaderConstantMap* pMap = new NiD3DShaderConstantMap(m_pkD3DRenderer, NiGPUProgram::PROGRAM_PIXEL);
        spTexEffectPConstantMap = pMap;
        spTexEffectPConstantMap.m_pObject->AddEntry("Fill Color", 0x10000007, 0, 0, 1, "", 0x10, 4, &FillColor, false);
        spTexEffectPConstantMap.m_pObject->AddEntry("Rim Color", 0x10000007, 0, 1, 1, "", 0x10, 4, &RimColor, false);
        spTexEffectPConstantMap.m_pObject->AddEntry("Vars", 0x10000007, 0, 2, 1, "", 0x10, 4, &fVars, false);
    }
}

void Lighting30Shader::ReloadShaders() {
    LoadShaders();
    PresetStages();
}

void Lighting30Shader::ClearPassStages() {
    for (const NiPointer<NiD3DPass>& spEntry : spPass) {
        ClearPass(spEntry.m_pObject);
    }
}

// Makes the updated pass current and applies its render states.
[[clang::disable_tail_calls]] void
Lighting30Shader::SetRenderStatesForPass(uint32_t auiPass, Lighting30ShaderProperty* apL30Prop) {
    NiD3DPass* pPass = UpdatePass(static_cast<BSShaderManager::eBSSMPassEnum>(auiPass), apL30Prop);
    NiD3DShader::m_spCurrentPass = pPass;
    pPass->m_pkRenderStateGroup->SetRenderStates();
}

void Lighting30Shader::LoadShaders() {
    LoadVertexShaders();
    LoadPixelShaders();
}

void Lighting30Shader::LoadVertexShaders() {
    BSShaderMacro apMacro[57];
    apMacro[0].strPath = "lighting\\3x\\SM3Lighting.v.hlsl";
    apMacro[0].kMacros[0].Name = nullptr;
    std::memset(
        &apMacro[0].kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    apMacro[1].strPath = "lighting\\3x\\SM3Lighting.v.hlsl";
    apMacro[1].kMacros[0].Name = nullptr;
    std::memset(
        &apMacro[1].kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    apMacro[2].strPath = "lighting\\3x\\SM3Lighting.v.hlsl";
    apMacro[2].kMacros[0].Name = "SKIN";
    apMacro[2].kMacros[0].Definition = "";
    apMacro[2].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[2].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[3].strPath = "lighting\\3x\\SM3Lighting.v.hlsl";
    apMacro[3].kMacros[0].Name = "VC";
    apMacro[3].kMacros[0].Definition = "";
    apMacro[3].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[3].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[4].strPath = "lighting\\3x\\SM3Lighting.v.hlsl";
    apMacro[4].kMacros[0].Name = "VC";
    apMacro[4].kMacros[0].Definition = "";
    apMacro[4].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[4].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[5].strPath = "lighting\\3x\\SM3Lighting.v.hlsl";
    apMacro[5].kMacros[0].Name = "SKIN";
    apMacro[5].kMacros[0].Definition = "";
    apMacro[5].kMacros[1].Name = "VC";
    apMacro[5].kMacros[1].Definition = "";
    apMacro[5].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[5].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[6].strPath = "lighting\\3x\\SM3Lighting.v.hlsl";
    apMacro[6].kMacros[0].Name = "PROJSHADOW";
    apMacro[6].kMacros[0].Definition = "";
    apMacro[6].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[6].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[7].strPath = "lighting\\3x\\SM3Lighting.v.hlsl";
    apMacro[7].kMacros[0].Name = "PROJSHADOW";
    apMacro[7].kMacros[0].Definition = "";
    apMacro[7].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[7].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[8].strPath = "lighting\\3x\\SM3Lighting.v.hlsl";
    apMacro[8].kMacros[0].Name = "SKIN";
    apMacro[8].kMacros[0].Definition = "";
    apMacro[8].kMacros[1].Name = "PROJSHADOW";
    apMacro[8].kMacros[1].Definition = "";
    apMacro[8].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[8].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[9].strPath = "lighting\\3x\\SM3Lighting.v.hlsl";
    apMacro[9].kMacros[0].Name = "VC";
    apMacro[9].kMacros[0].Definition = "";
    apMacro[9].kMacros[1].Name = "PROJSHADOW";
    apMacro[9].kMacros[1].Definition = "";
    apMacro[9].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[9].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[10].strPath = "lighting\\3x\\SM3Lighting.v.hlsl";
    apMacro[10].kMacros[0].Name = "VC";
    apMacro[10].kMacros[0].Definition = "";
    apMacro[10].kMacros[1].Name = "PROJSHADOW";
    apMacro[10].kMacros[1].Definition = "";
    apMacro[10].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[10].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[11].strPath = "lighting\\3x\\SM3Lighting.v.hlsl";
    apMacro[11].kMacros[0].Name = "SKIN";
    apMacro[11].kMacros[0].Definition = "";
    apMacro[11].kMacros[1].Name = "VC";
    apMacro[11].kMacros[1].Definition = "";
    apMacro[11].kMacros[2].Name = "PROJSHADOW";
    apMacro[11].kMacros[2].Definition = "";
    apMacro[11].kMacros[3].Name = nullptr;
    std::memset(
        &apMacro[11].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    apMacro[12].strPath = "lighting\\2x\\v\\EnvMap.v.hlsl";
    apMacro[12].kMacros[0].Name = "SM3";
    apMacro[12].kMacros[0].Definition = "";
    apMacro[12].kMacros[1].Name = "ENVMAP";
    apMacro[12].kMacros[1].Definition = nullptr;
    apMacro[12].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[12].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[13].strPath = "lighting\\2x\\v\\EnvMap.v.hlsl";
    apMacro[13].kMacros[0].Name = "SM3";
    apMacro[13].kMacros[0].Definition = "";
    apMacro[13].kMacros[1].Name = "ENVMAP";
    apMacro[13].kMacros[1].Definition = nullptr;
    apMacro[13].kMacros[2].Name = "VC";
    apMacro[13].kMacros[2].Definition = "";
    apMacro[13].kMacros[3].Name = nullptr;
    std::memset(
        &apMacro[13].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    apMacro[14].strPath = "lighting\\2x\\v\\EnvMap.v.hlsl";
    apMacro[14].kMacros[0].Name = "SM3";
    apMacro[14].kMacros[0].Definition = "";
    apMacro[14].kMacros[1].Name = "ENVMAP";
    apMacro[14].kMacros[1].Definition = nullptr;
    apMacro[14].kMacros[2].Name = "SKIN";
    apMacro[14].kMacros[2].Definition = "";
    apMacro[14].kMacros[3].Name = nullptr;
    std::memset(
        &apMacro[14].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    apMacro[15].strPath = "lighting\\2x\\v\\EnvMap.v.hlsl";
    apMacro[16].strPath = "lighting\\2x\\v\\EnvMap.v.hlsl";
    apMacro[15].kMacros[0].Name = "SM3";
    apMacro[15].kMacros[0].Definition = "";
    apMacro[15].kMacros[1].Name = "ENVMAP";
    apMacro[15].kMacros[1].Definition = nullptr;
    apMacro[15].kMacros[2].Name = "SKIN";
    apMacro[15].kMacros[2].Definition = "";
    apMacro[15].kMacros[3].Name = "VC";
    apMacro[15].kMacros[3].Definition = "";
    apMacro[15].kMacros[4].Name = nullptr;
    apMacro[15].kMacros[4].Definition = nullptr;
    apMacro[15].kMacros[5].Name = nullptr;
    apMacro[15].kMacros[5].Definition = nullptr;
    apMacro[15].kMacros[6].Name = nullptr;
    apMacro[15].kMacros[6].Definition = nullptr;
    apMacro[15].kMacros[7].Name = nullptr;
    apMacro[15].kMacros[7].Definition = nullptr;
    apMacro[15].kMacros[8].Name = nullptr;
    apMacro[15].kMacros[8].Definition = nullptr;
    apMacro[16].kMacros[0].Name = "SM3";
    apMacro[16].kMacros[0].Definition = "";
    apMacro[16].kMacros[1].Name = "ENVMAP";
    apMacro[16].kMacros[1].Definition = nullptr;
    apMacro[16].kMacros[2].Name = "EYE";
    apMacro[16].kMacros[2].Definition = nullptr;
    apMacro[16].kMacros[3].Name = nullptr;
    std::memset(
        &apMacro[16].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    apMacro[17].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    apMacro[17].kMacros[0].Name = "SM3";
    apMacro[17].kMacros[0].Definition = "";
    apMacro[17].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[17].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[18].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    apMacro[18].kMacros[0].Name = "SM3";
    apMacro[18].kMacros[0].Definition = "";
    apMacro[18].kMacros[1].Name = "SKIN";
    apMacro[18].kMacros[1].Definition = "";
    apMacro[18].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[18].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[19].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    apMacro[19].kMacros[0].Name = "SM3";
    apMacro[19].kMacros[0].Definition = "";
    apMacro[19].kMacros[1].Name = "TEXCOORD";
    apMacro[19].kMacros[1].Definition = "";
    apMacro[19].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[19].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[20].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    apMacro[20].kMacros[0].Name = "SM3";
    apMacro[20].kMacros[0].Definition = "";
    apMacro[20].kMacros[1].Name = "SKIN";
    apMacro[20].kMacros[1].Definition = "";
    apMacro[20].kMacros[2].Name = "TEXCOORD";
    apMacro[20].kMacros[2].Definition = "";
    apMacro[20].kMacros[3].Name = nullptr;
    std::memset(
        &apMacro[20].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    apMacro[21].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    apMacro[21].kMacros[0].Name = "SM3";
    apMacro[21].kMacros[0].Definition = "";
    apMacro[21].kMacros[1].Name = "VC";
    apMacro[21].kMacros[1].Definition = "";
    apMacro[21].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[21].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[22].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    apMacro[22].kMacros[0].Name = "SM3";
    apMacro[22].kMacros[0].Definition = "";
    apMacro[22].kMacros[1].Name = "SKIN";
    apMacro[22].kMacros[1].Definition = "";
    apMacro[22].kMacros[2].Name = "VC";
    apMacro[22].kMacros[2].Definition = "";
    apMacro[22].kMacros[3].Name = nullptr;
    std::memset(
        &apMacro[22].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    apMacro[23].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    apMacro[23].kMacros[0].Name = "SM3";
    apMacro[23].kMacros[0].Definition = "";
    apMacro[23].kMacros[1].Name = "TEXCOORD";
    apMacro[23].kMacros[1].Definition = "";
    apMacro[23].kMacros[2].Name = "VC";
    apMacro[23].kMacros[2].Definition = "";
    apMacro[23].kMacros[3].Name = nullptr;
    std::memset(
        &apMacro[23].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    apMacro[24].strPath = "lighting\\2x\\v\\WaterFog.v.hlsl";
    apMacro[24].kMacros[0].Name = "SM3";
    apMacro[24].kMacros[0].Definition = "";
    apMacro[24].kMacros[1].Name = "SKIN";
    apMacro[24].kMacros[1].Definition = "";
    apMacro[24].kMacros[2].Name = "TEXCOORD";
    apMacro[24].kMacros[2].Definition = "";
    apMacro[24].kMacros[3].Name = "VC";
    apMacro[24].kMacros[3].Definition = "";
    apMacro[24].kMacros[4].Name = nullptr;
    apMacro[24].kMacros[4].Definition = nullptr;
    apMacro[24].kMacros[5].Name = nullptr;
    apMacro[24].kMacros[5].Definition = nullptr;
    apMacro[24].kMacros[6].Name = nullptr;
    apMacro[24].kMacros[6].Definition = nullptr;
    apMacro[24].kMacros[7].Name = nullptr;
    apMacro[24].kMacros[7].Definition = nullptr;
    apMacro[24].kMacros[8].Name = nullptr;
    apMacro[24].kMacros[8].Definition = nullptr;
    apMacro[25].strPath = "lighting\\3x\\SM3SimpleShadow.v.hlsl";
    apMacro[25].kMacros[0].Name = nullptr;
    std::memset(
        &apMacro[25].kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    apMacro[26].strPath = "lighting\\3x\\SM3SimpleShadow.v.hlsl";
    apMacro[26].kMacros[0].Name = "VC";
    apMacro[26].kMacros[0].Definition = nullptr;
    apMacro[26].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[26].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[27].strPath = "lighting\\3x\\SM3SimpleShadow.v.hlsl";
    apMacro[27].kMacros[0].Name = "SKIN";
    apMacro[27].kMacros[0].Definition = "";
    apMacro[27].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[27].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[28].strPath = "lighting\\3x\\SM3SimpleShadow.v.hlsl";
    apMacro[28].kMacros[0].Name = "SKIN";
    apMacro[28].kMacros[0].Definition = "";
    apMacro[28].kMacros[1].Name = "VC";
    apMacro[28].kMacros[1].Definition = nullptr;
    apMacro[28].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[28].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[29].strPath = "lighting\\3x\\SM3DepthMap.v.hlsl";
    apMacro[29].kMacros[0].Name = nullptr;
    std::memset(
        &apMacro[29].kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    apMacro[30].strPath = "lighting\\3x\\SM3DepthMap.v.hlsl";
    apMacro[30].kMacros[0].Name = "VC";
    apMacro[30].kMacros[0].Definition = "";
    apMacro[30].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[30].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[31].strPath = "lighting\\3x\\SM3DepthMap.v.hlsl";
    apMacro[31].kMacros[0].Name = "SKIN";
    apMacro[31].kMacros[0].Definition = "";
    apMacro[31].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[31].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[32].strPath = "lighting\\3x\\SM3DepthMap.v.hlsl";
    apMacro[32].kMacros[0].Name = "VC";
    apMacro[32].kMacros[0].Definition = "";
    apMacro[32].kMacros[1].Name = "SKIN";
    apMacro[32].kMacros[1].Definition = "";
    apMacro[32].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[32].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[33].strPath = "lighting\\2x\\v\\renderNormals.v.hlsl";
    apMacro[33].kMacros[0].Name = "SM3";
    apMacro[33].kMacros[0].Definition = nullptr;
    apMacro[33].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[33].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[34].strPath = "lighting\\2x\\v\\renderNormals.v.hlsl";
    apMacro[34].kMacros[0].Name = "SM3";
    apMacro[34].kMacros[0].Definition = nullptr;
    apMacro[34].kMacros[1].Name = "SKIN";
    apMacro[34].kMacros[1].Definition = "";
    apMacro[34].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[34].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[35].strPath = "lighting\\2x\\v\\renderNormals.v.hlsl";
    apMacro[35].kMacros[0].Name = "SM3";
    apMacro[35].kMacros[0].Definition = nullptr;
    apMacro[35].kMacros[1].Name = "FIRE";
    apMacro[35].kMacros[1].Definition = "";
    apMacro[35].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[35].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[36].strPath = "lighting\\2x\\v\\renderNormals.v.hlsl";
    apMacro[36].kMacros[0].Name = "SM3";
    apMacro[36].kMacros[0].Definition = nullptr;
    apMacro[36].kMacros[1].Name = "CLEAR";
    apMacro[36].kMacros[1].Definition = "";
    apMacro[36].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[36].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[37].strPath = "lighting\\2x\\v\\renderNormals.v.hlsl";
    apMacro[37].kMacros[0].Name = "SM3";
    apMacro[37].kMacros[0].Definition = nullptr;
    apMacro[37].kMacros[1].Name = "CLEAR";
    apMacro[37].kMacros[1].Definition = "";
    apMacro[37].kMacros[2].Name = "SKIN";
    apMacro[37].kMacros[2].Definition = "";
    apMacro[37].kMacros[3].Name = nullptr;
    std::memset(
        &apMacro[37].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    apMacro[38].strPath = "lighting\\2x\\v\\renderDepth.v.hlsl";
    apMacro[38].kMacros[0].Name = "SM3";
    apMacro[38].kMacros[0].Definition = nullptr;
    apMacro[38].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[38].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[39].strPath = "lighting\\2x\\v\\renderDepth.v.hlsl";
    apMacro[39].kMacros[0].Name = "SM3";
    apMacro[39].kMacros[0].Definition = nullptr;
    apMacro[39].kMacros[1].Name = "VC";
    apMacro[39].kMacros[1].Definition = nullptr;
    apMacro[39].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[39].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[40].strPath = "lighting\\2x\\v\\renderDepth.v.hlsl";
    apMacro[40].kMacros[0].Name = "SM3";
    apMacro[40].kMacros[0].Definition = nullptr;
    apMacro[40].kMacros[1].Name = "SKIN";
    apMacro[40].kMacros[1].Definition = "";
    apMacro[40].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[40].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[41].strPath = "lighting\\2x\\v\\renderDepth.v.hlsl";
    apMacro[41].kMacros[0].Name = "SM3";
    apMacro[41].kMacros[0].Definition = nullptr;
    apMacro[41].kMacros[1].Name = "SKIN";
    apMacro[41].kMacros[1].Definition = "";
    apMacro[41].kMacros[2].Name = "VC";
    apMacro[41].kMacros[2].Definition = nullptr;
    apMacro[41].kMacros[3].Name = nullptr;
    std::memset(
        &apMacro[41].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    apMacro[42].strPath = "lighting\\2x\\v\\renderVelocity.v.hlsl";
    apMacro[42].kMacros[0].Name = "SM3";
    apMacro[42].kMacros[0].Definition = nullptr;
    apMacro[42].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[42].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[43].strPath = "lighting\\2x\\v\\renderVelocity.v.hlsl";
    apMacro[43].kMacros[0].Name = "SM3";
    apMacro[43].kMacros[0].Definition = nullptr;
    apMacro[43].kMacros[1].Name = "SKIN";
    apMacro[43].kMacros[1].Definition = "";
    apMacro[43].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[43].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[44].strPath = "lighting\\2x\\v\\localMap.v.hlsl";
    apMacro[44].kMacros[0].Name = "SM3";
    apMacro[44].kMacros[0].Definition = nullptr;
    apMacro[44].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[44].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[45].strPath = "lighting\\2x\\v\\localMap.v.hlsl";
    apMacro[45].kMacros[0].Name = "SM3";
    apMacro[45].kMacros[0].Definition = nullptr;
    apMacro[45].kMacros[1].Name = "SKIN";
    apMacro[45].kMacros[1].Definition = "";
    apMacro[45].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[45].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[46].strPath = "lighting\\2x\\v\\localMap.v.hlsl";
    apMacro[46].kMacros[0].Name = "SM3";
    apMacro[46].kMacros[0].Definition = nullptr;
    apMacro[46].kMacros[1].Name = "CLEAR";
    apMacro[46].kMacros[1].Definition = "";
    apMacro[46].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[46].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[47].strPath = "lighting\\1x\\v\\texEffect.v.hlsl";
    apMacro[47].kMacros[0].Name = nullptr;
    std::memset(
        &apMacro[47].kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    apMacro[48].strPath = "lighting\\1x\\v\\texEffect.v.hlsl";
    apMacro[48].kMacros[0].Name = "SKIN";
    apMacro[48].kMacros[0].Definition = "";
    apMacro[48].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[48].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[49].strPath = "lighting\\3x\\SM3ZOnly.v.hlsl";
    apMacro[49].kMacros[0].Name = nullptr;
    std::memset(
        &apMacro[49].kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    apMacro[50].strPath = "lighting\\3x\\SM3ZOnly.v.hlsl";
    apMacro[50].kMacros[0].Name = "SKIN";
    apMacro[50].kMacros[0].Definition = "";
    apMacro[50].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[50].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[51].strPath = "lighting\\3x\\SM3ZOnly.v.hlsl";
    apMacro[51].kMacros[0].Name = "VC";
    apMacro[51].kMacros[0].Definition = "";
    apMacro[51].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[51].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[52].strPath = "lighting\\3x\\SM3ZOnly.v.hlsl";
    apMacro[52].kMacros[0].Name = "VC";
    apMacro[52].kMacros[0].Definition = "";
    apMacro[52].kMacros[1].Name = "SKIN";
    apMacro[52].kMacros[1].Definition = "";
    apMacro[52].kMacros[2].Name = nullptr;
    std::memset(
        &apMacro[52].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    apMacro[53].strPath = "lighting\\3x\\SM3ZOnly.v.hlsl";
    apMacro[53].kMacros[0].Name = nullptr;
    std::memset(
        &apMacro[53].kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    apMacro[54].strPath = "lighting\\3x\\SM3ZOnly.v.hlsl";
    apMacro[54].kMacros[0].Name = "SKIN";
    apMacro[54].kMacros[0].Definition = "";
    apMacro[54].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[54].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    apMacro[55].strPath = "lighting\\3x\\SM3ZOnly.v.hlsl";
    apMacro[55].kMacros[0].Name = nullptr;
    std::memset(
        &apMacro[55].kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    apMacro[56].strPath = "lighting\\3x\\SM3ZOnly.v.hlsl";
    apMacro[56].kMacros[0].Name = "SKIN";
    apMacro[56].kMacros[0].Definition = "";
    apMacro[56].kMacros[1].Name = nullptr;
    std::memset(
        &apMacro[56].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    int32_t v1 = 0;
    const char* ptarget = BSShaderManager::GetVSTarget();
    int32_t v7 = 0;
    NiPointer<NiD3DVertexShader>* v2 = Lighting30Shader::spVertexShaders;
    BSShaderMacro* pMacro = apMacro;
    do {
        _D3DXMACRO* kMacros = pMacro->kMacros;
        NiD3DVertexShader* v4;
        NiD3DVertexShader* m_pObject;
        NiD3DVertexShader* v6;
        char path[260];
        char compiledShaderName[260];
        BSShader::MakePath(pMacro->strPath, path);
        BSsprintf(compiledShaderName, 260, "SM3%03i.vso", v1);
        if (v1 != 1 && (v1 != 4 && (v1 != 7 && v1 != 10))) {
            v2->SetNew(CompileVertexShaderHLSL(path, kMacros, ptarget, compiledShaderName));
            goto LABEL_19;
        }
        if (BSShaderManager::b30Shaders) {
            v4 = CompileVertexShaderHLSL(path, kMacros, "vs_3_0", compiledShaderName);
            m_pObject = v2->m_pObject;
            v6 = v4;
            if (v2->m_pObject != v4) {
                if (m_pObject) {
                    goto LABEL_13;
                }
                goto LABEL_16;
            }
        }
        else {
            v4 = CompileVertexShaderHLSL(path, kMacros, ptarget, compiledShaderName);
            m_pObject = v2->m_pObject;
            v6 = v4;
            if (v2->m_pObject != v4) {
                if (m_pObject) {
                LABEL_13:;
                    NiDecRefCount(m_pObject);
                    v4 = v6;
                }
            LABEL_16:;
                v2->m_pObject = v4;
                if (v4) {
                    NiIncRefCount(v4);
                }
            }
        }
        v1 = v7;
    LABEL_19:;
        ++v1;
        ++v2;
        ++pMacro;
        v7 = v1;
    }
    while (v2 < Lighting30Shader::spVertexShaders + 57);
}

void Lighting30Shader::LoadPixelShaders() {
    char v14[4];
    BSShaderMacro kMacroArray[50];
    kMacroArray[0].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[0].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[0].kMacros[0].Definition = "9";
    kMacroArray[0].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray[0].kMacros[1].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[1].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[1].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[1].kMacros[0].Definition = "8";
    kMacroArray[1].kMacros[1].Name = "SPECULAR";
    kMacroArray[1].kMacros[1].Definition = nullptr;
    kMacroArray[1].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[1].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[2].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[2].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[2].kMacros[0].Definition = "8";
    kMacroArray[2].kMacros[1].Name = "HAIR";
    kMacroArray[2].kMacros[1].Definition = "1";
    kMacroArray[2].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[2].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[3].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[3].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[3].kMacros[0].Definition = "7";
    kMacroArray[3].kMacros[1].Name = "HAIR";
    kMacroArray[3].kMacros[1].Definition = "1";
    kMacroArray[3].kMacros[2].Name = "SPECULAR";
    kMacroArray[3].kMacros[2].Definition = nullptr;
    kMacroArray[3].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray[3].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[4].kMacros[0].Definition = "8";
    kMacroArray[4].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[4].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[4].kMacros[1].Name = "PARALLAX";
    kMacroArray[4].kMacros[1].Definition = "";
    kMacroArray[4].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[4].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[5].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[5].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[5].kMacros[0].Definition = "8";
    kMacroArray[5].kMacros[1].Name = "PARALLAXOCC";
    kMacroArray[5].kMacros[1].Definition = "30";
    kMacroArray[5].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[5].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[6].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[6].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[6].kMacros[0].Definition = "8";
    kMacroArray[6].kMacros[1].Name = "PARALLAX";
    kMacroArray[6].kMacros[1].Definition = "";
    kMacroArray[6].kMacros[2].Name = "SPECULAR";
    kMacroArray[6].kMacros[2].Definition = nullptr;
    kMacroArray[6].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray[6].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[7].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[7].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[7].kMacros[0].Definition = "8";
    kMacroArray[7].kMacros[1].Name = "PARALLAXOCC";
    kMacroArray[7].kMacros[1].Definition = "30";
    kMacroArray[7].kMacros[2].Name = "SPECULAR";
    kMacroArray[7].kMacros[2].Definition = nullptr;
    kMacroArray[7].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray[7].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[8].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[8].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[8].kMacros[0].Definition = "8";
    kMacroArray[8].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[8].kMacros[1].Definition = "";
    kMacroArray[8].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[8].kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[9].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[9].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[9].kMacros[0].Definition = "8";
    kMacroArray[9].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[9].kMacros[1].Definition = "";
    kMacroArray[9].kMacros[2].Name = "SPECULAR";
    kMacroArray[9].kMacros[2].Definition = nullptr;
    kMacroArray[9].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray[9].kMacros[3].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[10].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[10].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[10].kMacros[0].Definition = "8";
    kMacroArray[10].kMacros[1].Name = "GLOW";
    kMacroArray[10].kMacros[1].Definition = "";
    kMacroArray[10].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[10].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[11].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[11].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[11].kMacros[0].Definition = "8";
    kMacroArray[11].kMacros[1].Name = "GLOW";
    kMacroArray[11].kMacros[1].Definition = "";
    kMacroArray[11].kMacros[2].Name = "SPECULAR";
    kMacroArray[11].kMacros[2].Definition = nullptr;
    kMacroArray[11].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray[11].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[12].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[12].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[12].kMacros[0].Definition = "8";
    kMacroArray[12].kMacros[1].Name = "PROJSHADOW";
    kMacroArray[12].kMacros[1].Definition = "";
    kMacroArray[12].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[12].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[13].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[13].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[13].kMacros[0].Definition = "8";
    kMacroArray[13].kMacros[1].Name = "SPECULAR";
    kMacroArray[13].kMacros[1].Definition = nullptr;
    kMacroArray[13].kMacros[2].Name = "PROJSHADOW";
    kMacroArray[13].kMacros[2].Definition = "";
    kMacroArray[13].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray[13].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[14].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[14].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[14].kMacros[0].Definition = "8";
    kMacroArray[14].kMacros[1].Name = "HAIR";
    kMacroArray[14].kMacros[1].Definition = "1";
    kMacroArray[14].kMacros[2].Name = "PROJSHADOW";
    kMacroArray[14].kMacros[2].Definition = "";
    kMacroArray[14].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray[14].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[15].kMacros[3].Name = "PROJSHADOW";
    kMacroArray[16].kMacros[2].Name = "PROJSHADOW";
    kMacroArray[15].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[15].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[15].kMacros[0].Definition = "7";
    kMacroArray[15].kMacros[1].Name = "HAIR";
    kMacroArray[15].kMacros[1].Definition = "1";
    kMacroArray[15].kMacros[2].Name = "SPECULAR";
    kMacroArray[15].kMacros[2].Definition = nullptr;
    kMacroArray[15].kMacros[3].Definition = "";
    kMacroArray[15].kMacros[4].Name = nullptr;
    kMacroArray[15].kMacros[4].Definition = nullptr;
    kMacroArray[15].kMacros[5].Name = nullptr;
    kMacroArray[15].kMacros[5].Definition = nullptr;
    kMacroArray[15].kMacros[6].Name = nullptr;
    kMacroArray[15].kMacros[6].Definition = nullptr;
    kMacroArray[15].kMacros[7].Name = nullptr;
    kMacroArray[15].kMacros[7].Definition = nullptr;
    kMacroArray[15].kMacros[8].Name = nullptr;
    kMacroArray[15].kMacros[8].Definition = nullptr;
    kMacroArray[16].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[16].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[16].kMacros[0].Definition = "8";
    kMacroArray[16].kMacros[1].Name = "PARALLAX";
    kMacroArray[16].kMacros[1].Definition = "";
    kMacroArray[16].kMacros[2].Definition = "";
    kMacroArray[16].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray[16].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[17].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[17].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[17].kMacros[0].Definition = "8";
    kMacroArray[17].kMacros[1].Name = "PARALLAXOCC";
    kMacroArray[17].kMacros[1].Definition = "30";
    kMacroArray[17].kMacros[2].Name = "PROJSHADOW";
    kMacroArray[17].kMacros[2].Definition = "";
    kMacroArray[17].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray[17].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[18].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[18].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[18].kMacros[0].Definition = "8";
    kMacroArray[18].kMacros[1].Name = "PARALLAX";
    kMacroArray[18].kMacros[1].Definition = "";
    kMacroArray[18].kMacros[2].Name = "SPECULAR";
    kMacroArray[19].kMacros[2].Name = "SPECULAR";
    kMacroArray[18].kMacros[2].Definition = nullptr;
    kMacroArray[18].kMacros[3].Name = "PROJSHADOW";
    kMacroArray[18].kMacros[3].Definition = "";
    kMacroArray[18].kMacros[4].Name = nullptr;
    kMacroArray[18].kMacros[4].Definition = nullptr;
    kMacroArray[18].kMacros[5].Name = nullptr;
    kMacroArray[18].kMacros[5].Definition = nullptr;
    kMacroArray[18].kMacros[6].Name = nullptr;
    kMacroArray[18].kMacros[6].Definition = nullptr;
    kMacroArray[18].kMacros[7].Name = nullptr;
    kMacroArray[18].kMacros[7].Definition = nullptr;
    kMacroArray[18].kMacros[8].Name = nullptr;
    kMacroArray[18].kMacros[8].Definition = nullptr;
    kMacroArray[19].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[19].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[19].kMacros[0].Definition = "8";
    kMacroArray[19].kMacros[1].Name = "PARALLAXOCC";
    kMacroArray[19].kMacros[1].Definition = "30";
    kMacroArray[19].kMacros[2].Definition = nullptr;
    kMacroArray[19].kMacros[3].Name = "PROJSHADOW";
    kMacroArray[19].kMacros[3].Definition = "";
    kMacroArray[19].kMacros[4].Name = nullptr;
    kMacroArray[19].kMacros[4].Definition = nullptr;
    kMacroArray[19].kMacros[5].Name = nullptr;
    kMacroArray[19].kMacros[5].Definition = nullptr;
    kMacroArray[19].kMacros[6].Name = nullptr;
    kMacroArray[19].kMacros[6].Definition = nullptr;
    kMacroArray[19].kMacros[7].Name = nullptr;
    kMacroArray[19].kMacros[7].Definition = nullptr;
    kMacroArray[19].kMacros[8].Name = nullptr;
    kMacroArray[19].kMacros[8].Definition = nullptr;
    kMacroArray[20].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[20].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[20].kMacros[0].Definition = "8";
    kMacroArray[20].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[20].kMacros[1].Definition = "";
    kMacroArray[20].kMacros[2].Name = "PROJSHADOW";
    kMacroArray[20].kMacros[2].Definition = "";
    kMacroArray[20].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray[20].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[21].kMacros[0].Definition = "8";
    kMacroArray[22].kMacros[0].Definition = "8";
    kMacroArray[21].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[21].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[21].kMacros[1].Name = "FACEGENBLEND";
    kMacroArray[21].kMacros[1].Definition = "";
    kMacroArray[21].kMacros[2].Name = "SPECULAR";
    kMacroArray[21].kMacros[2].Definition = nullptr;
    kMacroArray[21].kMacros[3].Name = "PROJSHADOW";
    kMacroArray[21].kMacros[3].Definition = "";
    kMacroArray[21].kMacros[4].Name = nullptr;
    kMacroArray[21].kMacros[4].Definition = nullptr;
    kMacroArray[21].kMacros[5].Name = nullptr;
    kMacroArray[21].kMacros[5].Definition = nullptr;
    kMacroArray[21].kMacros[6].Name = nullptr;
    kMacroArray[21].kMacros[6].Definition = nullptr;
    kMacroArray[21].kMacros[7].Name = nullptr;
    kMacroArray[21].kMacros[7].Definition = nullptr;
    kMacroArray[21].kMacros[8].Name = nullptr;
    kMacroArray[21].kMacros[8].Definition = nullptr;
    kMacroArray[22].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[22].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[22].kMacros[1].Name = "GLOW";
    kMacroArray[22].kMacros[1].Definition = "";
    kMacroArray[22].kMacros[2].Name = "PROJSHADOW";
    kMacroArray[22].kMacros[2].Definition = "";
    kMacroArray[22].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray[22].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[23].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[23].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[23].kMacros[0].Definition = "8";
    kMacroArray[23].kMacros[1].Name = "GLOW";
    kMacroArray[23].kMacros[1].Definition = "";
    kMacroArray[23].kMacros[2].Name = "SPECULAR";
    kMacroArray[23].kMacros[2].Definition = nullptr;
    kMacroArray[23].kMacros[3].Name = "PROJSHADOW";
    kMacroArray[23].kMacros[3].Definition = "";
    kMacroArray[23].kMacros[4].Name = nullptr;
    kMacroArray[23].kMacros[4].Definition = nullptr;
    kMacroArray[23].kMacros[5].Name = nullptr;
    kMacroArray[23].kMacros[5].Definition = nullptr;
    kMacroArray[23].kMacros[6].Name = nullptr;
    kMacroArray[23].kMacros[6].Definition = nullptr;
    kMacroArray[23].kMacros[7].Name = nullptr;
    kMacroArray[23].kMacros[7].Definition = nullptr;
    kMacroArray[23].kMacros[8].Name = nullptr;
    kMacroArray[23].kMacros[8].Definition = nullptr;
    kMacroArray[24].strPath = "lighting\\2x\\p\\EnvMap.p.hlsl";
    kMacroArray[24].kMacros[0].Name = "SM3";
    kMacroArray[24].kMacros[0].Definition = "";
    kMacroArray[24].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray[24].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[25].strPath = "lighting\\2x\\p\\EnvMap.p.hlsl";
    kMacroArray[25].kMacros[0].Name = "SM3";
    kMacroArray[25].kMacros[0].Definition = "";
    kMacroArray[25].kMacros[1].Name = "WINDOW";
    kMacroArray[25].kMacros[1].Definition = nullptr;
    kMacroArray[25].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[25].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[26].strPath = "lighting\\2x\\p\\EnvMap.p.hlsl";
    kMacroArray[26].kMacros[0].Name = "SM3";
    kMacroArray[26].kMacros[0].Definition = "";
    kMacroArray[26].kMacros[1].Name = "EYE";
    kMacroArray[26].kMacros[1].Definition = nullptr;
    kMacroArray[26].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[26].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[27].strPath = "lighting\\2x\\p\\WaterFog.p.hlsl";
    kMacroArray[27].kMacros[0].Name = "SM3";
    kMacroArray[27].kMacros[0].Definition = "";
    kMacroArray[27].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray[27].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[28].strPath = "lighting\\2x\\p\\WaterFog.p.hlsl";
    kMacroArray[28].kMacros[0].Name = "SM3";
    kMacroArray[28].kMacros[0].Definition = "";
    kMacroArray[28].kMacros[1].Name = "ALPHATESTING";
    kMacroArray[28].kMacros[1].Definition = "";
    kMacroArray[28].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[28].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[29].strPath = "lighting\\3x\\SM3SimpleShadow.p.hlsl";
    kMacroArray[29].kMacros[0].Name = "SOFTSHADOW";
    kMacroArray[29].kMacros[0].Definition = "4";
    kMacroArray[29].kMacros[1].Name = "DEPTHBIAS";
    kMacroArray[29].kMacros[1].Definition = "-2";
    kMacroArray[29].kMacros[2].Name = "SHADOWMODE";
    kMacroArray[29].kMacros[2].Definition = "3";
    kMacroArray[29].kMacros[3].Name = nullptr;
    std::memset(
        &kMacroArray[29].kMacros[3].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[3].Definition)
    );
    kMacroArray[30].strPath = "lighting\\3x\\SM3DepthMap.p.hlsl";
    kMacroArray[30].kMacros[0].Name = "SHADOWMODE";
    kMacroArray[30].kMacros[0].Definition = "3";
    kMacroArray[30].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray[30].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[31].strPath = "lighting\\3x\\SM3DepthMap.p.hlsl";
    kMacroArray[31].kMacros[0].Name = "SHADOWMODE";
    kMacroArray[31].kMacros[0].Definition = "3";
    kMacroArray[31].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray[31].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[32].strPath = "lighting\\2x\\p\\renderNormals.p.hlsl";
    kMacroArray[32].kMacros[0].Name = "SM3";
    kMacroArray[32].kMacros[0].Definition = nullptr;
    kMacroArray[32].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray[32].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[33].strPath = "lighting\\2x\\p\\renderNormals.p.hlsl";
    kMacroArray[33].kMacros[0].Name = "SM3";
    kMacroArray[33].kMacros[0].Definition = nullptr;
    kMacroArray[33].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray[33].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[34].strPath = "lighting\\2x\\p\\renderNormals.p.hlsl";
    kMacroArray[34].kMacros[0].Name = "SM3";
    kMacroArray[34].kMacros[0].Definition = nullptr;
    kMacroArray[34].kMacros[1].Name = "FIRE";
    kMacroArray[34].kMacros[1].Definition = "";
    kMacroArray[34].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[34].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[35].strPath = "lighting\\2x\\p\\renderNormals.p.hlsl";
    kMacroArray[35].kMacros[0].Name = "SM3";
    kMacroArray[35].kMacros[0].Definition = nullptr;
    kMacroArray[35].kMacros[1].Name = "CLEAR";
    kMacroArray[35].kMacros[1].Definition = "";
    kMacroArray[35].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[35].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[36].strPath = "lighting\\2x\\p\\renderNormals.p.hlsl";
    kMacroArray[36].kMacros[0].Name = "SM3";
    kMacroArray[36].kMacros[0].Definition = nullptr;
    kMacroArray[36].kMacros[1].Name = "CLEAR";
    kMacroArray[36].kMacros[1].Definition = "";
    kMacroArray[36].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[36].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[37].strPath = "lighting\\3x\\SM3Lighting.p.hlsl";
    kMacroArray[37].kMacros[0].Name = "MAXLIGHTS";
    kMacroArray[37].kMacros[0].Definition = "1";
    kMacroArray[37].kMacros[1].Name = "SILHOUETTE";
    kMacroArray[37].kMacros[1].Definition = "";
    kMacroArray[37].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[37].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[38].strPath = "lighting\\2x\\p\\renderDepth.p.hlsl";
    kMacroArray[38].kMacros[0].Name = "SM3";
    kMacroArray[38].kMacros[0].Definition = nullptr;
    kMacroArray[38].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray[38].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[39].strPath = "lighting\\2x\\p\\renderVelocity.p.hlsl";
    kMacroArray[39].kMacros[0].Name = "SM3";
    kMacroArray[39].kMacros[0].Definition = nullptr;
    kMacroArray[39].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray[39].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[40].strPath = "lighting\\2x\\p\\localMap.p.hlsl";
    kMacroArray[40].kMacros[0].Name = "SM3";
    kMacroArray[40].kMacros[0].Definition = nullptr;
    kMacroArray[40].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray[40].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[41].strPath = "lighting\\2x\\p\\localMap.p.hlsl";
    kMacroArray[41].kMacros[0].Name = "SM3";
    kMacroArray[41].kMacros[0].Definition = nullptr;
    kMacroArray[41].kMacros[1].Name = "CLEAR";
    kMacroArray[41].kMacros[1].Definition = nullptr;
    kMacroArray[41].kMacros[2].Name = nullptr;
    std::memset(
        &kMacroArray[41].kMacros[2].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
    );
    kMacroArray[42].strPath = "lighting\\1x\\p\\texEffect.p.hlsl";
    kMacroArray[42].kMacros[0].Name = "HQ";
    kMacroArray[42].kMacros[0].Definition = "";
    kMacroArray[42].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray[42].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[43].strPath = "lighting\\1x\\p\\texEffect.p.hlsl";
    kMacroArray[43].kMacros[0].Name = "HQ";
    kMacroArray[43].kMacros[0].Definition = "";
    kMacroArray[43].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray[43].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[44].strPath = "lighting\\3x\\SM3ZOnly.p.hlsl";
    kMacroArray[44].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray[44].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray[45].strPath = "lighting\\3x\\SM3ZOnly.p.hlsl";
    kMacroArray[45].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray[45].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray[46].strPath = "lighting\\3x\\SM3ZOnly.p.hlsl";
    kMacroArray[46].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray[46].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray[47].strPath = "lighting\\3x\\SM3ZOnly.p.hlsl";
    kMacroArray[47].kMacros[0].Name = nullptr;
    std::memset(
        &kMacroArray[47].kMacros[0].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    kMacroArray[48].strPath = "lighting\\3x\\SM3ZOnly.p.hlsl";
    kMacroArray[48].kMacros[0].Name = "CONSTALPHA";
    kMacroArray[48].kMacros[0].Definition = "";
    kMacroArray[48].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray[48].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    kMacroArray[49].strPath = "lighting\\3x\\SM3ZOnly.p.hlsl";
    kMacroArray[49].kMacros[0].Name = "CONSTALPHA";
    kMacroArray[49].kMacros[0].Definition = "";
    kMacroArray[49].kMacros[1].Name = nullptr;
    std::memset(
        &kMacroArray[49].kMacros[1].Definition, 0,
        sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1].Definition)
    );
    const char* ptarget_2 = BSShaderManager::GetPSTarget(false);
    // The MAXLIGHTS values "4" and "2" and the value "20", built on the stack as three-byte strings.
    char v18[3] = "4";
    char v15[3] = "20";
    char v16[3] = "2";
    const char* ptarget = ptarget_2;
    std::sprintf(v14, "%i", BSShaderManager::eShadowMode);
    kMacroArray[29].kMacros[2].Definition = v14;
    kMacroArray[30].kMacros[0].Definition = v14;
    kMacroArray[31].kMacros[0].Definition = v14;
    int32_t i = 0;
    BSShaderMacro* pMacro = kMacroArray;
    while (true) {
        _D3DXMACRO* kMacros = pMacro->kMacros;
        NiD3DPixelShader* v6;
        NiD3DPixelShader* m_pObject;
        NiD3DPixelShader* v8;
        NiD3DPixelShader* v9;
        const char* v10;
        NiD3DPixelShader* v11;
        NiD3DPixelShader* v12;
        char PSOFile[260];
        char HLSLFile[260];
        BSShader::MakePath(pMacro->strPath, HLSLFile);
        BSsprintf(PSOFile, 260, "SM3%03i.pso", i);
        BSShaderManager::etShaderLevel ShaderLevel = BSShaderManager::GetShaderLevel(false);
        if (ShaderLevel == BSShaderManager::BSSM_SV_3_0) {
            if (i < 24) {
                kMacros->Definition = v18;
            }
            if (i == 2 || (i == 3 || (i == 14 || i == 15))) {
                kMacros[1].Definition = v16;
            }
            goto LABEL_32;
        }
        if (ShaderLevel != BSShaderManager::BSSM_SV_2_A && ShaderLevel != BSShaderManager::BSSM_SV_2_B) {
            NiD3DPixelShader* v5;
            if (i == 5 || (i == 7 || (i == 17 || i == 19))) {
                if (BSShaderManager::b30Shaders) {
                    v5 = CompilePixelShaderHLSL(HLSLFile, kMacros, "ps_3_0", PSOFile);
                LABEL_28:;
                    Lighting30Shader::spPixelShaders[i].SetNew(v5);
                    goto LABEL_40;
                }
                kMacros[1].Definition = v15;
            }
        LABEL_27:;
            v5 = CompilePixelShaderHLSL(HLSLFile, kMacros, ptarget_2, PSOFile);
            goto LABEL_28;
        }
        if (i < 24) {
            kMacros->Definition = v18;
        }
        if (i != 5 && (i != 7 && (i != 17 && i != 19))) {
            goto LABEL_27;
        }
        if (BSShaderManager::b30Shaders) {
            v6 = CompilePixelShaderHLSL(HLSLFile, kMacros, "ps_3_0", PSOFile);
            goto LABEL_33;
        }
        kMacros[1].Definition = v15;
    LABEL_32:;
        v6 = CompilePixelShaderHLSL(HLSLFile, kMacros, ptarget_2, PSOFile);
    LABEL_33:;
        m_pObject = Lighting30Shader::spPixelShaders[i].m_pObject;
        v11 = v6;
        if (m_pObject != v6) {
            if (m_pObject) {
                NiDecRefCount(m_pObject);
                v6 = v11;
            }
            Lighting30Shader::spPixelShaders[i].m_pObject = v6;
            if (v6) {
                NiIncRefCount(v6);
            }
        }
    LABEL_40:;
        if (i >= 24) {
            goto LABEL_58;
        }
        BSsprintf(PSOFile, 260, "SM3LL%03i.pso", i);
        kMacros->Definition = v16;
        if (i != 5 && (i != 7 && (i != 17 && i != 19))) {
            Lighting30Shader::spPixelShadersLowLight[i].SetNew(
                CompilePixelShaderHLSL(HLSLFile, kMacros, ptarget, PSOFile)
            );
            goto LABEL_58;
        }
        if (BSShaderManager::b30Shaders) {
            v8 = CompilePixelShaderHLSL(HLSLFile, kMacros, "ps_3_0", PSOFile);
            v9 = Lighting30Shader::spPixelShadersLowLight[i].m_pObject;
            v12 = v8;
            if (v9 == v8) {
                goto LABEL_58;
            }
            if (!v9) {
                goto LABEL_56;
            }
            if (!InterlockedDecrementInt32(reinterpret_cast<int32_t*>(&v9->m_uiRefCount))) {
            LABEL_54:;
                v9->DeleteThis();
            }
        LABEL_55:;
            v8 = v12;
            goto LABEL_56;
        }
        v10 = ptarget;
        kMacros[1].Definition = v15;
        v8 = CompilePixelShaderHLSL(HLSLFile, kMacros, v10, PSOFile);
        v9 = Lighting30Shader::spPixelShadersLowLight[i].m_pObject;
        v12 = v8;
        if (v9 == v8) {
            goto LABEL_58;
        }
        if (v9) {
            if (!InterlockedDecrementInt32(reinterpret_cast<int32_t*>(&v9->m_uiRefCount))) {
                goto LABEL_54;
            }
            goto LABEL_55;
        }
    LABEL_56:;
        Lighting30Shader::spPixelShadersLowLight[i].m_pObject = v8;
        if (v8) {
            NiIncRefCount(v8);
        }
    LABEL_58:;
        ++i;
        ++pMacro;
        if (i >= 50) {
            break;
        }
        ptarget_2 = ptarget;
    }
}

bool Lighting30Shader::LoadStagesAndPasses() {
    NiPointer<NiD3DPass>* v2 = Lighting30Shader::spPass;
    int32_t v3 = 112;
    do {
        NiD3DPass::CreateNewPass(*v2++);
        --v3;
    }
    while (v3);
    PresetStages();
    return true;
}

void Lighting30Shader::SetTextureMode(NiD3DTextureStage* apStage, BSShaderPPLightingProperty* apProp) {
    int32_t v3 = (int32_t)apProp->GetClampNI();
    apStage->SetClampMode(static_cast<NiTexturingProperty::ClampMode>(v3));
}

void Lighting30Shader::DisableNonVolatileConstantsForPass(uint32_t auiPassEnum) {
    Lighting30Shader::bVSConstantEnable[15] = (uint8_t)0;
    Lighting30Shader::pVertexConstantMapEntry[15]->m_bEnabled = false;
}

void Lighting30Shader::SetupGeometryConstants_WorldBoundCenter(const NiBound& worldBound) {
    float v2;
    float v3;
    float v4;
    float v5;
    float v6;
    float v7;
    if (Lighting30Shader::pVertexConstantMapEntry[12]->m_bEnabled) {
        v2 = (&worldBound)->m_kCenter.x - NiRenderer::PosAdjust.x;
        v3 = (&worldBound)->m_kCenter.y - NiRenderer::PosAdjust.y;
        v4 = (&worldBound)->m_kCenter.z - NiRenderer::PosAdjust.z;
        v5 = v2 - BSShaderManager::pShadowSceneNode[0]->kLightingOffset.x;
        v6 = v3 - BSShaderManager::pShadowSceneNode[0]->kLightingOffset.y;
        v7 = v4 - BSShaderManager::pShadowSceneNode[0]->kLightingOffset.z;
        Lighting30Shader::kVSConstants[39].x = v5;
        Lighting30Shader::kVSConstants[39].y = v6;
        Lighting30Shader::kVSConstants[39].z = v7;
    }
}

// With the env-map constant enabled, flags whether the property has an environment map and passes its scale.
void Lighting30Shader::SetupGeometryConstants_Envmap(Lighting30ShaderProperty* apProp) {
    if (pPixelConstantMapEntry[7]->m_bEnabled) {
        if (apProp->GetTexture(BSShaderProperty::TEXTYPE_ENV_MASK, 0)) {
            kPSConstants[11].w = 1.0f;
        }
        else {
            kPSConstants[11].w = 0.0f;
        }
        kPSConstants[11].z = apProp->fEnvmapScale;
    }
}

void Lighting30Shader::ClearUnusedStages(NiD3DPass* apPass, uint32_t auiFirstUnusedStage) {
    uint32_t i;
    NiD3DTextureStage* m_pObject;
    for (i = auiFirstUnusedStage; i < apPass->m_uiStageCount; ++i) {
        m_pObject = apPass->m_kStages.m_pBase[i].m_pObject;
        if (m_pObject) {
            m_pObject->m_spTexture = nullptr;
        }
    }
}

void Lighting30Shader::UpdatePipelineZOnlyTexEffect(BSShaderPPLightingProperty* apBSSProp, NiD3DPass* apPass) {
    BSShaderPPLightingProperty::TextureEffectData* spTexEffectData;
    NiSourceTexture* m_pObject;
    NiD3DTextureStage* v6;
    uint32_t v7;
    NiD3DTextureStage* v8;
    spTexEffectData = apBSSProp->spTexEffectData.m_pObject;
    if (spTexEffectData) {
        m_pObject = static_cast<NiSourceTexture*>(spTexEffectData->spBlockOutTexture.m_pObject);
        v6 = apPass->m_kStages.m_pBase->m_pObject;
        if (m_pObject) {
            v6->m_spTexture = m_pObject;
        }
        else {
            v6->m_spTexture = BSShaderManager::spDefaultTexEffectMap;
        }
        v6->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
        v7 = 1;
        if (apPass->m_uiStageCount > 1) {
            do {
                v8 = apPass->m_kStages.m_pBase[v7].m_pObject;
                if (v8) {
                    v8->m_spTexture = nullptr;
                }
                ++v7;
            }
            while (v7 < apPass->m_uiStageCount);
        }
        ApplyTextureStages();
        BSRenderState::SetAlphaTestFuncRef(NiAlphaProperty::TEST_GREATER, spTexEffectData->iAlphaTest, BSRS_NOLOCK);
    }
}

void Lighting30Shader::SetupGeometryConstants_RefractionPower(Lighting30ShaderProperty* pprop) {
    if (Lighting30Shader::pPixelConstantMapEntry[5]->m_bEnabled) {
        Lighting30Shader::kPSConstants[6].x = pprop->fRefractionPower;
        if (((int32_t)(pprop->ulFlags[1] & 16)) != 0) {
            Lighting30Shader::kPSConstants[6].y = 0.8999999761581421f;
        }
        else {
            Lighting30Shader::kPSConstants[6].y = (float)1.0;
        }
    }
}

void Lighting30Shader::SetupStandardTextures(
    NiD3DPass* apPass,
    Lighting30ShaderProperty* apShaderProperty,
    bool abUseGlowMap
) {
    NiD3DTextureStage* m_pObject;
    apPass->m_kStages.m_pBase[4].m_pObject->m_spTexture = apShaderProperty->spTextures[0][1].m_pObject;
    apPass->m_kStages.m_pBase[5].m_pObject->m_spTexture = apShaderProperty->spTextures[1][1].m_pObject;
    if (abUseGlowMap) {
        m_pObject = apPass->m_kStages.m_pBase[6].m_pObject;
        m_pObject->m_spTexture = apShaderProperty->GetTexture(BSShaderProperty::TEXTYPE_GLOW, (uint32_t)0);
    }
}

void Lighting30Shader::SetupGeometryConstants_WaterFog() {
    double x;
    float v1;
    float v2;
    float v3;
    float v4;
    float v5;
    float v6;
    if (Lighting30Shader::pPixelConstantMapEntry[8]->m_bEnabled) {
        v2 = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.x +
            BSShaderLightingProperty::pShaderLightConstants[30].x;
        v3 = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.y +
            BSShaderLightingProperty::pShaderLightConstants[30].y;
        v4 = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.z +
            BSShaderLightingProperty::pShaderLightConstants[30].z;
        Lighting30Shader::kPSConstants[9].x = BSShaderManager::kWaterPlane.m_kNormal.x;
        Lighting30Shader::kPSConstants[9].y = BSShaderManager::kWaterPlane.m_kNormal.y;
        Lighting30Shader::kPSConstants[9].z = BSShaderManager::kWaterPlane.m_kNormal.z;
        v1 = ((BSShaderManager::kWaterPlane.m_kNormal.x * v2) + (BSShaderManager::kWaterPlane.m_kNormal.y * v3)) +
            (BSShaderManager::kWaterPlane.m_kNormal.z * v4);
        v6 = v1 - BSShaderManager::kWaterPlane.m_fConstant;
        Lighting30Shader::kPSConstants[9].w = v6;
        x = (double)BSShaderManager::kWaterFogDistances.x;
        if ((BSShaderManager::kWaterFogDistances.y == 0.0) && (0.0 == x)) {
            Lighting30Shader::kPSConstants[10].x = (float)0.0;
            Lighting30Shader::kPSConstants[10].y = (float)0.0;
            Lighting30Shader::kPSConstants[10].z = (-0x1p+0f);
            Lighting30Shader::kPSConstants[10].w = (float)0.0;
        }
        else {
            Lighting30Shader::kPSConstants[10].x = BSShaderManager::kWaterFogDistances.y;
            v5 = (float)(BSShaderManager::kWaterFogDistances.y - x);
            Lighting30Shader::kPSConstants[10].y = v5;
            Lighting30Shader::kPSConstants[10].z = (float)1.0;
            Lighting30Shader::kPSConstants[10].w = (float)1.0;
        }
    }
}

void Lighting30Shader::SetupGeometryConstants_ShadowProjection(
    const NiSkinInstance* pskin,
    const NiTransform& worldTransform
) {
    _D3DMATRIX v6;
    _D3DMATRIX v7;
    if (Lighting30Shader::pVertexConstantMapEntry[13]->m_bEnabled) {
        double m_fScale;
        NiPoint3 kLightingOffset = BSShaderManager::pShadowSceneNode[0]->kLightingOffset;
        if (pskin) {
            v6._11 = 1.0f;
            v6._12 = 0.0f;
            v6._13 = 0.0f;
            v6._14 = NiRenderer::PosAdjust.x + kLightingOffset.x;
            v6._21 = 0.0f;
            v6._23 = 0.0f;
            v6._22 = 1.0f;
            v6._24 = NiRenderer::PosAdjust.y + kLightingOffset.y;
            v6._31 = 0.0f;
            v6._32 = 0.0f;
            v6._33 = 1.0f;
            v6._34 = NiRenderer::PosAdjust.z + kLightingOffset.z;
            m_fScale = 1.0;
            v6._41 = 0.0f;
            v6._42 = 0.0f;
            v6._43 = 0.0f;
        }
        else {
            v6._11 = worldTransform.m_Rotate.m_pEntry[0][0];
            v6._12 = worldTransform.m_Rotate.m_pEntry[0][1];
            v6._13 = worldTransform.m_Rotate.m_pEntry[0][2];
            v6._14 = worldTransform.m_Translate.x + kLightingOffset.x;
            v6._21 = worldTransform.m_Rotate.m_pEntry[1][0];
            v6._22 = worldTransform.m_Rotate.m_pEntry[1][1];
            v6._23 = worldTransform.m_Rotate.m_pEntry[1][2];
            v6._24 = worldTransform.m_Translate.y + kLightingOffset.y;
            v6._31 = worldTransform.m_Rotate.m_pEntry[2][0];
            v6._32 = worldTransform.m_Rotate.m_pEntry[2][1];
            v6._33 = worldTransform.m_Rotate.m_pEntry[2][2];
            v6._34 = worldTransform.m_Translate.z + kLightingOffset.z;
            v6._41 = 0.0f;
            v6._42 = 0.0f;
            v6._43 = 0.0f;
            m_fScale = worldTransform.m_fScale;
        }
        v6._44 = m_fScale;
        v7 = v6;
        // The matrix's rows are the four projection registers.
        Lighting30Shader::kVSConstants[32].x = v7._11;
        Lighting30Shader::kVSConstants[32].y = v7._12;
        Lighting30Shader::kVSConstants[32].z = v7._13;
        Lighting30Shader::kVSConstants[32].w = v7._14;
        Lighting30Shader::kVSConstants[33].x = v7._21;
        Lighting30Shader::kVSConstants[33].y = v7._22;
        Lighting30Shader::kVSConstants[33].z = v7._23;
        Lighting30Shader::kVSConstants[33].w = v7._24;
        Lighting30Shader::kVSConstants[34].x = v7._31;
        Lighting30Shader::kVSConstants[34].y = v7._32;
        Lighting30Shader::kVSConstants[34].z = v7._33;
        Lighting30Shader::kVSConstants[34].w = v7._34;
        Lighting30Shader::kVSConstants[35].x = v7._41;
        Lighting30Shader::kVSConstants[35].y = v7._42;
        Lighting30Shader::kVSConstants[35].z = v7._43;
        Lighting30Shader::kVSConstants[35].w = v7._44;
        Lighting30Shader::kVSConstants[36] = ShadowLightShader::kShadowProjData;
        Lighting30Shader::kVSConstants[37] = ShadowLightShader::kShadowProjTransform;
    }
}

void Lighting30Shader::SetupGeometryConstants_HairTint(Lighting30ShaderProperty* pprop) {
    if (Lighting30Shader::pPixelConstantMapEntry[2]->m_bEnabled) {
        Lighting30Shader::kPSConstants[2].x = pprop->pHairTintColor.r;
        Lighting30Shader::kPSConstants[2].y = pprop->pHairTintColor.g;
        Lighting30Shader::kPSConstants[2].z = pprop->pHairTintColor.b;
        Lighting30Shader::kPSConstants[2].w = pprop->pHairTintColor.a;
    }
}

void Lighting30Shader::UpdatePipelineTexEffect(BSShaderPPLightingProperty* apBSSProp, NiD3DPass* apPass) {
    BSShaderPPLightingProperty::TextureEffectData* m_pObject = apBSSProp->spTexEffectData.m_pObject;
    if (m_pObject) {
        NiD3DTextureStage* v5 = apPass->m_kStages.m_pBase->m_pObject;
        NiSourceTexture* v6 = static_cast<NiSourceTexture*>(apBSSProp->spTextures[1]->m_pObject);
        if (!v6) {
            v6 = BSShaderManager::spDefaultNormalGlossMap;
            if ((apBSSProp->ulFlags[0] & 128) == 0) {
                v6 = static_cast<NiSourceTexture*>(BSShaderManager::spDefaultNormalMap.m_pObject);
            }
        }
        v5->m_spTexture = v6;
        v5->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
        NiTexture* v7 = m_pObject->spTexture.m_pObject;
        NiD3DTextureStage* v8 = apPass->m_kStages.m_pBase[1].m_pObject;
        if (v7) {
            v8->m_spTexture = v7;
        }
        else {
            v8->m_spTexture = BSShaderManager::spDefaultTexEffectMap;
        }
        v8->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
        for (uint32_t i = 2; i < apPass->m_uiStageCount; ++i) {
            NiD3DTextureStage* v10 = apPass->m_kStages.m_pBase[i].m_pObject;
            if (v10) {
                v10->m_spTexture = nullptr;
            }
        }
        NiAlphaProperty::AlphaFunction NiAlphaFunctionFromD3D =
            NiAlphaProperty::GetNiAlphaFunctionFromD3D(m_pObject->eDestBlendMode);
        NiAlphaProperty::AlphaFunction v11 = NiAlphaProperty::GetNiAlphaFunctionFromD3D(m_pObject->eSrcBlendMode);
        BSRenderState::SetAlphaBlendFunc(v11, NiAlphaFunctionFromD3D, BSRS_NOLOCK);
        ApplyTextureStages();
    }
}

void Lighting30Shader::UpdateTexEffectConstants(
    NiGeometry* apGeometry,
    BSShaderPPLightingProperty* apBSSProp,
    NiD3DPass* apPass
) {
    if (apBSSProp) {
        BSShaderPPLightingProperty::TextureEffectData* pdata = apBSSProp->spTexEffectData.m_pObject;
        if (pdata) {
            Lighting30Shader::FillColor.x = pdata->CurrentFillColor.r;
            Lighting30Shader::FillColor.y = pdata->CurrentFillColor.g;
            Lighting30Shader::FillColor.z = pdata->CurrentFillColor.b;
            Lighting30Shader::FillColor.w = pdata->CurrentFillColor.a;
            Lighting30Shader::RimColor.x = pdata->CurrentRimColor.r;
            Lighting30Shader::RimColor.y = pdata->CurrentRimColor.g;
            Lighting30Shader::RimColor.z = pdata->CurrentRimColor.b;
            Lighting30Shader::RimColor.w = pdata->CurrentRimColor.a;
            Lighting30Shader::fTextureUOffset = pdata->fTextureUOffset;
            Lighting30Shader::fTextureVOffset = pdata->fTextureVOffset;
            Lighting30Shader::fVars.x = pdata->fEdgeExponent;
            Lighting30Shader::fVars.y = 1.0f;
        }
        Lighting30Shader::TexEffectFogColor.x = 0.0f;
        Lighting30Shader::TexEffectFogColor.y = 0.0f;
        Lighting30Shader::TexEffectFogColor.z = 0.0f;
        Lighting30Shader::TexEffectFogColor.w = 0.0f;
        Lighting30Shader::TexEffectFogParam.x = 0.0f;
        Lighting30Shader::TexEffectFogParam.y = 0.0f;
        Lighting30Shader::TexEffectFogParam.z = 0.0f;
        Lighting30Shader::TexEffectFogParam.w = 0.0f;
        if (apGeometry) {
            BSFogProperty* pprop = BSShaderManager::GetFogProperty(BSShaderManager::cSceneGraph);
            if (pprop) {
                float fStartDistance = pprop->fStartDistance;
                float fEndDistance = pprop->fEndDistance;
                float fresnelPower = pprop->fPower;
                if (fEndDistance != 0.0f || 0.0f != fStartDistance) {
                    NiColor red = pprop->m_kColor;
                    float fRange = fEndDistance - fStartDistance;
                    Lighting30Shader::TexEffectFogParam.x = fEndDistance;
                    Lighting30Shader::TexEffectFogParam.y = fRange;
                    Lighting30Shader::TexEffectFogParam.z = fresnelPower;
                    Lighting30Shader::TexEffectFogParam.w = 0.0f;
                    Lighting30Shader::TexEffectFogColor.x = red.r;
                    Lighting30Shader::TexEffectFogColor.y = red.g;
                    Lighting30Shader::TexEffectFogColor.z = red.b;
                    Lighting30Shader::TexEffectFogColor.w = 0.0f;
                }
            }
        }
    }
}

void Lighting30Shader::SetupGeometryConstants_ObjToCubemap(
    BSShaderProperty::RenderPass* apRenderPass,
    uint32_t auiCurrentPass,
    const NiSkinInstance* apSkin,
    const NiTransform& aWorldTransform
) {
    alignas(16) D3DXMATRIX kobjtoshadow;
    alignas(16) D3DXMATRIX kobjtoworld;
    alignas(16) D3DXMATRIX kProduct;
    if (Lighting30Shader::pVertexConstantMapEntry[6]->m_bEnabled) {
        uint8_t ucNumLights;
        ShadowSceneLight** ppSceneLights = apRenderPass->ppSceneLights;
        if (!ppSceneLights ||
            (((ucNumLights = apRenderPass->ucNumLights) != 0 && (*ppSceneLights)->bCubeShadowMap) ||
             ((auiCurrentPass >= 445 && auiCurrentPass <= 451) || (auiCurrentPass >= 728 && auiCurrentPass <= 751)))) {
            kobjtoshadow = m_pkD3DRenderer->m_kD3DMat;
            D3DXMatrixTranspose_Thunk(&kobjtoshadow, &kobjtoshadow);
            Lighting30Shader::kVSConstants[8].x = kobjtoshadow._11;
            Lighting30Shader::kVSConstants[8].y = kobjtoshadow._12;
            Lighting30Shader::kVSConstants[8].z = kobjtoshadow._13;
            Lighting30Shader::kVSConstants[8].w = kobjtoshadow._14;
            Lighting30Shader::kVSConstants[9].x = kobjtoshadow._21;
            Lighting30Shader::kVSConstants[9].y = kobjtoshadow._22;
            Lighting30Shader::kVSConstants[9].z = kobjtoshadow._23;
            Lighting30Shader::kVSConstants[9].w = kobjtoshadow._24;
            Lighting30Shader::kVSConstants[10].x = kobjtoshadow._31;
            Lighting30Shader::kVSConstants[10].y = kobjtoshadow._32;
            Lighting30Shader::kVSConstants[10].z = kobjtoshadow._33;
            Lighting30Shader::kVSConstants[10].w = kobjtoshadow._34;
            Lighting30Shader::kVSConstants[11].x = kobjtoshadow._41;
            Lighting30Shader::kVSConstants[11].y = kobjtoshadow._42;
            Lighting30Shader::kVSConstants[11].z = kobjtoshadow._43;
            Lighting30Shader::kVSConstants[11].w = kobjtoshadow._44;
        }
        else if (ucNumLights) {
            kobjtoworld._43 = 0.0f;
            kobjtoworld._42 = 0.0f;
            kobjtoworld._41 = 0.0f;
            kobjtoworld._34 = 0.0f;
            kobjtoworld._32 = 0.0f;
            kobjtoworld._31 = 0.0f;
            kobjtoworld._24 = 0.0f;
            kobjtoworld._23 = 0.0f;
            kobjtoworld._21 = 0.0f;
            kobjtoworld._14 = 0.0f;
            kobjtoworld._13 = 0.0f;
            kobjtoworld._12 = 0.0f;
            kobjtoshadow._43 = 0.0f;
            kobjtoshadow._42 = 0.0f;
            kobjtoshadow._41 = 0.0f;
            kobjtoshadow._34 = 0.0f;
            kobjtoshadow._32 = 0.0f;
            kobjtoshadow._31 = 0.0f;
            kobjtoshadow._24 = 0.0f;
            kobjtoshadow._23 = 0.0f;
            kobjtoshadow._21 = 0.0f;
            kobjtoshadow._14 = 0.0f;
            kobjtoshadow._13 = 0.0f;
            kobjtoshadow._12 = 0.0f;
            kobjtoworld._44 = 1.0f;
            kobjtoworld._33 = 1.0f;
            kobjtoworld._22 = 1.0f;
            kobjtoworld._11 = 1.0f;
            kobjtoshadow._44 = 1.0f;
            kobjtoshadow._33 = 1.0f;
            kobjtoshadow._22 = 1.0f;
            kobjtoshadow._11 = 1.0f;
            if (!apSkin) {
                NiD3DUtility::GetD3DFromNi(kobjtoworld, aWorldTransform);
            }
            kobjtoworld._41 = NiRenderer::PosAdjust.x + kobjtoworld._41;
            kobjtoworld._42 = NiRenderer::PosAdjust.y + kobjtoworld._42;
            kobjtoworld._43 = NiRenderer::PosAdjust.z + kobjtoworld._43;
            // Passes 452-455 take the first scene light, the others the second (unsigned compare).
            ShadowSceneLight* pLight;
            if (auiCurrentPass - 452 > 3) {
                pLight = ppSceneLights[1];
            }
            else {
                pLight = *ppSceneLights;
            }
            D3DXMatrixMultiply_Thunk(&kProduct, &kobjtoworld, &pLight->kViewProjection);
            kobjtoshadow = kProduct;
            D3DXMatrixTranspose_Thunk(&kobjtoshadow, &kobjtoshadow);
            rt::memcpy(&Lighting30Shader::kVSConstants[8], &kobjtoshadow, sizeof(kobjtoshadow));
        }
    }
    if ((Lighting30Shader::pVertexConstantMapEntry[14]->m_bEnabled) && (apSkin)) {
        kobjtoshadow._43 = 0.0f;
        kobjtoshadow._42 = 0.0f;
        kobjtoshadow._41 = 0.0f;
        kobjtoshadow._34 = 0.0f;
        kobjtoshadow._32 = 0.0f;
        kobjtoshadow._31 = 0.0f;
        kobjtoshadow._24 = 0.0f;
        kobjtoshadow._23 = 0.0f;
        kobjtoshadow._21 = 0.0f;
        kobjtoshadow._14 = 0.0f;
        kobjtoshadow._13 = 0.0f;
        kobjtoshadow._12 = 0.0f;
        kobjtoshadow._44 = 1.0f;
        kobjtoshadow._33 = 1.0f;
        kobjtoshadow._22 = 1.0f;
        kobjtoshadow._11 = 1.0f;
        rt::memcpy(&Lighting30Shader::kVSConstants[28], &kobjtoshadow, sizeof(kobjtoshadow));
    }
}

void Lighting30Shader::SetupGeometryConstants_MaterialAlpha(
    const NiBound& aWorldBound,
    Lighting30ShaderProperty* apProp,
    const NiPropertyState* apState,
    uint32_t auiCurrentPass
) {
    Lighting30Shader::kPSConstants[3].x = NiColorA_WHITE.r;
    Lighting30Shader::kPSConstants[3].y = NiColorA_WHITE.g;
    Lighting30Shader::kPSConstants[3].z = NiColorA_WHITE.b;
    Lighting30Shader::kPSConstants[3].w = NiColorA_WHITE.a;
    if (BSShaderManager::eRenderMode == 8) {
        if ((apProp->ulFlags[1] & BSS2_VATS_Selection) != 0) {
            Lighting30Shader::kPSConstants[3].x = 0.0f;
        }
        else {
            Lighting30Shader::kPSConstants[3].x = 1.0f;
        }
        NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apState->m_aspProps[0].m_pObject);
        if (pAlpha &&
            ((pAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK) != 0 ||
             (pAlpha->m_uFlags & NiAlphaProperty::TEST_ENABLE_MASK) != 0)) {
            Lighting30Shader::kPSConstants[3].y = 0.0f;
        }
        else {
            Lighting30Shader::kPSConstants[3].y = 1.0f;
        }
    }
    else {
        if (BSShaderManager::eRenderMode == 10 || auiCurrentPass - 445 > 6) {
            if (!(apProp->fAlpha < 1.0f)) {
                Lighting30Shader::kPSConstants[3].x = 1.0f;
            }
            else {
                Lighting30Shader::kPSConstants[3].x = apProp->fAlpha;
            }
        }
        else {
            float fLODFade = apProp->GetEnvMapLODFade();
            float fAlpha = apProp->fAlpha * fLODFade;
            if (!(fAlpha < 1.0f)) {
                Lighting30Shader::kPSConstants[3].x = 1.0f;
            }
            else {
                Lighting30Shader::kPSConstants[3].x = fAlpha;
            }
            if (((apProp->ulFlags[1] & BSS2_Envmap_Light_Fade) != 0) &&
                (!apProp->GetFlag(BSShaderProperty::BSSP_FLAG_EYEREFLECT))) {
                float fLightLuminance = 0.0f;
                void* pIterator = nullptr;
                ShadowSceneLight* psslight = apProp->GetFirstActiveNonShadowLight(pIterator);
                NiColor kSunColor = BSShaderManager::pShadowSceneNode[0]->pSunLight->spLight.m_pObject->m_kDiff;
                float fSunLuminance = kSunColor.Luminance();
                if (psslight) {
                    NiLight* pLight = psslight->spLight.m_pObject;
                    NiPoint3 kToLight = pLight->m_kWorld.m_Translate - aWorldBound.m_kCenter;
                    NiColor kLightColor = pLight->m_kDiff * pLight->m_fDimmer;
                    float fDistance = kToLight.Length() / pLight->m_kSpec.r;
                    float fClamped = fMin_(1.0, BSMAX_0(0.0, fDistance));
                    fLightLuminance = 1.0 - fClamped;
                    fLightLuminance = kLightColor.Luminance() * fLightLuminance;
                }
                float fMax = fSunLuminance;
                if (fSunLuminance < fLightLuminance) {
                    fMax = fLightLuminance;
                }
                Lighting30Shader::kPSConstants[3].x = fMax * Lighting30Shader::kPSConstants[3].x;
            }
        }
        // The alpha property is read without a null test, as the game does.
        NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apState->m_aspProps[0].m_pObject);
        if ((pAlpha->m_uFlags & NiAlphaProperty::TEST_ENABLE_MASK) != 0) {
            Lighting30Shader::kPSConstants[3].y = pAlpha->m_ucAlphaTestRef / 255.0;
        }
        else {
            Lighting30Shader::kPSConstants[3].y = 0.0f;
        }
        Lighting30Shader::kPSConstants[3].z = 0.0f;
        Lighting30Shader::kPSConstants[3].w = 0.0f;
        // The destination blend function.
        int32_t iDestBlend = (pAlpha->m_uFlags & NiAlphaProperty::DEST_BLEND_MASK) >> NiAlphaProperty::DEST_BLEND_POS;
        if (iDestBlend == NiAlphaProperty::ALPHA_ONE) {
            Lighting30Shader::kPSConstants[3].z = 1.0f;
        }
        else if (iDestBlend == NiAlphaProperty::ALPHA_SRCCOLOR) {
            Lighting30Shader::kPSConstants[3].w = 1.0f;
        }
        int8_t cWasEnabled = Lighting30Shader::bMatAlphaEnabled;
        bool bBelowOne = 1.0f > Lighting30Shader::kPSConstants[3].x;
        Lighting30Shader::bMatAlphaEnabled = bBelowOne;
        Lighting30Shader::pPixelConstantMapEntry[3]->m_bEnabled = cWasEnabled || bBelowOne;
    }
}

void Lighting30Shader::SetupGeometryConstants_Emittance(
    NiMaterialProperty* pmaterialProperty,
    Lighting30ShaderProperty* pprop
) {
    NiColor* m_external_emit = pmaterialProperty->m_external_emit;
    if (!m_external_emit) {
        m_external_emit = &pmaterialProperty->m_emit;
    }
    double r = m_external_emit->r;
    double b = m_external_emit->b;
    double g = m_external_emit->g;
    bool v7 = pprop->spTextures[2]->m_pObject != nullptr;
    if (NiColor_BLACK.r != r || (NiColor_BLACK.g != g || (NiColor_BLACK.b != b || pprop->spTextures[2]->m_pObject))) {
        if (BSShaderManager::bHDR) {
            // EXE-INLINED 0x4EBBC0
            ImageSpaceEffectHDR* pHDR = static_cast<ImageSpaceEffectHDR*>(
                BSShaderManager::pImageSpaceManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_HDR]
            );
            float apMatProp = pHDR->pfData[3] * pmaterialProperty->m_fEmitMult;
            float v8 = r * apMatProp;
            float v11 = g * apMatProp;
            float v14 = apMatProp * b;
            r = v8;
            g = v11;
            b = v14;
        }
        if (pprop->spTextures[2]->m_pObject) {
            if (!BSShaderManager::bHDR) {
                if (!(r < 1.0)) {
                    r = 1.0f;
                }
                if (!(g < 1.0)) {
                    g = 1.0f;
                }
                if (!(b < 1.0)) {
                    b = 1.0f;
                }
            }
            float v10 = r;
            Lighting30Shader::kPSConstants[4].x = v10;
            float v13 = g;
            Lighting30Shader::kPSConstants[4].y = v13;
            float v16 = b;
            Lighting30Shader::kPSConstants[4].z = v16;
            Lighting30Shader::kPSConstants[4].w = 0.0f;
        }
        else {
            float v9 = r + Lighting30Shader::kPSConstants[0].x;
            float v12 = g + Lighting30Shader::kPSConstants[0].y;
            float v15 = b + Lighting30Shader::kPSConstants[0].z;
            if (!BSShaderManager::bHDR) {
                if (!(v9 < 1.0f)) {
                    v9 = 1.0f;
                }
                if (!(v12 < 1.0f)) {
                    v12 = 1.0f;
                }
                if (!(v15 < 1.0f)) {
                    v15 = 1.0f;
                }
            }
            Lighting30Shader::kPSConstants[0].x = v9;
            Lighting30Shader::kPSConstants[0].y = v12;
            Lighting30Shader::kPSConstants[0].z = v15;
            Lighting30Shader::pPixelConstantMapEntry[0]->m_bEnabled = true;
            Lighting30Shader::bAmbientEnabled = 1;
        }
    }
}

void Lighting30Shader::SetupGeometryConstants_EyePosition(_D3DMATRIX d3dInvWorld, const NiTransform& worldTransform) {
    _D3DVECTOR pOut;
    _D3DVECTOR pV;
    if (Lighting30Shader::pVertexConstantMapEntry[9]->m_bEnabled ||
        Lighting30Shader::pPixelConstantMapEntry[1]->m_bEnabled) {
        // Register 30 of the lighting shaders' shared constants.
        const NiPoint4& kConstant = BSShaderLightingProperty::pShaderLightConstants[30];
        pV.x = kConstant.x;
        pV.y = kConstant.y;
        pV.z = kConstant.z;
        float v5 = kConstant.w;
        pOut.x = kConstant.x;
        pOut.y = kConstant.y;
        pOut.z = kConstant.z;
        pV.x = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.x;
        pV.y = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.y;
        float z = BSShaderManager::pShadowSceneNode[0]->kLightingOffset.z;
        pOut.x = kConstant.x + pV.x;
        pOut.y = pV.y + kConstant.y;
        pOut.z = z + kConstant.z;
        pV.x = pOut.x;
        pV.y = pOut.y;
        pV.z = pOut.z;
        D3DXVec3TransformCoord_Thunk(&pOut, &pV, &d3dInvWorld);
        Lighting30Shader::kVSConstants[40].x = pOut.x;
        Lighting30Shader::kVSConstants[40].y = pOut.y;
        d3dInvWorld._14 = 1.0;
        Lighting30Shader::kVSConstants[40].z = pOut.z;
        d3dInvWorld._11 = pOut.x;
        Lighting30Shader::kVSConstants[40].w = 1.0;
        d3dInvWorld._12 = pOut.y;
        d3dInvWorld._13 = pOut.z;
        d3dInvWorld._14 = worldTransform.m_fScale;
        Lighting30Shader::kPSConstants[1].x = pOut.x;
        Lighting30Shader::kPSConstants[1].y = pOut.y;
        Lighting30Shader::kPSConstants[1].z = pOut.z;
        Lighting30Shader::kPSConstants[1].w = d3dInvWorld._14;
    }
}

void Lighting30Shader::SetupGeometryConstants_Lights(
    Lighting30ShaderProperty* pprop,
    BSShaderProperty::RenderPass* apRenderPass,
    uint32_t auiCurrentPass,
    NiMaterialProperty* pmaterialProperty,
    _D3DMATRIX& d3dInvWorld,
    const NiSkinInstance* pskin,
    const NiTransform& worldTransform
) {
    float fdarkness = pprop->fForcedDarkness;
    if (BSShaderManager::eRenderMode == 15) {
        const NiColorA& kSilhouetteColor = BSShaderManager::GetCurrentAccumulator()->kSilhouetteColor;
        Lighting30Shader::kPSConstants[0].x = kSilhouetteColor.r;
        Lighting30Shader::kPSConstants[0].y = kSilhouetteColor.g;
        Lighting30Shader::kPSConstants[0].z = kSilhouetteColor.b;
        Lighting30Shader::kPSConstants[0].w = kSilhouetteColor.a;
        return;
    }
    uint16_t usPassEnum = apRenderPass->usPassEnum;
    int32_t inumLights = apRenderPass->ucNumLights;
    if ((usPassEnum < 456u || usPassEnum > 459u) && (usPassEnum < 452u || usPassEnum > 455u)) {
        if (inumLights > 0) {
            BSShaderLightingProperty::SetDirectionalLight3x(0, apRenderPass->ppSceneLights[0], fdarkness);
        }
        for (int32_t i = 1; i < inumLights; ++i) {
            BSShaderLightingProperty::SetPointLight3x(i, apRenderPass->ppSceneLights[i], fdarkness);
        }
    }
    else {
        BSShaderLightingProperty::SetPointLight3x(0, apRenderPass->ppSceneLights[0], fdarkness);
    }
    if (BSShaderManager::bHDR && (pprop->ulFlags[0] & BSS1_FaceGen) != 0) {
        NiPoint4& kLightColor = BSShaderLightingProperty::pShaderLightConstants[1];
        kLightColor.x = kLightColor.x * BSShaderManager::fSkinDimmer;
        kLightColor.y = kLightColor.y * BSShaderManager::fSkinDimmer;
        kLightColor.z = kLightColor.z * BSShaderManager::fSkinDimmer;
        kLightColor.w = BSShaderManager::fSkinDimmer * kLightColor.w;
    }
    if (BSShaderManager::bLiteBrite) {
        BSShaderLightingProperty::pShaderLightConstants[1].x = NiColorA_BLACK.r;
        BSShaderLightingProperty::pShaderLightConstants[1].y = NiColorA_BLACK.g;
        BSShaderLightingProperty::pShaderLightConstants[1].z = NiColorA_BLACK.b;
        BSShaderLightingProperty::pShaderLightConstants[1].w = NiColorA_BLACK.a;
    }
    if (auiCurrentPass < 445) {
        uint16_t usLastLight;
        uint8_t ucNumLights = apRenderPass->ucNumLights;
        if (ucNumLights) {
            int32_t iLights = ucNumLights;
            if (Lighting30Shader::iMaxLights < iLights) {
                iLights = Lighting30Shader::iMaxLights;
            }
            usLastLight = static_cast<uint16_t>(iLights - 1);
        }
        else {
            usLastLight = 0;
        }
        Lighting30Shader::kPSConstants[20].x = 1.0f;
        Lighting30Shader::kPSConstants[20].y = static_cast<float>(static_cast<int32_t>(usLastLight));
        Lighting30Shader::kPSConstants[20].z = pmaterialProperty->m_fShine;
        Lighting30Shader::kPSConstants[20].w = BSShaderManager::bInterior ? 0.0f : 1.0f;
        Lighting30Shader::pPixelConstantMapEntry[9]->m_uiRegisterCount = 2 * usLastLight + 3;
    }
    else if (auiCurrentPass == 451) {
        if (!pprop->GetNumberofActiveLights()) {
            if (BSShaderManager::eRenderMode == 10) {
                Lighting30Shader::kPSConstants[21].x = 0.5f;
                Lighting30Shader::kPSConstants[21].z = 0.5f;
                Lighting30Shader::kPSConstants[21].y = 0.5f;
                Lighting30Shader::kPSConstants[21].w = 1.0f;
                Lighting30Shader::pPixelConstantMapEntry[9]->m_uiRegisterCount = 2;
            }
            else {
                Lighting30Shader::kPSConstants[21] = Lighting30Shader::kPSConstants[0];
                Lighting30Shader::pPixelConstantMapEntry[9]->m_uiRegisterCount = 2;
            }
        }
        else {
            Lighting30Shader::pPixelConstantMapEntry[9]->m_uiRegisterCount = 2;
        }
    }
    else {
        Lighting30Shader::pPixelConstantMapEntry[9]->m_uiRegisterCount = 3;
        Lighting30Shader::kPSConstants[20].x = 0.0f;
        Lighting30Shader::kPSConstants[20].y = auiCurrentPass - 452 <= 3 ? 1.0f : 0.0f;
    }
    // kPSConstants[20] holds the directional and point light counts; each light takes two registers from 21 on.
    int32_t iDirLights = static_cast<int32_t>(Lighting30Shader::kPSConstants[20].x);
    int32_t iPointLights = static_cast<int32_t>(Lighting30Shader::kPSConstants[20].y);
    int32_t iDone = 0;
    if ((auiCurrentPass < 452 || auiCurrentPass > 455) && iDirLights > 0) {
        NiVector4* pkDir = &Lighting30Shader::kPSConstants[22];
        iDone = iDirLights;
        do {
            _D3DVECTOR vLightDir;
            vLightDir.x = -pkDir->x;
            vLightDir.y = -pkDir->y;
            vLightDir.z = -pkDir->z;
            _D3DVECTOR vLightObjectDir;
            D3DXVec3TransformNormal_Thunk(&vLightObjectDir, &vLightDir, &d3dInvWorld);
            D3DXVec3Normalize_Thunk(&vLightDir, &vLightObjectDir);
            NiPoint3 modelspacedir;
            modelspacedir.x = vLightDir.x;
            modelspacedir.y = vLightDir.y;
            modelspacedir.z = vLightDir.z;
            modelspacedir.Unitize();
            pkDir->x = modelspacedir.x;
            pkDir->y = modelspacedir.y;
            pkDir->z = modelspacedir.z;
            pkDir += 2;
        }
        while (--iDirLights);
    }
    if (iPointLights > 0) {
        NiVector4* pkPoint = &Lighting30Shader::kPSConstants[2 * iDone + 22];
        do {
            NiVector4 kLight = *pkPoint;
            NiPoint3 kLightingOffset = BSShaderManager::pShadowSceneNode[0]->kLightingOffset;
            _D3DVECTOR vLightPos;
            vLightPos.x = kLight.x + kLightingOffset.x;
            vLightPos.y = kLight.y + kLightingOffset.y;
            vLightPos.z = kLight.z + kLightingOffset.z;
            _D3DVECTOR vLightObjectPos;
            D3DXVec3TransformCoord_Thunk(&vLightObjectPos, &vLightPos, &d3dInvWorld);
            float fRadius = kLight.w;
            if (!pskin) {
                if (auiCurrentPass == 452 || auiCurrentPass == 453) {
                    float fScale = worldTransform.m_fScale;
                    vLightObjectPos.x = vLightObjectPos.x * fScale;
                    vLightObjectPos.y = vLightObjectPos.y * fScale;
                    vLightObjectPos.z = vLightObjectPos.z * fScale;
                }
                else {
                    fRadius = kLight.w / worldTransform.m_fScale;
                }
            }
            pkPoint->x = vLightObjectPos.x;
            pkPoint->y = vLightObjectPos.y;
            pkPoint->z = vLightObjectPos.z;
            pkPoint->w = fRadius;
            pkPoint += 2;
        }
        while (--iPointLights);
    }
}

NiD3DPass* Lighting30Shader::SelectPass(BSShaderManager::eBSSMPassEnum a1) {
    NiD3DPass* result;
    NiD3DPass* v2 = nullptr;
    if (a1 > BSShaderManager::BSSM_3XOCCLUSION) {
        switch (a1) {
            case BSShaderManager::BSSM_3XLIGHTING: {
                NiD3DPass* m_pObject = Lighting30Shader::spPass[7].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_S: {
                NiD3DPass* v12 = Lighting30Shader::spPass[8].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v12;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_H: {
                NiD3DPass* v13 = Lighting30Shader::spPass[9].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v13;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Px: {
                NiD3DPass* v14 = Lighting30Shader::spPass[10].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v14;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Pxo: {
                NiD3DPass* v15 = Lighting30Shader::spPass[11].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v15;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Fg: {
                NiD3DPass* v16 = Lighting30Shader::spPass[12].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v16;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SFg: {
                NiD3DPass* v17 = Lighting30Shader::spPass[13].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v17;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_G: {
                NiD3DPass* v18 = Lighting30Shader::spPass[14].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v18;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SG: {
                NiD3DPass* v19 = Lighting30Shader::spPass[15].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v19;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Vc: {
                NiD3DPass* v20 = Lighting30Shader::spPass[16].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v20;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcS: {
                NiD3DPass* v21 = Lighting30Shader::spPass[17].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v21;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPx: {
                NiD3DPass* v22 = Lighting30Shader::spPass[18].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v22;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxo: {
                NiD3DPass* v23 = Lighting30Shader::spPass[19].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v23;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcG: {
                NiD3DPass* v24 = Lighting30Shader::spPass[20].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v24;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSG: {
                NiD3DPass* v25 = Lighting30Shader::spPass[21].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDeclVC.m_pObject);
                result = v25;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Spc: {
                NiD3DPass* v26 = Lighting30Shader::spPass[22].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v26;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SSpc: {
                NiD3DPass* v27 = Lighting30Shader::spPass[23].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v27;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_HSpc: {
                NiD3DPass* v28 = Lighting30Shader::spPass[24].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v28;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxSpc: {
                NiD3DPass* v29 = Lighting30Shader::spPass[25].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v29;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxoSpc: {
                NiD3DPass* v30 = Lighting30Shader::spPass[26].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v30;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_FgSpc: {
                NiD3DPass* v31 = Lighting30Shader::spPass[27].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v31;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SFgSpc: {
                NiD3DPass* v32 = Lighting30Shader::spPass[28].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v32;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_GSpc: {
                NiD3DPass* v33 = Lighting30Shader::spPass[29].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v33;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SGSpc: {
                NiD3DPass* v34 = Lighting30Shader::spPass[30].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v34;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSpc: {
                NiD3DPass* v35 = Lighting30Shader::spPass[31].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v35;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSSpc: {
                NiD3DPass* v36 = Lighting30Shader::spPass[32].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDeclVC.m_pObject);
                result = v36;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxSpc: {
                NiD3DPass* v37 = Lighting30Shader::spPass[33].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v37;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxoSpc: {
                NiD3DPass* v38 = Lighting30Shader::spPass[34].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v38;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcGSpc: {
                NiD3DPass* v39 = Lighting30Shader::spPass[35].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v39;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSGSpc: {
                NiD3DPass* v40 = Lighting30Shader::spPass[36].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDeclVC.m_pObject);
                result = v40;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_Shp: {
                NiD3DPass* v41 = Lighting30Shader::spPass[37].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v41;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SShp: {
                NiD3DPass* v42 = Lighting30Shader::spPass[38].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v42;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_HShp: {
                NiD3DPass* v43 = Lighting30Shader::spPass[39].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v43;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxShp: {
                NiD3DPass* v44 = Lighting30Shader::spPass[40].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v44;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxoShp: {
                NiD3DPass* v45 = Lighting30Shader::spPass[41].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v45;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_FgShp: {
                NiD3DPass* v46 = Lighting30Shader::spPass[42].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v46;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SFgShp: {
                NiD3DPass* v47 = Lighting30Shader::spPass[43].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v47;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_GShp: {
                NiD3DPass* v48 = Lighting30Shader::spPass[44].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v48;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SGShp: {
                NiD3DPass* v49 = Lighting30Shader::spPass[45].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v49;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcShp: {
                NiD3DPass* v50 = Lighting30Shader::spPass[46].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v50;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSShp: {
                NiD3DPass* v51 = Lighting30Shader::spPass[47].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDeclVC.m_pObject);
                result = v51;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxShp: {
                NiD3DPass* v52 = Lighting30Shader::spPass[48].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v52;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxoShp: {
                NiD3DPass* v53 = Lighting30Shader::spPass[49].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v53;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcGShp: {
                NiD3DPass* v54 = Lighting30Shader::spPass[50].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v54;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSGShp: {
                NiD3DPass* v55 = Lighting30Shader::spPass[51].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDeclVC.m_pObject);
                result = v55;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_ShpSpc: {
                NiD3DPass* v56 = Lighting30Shader::spPass[52].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v56;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SShpSpc: {
                NiD3DPass* v57 = Lighting30Shader::spPass[53].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v57;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_HShpSpc: {
                NiD3DPass* v58 = Lighting30Shader::spPass[54].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v58;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxShpSpc: {
                NiD3DPass* v59 = Lighting30Shader::spPass[55].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v59;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_PxoShpSpc: {
                NiD3DPass* v60 = Lighting30Shader::spPass[56].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v60;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_FgShpSpc: {
                NiD3DPass* v61 = Lighting30Shader::spPass[57].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v61;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SFgShpSpc: {
                NiD3DPass* v62 = Lighting30Shader::spPass[58].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v62;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_GShpSpc: {
                NiD3DPass* v63 = Lighting30Shader::spPass[59].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v63;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SGShpSpc: {
                NiD3DPass* v64 = Lighting30Shader::spPass[60].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v64;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcShpSpc: {
                NiD3DPass* v65 = Lighting30Shader::spPass[61].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v65;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSShpSpc: {
                NiD3DPass* v66 = Lighting30Shader::spPass[62].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDeclVC.m_pObject);
                result = v66;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxShpSpc: {
                NiD3DPass* v67 = Lighting30Shader::spPass[63].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v67;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcPxoShpSpc: {
                NiD3DPass* v68 = Lighting30Shader::spPass[64].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v68;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcGShpSpc: {
                NiD3DPass* v69 = Lighting30Shader::spPass[65].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v69;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_VcSGShpSpc: {
                NiD3DPass* v70 = Lighting30Shader::spPass[66].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDeclVC.m_pObject);
                result = v70;
                break;
            }
            case BSShaderManager::BSSM_3XSILHOUETTE: {
                NiD3DPass* v96 = Lighting30Shader::spPass[67].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v96;
                break;
            }
            case BSShaderManager::BSSM_3XSILHOUETTE_Vc: {
                NiD3DPass* v97 = Lighting30Shader::spPass[68].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v97;
                break;
            }
            case BSShaderManager::BSSM_3XENVMAP: {
                NiD3DPass* v71 = Lighting30Shader::spPass[69].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v71;
                break;
            }
            case BSShaderManager::BSSM_3XENVMAP_W: {
                NiD3DPass* v72 = Lighting30Shader::spPass[70].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v72;
                break;
            }
            case BSShaderManager::BSSM_3XENVMAP_Vc: {
                NiD3DPass* v73 = Lighting30Shader::spPass[71].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v73;
                break;
            }
            case BSShaderManager::BSSM_3XENVMAP_WVc: {
                NiD3DPass* v74 = Lighting30Shader::spPass[72].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v74;
                break;
            }
            case BSShaderManager::BSSM_3XENVMAP_S: {
                NiD3DPass* v75 = Lighting30Shader::spPass[73].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v75;
                break;
            }
            case BSShaderManager::BSSM_3XENVMAP_SVc: {
                NiD3DPass* v76 = Lighting30Shader::spPass[74].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDeclVC.m_pObject);
                result = v76;
                break;
            }
            case BSShaderManager::BSSM_3XENVMAP_EYE: {
                NiD3DPass* v77 = Lighting30Shader::spPass[75].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v77;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW: {
                NiD3DPass* v86 = Lighting30Shader::spPass[76].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v86;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_Vc: {
                NiD3DPass* v87 = Lighting30Shader::spPass[77].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v87;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_S: {
                NiD3DPass* v88 = Lighting30Shader::spPass[78].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v88;
                break;
            }
            case BSShaderManager::BSSM_3XLIGHTING_SIMPLESHADOW_VcS: {
                NiD3DPass* v89 = Lighting30Shader::spPass[79].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v89;
                break;
            }
            case BSShaderManager::BSSM_3XDEPTHMAP: {
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = Lighting30Shader::spPass[80].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XDEPTHMAP_Vc: {
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = Lighting30Shader::spPass[81].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XDEPTHMAP_S: {
                NiD3DPass* v92 = Lighting30Shader::spPass[82].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v92;
                break;
            }
            case BSShaderManager::BSSM_3XDEPTHMAP_VcS: {
                NiD3DPass* v93 = Lighting30Shader::spPass[83].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDeclVC.m_pObject);
                result = v93;
                break;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS: {
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = Lighting30Shader::spPass[84].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS_Vc: {
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = Lighting30Shader::spPass[84].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS_S:
            case BSShaderManager::BSSM_3XRENDERNORMALS_VcS: {
                NiD3DPass* v94 = Lighting30Shader::spPass[85].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v94;
                break;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS_FIRE: {
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = Lighting30Shader::spPass[86].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS_CLEAR: {
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = Lighting30Shader::spPass[87].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XRENDERNORMALS_CLEAR_S: {
                NiD3DPass* v95 = Lighting30Shader::spPass[88].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v95;
                break;
            }
            case BSShaderManager::BSSM_3XCONSTALPHA: {
                NiD3DPass* v98 = Lighting30Shader::spPass[89].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v98;
                break;
            }
            case BSShaderManager::BSSM_3XCONSTALPHA_Vc: {
                NiD3DPass* v99 = Lighting30Shader::spPass[90].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v99;
                break;
            }
            case BSShaderManager::BSSM_3XCONSTALPHA_S: {
                NiD3DPass* v100 = Lighting30Shader::spPass[91].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v100;
                break;
            }
            case BSShaderManager::BSSM_3XCONSTALPHA_SVc: {
                NiD3DPass* v101 = Lighting30Shader::spPass[92].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDeclVC.m_pObject);
                result = v101;
                break;
            }
            case BSShaderManager::BSSM_3XDEPTH: {
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = Lighting30Shader::spPass[93].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XDEPTH_Vc: {
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = Lighting30Shader::spPass[94].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XDEPTH_S: {
                NiD3DPass* v102 = Lighting30Shader::spPass[95].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v102;
                break;
            }
            case BSShaderManager::BSSM_3XDEPTH_SVc: {
                NiD3DPass* v103 = Lighting30Shader::spPass[96].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDeclVC.m_pObject);
                result = v103;
                break;
            }
            case BSShaderManager::BSSM_3XVELOCITY: {
                NiD3DPass* v104 = Lighting30Shader::spPass[97].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v104;
                break;
            }
            case BSShaderManager::BSSM_3XVELOCITY_S: {
                NiD3DPass* v105 = Lighting30Shader::spPass[98].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v105;
                break;
            }
            case BSShaderManager::BSSM_3XLOCALMAP: {
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = Lighting30Shader::spPass[99].m_pObject;
                break;
            }
            case BSShaderManager::BSSM_3XLOCALMAP_S: {
                NiD3DPass* v106 = Lighting30Shader::spPass[100].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v106;
                break;
            }
            case BSShaderManager::BSSM_3XLOCALMAP_CLEAR: {
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                v2 = Lighting30Shader::spPass[101].m_pObject;
                goto LABEL_118;
            }
            case BSShaderManager::BSSM_3XTEXEFFECT: {
                NiD3DPass* v90 = Lighting30Shader::spPass[102].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v90;
                break;
            }
            case BSShaderManager::BSSM_3XTEXEFFECT_S: {
                NiD3DPass* v91 = Lighting30Shader::spPass[103].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v91;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH: {
                NiD3DPass* v78 = Lighting30Shader::spPass[104].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v78;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_S: {
                NiD3DPass* v79 = Lighting30Shader::spPass[105].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v79;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_At: {
                NiD3DPass* v80 = Lighting30Shader::spPass[106].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v80;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_SAt: {
                NiD3DPass* v81 = Lighting30Shader::spPass[107].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v81;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_Vc: {
                NiD3DPass* v82 = Lighting30Shader::spPass[108].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v82;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_SVc: {
                NiD3DPass* v83 = Lighting30Shader::spPass[109].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDeclVC.m_pObject);
                result = v83;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_AtVc: {
                NiD3DPass* v84 = Lighting30Shader::spPass[110].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v84;
                break;
            }
            case BSShaderManager::BSSM_3XWATER_DEPTH_SAtVc: {
                NiD3DPass* v85 = Lighting30Shader::spPass[111].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDeclVC.m_pObject);
                result = v85;
                break;
            }
            default: {
            LABEL_118:;
                result = v2;
                break;
            }
        }
    }
    else {
        if (a1 == BSShaderManager::BSSM_3XOCCLUSION) {
            NiD3DPass* v10 = Lighting30Shader::spPass[0].m_pObject;
            m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
            return v10;
        }

        switch (a1) {
            case BSShaderManager::BSSM_3XZONLY: {
                NiD3DPass* v3 = Lighting30Shader::spPass[1].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v3;
                break;
            }
            case BSShaderManager::BSSM_3XZONLY_S: {
                NiD3DPass* v5 = Lighting30Shader::spPass[2].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v5;
                break;
            }
            case BSShaderManager::BSSM_3XZONLY_Vc: {
                NiD3DPass* v6 = Lighting30Shader::spPass[3].m_pObject;
                m_spShaderDecl.SetNew(spShaderDeclVC.m_pObject);
                result = v6;
                break;
            }
            case BSShaderManager::BSSM_3XZONLY_SVc: {
                NiD3DPass* v7 = Lighting30Shader::spPass[4].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDeclVC.m_pObject);
                result = v7;
                break;
            }
            case BSShaderManager::BSSM_3XZONLY_TEXEFFECT: {
                NiD3DPass* v8 = Lighting30Shader::spPass[5].m_pObject;
                m_spShaderDecl.SetNew(spShaderDecl.m_pObject);
                result = v8;
                break;
            }
            case BSShaderManager::BSSM_3XZONLY_TEXEFFECT_S: {
                NiD3DPass* v9 = Lighting30Shader::spPass[6].m_pObject;
                m_spShaderDecl.SetNew(spSkinnedShaderDecl.m_pObject);
                result = v9;
                break;
            }
            default: {
                goto LABEL_118;
            }
        }
    }
    return result;
}

// Behaves as NiPointer<T>::operator=(T*), while the slots are kept as raw pointers.
NiShaderConstantMapEntry**
Lighting30Shader::SetVertexConstantMapEntry(NiShaderConstantMapEntry** appSlot, NiShaderConstantMapEntry* apEntry) {
    // NiPointer's assignment on the raw slot (NiObjectNET::AssignControllers is the same body).
    NiShaderConstantMapEntry* pkOld = *appSlot;
    if (pkOld != apEntry) {
        if (pkOld) {
            NiDecRefCount(pkOld);
        }
        *appSlot = apEntry;
        if (apEntry) {
            NiIncRefCount(apEntry);
        }
    }
    return appSlot;
}

// The data vector goes to the higher slot of the pair, the color to the lower one.
void Lighting30Shader::SetLight(int32_t aiOffset, NiColorA akData, NiColorA akColor) {
    NiVector4& kData = kPSConstants[2 * aiOffset + 22];
    kData.x = akData.r;
    kData.y = akData.g;
    kData.z = akData.b;
    kData.w = akData.a;
    NiVector4& kColor = kPSConstants[2 * aiOffset + 21];
    kColor.x = akColor.r;
    kColor.y = akColor.g;
    kColor.z = akColor.b;
    kColor.w = akColor.a;
}

void Lighting30Shader::EnableConstantsForPass(uint32_t auiPassEnum, bool abAlphaPass) {
    int32_t ioffset;
    Lighting30Shader* pkThis = static_cast<Lighting30Shader*>(BSShaderManager::GetShader(29));
    uint32_t v3 = auiPassEnum;
    if (pkThis && (auiPassEnum == 480 || auiPassEnum == 481)) {
        pkThis->m_spPixelConstantMap = pkThis->spTexEffectPConstantMap;
        pkThis->m_spVertexConstantMap = pkThis->spTexEffectVConstantMap;
        return;
    }
    NiD3DShaderConstantMap* m_pObject = pkThis->m_spPixelConstantMap.m_pObject;
    if (m_pObject != pkThis->spDefaultPixelConstantMap.m_pObject) {
        if (m_pObject) {
            NiDecRefCount(m_pObject);
            v3 = auiPassEnum;
        }
        NiD3DShaderConstantMap* v6 = pkThis->spDefaultPixelConstantMap.m_pObject;
        pkThis->m_spPixelConstantMap.m_pObject = v6;
        if (v6) {
            NiIncRefCount(v6);
            v3 = auiPassEnum;
        }
    }
    NiD3DShaderConstantMap* v7 = pkThis->m_spVertexConstantMap.m_pObject;
    if (v7 != pkThis->spDefaultVertexConstantMap.m_pObject) {
        if (v7) {
            NiDecRefCount(v7);
            v3 = auiPassEnum;
        }
        NiD3DShaderConstantMap* v8 = pkThis->spDefaultVertexConstantMap.m_pObject;
        pkThis->m_spVertexConstantMap.m_pObject = v8;
        if (v8) {
            NiIncRefCount(v8);
            v3 = auiPassEnum;
        }
    }
    if (v3 > 730) {
        switch (v3) {
            case 731u: {
                ioffset = 105;
                break;
            }
            case 736u: {
                ioffset = 106;
                break;
            }
            case 737u: {
                ioffset = 107;
                break;
            }
            case 742u: {
                ioffset = 108;
                break;
            }
            case 743u: {
                ioffset = 109;
                break;
            }
            case 748u: {
                ioffset = 110;
                break;
            }
            case 749u: {
                ioffset = 111;
                break;
            }
            default: {
            LABEL_37:;
                ioffset = v3 - 376;
                break;
            }
        }
    }
    else {
        if (v3 == 730) {
            ioffset = 104;
        }
        else {
            switch (v3) {
                case 10u: {
                    ioffset = 1;
                    break;
                }
                case 11u: {
                    ioffset = 2;
                    break;
                }
                case 12u: {
                    ioffset = 3;
                    break;
                }
                case 13u: {
                    ioffset = 4;
                    break;
                }
                case 14u: {
                    ioffset = 5;
                    break;
                }
                case 15u: {
                    ioffset = 6;
                    break;
                }
                default: {
                    goto LABEL_37;
                }
            }
        }
    }
    // Bit i + 1 of the pass's flags enables vertex map entry i; the mask never wraps here.
    uint32_t uiMask = 1;
    int32_t i = 0;
    do {
        NiShaderConstantMapEntry* pEntry = Lighting30Shader::pVertexConstantMapEntry[i];
        if (pEntry) {
            pEntry->m_bEnabled = (Lighting30Shader::ulVertexConstantFlags[ioffset] & (uiMask << 1)) != 0;
        }
        uiMask <<= 1;
        Lighting30Shader::bVSConstantEnable[i++] = (uiMask & Lighting30Shader::ulVertexConstantFlags[ioffset]) != 0;
    }
    while (i < 17);
    if (Lighting30Shader::pPixelConstantMapEntry[0]) {
        Lighting30Shader::pPixelConstantMapEntry[0]->m_bEnabled =
            (Lighting30Shader::ulPixelConstantFlags[ioffset] & 2) != 0;
    }
    uint32_t v13 = Lighting30Shader::ulPixelConstantFlags[ioffset];
    Lighting30Shader::bPSConstantEnable[0] = (v13 & 2) != 0;
    if (Lighting30Shader::pPixelConstantMapEntry[1]) {
        Lighting30Shader::pPixelConstantMapEntry[1]->m_bEnabled = (v13 & 4) != 0;
    }
    uint32_t v14 = Lighting30Shader::ulPixelConstantFlags[ioffset];
    Lighting30Shader::bPSConstantEnable[1] = (v14 & 4) != 0;
    if (Lighting30Shader::pPixelConstantMapEntry[2]) {
        Lighting30Shader::pPixelConstantMapEntry[2]->m_bEnabled = (v14 & 8) != 0;
    }
    uint32_t v15 = Lighting30Shader::ulPixelConstantFlags[ioffset];
    Lighting30Shader::bPSConstantEnable[2] = (v15 & 8) != 0;
    if (Lighting30Shader::pPixelConstantMapEntry[3]) {
        Lighting30Shader::pPixelConstantMapEntry[3]->m_bEnabled = (v15 & 16) != 0;
    }
    uint32_t v16 = Lighting30Shader::ulPixelConstantFlags[ioffset];
    Lighting30Shader::bPSConstantEnable[3] = (v16 & 16) != 0;
    if (Lighting30Shader::pPixelConstantMapEntry[4]) {
        Lighting30Shader::pPixelConstantMapEntry[4]->m_bEnabled = (v16 & 32) != 0;
    }
    uint32_t v17 = Lighting30Shader::ulPixelConstantFlags[ioffset];
    Lighting30Shader::bPSConstantEnable[4] = (v17 & 32) != 0;
    if (Lighting30Shader::pPixelConstantMapEntry[5]) {
        Lighting30Shader::pPixelConstantMapEntry[5]->m_bEnabled = (v17 & 64) != 0;
    }
    uint32_t v18 = Lighting30Shader::ulPixelConstantFlags[ioffset];
    Lighting30Shader::bPSConstantEnable[5] = (v18 & 64) != 0;
    if (Lighting30Shader::pPixelConstantMapEntry[6]) {
        Lighting30Shader::pPixelConstantMapEntry[6]->m_bEnabled = (v18 & 128) != 0;
    }
    uint32_t v19 = Lighting30Shader::ulPixelConstantFlags[ioffset];
    Lighting30Shader::bPSConstantEnable[6] = (v19 & 128) != 0;
    if (Lighting30Shader::pPixelConstantMapEntry[7]) {
        Lighting30Shader::pPixelConstantMapEntry[7]->m_bEnabled = static_cast<uint8_t>(v19 >> 8) & 1;
    }
    uint32_t v20 = Lighting30Shader::ulPixelConstantFlags[ioffset];
    Lighting30Shader::bPSConstantEnable[7] = static_cast<uint8_t>(v20 >> 8) & 1;
    if (Lighting30Shader::pPixelConstantMapEntry[8]) {
        Lighting30Shader::pPixelConstantMapEntry[8u]->m_bEnabled = (v20 & 512) != 0;
    }
    uint32_t v21 = Lighting30Shader::ulPixelConstantFlags[ioffset];
    Lighting30Shader::bPSConstantEnable[8] = (v21 & 512) != 0;
    if (Lighting30Shader::pPixelConstantMapEntry[9]) {
        Lighting30Shader::pPixelConstantMapEntry[9]->m_bEnabled = (v21 & 1024) != 0;
    }
    Lighting30Shader::bPSConstantEnable[9] = (Lighting30Shader::ulPixelConstantFlags[ioffset] & 1024) != 0;
    if ((v3 >= 10 && v3 <= 15) || (BSShaderManager::usRenderFlags & 1) != 0) {
        goto LABEL_77;
    }
    if (v3 - 467 <= 3) {
        NiD3DPass* v22 = Lighting30Shader::spPass[ioffset].m_pObject;
        if (!v22->m_pkRenderStateGroup) {
            v22->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v22->m_pkRenderStateGroup->SetRenderState(168u, 3u, true);
        return;
    }
    if (v3 - 460 <= 19) {
        NiD3DPass* v23 = Lighting30Shader::spPass[ioffset].m_pObject;
        if (!v23->m_pkRenderStateGroup) {
            v23->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v23->m_pkRenderStateGroup->SetRenderState(168u, 15u, true);
        return;
    }
    if (v3 - 377 > 104) {
    LABEL_77:;
        NiD3DPass* v25 = Lighting30Shader::spPass[ioffset].m_pObject;
        if (!v25->m_pkRenderStateGroup) {
            v25->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v25->m_pkRenderStateGroup->SetRenderState(168u, 0, true);
    }
    else {
        NiD3DPass* v24 = Lighting30Shader::spPass[ioffset].m_pObject;
        if (!v24->m_pkRenderStateGroup) {
            v24->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
        }
        v24->m_pkRenderStateGroup->SetRenderState(168u, 7u, true);
    }
}

void Lighting30Shader::SetLightColor(int32_t aiOffset, NiColorA akColor) {
    NiVector4& kConstant = Lighting30Shader::kPSConstants[2 * aiOffset + 21];
    kConstant.x = akColor.r;
    kConstant.y = akColor.g;
    kConstant.z = akColor.b;
    kConstant.w = akColor.a;
}

void Lighting30Shader::SetAmbientColor(const NiColorA& akColor, bool abModified) {
    Lighting30Shader::kPSConstants[0].x = akColor.r;
    Lighting30Shader::kPSConstants[0].y = akColor.g;
    Lighting30Shader::kPSConstants[0].z = akColor.b;
    Lighting30Shader::kPSConstants[0].w = akColor.a;
    if (abModified || Lighting30Shader::bAmbientEnabled) {
        Lighting30Shader::pPixelConstantMapEntry[0]->m_bEnabled = true;
        Lighting30Shader::bAmbientEnabled = abModified;
    }
    else {
        Lighting30Shader::pPixelConstantMapEntry[0]->m_bEnabled = false;
        Lighting30Shader::bAmbientEnabled = 0;
    }
}

void Lighting30Shader::ResetPerFrameConstantData() {
    Lighting30Shader::kPSConstants[0].x = NiColorA_BLACK.r;
    Lighting30Shader::kPSConstants[0].y = NiColorA_BLACK.g;
    Lighting30Shader::kPSConstants[0].z = NiColorA_BLACK.b;
    Lighting30Shader::kPSConstants[0].w = NiColorA_BLACK.a;
    Lighting30Shader::pPixelConstantMapEntry[0]->m_bEnabled = true;
    Lighting30Shader::bAmbientEnabled = 1;
    Lighting30Shader::bMatAlphaEnabled = 1;
}

BSShader* Lighting30Shader::CreateShader() {
    Lighting30Shader* pshader;
    NiDX9ShaderDeclaration* pshaderDecl = NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 6u, 1u);
    NiDX9ShaderDeclaration* pshaderDeclNC =
        NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 6u, 1u);
    NiDX9ShaderDeclaration* pskinnedShaderDecl =
        NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 8u, 1u);
    NiDX9ShaderDeclaration* pskinnedShaderDeclNC =
        NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 8u, 1u);
    NiDX9ShaderDeclaration* pshaderDeclVC =
        NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 6u, 1u);
    NiDX9ShaderDeclaration* pshaderDeclVCNC =
        NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 6u, 1u);
    NiDX9ShaderDeclaration* pskinnedShaderDeclVC =
        NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 8u, 1u);
    NiDX9ShaderDeclaration* pskinnedShaderDeclVCNC =
        NiDX9ShaderDeclaration::Create(BSShaderManager::pRenderer.m_pObject, 8u, 1u);
    pshaderDecl->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pshaderDecl->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pshaderDecl->SetEntry(2u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0);
    pshaderDecl->SetEntry(3u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pshaderDecl->SetEntry(4u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pshaderDeclNC->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pshaderDeclNC->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pshaderDeclNC->SetEntry(
        2u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    pshaderDeclNC->SetEntry(
        3u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pshaderDeclNC->SetEntry(
        4u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pshaderDecl->GetD3DDeclaration();
    pshaderDeclNC->GetD3DDeclaration();
    pshaderDeclVC->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pshaderDeclVC->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    pshaderDeclVC->SetEntry(2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
    pshaderDeclVC->SetEntry(
        3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    pshaderDeclVC->SetEntry(
        4u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pshaderDeclVC->SetEntry(
        5u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pshaderDeclVCNC->SetEntry(
        0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pshaderDeclVCNC->SetEntry(
        1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pshaderDeclVCNC->SetEntry(2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
    pshaderDeclVCNC->SetEntry(
        3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    pshaderDeclVCNC->SetEntry(
        4u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pshaderDeclVCNC->SetEntry(
        5u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pshaderDeclVC->GetD3DDeclaration();
    pshaderDeclVCNC->GetD3DDeclaration();
    pskinnedShaderDecl->SetEntry(
        0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDecl->SetEntry(
        1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_BLENDWEIGHT, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pskinnedShaderDecl->SetEntry(
        2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_BLENDINDICES, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pskinnedShaderDecl->SetEntry(
        3u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDecl->SetEntry(
        4u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    pskinnedShaderDecl->SetEntry(
        5u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDecl->SetEntry(
        6u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDeclNC->SetEntry(
        0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDeclNC->SetEntry(
        1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_BLENDWEIGHT, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pskinnedShaderDeclNC->SetEntry(
        2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_BLENDINDICES, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pskinnedShaderDeclNC->SetEntry(
        3u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDeclNC->SetEntry(
        4u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    pskinnedShaderDeclNC->SetEntry(
        5u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDeclNC->SetEntry(
        6u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDecl->GetD3DDeclaration();
    pskinnedShaderDeclNC->GetD3DDeclaration();
    pskinnedShaderDeclVC->SetEntry(
        0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDeclVC->SetEntry(
        1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_BLENDWEIGHT, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pskinnedShaderDeclVC->SetEntry(
        2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_BLENDINDICES, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pskinnedShaderDeclVC->SetEntry(
        3u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDeclVC->SetEntry(
        4u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pskinnedShaderDeclVC->SetEntry(
        5u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    pskinnedShaderDeclVC->SetEntry(
        6u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDeclVC->SetEntry(
        7u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDeclVCNC->SetEntry(
        0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDeclVCNC->SetEntry(
        1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_BLENDWEIGHT, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pskinnedShaderDeclVCNC->SetEntry(
        2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_BLENDINDICES, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0
    );
    pskinnedShaderDeclVCNC->SetEntry(
        3u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDeclVCNC->SetEntry(
        4u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0
    );
    pskinnedShaderDeclVCNC->SetEntry(
        5u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0
    );
    pskinnedShaderDeclVCNC->SetEntry(
        6u, 14u, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDeclVCNC->SetEntry(
        7u, 15u, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0
    );
    pskinnedShaderDeclVC->GetD3DDeclaration();
    pskinnedShaderDeclVCNC->GetD3DDeclaration();
    pshader = new Lighting30Shader(pshaderDecl, pskinnedShaderDecl, pshaderDeclVC, pskinnedShaderDeclVC);
    pshader->Initialize();
    NiDX9ShaderDeclaration* m_pObject = pshader->spShaderDeclNC.m_pObject;
    if (m_pObject != pshaderDeclNC) {
        if (m_pObject) {
            NiDecRefCount(m_pObject);
        }
        pshader->spShaderDeclNC.m_pObject = pshaderDeclNC;
        NiIncRefCount(pshaderDeclNC);
    }
    NiDX9ShaderDeclaration* v7 = pshader->spShaderDeclVCNC.m_pObject;
    if (v7 != pshaderDeclVCNC) {
        if (v7) {
            NiDecRefCount(v7);
        }
        pshader->spShaderDeclVCNC.m_pObject = pshaderDeclVCNC;
        NiIncRefCount(pshaderDeclVCNC);
    }
    NiDX9ShaderDeclaration* v8 = pshader->spSkinnedShaderDeclNC.m_pObject;
    if (v8 != pskinnedShaderDeclNC) {
        if (v8) {
            NiDecRefCount(v8);
        }
        pshader->spSkinnedShaderDeclNC.m_pObject = pskinnedShaderDeclNC;
        NiIncRefCount(pskinnedShaderDeclNC);
    }
    NiDX9ShaderDeclaration* v9 = pshader->spSkinnedShaderDeclVCNC.m_pObject;
    if (v9 != pskinnedShaderDeclVCNC) {
        if (v9) {
            NiDecRefCount(v9);
        }
        pshader->spSkinnedShaderDeclVCNC.m_pObject = pskinnedShaderDeclVCNC;
        NiIncRefCount(pskinnedShaderDeclVCNC);
    }
    pshader->LoadShaders();
    pshader->CreateConstantMaps();
    pshader->LoadStagesAndPasses();
    pshader->SetShaderDecl(pshaderDecl);
    return pshader;
}

void Lighting30Shader::CreateDefaultStages(NiD3DPass* a1, int32_t x0) {
    NiPointer<NiD3DTextureStage> spStage;
    if (a1->m_uiStageCount < 7) {
        // Stage i applies when bit i of x0's low byte is set.
        uint32_t uiMask = 1;
        uint32_t i = 0;
        do {
            NiD3DTextureStage::CreateNewStage(spStage);
            BSShader::SetupDefaultStage(spStage.m_pObject, i, 1u, 2u, true);
            a1->SetStage(a1->m_uiCurrentStage, spStage.m_pObject);
            bool bApply = (static_cast<int8_t>(x0) & uiMask) != 0;
            ++i;
            uiMask <<= 1;
            spStage.m_pObject->bApply = bApply;
        }
        while (i < 7);
    }
    a1->cEnabledStages = x0;
}
