#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9MaterialManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"

class NiDitherProperty;
class NiDX9Renderer;
class NiDX9ShaderConstantManager;
class NiFogProperty;
class NiMaterialProperty;
class NiPropertyState;
class NiShadeProperty;
class NiSpecularProperty;
class NiTransform;

// The renderer's cache of Direct3D device state, filled from the scene's properties. Platform specifics (shaders,
// vertex formats, samplers) are pure here and implemented by NiDX9RenderState.
class NiD3DRenderState : public NiRefObject {
public:
    enum NiD3DSamplerState : uint32_t {
        NISAMP_INVALID = 0xFFFFFFFF,
        NISAMP_ADDRESSU = 0,
        NISAMP_ADDRESSV = 1,
        NISAMP_ADDRESSW = 2,
        NISAMP_BORDERCOLOR = 3,
        NISAMP_MAGFILTER = 4,
        NISAMP_MINFILTER = 5,
        NISAMP_MIPFILTER = 6,
        NISAMP_MIPMAPLODBIAS = 7,
        NISAMP_MAXMIPLEVEL = 8,
        NISAMP_MAXANISOTROPY = 9,
        NISAMP_SRGBTEXTURE = 10,
        NISAMP_ELEMENTINDEX = 11,
        NISAMP_DMAPOFFSET = 12,
        NISAMP_TOTAL_COUNT = 13,
    };

    // One cached device state and the value saved before it.
    struct NiRenderStateSetting {
        uint32_t m_uiCurrValue;
        uint32_t m_uiPrevValue;
    };

    using StateRevMappings14 = uint16_t[14];
    using StateRevMappings34 = uint16_t[34];

    NiD3DRenderState(NiDX9Renderer* pkD3DRenderer);
    ~NiD3DRenderState() override;

    virtual void UpdateRenderState(const NiPropertyState* apProperties);
    virtual void ApplyAlpha(const NiAlphaProperty* apAlpha);
    virtual void ApplyDitherMode(const NiDitherProperty* apDither);
    virtual void ApplyFog(const NiFogProperty* apFog);
    virtual void ApplyMaterial(const NiMaterialProperty* apMaterial);
    virtual void ApplyShading(const NiShadeProperty* apShading);
    virtual void ApplySpecular(const NiSpecularProperty* apSpecular);
    virtual void ApplyStencil(const NiStencilProperty* apStencil);
    virtual void DisableAlpha();
    virtual void SetBoneCount(uint16_t ausBoneCount);
    virtual void vSetModelTransform(const NiTransform* apTransform);
    virtual void SetAntiAlias(bool) {}
    virtual float GetMaxFogValue();
    virtual float GetMaxFogFactor();
    virtual void SetMaxFogValue(float afFogValue);
    virtual void GetCameraNearAndFar(float* apfNear, float* apfFar);
    virtual void SetCameraNearAndFar(float afNear, float afFar);
    virtual bool GetLeftHanded();
    virtual void SetLeftHanded(bool abLeftHanded);
    virtual void PurgeMaterial(NiMaterialProperty* apMaterial);
    virtual void InitRenderState();
    virtual void
    SetDestAndSrcBlends(uint32_t auiSourceBlend, uint32_t auiDestBlend, int32_t aiUnused0, int32_t aiUnused1);
    virtual void SetAlphaBlendEnable(uint32_t auiValue, uint32_t auiUnused0, uint32_t auiUnused1);
    virtual void SetDepthBias(float afBias, bool abSave);
    virtual void SetRenderState(uint32_t auiState, uint32_t auiValue, int32_t aiUnused, bool abSave);
    virtual uint32_t GetRenderState(uint32_t auiState);
    virtual void InitShaderState();
    virtual void SaveShaderState();
    virtual void RestoreShaderState();
    virtual void SetPixelShader(IDirect3DPixelShader9* apShader, bool abSave);
    virtual IDirect3DPixelShader9* GetPixelShader();
    virtual void RestorePixelShader();
    virtual void ClearPixelShader(IDirect3DPixelShader9* apShader);
    virtual void SetVertexShader(IDirect3DVertexShader9* apShader, bool abSave) = 0;
    virtual IDirect3DVertexShader9* GetVertexShader() = 0;
    virtual void RestoreVertexShader() = 0;
    virtual void ClearVertexShader(IDirect3DVertexShader9* apShader) = 0;
    virtual void SetFVF(uint32_t auiFVF, bool abSave) = 0;
    virtual uint32_t GetFVF() = 0;
    virtual void RestoreFVF() = 0;
    virtual void ClearFVF(uint32_t auiFVF) = 0;
    virtual void SetDeclaration(IDirect3DVertexDeclaration9* apDeclaration, bool abSave) = 0;
    virtual IDirect3DVertexDeclaration9* GetDeclaration() = 0;
    virtual void RestoreDeclaration() = 0;
    virtual void ClearDeclaration(IDirect3DVertexDeclaration9* apDeclaration) = 0;
    virtual void InitTextureStageState();
    virtual void SetTextureStageState(uint32_t auiStage, uint32_t auiState, uint32_t auiValue, bool abSave);
    virtual uint32_t GetTextureStageState(uint32_t auiStage, uint32_t auiState);
    virtual void RestoreTextureStageState(uint32_t, uint32_t) {}
    virtual void SetSamplerState(uint32_t auiStage, uint32_t auiState, uint32_t auiValue, bool abSave) = 0;
    virtual uint32_t GetSamplerState(uint32_t auiStage, NiD3DRenderState::NiD3DSamplerState aeState) = 0;
    virtual void RestoreSamplerState(uint32_t auiStage, NiD3DRenderState::NiD3DSamplerState aeState) = 0;
    virtual void InitTextures();
    virtual void SetTexture(uint32_t auiStage, IDirect3DBaseTexture9* apTexture);
    virtual IDirect3DBaseTexture9* GetTexture(uint32_t auiStage);
    virtual void ClearTexture(IDirect3DBaseTexture9* apTexture);
    virtual void SetSoftwareVertexProcessing(bool) {}
    virtual bool GetSoftwareVertexProcessing() {
        return false;
    }
    virtual void SetForceNormalizeNormals(bool abSet);
    virtual bool GetForceNormalizeNormals() {
        return m_bForceNormalizeNormals;
    }
    virtual void SetInternalNormalizeNormals(bool abSet);
    virtual bool GetInternalNormalizeNormals() {
        return m_bInternalNormalizeNormals;
    }
    virtual void InitRenderModes();
    virtual void ReadRenderStateCaps() = 0;

    // Set vertex shader constants on the device; abSave is unused. False when the device call fails.
    bool SetVertexShaderConstantB(
        uint32_t auiStartRegister,
        const int32_t* apiConstantData,
        uint32_t auiBoolCount,
        bool abSave
    );
    bool SetVertexShaderConstantF(
        uint32_t auiStartRegister,
        const float* apfConstantData,
        uint32_t auiVector4fCount,
        bool abSave
    );
    bool SetVertexShaderConstantI(
        uint32_t auiStartRegister,
        const int32_t* apiConstantData,
        uint32_t auiVector4iCount,
        bool abSave
    );

    uint32_t m_uiFlags;
    uint32_t m_aD3DVertexBlendMappings[5];
    uint32_t m_auiAlphaMapping[NiAlphaProperty::ALPHA_MAX_MODES];
    uint32_t m_auiAlphaTestMapping[NiAlphaProperty::TEST_MAX_MODES];
    uint32_t m_uiSrcAlphaCaps;
    uint32_t m_uiDestAlphaCaps;
    NiPointer<NiAlphaProperty> m_spDisabledAlphaProperty;
    float m_fCameraNear;
    float m_fCameraFar;
    float m_fCameraDepthRange;
    float m_fMaxFogFactor;
    float m_fMaxFogValue;
    NiColor m_kCurrentFogColor;
    uint32_t m_uiCurrentFogColor;
    uint32_t m_auiStencilTestMapping[NiStencilProperty::TEST_MAX];
    uint32_t m_auiStencilActionMapping[NiStencilProperty::ACTION_MAX];
    uint32_t m_auiCullModeMapping[NiStencilProperty::DRAW_MAX][2];
    uint32_t m_uiLeftHanded;
    NiDX9MaterialManager m_kMaterials;
    uint32_t m_auiZBufferTestMapping[8];
    NiRenderStateSetting m_akRenderStateSettings[256];
    bool m_bSavedStates[256];
    NiRenderStateSetting m_akTextureStageStateSettings[16][8];
    NiRenderStateSetting m_akSamplerStateSettings[16][5];
    IDirect3DBaseTexture9* m_apkTextureStageTextures[16];
    IDirect3DVertexShader9* m_hCurrentVertexShader;
    IDirect3DVertexShader9* m_hPreviousVertexShader;
    IDirect3DPixelShader9* m_hCurrentPixelShader;
    IDirect3DPixelShader9* m_hPreviousPixelShader;
    NiDX9ShaderConstantManager* m_spShaderConstantManager;
    bool m_bForceNormalizeNormals;
    bool m_bInternalNormalizeNormals;
    IDirect3DDevice9* m_pkD3DDevice;
    NiDX9Renderer* m_pkD3DRenderer;

    // Per Direct3D sampler state and texture stage state type, the index of its cache slot (at least the slot
    // count when the state is not cached).
    static inline StateRevMappings14 ms_auiSamplesStatesRevMappings{};
    static inline StateRevMappings34 ms_auiTextureStagesStatesRevMappings{};
};
