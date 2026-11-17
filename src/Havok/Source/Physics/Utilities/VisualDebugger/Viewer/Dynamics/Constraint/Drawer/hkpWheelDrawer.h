#pragma once

#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintDrawer.h"
#include "Reconstruction.hpp"

class hkDebugDisplayHandler;
class hkpConstraintInstance;

// Draws a wheel constraint. No vtable and no members of its own (Havok SDK); hkpConstraintViewer::draw
// builds a plain hkpConstraintDrawer for it, for constraint type 9 (CONSTRAINT_TYPE_WHEEL).
class hkpWheelDrawer : public hkpConstraintDrawer {
public:
	// Havok SDK.
	void drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler, int32_t id,
		int32_t tag);
};
