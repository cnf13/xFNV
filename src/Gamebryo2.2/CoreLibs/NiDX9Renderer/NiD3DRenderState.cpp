#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderConstantManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDitherProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFogProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSpecularProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStageGroup.h"

#include <bit>

NiD3DRenderState::NiD3DRenderState(NiDX9Renderer* pkD3DRenderer)
  : m_uiFlags(0), m_aD3DVertexBlendMappings{0, 1, 1, 2, 3}, m_uiSrcAlphaCaps(0), m_uiDestAlphaCaps(0),
    m_spDisabledAlphaProperty(nullptr), m_fCameraNear(0.0f), m_fCameraFar(0.0f), m_fCameraDepthRange(0.0f),
    m_fMaxFogFactor(0.0f), m_fMaxFogValue(1.0f), m_uiLeftHanded(0), m_auiZBufferTestMapping{8, 2, 3, 4, 5, 6, 7, 1},
    m_spShaderConstantManager(nullptr), m_bForceNormalizeNormals(false), m_bInternalNormalizeNormals(false),
    m_pkD3DDevice(nullptr), m_pkD3DRenderer(nullptr) {
    // The device state caches start unset.
    for (int32_t i = 0; i < 256; ++i) {
        m_akRenderStateSettings[i].m_uiCurrValue = 0x7FFFFFFF;
    }
    for (int32_t i = 0; i < 16; ++i) {
        for (int32_t j = 0; j < 8; ++j) {
            m_akTextureStageStateSettings[i][j].m_uiCurrValue = 0x7FFFFFFF;
        }
    }
    for (int32_t i = 0; i < 16; ++i) {
        for (int32_t j = 0; j < 5; ++j) {
            m_akSamplerStateSettings[i][j].m_uiCurrValue = 0x7FFFFFFF;
        }
    }
    static_cast<NiDX9RenderState*>(this)->SetD3DRenderer(pkD3DRenderer);

    m_spDisabledAlphaProperty = new NiAlphaProperty;

    // D3DBLEND per NiAlphaProperty::AlphaFunction.
    m_auiAlphaMapping[NiAlphaProperty::ALPHA_ONE] = 2;
    m_auiAlphaMapping[NiAlphaProperty::ALPHA_ZERO] = 1;
    m_auiAlphaMapping[NiAlphaProperty::ALPHA_SRCCOLOR] = 3;
    m_auiAlphaMapping[NiAlphaProperty::ALPHA_INVSRCCOLOR] = 4;
    m_auiAlphaMapping[NiAlphaProperty::ALPHA_DESTCOLOR] = 9;
    m_auiAlphaMapping[NiAlphaProperty::ALPHA_INVDESTCOLOR] = 10;
    m_auiAlphaMapping[NiAlphaProperty::ALPHA_SRCALPHA] = 5;
    m_auiAlphaMapping[NiAlphaProperty::ALPHA_INVSRCALPHA] = 6;
    m_auiAlphaMapping[NiAlphaProperty::ALPHA_DESTALPHA] = 7;
    m_auiAlphaMapping[NiAlphaProperty::ALPHA_INVDESTALPHA] = 8;
    m_auiAlphaMapping[NiAlphaProperty::ALPHA_SRCALPHASAT] = 11;
    // D3DCMP per test function (alpha, stencil).
    m_auiAlphaTestMapping[0] = 8;
    m_auiAlphaTestMapping[1] = 2;
    m_auiAlphaTestMapping[2] = 3;
    m_auiAlphaTestMapping[3] = 4;
    m_auiAlphaTestMapping[4] = 5;
    m_auiAlphaTestMapping[5] = 6;
    m_auiAlphaTestMapping[6] = 7;
    m_auiAlphaTestMapping[7] = 1;
    for (uint32_t i = 0; i < 8; ++i) {
        m_auiStencilTestMapping[i] = i + 1;
    }
    // D3DSTENCILOP per NiStencilProperty::Action.
    for (uint32_t i = 0; i < 6; ++i) {
        m_auiStencilActionMapping[i] = i + 1;
    }
    // D3DCULL_CW 2, CCW 3, NONE 1, per draw mode and handedness.
    m_auiCullModeMapping[0][0] = 2;
    m_auiCullModeMapping[0][1] = 3;
    m_auiCullModeMapping[1][0] = 2;
    m_auiCullModeMapping[1][1] = 3;
    m_auiCullModeMapping[2][0] = 3;
    m_auiCullModeMapping[2][1] = 2;
    m_auiCullModeMapping[3][0] = 1;
    m_auiCullModeMapping[3][1] = 1;

    // D3DCOLOR_XRGB of the fog color; each channel is the low byte of the truncated double product.
    m_kCurrentFogColor = NiColor::BLACK;
    uint8_t ucR = static_cast<uint8_t>(static_cast<int32_t>(m_kCurrentFogColor.r * 255.0));
    uint8_t ucG = static_cast<uint8_t>(static_cast<int32_t>(m_kCurrentFogColor.g * 255.0));
    uint8_t ucB = static_cast<uint8_t>(static_cast<int32_t>(m_kCurrentFogColor.b * 255.0));
    m_uiCurrentFogColor = (((ucR | 0xFFFFFF00u) << 8 | ucG) << 8) | ucB;

    // The shared reverse tables from D3D state type to cache slot; only the first 33 texture stage entries are
    // cleared.
    for (int32_t i = 0; i < 33; ++i) {
        ms_auiTextureStagesStatesRevMappings[i] = 0xFFFF;
    }
    ms_auiTextureStagesStatesRevMappings[1] = 0;
    ms_auiTextureStagesStatesRevMappings[2] = 1;
    ms_auiTextureStagesStatesRevMappings[3] = 2;
    ms_auiTextureStagesStatesRevMappings[4] = 3;
    ms_auiTextureStagesStatesRevMappings[5] = 4;
    ms_auiTextureStagesStatesRevMappings[6] = 5;
    ms_auiTextureStagesStatesRevMappings[7] = 6;
    ms_auiTextureStagesStatesRevMappings[24] = 7;
    // Cache slot -> D3DTEXTURESTAGESTATETYPE.
    for (uint16_t i = 0; i < 7; ++i) {
        NiD3DTextureStageGroup::textureStageTypes[i] = i + 1;
    }
    NiD3DTextureStageGroup::textureStageTypes[7] = 24;
    // Sampler state type -> cache slot.
    for (uint16_t& usMapping : ms_auiSamplesStatesRevMappings) {
        usMapping = 0xFFFF;
    }
    ms_auiSamplesStatesRevMappings[1] = 0;
    ms_auiSamplesStatesRevMappings[2] = 1;
    ms_auiSamplesStatesRevMappings[5] = 2;
    ms_auiSamplesStatesRevMappings[6] = 3;
    ms_auiSamplesStatesRevMappings[7] = 4;
    // Cache slot -> D3DSAMPLERSTATETYPE (ADDRESSU, ADDRESSV, MAGFILTER, MINFILTER, MIPFILTER).
    NiD3DTextureStageGroup::ms_aeArrayToSamplerState[0] = 1;
    NiD3DTextureStageGroup::ms_aeArrayToSamplerState[1] = 2;
    NiD3DTextureStageGroup::ms_aeArrayToSamplerState[2] = 5;
    NiD3DTextureStageGroup::ms_aeArrayToSamplerState[3] = 6;
    NiD3DTextureStageGroup::ms_aeArrayToSamplerState[4] = 7;
}

// The alpha property, the materials and the reference count go with their own destructors.
NiD3DRenderState::~NiD3DRenderState() {
    m_spDisabledAlphaProperty = nullptr;
    m_pkD3DRenderer = nullptr;
    if (m_pkD3DDevice) {
        m_pkD3DDevice->Release();
    }
    m_pkD3DDevice = nullptr;
    if (m_spShaderConstantManager) {
        m_spShaderConstantManager->DecRefCount();
    }
}

// Apply the present properties in the order used by the state cache.
void NiD3DRenderState::UpdateRenderState(const NiPropertyState* apProperties) {
    ApplyShading(static_cast<const NiShadeProperty*>(apProperties->m_aspProps[NiProperty::SHADE].m_pObject));
    ApplyAlpha(static_cast<const NiAlphaProperty*>(apProperties->m_aspProps[NiProperty::ALPHA].m_pObject));
    ApplyStencil(static_cast<const NiStencilProperty*>(apProperties->m_aspProps[NiProperty::STENCIL].m_pObject));
    ApplyMaterial(static_cast<const NiMaterialProperty*>(apProperties->m_aspProps[NiProperty::MATERIAL].m_pObject));
}

// Alpha blending with the mapped source and destination blends, then the alpha test with its function and reference.
void NiD3DRenderState::ApplyAlpha(const NiAlphaProperty* apAlpha) {
    if (apAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK) {
        SetRenderState(D3DRS_ALPHABLENDENABLE, 1, 0, false);
        SetRenderState(D3DRS_SRCBLEND, m_auiAlphaMapping[(apAlpha->m_uFlags >> 1) & 0xF], 0, false);
        SetRenderState(D3DRS_DESTBLEND, m_auiAlphaMapping[(apAlpha->m_uFlags >> 5) & 0xF], 0, false);
    }
    else {
        SetRenderState(D3DRS_ALPHABLENDENABLE, 0, 0, false);
    }
    if ((apAlpha->m_uFlags >> 9) & 1) {
        SetRenderState(D3DRS_ALPHATESTENABLE, 1, 0, false);
        SetRenderState(D3DRS_ALPHAFUNC, m_auiAlphaTestMapping[(apAlpha->m_uFlags >> 10) & 7], 0, false);
        SetRenderState(D3DRS_ALPHAREF, apAlpha->m_ucAlphaTestRef, 0, false);
    }
    else {
        SetRenderState(D3DRS_ALPHATESTENABLE, 0, 0, false);
    }
}

// Dithering follows the property's enable bit.
void NiD3DRenderState::ApplyDitherMode(const NiDitherProperty* apDither) {
    SetRenderState(D3DRS_DITHERENABLE, apDither->m_uFlags & NiDitherProperty::DITHER_MASK, 0, false);
}

// Fog over the property's share of the camera depth range, its function (table or vertex fog, plain or range
// based), and its color, packed again only when it changed.
void NiD3DRenderState::ApplyFog(const NiFogProperty* apFog) {
    if (!(apFog->m_uFlags & NiFogProperty::FOG_MASK)) {
        SetRenderState(D3DRS_FOGENABLE, 0, 0, false);
        return;
    }
    SetRenderState(D3DRS_FOGENABLE, 1, 0, false);
    float fDepth = m_fCameraDepthRange * apFog->m_fDepth;
    float fFogStart = m_fCameraFar - fDepth;
    float fFogEnd = m_fMaxFogFactor * fDepth + m_fCameraFar;
    float fDensity = 1.0f / fDepth;
    SetRenderState(D3DRS_FOGDENSITY, std::bit_cast<uint32_t>(fDensity), 0, false);
    SetRenderState(D3DRS_FOGSTART, std::bit_cast<uint32_t>(fFogStart), 0, false);
    SetRenderState(D3DRS_FOGEND, std::bit_cast<uint32_t>(fFogEnd), 0, false);

    uint16_t usFunction = (apFog->m_uFlags >> 1) & 3;
    if (usFunction == 0) {
        if (m_uiFlags & CAPS_TABLE_FOG) {
            // D3DFOG_LINEAR table fog, no vertex fog.
            SetRenderState(D3DRS_FOGTABLEMODE, 3, 0, false);
            SetRenderState(D3DRS_FOGVERTEXMODE, 0, 0, false);
        }
        SetRenderState(D3DRS_RANGEFOGENABLE, 0, 0, false);
    }
    else if (usFunction == 1) {
        if (m_uiFlags & CAPS_TABLE_FOG) {
            // D3DFOG_LINEAR vertex fog, no table fog.
            SetRenderState(D3DRS_FOGTABLEMODE, 0, 0, false);
            SetRenderState(D3DRS_FOGVERTEXMODE, 3, 0, false);
        }
        SetRenderState(D3DRS_RANGEFOGENABLE, 1, 0, false);
    }
    else {
        SetRenderState(D3DRS_FOGENABLE, 0, 0, false);
    }

    if (apFog->m_kColor.r != m_kCurrentFogColor.r ||
        apFog->m_kColor.g != m_kCurrentFogColor.g ||
        apFog->m_kColor.b != m_kCurrentFogColor.b) {
        uint32_t uiRed = static_cast<uint8_t>(static_cast<int32_t>(apFog->m_kColor.r * 255.0));
        uint32_t uiGreen = static_cast<uint8_t>(static_cast<int32_t>(apFog->m_kColor.g * 255.0));
        uint32_t uiBlue = static_cast<uint8_t>(static_cast<int32_t>(apFog->m_kColor.b * 255.0));
        m_uiCurrentFogColor = 0xFF000000 | (uiRed << 16) | (uiGreen << 8) | uiBlue;
        m_kCurrentFogColor = apFog->m_kColor;
    }
    SetRenderState(D3DRS_FOGCOLOR, m_uiCurrentFogColor, 0, false);
}

// Apply the material through the material manager.
void NiD3DRenderState::ApplyMaterial(const NiMaterialProperty* apMaterial) {
    m_kMaterials.SetCurrentMaterial(apMaterial);
    m_kMaterials.UpdateToDevice(m_pkD3DDevice);
}

// Gouraud shading when the property's smooth bit is set, flat otherwise.
void NiD3DRenderState::ApplyShading(const NiShadeProperty* apShading) {
    SetRenderState(
        D3DRS_SHADEMODE,
        (apShading->m_uFlags & NiShadeProperty::SMOOTH_MASK) ? D3DSHADE_GOURAUD : D3DSHADE_FLAT,
        0,
        false
    );
}

// Specular lighting follows the property's enable bit.
void NiD3DRenderState::ApplySpecular(const NiSpecularProperty* apSpecular) {
    SetRenderState(D3DRS_SPECULARENABLE, apSpecular->m_uFlags & NiSpecularProperty::SPECULAR_MASK, 0, false);
}

// Stencil test and actions when enabled; the cull mode follows the draw mode and handedness either way.
void NiD3DRenderState::ApplyStencil(const NiStencilProperty* apStencil) {
    if (apStencil->m_uFlags & NiStencilProperty::ENABLE_MASK) {
        SetRenderState(D3DRS_STENCILENABLE, 1, 0, false);
        SetRenderState(D3DRS_STENCILFUNC, m_auiStencilTestMapping[apStencil->m_uFlags >> 12], 0, false);
        SetRenderState(D3DRS_STENCILREF, apStencil->m_uiRef, 0, false);
        SetRenderState(D3DRS_STENCILMASK, apStencil->m_uiMask, 0, false);
        SetRenderState(D3DRS_STENCILFAIL, m_auiStencilActionMapping[(apStencil->m_uFlags >> 1) & 7], 0, false);
        SetRenderState(D3DRS_STENCILZFAIL, m_auiStencilActionMapping[(apStencil->m_uFlags >> 4) & 7], 0, false);
        SetRenderState(D3DRS_STENCILPASS, m_auiStencilActionMapping[(apStencil->m_uFlags >> 7) & 7], 0, false);
        SetRenderState(D3DRS_STENCILWRITEMASK, 0xFF, 0, false);
    }
    SetRenderState(D3DRS_CULLMODE, m_auiCullModeMapping[(apStencil->m_uFlags >> 10) & 3][m_uiLeftHanded], 0, false);
}

// Apply a default alpha property so alpha state is reset.
void NiD3DRenderState::DisableAlpha() {
    ApplyAlpha(m_spDisabledAlphaProperty.m_pObject);
}

// The vertex blend state the bone count maps to.
void NiD3DRenderState::SetBoneCount(uint16_t ausBoneCount) {
    SetRenderState(D3DRS_VERTEXBLEND, m_aD3DVertexBlendMappings[ausBoneCount], 0, false);
}

// Normalize normals when the model's scale is not within [0.99, 1.01] (or is NaN), or when asked to.
void NiD3DRenderState::vSetModelTransform(const NiTransform* apTransform) {
    if (!(apTransform->m_fScale >= 0.9900000095367432) ||
        apTransform->m_fScale > 1.01f ||
        m_bInternalNormalizeNormals ||
        m_bForceNormalizeNormals) {
        SetRenderState(D3DRS_NORMALIZENORMALS, 1, 0, false);
    }
    else {
        SetRenderState(D3DRS_NORMALIZENORMALS, 0, 0, false);
    }
}

// Return the cached maximum fog value.
float NiD3DRenderState::GetMaxFogValue() {
    return m_fMaxFogValue;
}

// Return the cached maximum fog factor.
float NiD3DRenderState::GetMaxFogFactor() {
    return m_fMaxFogFactor;
}

// A fog value at or below zero becomes 1e-5; the fog factor is the value's reciprocal minus one.
void NiD3DRenderState::SetMaxFogValue(float afFogValue) {
    if (afFogValue <= 0.0f) {
        afFogValue = 1e-05f;
    }
    m_fMaxFogValue = afFogValue;
    m_fMaxFogFactor = 1.0f / afFogValue - 1.0f;
}

// Copy cached camera clipping distances to the caller's outputs.
void NiD3DRenderState::GetCameraNearAndFar(float* apfNear, float* apfFar) {
    *apfNear = m_fCameraNear;
    *apfFar = m_fCameraFar;
}

void NiD3DRenderState::SetCameraNearAndFar(float afNear, float afFar) {
    m_fCameraNear = afNear;
    m_fCameraFar = afFar;
    m_fCameraDepthRange = afFar - afNear;
}

bool NiD3DRenderState::GetLeftHanded() {
    return m_uiLeftHanded != 0;
}

void NiD3DRenderState::SetLeftHanded(bool abLeftHanded) {
    m_uiLeftHanded = abLeftHanded ? 1 : 0;
}

// Discard the material manager's cached entry for apMaterial.
void NiD3DRenderState::PurgeMaterial(NiMaterialProperty* apMaterial) {
    m_kMaterials.PurgeMaterial(apMaterial);
}

// Send every initial render state to the device and cache it as both the current and the previous value.
void NiD3DRenderState::InitRenderState() {
    for (NiDX9RenderState::RenderStateInitialValue* pkInit = NiDX9RenderState::ms_akInitialRenderStateSettings;
         pkInit->m_eType != 0xFFFFFFFF;
         ++pkInit) {
        m_akRenderStateSettings[pkInit->m_eType].m_uiPrevValue = pkInit->m_uiValue;
        m_akRenderStateSettings[pkInit->m_eType].m_uiCurrValue = pkInit->m_uiValue;
        m_pkD3DDevice->SetRenderState(pkInit->m_eType, pkInit->m_uiValue);
    }
}

void NiD3DRenderState::SetDestAndSrcBlends(
    uint32_t auiSourceBlend,
    uint32_t auiDestBlend,
    int32_t aiUnused0,
    int32_t aiUnused1
) {
    if (m_akRenderStateSettings[D3DRS_SRCBLEND].m_uiCurrValue != auiSourceBlend) {
        m_pkD3DDevice->SetRenderState(D3DRS_SRCBLEND, auiSourceBlend);
    }
    m_akRenderStateSettings[D3DRS_SRCBLEND].m_uiCurrValue = auiSourceBlend;
    if (m_akRenderStateSettings[D3DRS_DESTBLEND].m_uiCurrValue != auiDestBlend) {
        m_pkD3DDevice->SetRenderState(D3DRS_DESTBLEND, auiDestBlend);
    }
    m_akRenderStateSettings[D3DRS_DESTBLEND].m_uiCurrValue = auiDestBlend;
}

void NiD3DRenderState::SetAlphaBlendEnable(uint32_t auiValue, uint32_t auiUnused0, uint32_t auiUnused1) {
    if (m_akRenderStateSettings[D3DRS_ALPHABLENDENABLE].m_uiCurrValue != auiValue) {
        m_pkD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, auiValue);
    }
    m_akRenderStateSettings[D3DRS_ALPHABLENDENABLE].m_uiCurrValue = auiValue;
}

// The bias goes to the device as the float's bits.
void NiD3DRenderState::SetDepthBias(float afBias, bool abSave) {
    SetRenderState(D3DRS_DEPTHBIAS, std::bit_cast<uint32_t>(afBias), 0, abSave);
}

// A state saved earlier is locked until it is restored; otherwise the device is updated only on a change.
void NiD3DRenderState::SetRenderState(uint32_t auiState, uint32_t auiValue, int32_t aiUnused, bool abSave) {
    if (m_bSavedStates[auiState]) {
        return;
    }
    m_bSavedStates[auiState] = abSave;
    if (m_akRenderStateSettings[auiState].m_uiCurrValue != auiValue) {
        m_pkD3DDevice->SetRenderState(auiState, auiValue);
        m_akRenderStateSettings[auiState].m_uiCurrValue = auiValue;
    }
}

// Read the cached value for a render-state index.
uint32_t NiD3DRenderState::GetRenderState(uint32_t auiState) {
    return m_akRenderStateSettings[auiState].m_uiCurrValue;
}

// Reset current shader pointers and set the saved pointers to the invalid sentinel.
void NiD3DRenderState::InitShaderState() {
    m_hCurrentVertexShader = nullptr;
    m_hPreviousVertexShader = reinterpret_cast<IDirect3DVertexShader9*>(1);
    m_hCurrentPixelShader = nullptr;
    m_hPreviousPixelShader = reinterpret_cast<IDirect3DPixelShader9*>(1);
}

// Save the current vertex and pixel shaders.
void NiD3DRenderState::SaveShaderState() {
    m_hPreviousVertexShader = m_hCurrentVertexShader;
    m_hPreviousPixelShader = m_hCurrentPixelShader;
}

// Restore pixel shader state before vertex shader state.
void NiD3DRenderState::RestoreShaderState() {
    RestorePixelShader();
    RestoreVertexShader();
}

// Cache the pixel shader and update the device only when it changes.
void NiD3DRenderState::SetPixelShader(IDirect3DPixelShader9* apShader, bool abSave) {
    if (abSave) {
        m_hPreviousPixelShader = m_hCurrentPixelShader;
    }

    if (m_hCurrentPixelShader != apShader) {
        m_hCurrentPixelShader = apShader;
        m_pkD3DDevice->SetPixelShader(apShader);
    }
}

IDirect3DPixelShader9* NiD3DRenderState::GetPixelShader() {
    return m_hCurrentPixelShader;
}

// Restore the saved pixel shader without replacing the saved value.
void NiD3DRenderState::RestorePixelShader() {
    SetPixelShader(m_hPreviousPixelShader, false);
}

// Forget apShader as the current (unbinding it from the device) and as the saved pixel shader.
void NiD3DRenderState::ClearPixelShader(IDirect3DPixelShader9* apShader) {
    if (m_hCurrentPixelShader == apShader) {
        m_hCurrentPixelShader = nullptr;
        m_pkD3DDevice->SetPixelShader(nullptr);
    }
    if (m_hPreviousPixelShader == apShader) {
        m_hPreviousPixelShader = nullptr;
    }
}

// Send the initial texture stage states to every blend stage (each stage reads its own coordinate set; stages
// after the first start disabled, the table keeps those values), then the initial sampler states to every sampler.
void NiD3DRenderState::InitTextureStageState() {
    for (uint32_t uiStage = 0; uiStage < NiD3DPass::ms_uiMaxTextureBlendStages; ++uiStage) {
        for (NiDX9RenderState::TextureStageStateInitialValue* pkInit =
                 NiDX9RenderState::ms_akInitialTextureStageStateSettings;
             pkInit->m_eType != 0xFFFFFFFF;
             ++pkInit) {
            D3DTEXTURESTAGESTATETYPE eType = pkInit->m_eType;
            if (eType == D3DTSS_TEXCOORDINDEX) {
                pkInit->m_uiValue = uiStage;
            }
            else if ((eType == D3DTSS_COLOROP || eType == D3DTSS_ALPHAOP) && uiStage != 0) {
                // D3DTOP_DISABLE
                pkInit->m_uiValue = 1;
            }
            m_akTextureStageStateSettings[uiStage][ms_auiTextureStagesStatesRevMappings[eType]]
                .m_uiPrevValue = pkInit->m_uiValue;
            m_pkD3DDevice->SetTextureStageState(uiStage, eType, pkInit->m_uiValue);
        }
    }
    for (uint32_t uiSampler = 0; uiSampler < NiD3DPass::ms_uiMaxSamplers; ++uiSampler) {
        for (NiDX9RenderState::SamplerStateInitialValue* pkInit = NiDX9RenderState::ms_akInitialSamplerStateSettings;
             pkInit->m_eType != 0xFFFFFFFF;
             ++pkInit) {
            D3DSAMPLERSTATETYPE eType = pkInit->m_eType;
            m_akSamplerStateSettings[uiSampler][ms_auiSamplesStatesRevMappings[eType]].m_uiPrevValue = pkInit
                                                                                                           ->m_uiValue;
            SetSamplerState(uiSampler, eType, pkInit->m_uiValue, false);
            m_pkD3DDevice->SetSamplerState(uiSampler, eType, pkInit->m_uiValue);
        }
    }
}

// Cached texture stage states only (slot below 8); the device is updated only on a change.
void NiD3DRenderState::SetTextureStageState(uint32_t auiStage, uint32_t auiState, uint32_t auiValue, bool abSave) {
    uint16_t usIndex = ms_auiTextureStagesStatesRevMappings[auiState];
    if (usIndex < 8) {
        NiRenderStateSetting* pkSetting = &m_akTextureStageStateSettings[auiStage][usIndex];
        if (pkSetting->m_uiCurrValue != auiValue) {
            m_pkD3DDevice->SetTextureStageState(auiStage, auiState, auiValue);
        }
        pkSetting->m_uiCurrValue = auiValue;
    }
}

// The cached value of a state that has a cache slot; zero for any other.
uint32_t NiD3DRenderState::GetTextureStageState(uint32_t auiStage, uint32_t auiState) {
    const uint16_t usIndex = ms_auiTextureStagesStatesRevMappings[auiState];
    if (usIndex >= 8) {
        return 0;
    }
    return m_akTextureStageStateSettings[auiStage][usIndex].m_uiCurrValue;
}

// Clear the cached texture pointers.
void NiD3DRenderState::InitTextures() {
    m_apkTextureStageTextures[0] = nullptr;
    m_apkTextureStageTextures[1] = nullptr;
    m_apkTextureStageTextures[2] = nullptr;
    m_apkTextureStageTextures[3] = nullptr;
    m_apkTextureStageTextures[4] = nullptr;
    m_apkTextureStageTextures[5] = nullptr;
    m_apkTextureStageTextures[6] = nullptr;
    m_apkTextureStageTextures[7] = nullptr;
    m_apkTextureStageTextures[8] = nullptr;
    m_apkTextureStageTextures[9] = nullptr;
    m_apkTextureStageTextures[10] = nullptr;
    m_apkTextureStageTextures[11] = nullptr;
    m_apkTextureStageTextures[12] = nullptr;
    m_apkTextureStageTextures[13] = nullptr;
    m_apkTextureStageTextures[14] = nullptr;
    m_apkTextureStageTextures[15] = nullptr;
}

void NiD3DRenderState::SetTexture(uint32_t auiStage, IDirect3DBaseTexture9* apTexture) {
    if (m_apkTextureStageTextures[auiStage] != apTexture) {
        m_apkTextureStageTextures[auiStage] = apTexture;
        m_pkD3DDevice->SetTexture(auiStage, apTexture);
    }
}

IDirect3DBaseTexture9* NiD3DRenderState::GetTexture(uint32_t auiStage) {
    return m_apkTextureStageTextures[auiStage];
}

// Unbind apTexture from every stage that holds it.
void NiD3DRenderState::ClearTexture(IDirect3DBaseTexture9* apTexture) {
    if (apTexture) {
        for (uint32_t i = 0; i < 16; ++i) {
            if (m_apkTextureStageTextures[i] == apTexture) {
                m_apkTextureStageTextures[i] = nullptr;
                m_pkD3DDevice->SetTexture(i, nullptr);
            }
        }
    }
}

void NiD3DRenderState::SetForceNormalizeNormals(bool abSet) {
    m_bForceNormalizeNormals = abSet;
}

void NiD3DRenderState::SetInternalNormalizeNormals(bool abSet) {
    m_bInternalNormalizeNormals = abSet;
}

// Reset the device state caches before initializing the material manager.
void NiD3DRenderState::InitRenderModes() {
    InitRenderState();
    InitShaderState();
    InitTextureStageState();
    InitTextures();
    m_kMaterials.Initialize();
}

bool NiD3DRenderState::SetVertexShaderConstantB(
    uint32_t auiStartRegister,
    const int32_t* apiConstantData,
    uint32_t auiBoolCount,
    bool abSave
) {
    return m_pkD3DDevice->SetVertexShaderConstantB(auiStartRegister, apiConstantData, auiBoolCount) >= 0;
}

bool NiD3DRenderState::SetVertexShaderConstantF(
    uint32_t auiStartRegister,
    const float* apfConstantData,
    uint32_t auiVector4fCount,
    bool abSave
) {
    return m_pkD3DDevice->SetVertexShaderConstantF(auiStartRegister, apfConstantData, auiVector4fCount) >= 0;
}

bool NiD3DRenderState::SetVertexShaderConstantI(
    uint32_t auiStartRegister,
    const int32_t* apiConstantData,
    uint32_t auiVector4iCount,
    bool abSave
) {
    return m_pkD3DDevice->SetVertexShaderConstantI(auiStartRegister, apiConstantData, auiVector4iCount) >= 0;
}
