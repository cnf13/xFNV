#pragma once

#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintDrawer.h"
#include "Reconstruction.hpp"

class hkDebugDisplayHandler;
class hkpConstraintInstance;

// Draws a ball and socket constraint: both bodies' frames, the two pivots and a line between them. It adds no fields
// and has no vtable; the PC PDB sizes it at 0xD0, hkpConstraintDrawer's size.
class hkpBallSocketDrawer : public hkpConstraintDrawer {
public:
	void drawConstraint(const hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler, int32_t id,
		int32_t tag);
};
