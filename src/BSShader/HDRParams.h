#pragma once

// The HDR tone-mapping parameters of an image space (eye adaptation, blur passes, skin dimmer, emissive multiplier,
// target luminance, clamps and the sky's luminance ramp). The block is read as a float array
// (ImageSpaceEffectHDR::pfData), so every member is a float.
class HDRParams {
public:
    float fEyeAdaptSpeed;
    float fNumBlurPasses;
    float fSkinDimmer;
    float fEmissiveHDRMult;
    float fTargetLUM;
    float fUpperLUMClamp;
    float fBrightScale;
    float fBrightClamp;
    // The sky's luminance ramp: SkyShader::pLUMRamp[0], or pLUMRamp[1] during a sky transition.
    float fLUMRampNoTex;
    float fLUMRampMin;
    float fLUMRampMax;
};
