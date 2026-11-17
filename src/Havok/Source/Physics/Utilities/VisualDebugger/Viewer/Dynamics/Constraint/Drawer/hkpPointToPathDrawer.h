#pragma once

#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintDrawer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkDebugDisplayHandler;
class hkpConstraintInstance;

// Draws a point to path constraint. It adds no fields and has no vtable; Havok's release hkputilities.pdb sizes it at
// 0xD0, hkpConstraintDrawer's size. hkpConstraintViewer::draw (0xD69260) builds one for constraint type 3 and calls
// its drawConstraint, the exe's 0xDE7020 (src: hkpConstraintDrawer::drawConstraint).
class hkpPointToPathDrawer : public hkpConstraintDrawer {
public:
	// Havok SDK signature (hkpUtilities.lib, tied by reloc).
	void drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler, int32_t id,
		int32_t tag);
};
