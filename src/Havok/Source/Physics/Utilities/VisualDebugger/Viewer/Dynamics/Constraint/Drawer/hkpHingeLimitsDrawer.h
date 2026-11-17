#pragma once

#include "Havok/Source/Common/Visualize/Shape/hkDisplaySemiCircle.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintDrawer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkDebugDisplayHandler;
class hkpConstraintInstance;

// Draws a hinge limits constraint's angular limit. No vtable; members from the PDB. hkpConstraintViewer::draw
// (0xD69260) builds it for constraint type 18 (CONSTRAINT_TYPE_HINGE_LIMITS).
class hkpHingeLimitsDrawer : public hkpConstraintDrawer {
public:
	hkpHingeLimitsDrawer();

	// Havok SDK signature (hkpUtilities.lib, tied by reloc).
	void drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler, int32_t id,
		int32_t tag);

	hkDisplaySemiCircle m_angularLimit;
};
