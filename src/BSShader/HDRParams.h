#pragma once

#include "Reconstruction.hpp"

// The HDR tone-mapping parameters of an image space (eye adaptation, blur passes, emissive multiplier, target
// luminance and clamps). IDA's name; the PDB has no such class. The exe's block at 0x11ADD0C is read as a float
// array (ImageSpaceEffectHDR::pfData, 0xBAD500 and 0x5C9B50..0x5C9CD0), so every member is a float.
class HDRParams {
public:
	float fEyeAdaptSpeed;
	float fNumBlurPasses;
	// IDA: float Unk08.
	float unk8;
	float fEmissiveHDRMult;
	float fTargetLUM;
	float fUpperLUMClamp;
	float fBrightScale;
	float fBrightClamp;
	// IDA: float Unk20.
	float unk20;
	// IDA: float Unk24.
	float unk24;
	// IDA: float Unk28.
	float unk28;
};
