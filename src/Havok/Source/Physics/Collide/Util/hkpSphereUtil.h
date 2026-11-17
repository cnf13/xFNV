#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

struct hkpShapeRayCastInput;
struct hkpShapeRayCastOutput;

// The ray cast against a sphere of a given radius that the sphere and capsule shapes share (the SDK's inline
// castRayUtil). Statics only: sizeof 1 in Havok's release hkpcollide.pdb; the game PDB has castRayUtil.
class hkpSphereUtil {
public:
	// Casts the ray against a sphere of radius aRadius at the origin; on a hit, writes the hit fraction, the normal
	// and an invalid key at the output's key index (cdecl, ret 0, the hkBool through a hidden pointer, 0xCD85C0).
	static hkBool castRayUtil(float radius, const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results);
};
