#pragma once

#include "BSShader/ShadowLightShader.h"

class SkinShader : public ShadowLightShader {
public:
    using kPasses_t = NiPointer<NiD3DPass>[26];

    SkinShader(NiDX9ShaderDeclaration* apShaderDecl, NiDX9ShaderDeclaration* apSkinnedShaderDecl);
    ~SkinShader() override;

    const NiRTTI* GetRTTI() const override;
    bool Initialize() override;
    void SetupGeometryTextures(const NiPropertyState* apState) override;
    void RecreateRendererData() override;
    void InitPass(uint32_t auiCurrentPass) override;
    void SetupPassTextures(uint32_t auiCurrentPass) override;
    using ShadowLightShader::GetShaderDecl;
    NiD3DShaderDeclaration* GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) override;
    void CreateConstantMaps() override;
    void ReloadShaders() override;
    void ClearPassStages() override;

protected:
    void LoadVertexShaders() override;
    void LoadPixelShaders() override;
    bool LoadStagesAndPasses() override;
    virtual void PresetPasses();

public:
    static BSShader* CreateShader();

    NiD3DVertexShader* pVertexShader[26];
    NiD3DPixelShader* pPixelShader[13];

    static NiRTTI ms_RTTI;
    static kPasses_t kPasses;
};
