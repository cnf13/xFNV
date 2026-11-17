#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

class hkDebugDisplayHandler;
class hkRotation;
class hkVector4;

// Draws the primitives of the constraint viewers through a display handler. Only what the reconstructed code uses is
// declared.
class hkpPrimitiveDrawer {
public:
	hkpPrimitiveDrawer();

	void setDisplayHandler(hkDebugDisplayHandler* displayHandler);
	// Draws an arrow from from along dir, its head opened in the plane of perpDir.
	void displayArrow(const hkVector4& startPos, const hkVector4& arrowDirection, const hkVector4& perpDirection,
		int32_t color, float scale, int32_t id, int32_t tag);
	// Draws an arc of numSegments lines about center from startPerp (scaled by radius) between the angles thetaMin
	// and thetaMax about normal; normalizes normal and startPerp in place.
	void drawSemiCircle(const hkVector4& center, hkVector4& normal, hkVector4& startPerp, float thetaMin,
		float thetaMax, float radius, int32_t numSegments, int32_t color, int32_t id, int32_t tag);
	// Havok SDK signature (hkpUtilities.lib, tied by reloc).
	void displayOrientedPoint(const hkVector4& position, const hkRotation& rot, float size, int32_t color, int32_t id,
		int32_t tag);
	void displayCone(float coneAngle, const hkVector4& startPos, const hkVector4& coneAxis, const hkVector4& perpVector,
		int32_t numSegments, int32_t color, float coneSize, int32_t id, int32_t tag);
	// Draws a square in the plane of planeNormal about startPos, with its diagonals, and an arrow along planeNormal.
	void displayPlane(const hkVector4& startPos, const hkVector4& planeNormal, const hkVector4& vectorOnPlane,
		int32_t color, float scale, int32_t id, int32_t tag);

	hkDebugDisplayHandler* m_displayHandler;
};
