#pragma once

#include <cstdint>

// The shader parameters of a water form.
struct WaterShaderData {
    void Initialize();
    // Swaps every field but the blend byte, read from a big-endian file.
    void Endian();

    float fWindVelocity;
    float fWindDirection;
    float fWaveAmplitude;
    float fWaveFrequency;
    float fSunPower;
    float fReflectionAmount;
    float fFresnelAmount;
    float fWaveSpeed;
    float fFogDistNear;
    float fFogDistFar;
    uint32_t iShallowWaterColor;
    uint32_t iDeepWaterColor;
    uint32_t iReflectionWaterColor;
    uint8_t cBlend;
    float fRainForce;
    float fRainVelocity;
    float fRainFalloff;
    float fRainDampener;
    float fRainSize;
    float fDisplacementForce;
    float fDisplacementVelocity;
    float fDisplacementFalloff;
    float fDisplacementDampener;
    float fDisplacementSize;
    float fNoiseScale;
    float fNoiseWindDirection0;
    float fNoiseWindDirection1;
    float fNoiseWindDirection2;
    float fNoiseWindSpeed0;
    float fNoiseWindSpeed1;
    float fNoiseWindSpeed2;
    float fDepthFalloffStart;
    float fDepthFalloffEnd;
    float fFogAmount;
    float fUVScale;
    float fUnderwaterFogAmount;
    float fUnderwaterFogDistNear;
    float fUnderwaterFogDistFar;
    float fRefractivityAmount;
    float fShininess;
    float fHDRMultiplier;
    float fLightRadius;
    float fLightBrightness;
    float fHeightUVScale0;
    float fHeightUVScale1;
    float fHeightUVScale2;
    float fAmplitude0;
    float fAmplitude1;
    float fAmplitude2;
};
