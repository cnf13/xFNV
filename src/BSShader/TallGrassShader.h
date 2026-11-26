#pragma once

#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/TallGrassGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"

// The grass shader. Only its virtual table, its RTTI, its fields and the statics this code uses are declared.
class TallGrassShader : public BSShader {
public:
    // The vertex shader constants the grass passes share. Only the fields up to the wind magnitude are declared.
    struct ConstantGroup {
        NiPoint4 kModelViewProj;
        NiColorA kDiffuseColor;
        NiPoint4 kScaleMask;
        NiPoint4 kPointLighting;
        NiPoint2 kGrassSway;
        NiPoint2 kGrassWindMagnitude;
        // TallGrassShaderProperty::kFadeParams.
        NiPoint4 kDrawDistances;
        // Register 6: the sun's ambient color, or the shadow matrix's last row.
        NiPoint4 kAmbientColor;
        // r: BSShaderManager::fGrassDimmer.
        NiColorA kDimmer;
        NiPoint4 kShadowProjData;
    };

    TallGrassShader(NiDX9ShaderDeclaration* apShaderDecl);
    ~TallGrassShader() override;

    const NiRTTI* GetRTTI() const override;
    // Loads the fog distances, power and color into the shared fog vertex constants.
    void SetupPassConstants(uint32_t auiCurrentPass) override;
    bool Initialize() override;
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
    uint32_t SetupTransformations(
        NiGeometry* apGeometry,
        const NiSkinInstance* apSkin,
        const NiSkinPartition::Partition* apPartition,
        NiGeometryData::RendererData* apBuffData,
        const NiPropertyState* apState,
        const NiDynamicEffectState* apEffects,
        const NiTransform& aWorld,
        const NiBound& aWorldBound
    ) override;
    void SetupGeometryTextures(const NiPropertyState* apState) override;
    void SetupGeometryConstants(const NiPropertyState* apState) override;
    void SetupGeometryAlphaTesting(const NiPropertyState* apState) override {}
    void InitPass(uint32_t auiCurrentPass) override;
    void SetupPassTextures(uint32_t auiCurrentPass) override;
    void CreateConstantMaps() override;
    void ReloadShaders() override;
    void ClearPassStages() override;
    void SetCurrentPass(uint32_t aPass, bool aFirstPass) override;

protected:
    virtual void LoadShaders();
    virtual bool LoadStagesAndPasses();

    // Sets up the pass stages, render states and shaders.
    void PresetStages();

public:
    // Loads the property's non-shadow point lights into the light constants, in the space of d3dInvWorld.
    void UpdatePointLights(
        TallGrassShaderProperty* apTGSProp,
        D3DXMATRIX& d3dInvWorld,
        float afWorldScale,
        BSShaderManager::eBSSMPassEnum apPass
    );
    // Loads the shadow light's world-view-projection (aWorld's, or identity for skinned geometry) and its color and
    // position into the constants.
    void UpdateShadowLights(const NiSkinInstance* apSkin, const NiTransform& aWorld);
    // Loads apTgsProp's grass instances of apGeometry into the instance constants.
    void SetupConstants(NiGeometry* apGeometry, TallGrassShaderProperty* apTgsProp);

    static void SetWindRanges(float afWindMagMin, float afWindMagMax);
    // Clamps afValue to [0, 1] (NaN gives 0) and stores it as the grass's canopy shadow multiplier.
    static void SetProjShadowMult(float afValue);
    static BSShader* CreateShader();

    NiPointer<NiD3DPass> spPass[44];
    float fWindTimer;
    // The current pass and whether it is the first, set by SetCurrentPass.
    BSShaderManager::eBSSMPassEnum ePass;
    bool bFirstPass;
    NiPointer<NiD3DVertexShader> spVertexShaders[24];
    // There are 7 pixel shaders; the constructor builds all 7.
    NiPointer<NiD3DPixelShader> spPixelShaders[7];
    // iInstanceDataCount entries, allocated by the constructor; SetupConstants copies the grass group's
    // instances into it.
    TallGrassGroup::GrassInstanceData* pGrassInstanceData;
    NiShaderConstantMapEntry* pGrassInstanceDataEntry;
    // 80 below shader level 2, else 228.
    uint16_t iInstanceDataCount;
    // These are declared NiPoint4 because the bodies write them through x/y/z/w.
    NiPoint4 pfPointLightColor;
    NiPoint4 pfPointLightData;
    NiPoint4 pfAlphaTestRef;

    static NiRTTI ms_RTTI;
    // The wind magnitude range the scene's wind is mapped onto.
    static inline float fWindMagMin;
    static inline float fWindMagMax;
    static ConstantGroup pGroupedVertexConstants;
    // The canopy shadow's strength on the grass (kShadowProjData.z); BSTreeManager's fCanopyShadowGrassMult setting
    // stores it.
    static inline float fProjShadowMult = 1.0f;
};

// NaN is sent to 0.
inline void TallGrassShader::SetProjShadowMult(float afValue) {
    float fValue;
    if (!(afValue >= 0.0)) {
        fValue = 0.0f;
    }
    else if (afValue > 1.0) {
        fValue = 1.0f;
    }
    else {
        fValue = afValue;
    }
    TallGrassShader::fProjShadowMult = fValue;
}
