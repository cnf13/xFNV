#pragma once

#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"

class NiD3DPixelShader;
class NiD3DVertexShader;

class GeometryDecalShader : public BSShader {
public:
    GeometryDecalShader();
    ~GeometryDecalShader() override;

    const NiRTTI* GetRTTI() const override;
    bool Initialize() override;
    uint32_t SetupBoneMatrices(
        NiGeometry*,
        const NiSkinInstance*,
        const NiSkinPartition::Partition*,
        NiGeometryData::RendererData*,
        const NiPropertyState*,
        const NiDynamicEffectState*,
        const NiTransform&,
        const NiBound&
    ) override;
    void SetupGeometryTextures(const NiPropertyState* apState) override;
    void SetupGeometryConstants(const NiPropertyState* apState) override;
    void InitPass(uint32_t auiCurrentPass) override;
    void SetupPassConstants(uint32_t auiCurrentPass) override;
    using BSShader::GetShaderDecl;
    NiD3DShaderDeclaration* GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) override;
    void CreateConstantMaps() override;
    void ReloadShaders() override;
    void ClearPassStages() override;
    virtual void PresetPasses();

    void LoadShaders();

    static BSShader* CreateShader();

    // CreateShader passes each to NiD3DPass::CreateNewPass(NiPointer<NiD3DPass>&).
    NiPointer<NiD3DPass> spGeometryDecalPass[2];
    NiD3DVertexShader* spGeometryDecalVertexShader[2];
    NiD3DPixelShader* spGeometryDecalPixelShader[2];
    NiShaderConstantMapEntry* spVSCMETransform;
    NiShaderConstantMapEntry* spVSCMESkinTransform;
    NiShaderConstantMapEntry* spVSCMEBoneMatrix;

    static NiRTTI ms_RTTI;
    static NiPointer<NiD3DShaderDeclaration> spShaderDeclaration;
    static NiPointer<NiD3DShaderDeclaration> spShaderDeclarationSkinned;
    // The "decal fade" vertex and "Decal Offset" pixel shader constants, set up in CreateConstantMaps.
    static inline NiVector4 pDecalFade{};
    static inline NiVector4 pDecalOffset{};
};
