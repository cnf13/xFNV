#pragma once

#include "BSShader/BSShaderProperty.h"
#include "Reconstruction.hpp"

// The shader property of a beam projectile's strip. Only the members called so far; the rest is not
// reconstructed yet.
class BeamShaderProperty : public BSShaderProperty {
public:
	// Defined in BeamProjectile.cpp (its PDB file): the exe calls it out of line.
	void SetLength(float afValue);
	// Defined in BeamProjectile.cpp (its PDB file): the exe calls it out of line.
	void SetAlpha(float afValue);

	float fLength;
	float fWidth;
	float fAspectRatio;
	float fAlpha;

	static NiRTTI ms_RTTI;
};
