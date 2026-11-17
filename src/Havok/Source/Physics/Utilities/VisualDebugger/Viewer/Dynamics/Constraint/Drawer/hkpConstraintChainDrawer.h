#pragma once

#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintDrawer.h"

#include <stdint.h>

class hkDebugDisplayHandler;
class hkpConstraintInstance;

// Draws a constraint chain. It adds no fields and has no vtable, so it is the size of hkpConstraintDrawer.
// hkpConstraintViewer::draw builds one for the chain constraint types (100 to 102) and calls its drawConstraint.
class hkpConstraintChainDrawer : public hkpConstraintDrawer {
public:
	// Havok SDK signature (hkpUtilities.lib, tied by reloc).
	void drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler, int32_t id,
		int32_t tag);
};
