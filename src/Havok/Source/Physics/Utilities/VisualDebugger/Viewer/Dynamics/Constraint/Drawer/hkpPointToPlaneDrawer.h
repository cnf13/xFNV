#pragma once

#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintDrawer.h"
#include "Reconstruction.hpp"

class hkDebugDisplayHandler;
class hkpConstraintInstance;

// Draws a point-to-plane constraint. No vtable and no members of its own (Havok SDK); hkpConstraintViewer::draw
// builds a plain hkpConstraintDrawer for it, for constraint type 14 (CONSTRAINT_TYPE_POINTTOPLANE).
class hkpPointToPlaneDrawer : public hkpConstraintDrawer {
public:
	// Havok SDK (thiscall, ret 0x10, 0xDE6D80; edx is not read).
	void drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler, int32_t id,
		int32_t tag);
};
