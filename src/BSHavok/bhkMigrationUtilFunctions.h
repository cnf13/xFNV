#pragma once

#include "BSHavok/bhkShape.h"

class NiColor;
class NiGeometry;
class NiNode;

// Owner of the static Havok debug-geometry migration helpers, with no instance state.
class bhkMigrationUtilFunctions {
public:
    // Sets up a debug geometry built for a Havok shape under apNode: its material index auiMaterial and colour apColor.
    static void
    PrepareDebugGeom(NiGeometry* apGeom, NiNode* apNode, HavokMaterialType auiMaterial, const NiColor* apColor);
};
