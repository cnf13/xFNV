#pragma once

#include "BSShader/BSShader.h"

#include <stdint.h>

class NiD3DPixelShader;
class NiD3DVertexShader;
class NiScreenElements;

class DebugShader : public BSShader {
public:
    // The channel mask the debug pixel shader's ChannelMask constant reads.
    using ChannelMaskArray = float[4];

    DebugShader();
    ~DebugShader() override;

    bool Initialize() override;
    bool SetupGeometry(NiGeometry* apGeometry) override;
    uint32_t PreProcessPipeline(
        NiGeometry* apGeometry,
        const NiSkinInstance* apSkin,
        NiGeometryData::RendererData* apBuffData,
        const NiPropertyState* apState,
        const NiDynamicEffectState* apEffects,
        const NiTransform& aWorld,
        const NiBound& aWorldBound
    ) override;
    uint32_t UpdatePipeline(
        NiGeometry* apGeometry,
        const NiSkinInstance* apSkin,
        NiGeometryData::RendererData* apBuffData,
        const NiPropertyState* apState,
        const NiDynamicEffectState* apEffects,
        const NiTransform& aWorld,
        const NiBound& aWorldBound
    ) override;
    void ReloadShaders() override;

    // Draws the shader's effect into apDstBuffer.
    void RenderEffect(BSRenderedTexture* apDstBuffer);
    // Makes apTexture the texture the shader draws: a NiSourceTexture or a NiRenderedTexture, told apart by RTTI.
    void SetTextureToRender(NiTexture* apTexture);

protected:
    void LoadShaders();

public:
    bool LoadStagesAndPasses();

    static BSShader* CreateShader();

    NiRenderedTexture* spTextureToRender[16];
    NiScreenElements* spScreenTriShape;
    NiD3DVertexShader* spVertexShader;
    NiD3DPixelShader* spPixelShader[2];
    uint32_t curTexIdx;

    static inline ChannelMaskArray fChannelMask{1.0f, 1.0f, 1.0f, 1.0f};
    static inline int32_t uiPixelShaderCount;
    static inline int32_t iCurrentShaderPass;
};
