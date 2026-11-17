#pragma once

#include "Reconstruction.hpp"

class hkpConstraintData;

// Clones constraint data that holds motors (deepClone, cloneIfCanHaveMotors). Statics only: sizeof 1 in Havok's
// release hkpdynamics.pdb; the game PDB has both functions.
class hkpConstraintDataCloningUtil {
public:
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
	static hkpConstraintData* deepClone(const hkpConstraintData* data);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
	static hkpConstraintData* cloneIfCanHaveMotors(const hkpConstraintData* data);
};
