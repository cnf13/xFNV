#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

struct hkpShapeRayCastInput;
struct hkpShapeRayCastOutput;

// The ray cast against a sphere of a given radius that the sphere and capsule shapes share. Statics only.
class hkpSphereUtil {
public:
	// Casts the ray against a sphere of radius aRadius at the origin; on a hit, writes the hit fraction, the normal and
	// an invalid key at the output's key index.
	static hkBool castRayUtil(float radius, const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results);
};
