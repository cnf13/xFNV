#pragma once

#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintDrawer.h"

#include <stdint.h>

class hkDebugDisplayHandler;
class hkpConstraintInstance;

// Draws a prismatic constraint. It adds no fields and has no vtable, so it is the same size as hkpConstraintDrawer.
// hkpConstraintViewer::draw builds one for constraint type 6 and calls its drawConstraint.
class hkpPrismaticDrawer : public hkpConstraintDrawer {
public:
	// Havok SDK signature (hkpUtilities.lib, tied by reloc).
	void drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler, int32_t id,
		int32_t tag);
};
