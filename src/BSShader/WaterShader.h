#pragma once

#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

#include <stdint.h>

class NiD3DPixelShader;
class NiD3DVertexShader;

using WaterShaderEnableArray = uint8_t[8];
using WaterShaderFlagsArray = uint32_t[74];
// The vertex table has 3 entries (the gap to pPixelConstantMapEntry); the pixel table is indexed to 5.
using WaterShaderVertexMapArray = NiShaderConstantMapEntry* [3];
using WaterShaderPixelMapArray = NiShaderConstantMapEntry* [6];
using WaterShaderVertexConstantArray = NiColorA[11];
using WaterShaderPixelConstantArray = NiColorA[31];

class WaterShader : public BSShader {
public:
    // Light positions and colours; no code reads them.
    using LightArray = NiColorA[8];

    WaterShader();
    ~WaterShader() override;

    const NiRTTI* GetRTTI() const override;
    bool Initialize() override;
    void SetupGeometryTextures(const NiPropertyState* apState) override;
    void SetupGeometryConstants(const NiPropertyState* apState) override;
    void PostGeometry(const NiPropertyState* apState) override;
    void InitPass(uint32_t auiCurrentPass) override;
    void SetupPassConstants(uint32_t auiCurrentPass) override;
    void CreateConstantMaps() override;
    void ReloadShaders() override;
    void ClearPassStages() override;

    bool LoadStagesAndPasses();
    void LoadShaders();
    void SetWaterShaderReflectionTexture(NiTexture* ptexture);
    void SetWaterShaderNormalMapTexture(NiTexture* ptexture);
    void SetWaterShaderDetailTexture(NiTexture* ptexture);
    void SetWaterShaderNoiseMap(NiTexture* ptexture);

    static WaterShader* GetInstance();
    static void EnableConstantsForPass(uint32_t auiPassEnum, bool abAlphaPass);
    static BSShader* CreateShader();

    NiPointer<NiD3DPass> spPass[74];
    NiPointer<NiD3DVertexShader> spWaterVertexShader;
    NiPointer<NiD3DVertexShader> spWaterVertexShader_Vc;
    NiPointer<NiD3DVertexShader> spWaterVertexShader_Lighting;
    NiPointer<NiD3DPixelShader> spWaterPixelShader[38];
    NiPointer<NiRefObject> pNormalMapTexture;
    NiPointer<NiRefObject> pReflectionTexture;
    // The constructor stores 0 and the destructor releases it like the NiPointers around it.
    NiPointer<NiRefObject> pRefractionTexture;
    NiPointer<NiRefObject> pInteriorReflectionTexture;
    NiPointer<NiRefObject> pDetailTexture;
    // SetWaterShaderNoiseMap assigns it through the NiPointer raw-pointer assignment.
    NiPointer<NiRefObject> pNoiseMap;

    static inline WaterShader* pInstance;
    // The shader constants the water update fills: the sun direction (w the clamped intensity) and color, and
    // whether the camera is in an interior or under water.
    static inline NiPoint4 pSunDir{};
    static inline NiColorA pSunColor{};
    static inline bool bInterior;
    static inline bool bUnderWater;
    static NiRTTI ms_RTTI;
    // The per-pass constant tables (sizes from the gaps to the next globals; the pixel map table is sized to the
    // highest index the water update uses).
    static inline WaterShaderEnableArray bPSConstantEnable{};
    static inline WaterShaderEnableArray bVSConstantEnable{};
    static inline WaterShaderFlagsArray ulPixelConstantFlags{};
    static inline WaterShaderFlagsArray ulVertexConstantFlags{};
    static inline WaterShaderVertexMapArray pVertexConstantMapEntry;
    static inline WaterShaderPixelMapArray pPixelConstantMapEntry;
    // The shader constant values Initialize clears to black: 11 vertex and 31 pixel constants.
    static inline WaterShaderVertexConstantArray VertexConstantData;
    static inline WaterShaderPixelConstantArray PixelConstantData;
    static inline LightArray pLightList;
    static inline LightArray pLightColorList;
    static inline bool bDepth;
    static inline bool bNoRefractions;
    static inline bool bNoReflections;
    static inline NiColorA kShallowColor{};
    static inline NiColorA kDeepColor{};
    static inline NiColorA kReflectionColor{};
    // Shader constants after pReflectionColor: only Cmd_ModWaterShader uses them (the reflectivity, opacity and
    // distortion amounts are pVarAmounts' g, b and a; the fog amount pFogColor's a).
    static inline NiColorA pVarAmounts{};
    static inline NiColorA pFogParam{};
    static inline NiColorA pFogColor{};
    static inline NiColorA pFresnelRI{};
    static inline NiColorA fTexScale{};
    static inline NiColorA pBlendRadius{};
    static inline NiColorA pDepthFalloff{};
    static inline NiVector4 kNormalOffset{};
};
