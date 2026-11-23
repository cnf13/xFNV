#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class BGSDebris;

// The data of an effect shader form (the DATA chunk): the fill and edge texture timing, the particle emitter and
// the addon model settings.
struct EffectShaderData {
    EffectShaderData();

    // Sets every field to its default.
    void Initialize();
    // Byte-swaps every 32-bit field but the addon pointer.
    void Endian();

    uint8_t cFlags;
    D3DBLEND eTextureBlendModeSource;
    D3DBLENDOP eTextureBlendOperation;
    D3DCMPFUNC eTextureZTestFunction;
    uint32_t iFillColor;
    float fFillAlphaFadeInTime;
    float fFillAlphaFullTime;
    float fFillAlphaFadeOutTime;
    float fFillAlphaPersistentPercent;
    float fFillAlphaPulseAmplitude;
    float fFillAlphaPulseFrequency;
    float fFillTextureUAnimSpeed;
    float fFillTextureVAnimSpeed;
    float fEdgeExponentValue;
    uint32_t iEdgeColor;
    float fEdgeAlphaFadeInTime;
    float fEdgeAlphaFullTime;
    float fEdgeAlphaFadeOutTime;
    float fEdgeAlphaPersistentPercent;
    float fEdgeAlphaPulseAmplitude;
    float fEdgeAlphaPulseFrequency;
    float fFillAlphaFullPercent;
    float fEdgeAlphaFullPercent;
    D3DBLEND eTextureBlendModeDest;
    D3DBLEND eParticleBlendModeSource;
    D3DBLENDOP eParticleBlendOperation;
    D3DCMPFUNC eParticleZTestFunction;
    D3DBLEND eParticleBlendModeDest;
    float fParticleFadeInTime;
    float fParticleFullTime;
    float fParticleFadeOutTime;
    float fParticleFull;
    float fParticlePersistent;
    float fParticleLifetime;
    float fParticleLifeVar;
    float fParticleNormalSpeed;
    float fParticleNormalAcc;
    NiPoint3 ParticleVelocity;
    NiPoint3 ParticleAcceleration;
    float fParticleScale1;
    float fParticleScale2;
    float fParticleScale1Time;
    float fParticleScale2Time;
    uint32_t iParticleColor1;
    uint32_t iParticleColor2;
    uint32_t iParticleColor3;
    float fParticleColor1Alpha;
    float fParticleColor2Alpha;
    float fParticleColor3Alpha;
    float fParticleColor1Time;
    float fParticleColor2Time;
    float fParticleColor3Time;
    float fParticleNormalSpeedVar;
    float fParticleRotate;
    float fParticleRotateVar;
    float fParticleRotateSpeed;
    float fParticleRotateSpeedVar;
    BGSDebris* pAddonModels;
    float fAlphaTestStartTime;
    float fAlphaTestEndTime;
    float fAlphaTestStartValue;
    float fAlphaTestEndValue;
    float fAlphaEdgeWidth;
    uint32_t iAlphaEdgeColor;
    float fExplosionSpeed;
    uint32_t iParticleTextureCountX;
    uint32_t iParticleTextureCountY;
    float fAddOnFadeInTime;
    float fAddOnFadeOutTime;
    float fAddOnScaleStart;
    float fAddOnScaleEnd;
    float fAddOnScaleInTime;
    float fAddOnScaleOutTime;
};
