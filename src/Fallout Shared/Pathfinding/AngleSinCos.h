#pragma once

#include <cstdint>

class NiPoint3;

// An angle kept as its cosine and sine.
struct AngleSinCos {
    // Default construction leaves the members unset until assigned.
    AngleSinCos() = default;
    // The angle afAngle (radians): its cosine and sine.
    AngleSinCos(float afAngle);

    // The sum's cosine and sine.
    AngleSinCos operator+(const AngleSinCos& aSinCos) const;

    // The angle's quadrant: bit 0 flips when the cosine is negative, bits 0-1 when the sine
    // is.
    int32_t QQuadrant() const;
    // Whether this angle is at most aAngle: by quadrant, then by sine within the quadrant.
    bool operator<=(const AngleSinCos& aAngle) const;
    // The angle of this minus aAngle.
    AngleSinCos operator-(const AngleSinCos& aSinCos) const;
    float GetAngle() const;
    // Copies aSinCos's cosine and sine; returns this.
    AngleSinCos& operator=(const AngleSinCos& aSinCos);
    // The point at afRadius along this angle in the XY plane.
    NiPoint3 ComputePoint3(float afRadius) const;
    AngleSinCos& operator+=(const AngleSinCos& aSinCos);
    AngleSinCos& operator-=(const AngleSinCos& aSinCos);

    float fCos;
    float fSin;
};
