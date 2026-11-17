#pragma once

#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"

class NiD3DPass;

// The DX9 renderer's fallback shader, with a single pass.
class NiDX9ErrorShader : public NiD3DShader {
public:
    NiDX9ErrorShader();
    ~NiDX9ErrorShader() override;

    const NiRTTI* GetRTTI() const override;
    bool Initialize() override;
    uint32_t UpdatePipeline(
        NiGeometry*,
        const NiSkinInstance*,
        NiGeometryData::RendererData*,
        const NiPropertyState*,
        const NiDynamicEffectState*,
        const NiTransform&,
        const NiBound&
    ) override;
    uint32_t SetupTransformations(
        NiGeometry* apGeometry,
        const NiSkinInstance* apSkin,
        const NiSkinPartition::Partition* apPartition,
        NiGeometryData::RendererData* apRendererData,
        const NiPropertyState* apState,
        const NiDynamicEffectState* apEffects,
        const NiTransform& akWorld,
        const NiBound& akWorldBound
    ) override;

    bool CreateShaderDeclaration();
    char CreateStagesAndPasses();

    NiD3DPass* m_pkPass;

    static NiRTTI ms_RTTI;
};
