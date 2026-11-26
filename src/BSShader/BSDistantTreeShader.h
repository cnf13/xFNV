#pragma once

#include "BSShader/BSShader.h"
#include "BSShader/FogProperties.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

class NiD3DPixelShader;
class NiD3DVertexShader;
class NiDX9ShaderDeclaration;

class BSDistantTreeShader : public BSShader {
public:
    // One tree instance's constants, two registers.
    struct InstanceData {
        float fx;
        float fy;
        float fz;
        float fScale;
        float fzrot1;
        float fzrot2;
        float fUnused1;
        float fUnused2;
    };

    explicit BSDistantTreeShader(NiDX9ShaderDeclaration* apDecl);
    ~BSDistantTreeShader() override;

    const NiRTTI* GetRTTI() const override;
    bool Initialize() override;
    void SetupGeometryConstants(const NiPropertyState* apState) override;
    void PostGeometry(const NiPropertyState* apState) override;
    void InitPass(uint32_t auiCurrentPass) override;
    void SetupPassTextures(uint32_t auiCurrentPass) override;
    void SetupPassConstants(uint32_t auiCurrentPass) override;
    using BSShader::GetShaderDecl;
    NiD3DShaderDeclaration* GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) override;
    void CreateConstantMaps() override;
    void ReloadShaders() override;
    void ClearPassStages() override;
    virtual void SetupGeometryConstants_Instances(const NiPropertyState* apState);

    bool LoadStagesAndPasses();
    void LoadShaders();

    // Sets the shared texture atlas.
    static void SetTextureAtlas(NiTexture* apTextureAtlas);
    static BSShader* CreateShader();

    // These elements are NiPointer because one is passed to NiD3DPass::CreateNewPass, which takes a
    // NiPointer<NiD3DPass>&.
    NiPointer<NiD3DPass> spPass[2];
    NiD3DVertexShader* spVertexShader;
    NiD3DPixelShader* spPixelShader[2];
    NiD3DShaderDeclaration* spDecl;
    FogProperties kFogData;
    // Nothing reads or writes these bytes, and the constructor leaves them uninitialised.
    uint32_t uiNumInstances;
    // uiMaxInstanceCount instances, the "instances" vertex constants.
    InstanceData* pInstanceDataA;
    NiColorA pfAmbientColor;
    NiPoint4 pfDiffuseDir;
    NiColorA pfDiffuseColor;
    NiPoint4 pEyeVector;
    NiPoint4 pEyePosition;

    static NiRTTI ms_RTTI;
    static NiPointer<NiTexture> spTextureAtlas;
    static inline int32_t uiMaxInstanceCount;
};

inline void BSDistantTreeShader::SetTextureAtlas(NiTexture* apTextureAtlas) {
    BSDistantTreeShader::spTextureAtlas = apTextureAtlas;
}
