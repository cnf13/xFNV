#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"

// The Direct3D 9 render state: the shared cache plus vertex shader, FVF and declaration tracking.
class NiDX9RenderState : public NiD3DRenderState {
public:
    // One entry of the tables of initial device states; each table ends with an m_eType of 0xFFFFFFFF.
    struct RenderStateInitialValue {
        D3DRENDERSTATETYPE m_eType;
        uint32_t m_uiValue;
    };

    struct TextureStageStateInitialValue {
        D3DTEXTURESTAGESTATETYPE m_eType;
        uint32_t m_uiValue;
    };

    struct SamplerStateInitialValue {
        D3DSAMPLERSTATETYPE m_eType;
        uint32_t m_uiValue;
    };

    using RenderStateInitialValueArray257 = RenderStateInitialValue[257];
    using TextureStageStateInitialValueArray9 = TextureStageStateInitialValue[9];
    using SamplerStateInitialValueArray6 = SamplerStateInitialValue[6];

    NiDX9RenderState(NiDX9Renderer* pkRenderer, const _D3DCAPS9* apkD3DCaps9);
    ~NiDX9RenderState() override;

    void InitShaderState() override;
    void SaveShaderState() override;
    void RestoreShaderState() override;
    void SetVertexShader(IDirect3DVertexShader9* apShader, bool abSave) override;
    IDirect3DVertexShader9* GetVertexShader() override;
    void RestoreVertexShader() override;
    void ClearVertexShader(IDirect3DVertexShader9* apShader) override;
    void SetFVF(uint32_t auiFVF, bool abSave) override;
    uint32_t GetFVF() override;
    void RestoreFVF() override;
    void ClearFVF(uint32_t auiFVF) override;
    void SetDeclaration(IDirect3DVertexDeclaration9* apDeclaration, bool abSave) override;
    IDirect3DVertexDeclaration9* GetDeclaration() override;
    void RestoreDeclaration() override;
    void ClearDeclaration(IDirect3DVertexDeclaration9* apDeclaration) override;
    void SetSamplerState(uint32_t auiStage, uint32_t auiState, uint32_t auiValue, bool abSave) override;
    uint32_t GetSamplerState(uint32_t auiStage, NiD3DRenderState::NiD3DSamplerState aeState) override;
    void RestoreSamplerState(uint32_t auiStage, NiD3DRenderState::NiD3DSamplerState aeState) override;
    void SetSoftwareVertexProcessing(bool abSoftware) override;
    bool GetSoftwareVertexProcessing() override {
        return m_bUsingSoftwareVP;
    }
    void InitRenderModes() override;
    void ReadRenderStateCaps() override;

    void SetD3DRenderer(NiDX9Renderer* apRenderer);
    // Sets the cull mode the draw mode maps to for the current handedness.
    void SetCullMode(NiStencilProperty::DrawMode aeMode, bool abSave);
    // Set pixel shader constants on the device; abSave is unused. False when the device call fails.
    bool SetPixelShaderConstantB(
        uint32_t auiStartRegister,
        const int32_t* apiConstantData,
        uint32_t auiBoolCount,
        bool abSave
    );
    bool SetPixelShaderConstantF(
        uint32_t auiStartRegister,
        const float* apfConstantData,
        uint32_t auiVector4fCount,
        bool abSave
    );
    bool SetPixelShaderConstantI(
        uint32_t auiStartRegister,
        const int32_t* apiConstantData,
        uint32_t auiVector4iCount,
        bool abSave
    );

    static NiDX9RenderState* Create(NiDX9Renderer* pkRenderer, _D3DCAPS9* pkD3DCaps9, bool bZBuffer);

    bool m_bDeclaration;
    uint32_t m_uiCurrentFVF;
    uint32_t m_uiPreviousFVF;
    IDirect3DVertexDeclaration9* m_hCurrentVertexDeclaration;
    IDirect3DVertexDeclaration9* m_hPreviousVertexDeclaration;
    bool m_bUsingSoftwareVP;
    D3DCAPS9 m_kD3DCaps9;

    static inline RenderStateInitialValueArray257 ms_akInitialRenderStateSettings{
        {D3DRS_ZENABLE, 1},
        {D3DRS_FILLMODE, 3},
        {D3DRS_SHADEMODE, 2},
        {D3DRS_ZWRITEENABLE, 1},
        {D3DRS_ALPHATESTENABLE},
        {D3DRS_LASTPIXEL, 1},
        {D3DRS_SRCBLEND, 2},
        {D3DRS_DESTBLEND, 1},
        {D3DRS_CULLMODE, 2},
        {D3DRS_ZFUNC, 4},
        {D3DRS_ALPHAREF},
        {D3DRS_ALPHAFUNC, 5},
        {D3DRS_DITHERENABLE},
        {D3DRS_ALPHABLENDENABLE},
        {D3DRS_FOGENABLE},
        {D3DRS_SPECULARENABLE},
        {D3DRS_FOGCOLOR},
        {D3DRS_FOGTABLEMODE},
        {D3DRS_FOGSTART},
        {D3DRS_FOGEND, 0x3F800000},
        {D3DRS_FOGDENSITY, 0x3F800000},
        {D3DRS_RANGEFOGENABLE},
        {D3DRS_STENCILENABLE},
        {D3DRS_STENCILFAIL, 1},
        {D3DRS_STENCILZFAIL, 1},
        {D3DRS_STENCILPASS, 1},
        {D3DRS_STENCILFUNC, 8},
        {D3DRS_STENCILREF},
        {D3DRS_STENCILMASK, 0xFFFFFFFF},
        {D3DRS_STENCILWRITEMASK, 0xFFFFFFFF},
        {D3DRS_TEXTUREFACTOR, 0xFFFFFFFF},
        {D3DRS_WRAP0},
        {D3DRS_WRAP1},
        {D3DRS_WRAP2},
        {D3DRS_WRAP3},
        {D3DRS_WRAP4},
        {D3DRS_WRAP5},
        {D3DRS_WRAP6},
        {D3DRS_WRAP7},
        {D3DRS_CLIPPING, 1},
        {D3DRS_LIGHTING, 1},
        {D3DRS_AMBIENT},
        {D3DRS_FOGVERTEXMODE},
        {D3DRS_COLORVERTEX, 1},
        {D3DRS_LOCALVIEWER, 1},
        {D3DRS_NORMALIZENORMALS},
        {D3DRS_DIFFUSEMATERIALSOURCE, 1},
        {D3DRS_SPECULARMATERIALSOURCE, 2},
        {D3DRS_AMBIENTMATERIALSOURCE},
        {D3DRS_EMISSIVEMATERIALSOURCE},
        {D3DRS_VERTEXBLEND},
        {D3DRS_CLIPPLANEENABLE},
        {D3DRS_POINTSIZE, 0x3F800000},
        {D3DRS_POINTSIZE_MIN, 0x3F800000},
        {D3DRS_POINTSPRITEENABLE},
        {D3DRS_POINTSCALEENABLE},
        {D3DRS_POINTSCALE_A, 0x3F800000},
        {D3DRS_POINTSCALE_B},
        {D3DRS_POINTSCALE_C},
        {D3DRS_MULTISAMPLEANTIALIAS, 1},
        {D3DRS_MULTISAMPLEMASK, 0xFFFFFFFF},
        {D3DRS_PATCHEDGESTYLE},
        {D3DRS_DEBUGMONITORTOKEN},
        {D3DRS_POINTSIZE_MAX, 0x42800000},
        {D3DRS_INDEXEDVERTEXBLENDENABLE},
        {D3DRS_COLORWRITEENABLE, 7},
        {D3DRS_TWEENFACTOR},
        {D3DRS_BLENDOP, 1},
        {D3DRS_POSITIONDEGREE, 3},
        {D3DRS_NORMALDEGREE, 1},
        {D3DRS_SCISSORTESTENABLE},
        {D3DRS_SLOPESCALEDEPTHBIAS},
        {D3DRS_ANTIALIASEDLINEENABLE},
        {D3DRS_MINTESSELLATIONLEVEL, 0x3F800000},
        {D3DRS_MAXTESSELLATIONLEVEL, 0x3F800000},
        {D3DRS_ADAPTIVETESS_X},
        {D3DRS_ADAPTIVETESS_Y},
        {D3DRS_ADAPTIVETESS_Z, 0x3F800000},
        {D3DRS_ADAPTIVETESS_W},
        {D3DRS_ENABLEADAPTIVETESSELLATION},
        {D3DRS_TWOSIDEDSTENCILMODE},
        {D3DRS_CCW_STENCILFAIL, 1},
        {D3DRS_CCW_STENCILZFAIL, 1},
        {D3DRS_CCW_STENCILPASS, 1},
        {D3DRS_CCW_STENCILFUNC, 8},
        {D3DRS_COLORWRITEENABLE1, 7},
        {D3DRS_COLORWRITEENABLE2, 7},
        {D3DRS_COLORWRITEENABLE3, 7},
        {D3DRS_BLENDFACTOR, 0xFFFFFFFF},
        {D3DRS_SRGBWRITEENABLE},
        {D3DRS_DEPTHBIAS},
        {D3DRS_WRAP8},
        {D3DRS_WRAP9},
        {D3DRS_WRAP10},
        {D3DRS_WRAP11},
        {D3DRS_WRAP12},
        {D3DRS_WRAP13},
        {D3DRS_WRAP14},
        {D3DRS_WRAP15},
        {D3DRS_SEPARATEALPHABLENDENABLE},
        {D3DRS_SRCBLENDALPHA, 2},
        {D3DRS_DESTBLENDALPHA, 1},
        {D3DRS_BLENDOPALPHA, 1},
        {D3DRS_NI_END_OF_LIST, 0xFFFFFFFF}
    };
    static inline TextureStageStateInitialValueArray9 ms_akInitialTextureStageStateSettings{
        {D3DTSS_COLOROP, 4},
        {D3DTSS_COLORARG1, 2},
        {D3DTSS_COLORARG2, 1},
        {D3DTSS_ALPHAOP, 2},
        {D3DTSS_ALPHAARG1, 2},
        {D3DTSS_ALPHAARG2, 1},
        {D3DTSS_BUMPENVMAT00},
        {static_cast<D3DTEXTURESTAGESTATETYPE>(4294967295), 0xFFFFFFFF}
    };
    static inline SamplerStateInitialValueArray6 ms_akInitialSamplerStateSettings{
        {D3DSAMP_ADDRESSU, 3},
        {D3DSAMP_ADDRESSV, 3},
        {D3DSAMP_MAGFILTER, 2},
        {D3DSAMP_MINFILTER, 2},
        {D3DSAMP_MIPFILTER, 2},
        {static_cast<D3DSAMPLERSTATETYPE>(4294967295), 0xFFFFFFFF}
    };
};
