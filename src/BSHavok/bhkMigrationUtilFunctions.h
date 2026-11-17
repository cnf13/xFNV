#pragma once

#include "BSHavok/bhkShape.h"
#include "Reconstruction.hpp"

class NiColor;
class NiGeometry;
class NiNode;

// Owner of the static Havok debug-geometry migration helpers, with no instance state.
// The PDB has no source path; the Havok family and the geometry helper's callees determine this location.
class bhkMigrationUtilFunctions {
public:
	// IDA name; not in the PDB. Sets up a debug geometry built for a Havok shape under
	// apNode: its material index auiMaterial and colour apColor (cdecl, ret 0, 0xC811F0). The PDB's
	// bhkUtilFunctions::PrepareDebugGeom takes the colour as const NiColor&.
	// Inferred type: apNode: NiNode* from 0xc85182 (parameter 0 of bhkShape::BuildDisplayFromhkGeometry), its members
	// used at 0xc812b0; auiMaterial: HavokMaterialType: passed to bhkShape::GetMaterialColor 0 at 0xc8123e; apColor:
	// NiColor* from 0xc85182 (parameter 2 of bhkShape::BuildDisplayFromhkGeometry), its members used at 0xc81221
	static void PrepareDebugGeom(NiGeometry* apGeom, NiNode* apNode, HavokMaterialType auiMaterial,
		const NiColor* apColor);
};
