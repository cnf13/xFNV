#pragma once

#include "Reconstruction.hpp"

class NiPoint3;

// An angle kept as its cosine and sine.
struct AngleSinCos {
	// Locals the exe leaves unset until assigned.
	AngleSinCos() = default;
	// The angle a1 (radians): its cosine and sine.
	AngleSinCos(float a1);

	// The sum's cosine and sine (the exe returns it through a hidden pointer).
	AngleSinCos operator+(const AngleSinCos& aSinCos) const;

	// The angle's quadrant: bit 0 flips when the cosine is negative, bits 0-1 when the sine
	// is.
	int32_t QQuadrant() const;
	// Whether this angle is at most aAngle: by quadrant, then by sine within the quadrant.
	bool operator<=(const AngleSinCos& aAngle) const;
	// The angle of this minus aAngle (thiscall, ret 8, hidden result, 0x698620).
	AngleSinCos operator-(const AngleSinCos& aSinCos) const;
	float GetAngle() const;
	// Copies aSinCos's cosine and sine; returns this. The exe folds it with the copy
	// constructor (thiscall, ret 4, 0x6985F0).
	AngleSinCos& operator=(const AngleSinCos& aSinCos);
	// The point at afRadius along this angle in the XY plane (the exe returns it through a hidden pointer).
	NiPoint3* ComputePoint3(NiPoint3* apResult, float afRadius) const;
	AngleSinCos& operator+=(const AngleSinCos& aSinCos);
	AngleSinCos& operator-=(const AngleSinCos& aSinCos);

	float fCos;
	float fSin;
};
