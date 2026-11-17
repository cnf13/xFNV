#pragma once

// The HDR tone-mapping parameters of an image space (eye adaptation, blur passes, emissive multiplier, target luminance
// and clamps). The block is read as a float array (ImageSpaceEffectHDR::pfData), so every member is a float.
class HDRParams {
public:
	float fEyeAdaptSpeed;
	float fNumBlurPasses;
	float unk8;
	float fEmissiveHDRMult;
	float fTargetLUM;
	float fUpperLUMClamp;
	float fBrightScale;
	float fBrightClamp;
	float unk20;
	float unk24;
	float unk28;
};
