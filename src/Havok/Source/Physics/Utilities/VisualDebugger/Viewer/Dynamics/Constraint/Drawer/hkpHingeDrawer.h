#pragma once

#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintDrawer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkDebugDisplayHandler;
class hkpConstraintInstance;

// Draws a hinge constraint. It adds no fields and has no vtable; Havok's release hkputilities.pdb sizes it at 0xD0,
// hkpConstraintDrawer's size. hkpConstraintViewer::draw (0xD69260) builds one for constraint type 1 and calls its
// drawConstraint, the exe's 0xDE7F20 (src: hkpConstraintDrawer::drawConstraint).
class hkpHingeDrawer : public hkpConstraintDrawer {
public:
	// Havok SDK signature (hkpUtilities.lib, tied by reloc).
	void drawConstraint(const hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler, int32_t id,
		int32_t tag);
};
