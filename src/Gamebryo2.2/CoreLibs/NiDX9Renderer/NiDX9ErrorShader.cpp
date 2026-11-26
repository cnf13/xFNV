#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ErrorShader.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffectState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderInterface.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStageGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "SDK/MSVC/stdlib.h"

NiRTTI NiDX9ErrorShader::ms_RTTI("NiDX9ErrorShader", &NiD3DShader::ms_RTTI);

// The base shader's fields start cleared; the one pass is built and the shader named.
NiDX9ErrorShader::NiDX9ErrorShader() {
    CreateStagesAndPasses();
    NiFixedString kName("NiDX9ErrorShader");
    SetName(kName);
    m_bIsBestImplementation = true;
}

NiDX9ErrorShader::~NiDX9ErrorShader() {
    NiD3DShader::m_spCurrentPass = nullptr;
}

const NiRTTI* NiDX9ErrorShader::GetRTTI() const {
    return &ms_RTTI;
}

// Initialises the base shader once and builds the vertex declaration; true when initialised.
bool NiDX9ErrorShader::Initialize() {
    if (!m_bInitialized) {
        if (!NiD3DShader::Initialize()) {
            return false;
        }
        CreateShaderDeclaration();
    }
    return true;
}

// A random light colour as the texture factor, then specular, blending, alpha test and stencil off. Each channel is
// 0x80 minus the truncated product of a negated NiUnitRandom and 127, in byte arithmetic.
uint32_t NiDX9ErrorShader::UpdatePipeline(
    NiGeometry* pkGeometry,
    const NiSkinInstance* pkSkin,
    NiGeometryData::RendererData* pkRendererData,
    const NiPropertyState* pkState,
    const NiDynamicEffectState* pkEffects,
    const NiTransform& kWorld,
    const NiBound& kWorldBound
) {
    float fR = -NiUnitRandom();
    uint8_t ucR = static_cast<uint8_t>(0x80 - static_cast<uint8_t>(static_cast<int32_t>(fR * 127.0)));
    float fG = -NiUnitRandom();
    uint8_t ucG = static_cast<uint8_t>(0x80 - static_cast<uint8_t>(static_cast<int32_t>(fG * 127.0)));
    float fB = -NiUnitRandom();
    uint8_t ucB = static_cast<uint8_t>(0x80 - static_cast<uint8_t>(static_cast<int32_t>(fB * 127.0)));
    uint32_t kColor = ((((ucR | 0xFFFFFF00u) << 8) | ucG) << 8) | ucB;
    m_pkPass->SetRenderState(D3DRS_TEXTUREFACTOR, kColor, true);
    m_pkPass->SetRenderState(D3DRS_SPECULARENABLE, 0, true);
    m_pkPass->SetRenderState(D3DRS_ALPHABLENDENABLE, 0, true);
    m_pkPass->SetRenderState(D3DRS_ALPHATESTENABLE, 0, true);
    m_pkPass->SetRenderState(D3DRS_STENCILENABLE, 0, true);
    return NiD3DShader::UpdatePipeline(pkGeometry, pkSkin, pkRendererData, pkState, pkEffects, kWorld, kWorldBound);
}

uint32_t NiDX9ErrorShader::SetupTransformations(
    NiGeometry* apGeometry,
    const NiSkinInstance* apSkin,
    const NiSkinPartition::Partition* apPartition,
    NiGeometryData::RendererData* apRendererData,
    const NiPropertyState* apState,
    const NiDynamicEffectState* apEffects,
    const NiTransform& akWorld,
    const NiBound& akWorldBound
) {
    NiTransform kNewWorld;
    kNewWorld = akWorld;
    float kWorld = akWorldBound.m_fRadius * 0.10000000149011612;
    float kWorldBound = NiSymmetricRandom();
    kNewWorld.m_Translate.x = kWorldBound * kWorld + kNewWorld.m_Translate.x;
    float kWorldBounda = NiSymmetricRandom();
    kNewWorld.m_Translate.y = kWorldBounda * kWorld + kNewWorld.m_Translate.y;
    float kWorldBoundb = NiSymmetricRandom();
    kNewWorld.m_Translate.z = kWorldBoundb * kWorld + kNewWorld.m_Translate.z;
    return NiD3DShader::SetupTransformations(
        apGeometry,
        nullptr,
        nullptr,
        apRendererData,
        apState,
        apEffects,
        kNewWorld,
        akWorldBound
    );
}

bool NiDX9ErrorShader::CreateShaderDeclaration() {
    NiDX9ShaderDeclaration* v2 = NiDX9ShaderDeclaration::Create(m_pkD3DRenderer, 1u, 1u);
    m_spShaderDecl = v2;
    static_cast<NiShaderDeclaration*>(m_spShaderDecl.m_pObject)
        ->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
    return true;
}

// One pass with one stage that outputs the texture factor (UpdatePipeline sets it). The current pass is set without a
// reference.
char NiDX9ErrorShader::CreateStagesAndPasses() {
    NiPointer<NiD3DPass> spPass;
    NiD3DPass::CreateNewPass(spPass);
    m_pkPass = spPass.m_pObject;
    NiPointer<NiD3DTextureStage> spStage;
    NiD3DTextureStage::CreateNewStage(spStage);
    NiD3DTextureStage* pkStage = spStage.m_pObject;
    NiD3DTextureStageGroup* pkGroup = pkStage->m_pkTextureStageGroup;
    pkStage->m_uiStage = 0;
    pkStage->m_spTexture = nullptr;
    pkGroup->SetStageState(D3DTSS_COLOROP, D3DTOP_SELECTARG1, false);
    pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_COLORARG1, D3DTA_TFACTOR, false);
    pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAOP, D3DTOP_SELECTARG1, false);
    pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAARG1, D3DTA_TFACTOR, false);
    pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXCOORDINDEX, 0, false);
    pkStage->m_pkTextureStageGroup->SetSamplerState(D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP, false, false);
    pkStage->m_pkTextureStageGroup->SetSamplerState(D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP, false, false);
    pkStage->m_pkTextureStageGroup->SetSamplerState(D3DSAMP_MAGFILTER, D3DTEXF_LINEAR, false, false);
    pkStage->m_pkTextureStageGroup->SetSamplerState(D3DSAMP_MINFILTER, D3DTEXF_LINEAR, false, false);
    pkStage->m_pkTextureStageGroup->SetSamplerState(D3DSAMP_MIPFILTER, D3DTEXF_NONE, false, false);
    pkGroup = pkStage->m_pkTextureStageGroup;
    pkStage->m_bTextureTransform = false;
    pkGroup->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE, false);
    spPass->SetStage(spPass->m_uiCurrentStage, pkStage);
    NiD3DShader::m_spCurrentPass = spPass.m_pObject;
    return 1;
}
