#pragma once

#include "BSShader/BSShaderProperty.h"

// The shader property of a beam projectile's strip. Only some of its members are declared.
class BeamShaderProperty : public BSShaderProperty {
public:
    // Defined in BeamProjectile.cpp.
    void SetLength(float afValue);
    // Defined in BeamProjectile.cpp.
    void SetAlpha(float afValue);

    float fLength;
    float fWidth;
    float fAspectRatio;
    float fAlpha;

    static NiRTTI ms_RTTI;
};
