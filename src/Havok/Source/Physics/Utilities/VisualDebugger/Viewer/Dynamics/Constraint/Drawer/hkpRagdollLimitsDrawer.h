#pragma once

#include "Havok/Source/Common/Visualize/Shape/hkDisplayCone.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayPlane.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintDrawer.h"
#include "Reconstruction.hpp"

class hkDebugDisplayHandler;
class hkpConstraintInstance;

// Draws a ragdoll limits constraint: both twist axes, the twist cone, the plane and the two plane cones. No vtable;
// members from the PDB.
class hkpRagdollLimitsDrawer : public hkpConstraintDrawer {
public:
	hkpRagdollLimitsDrawer();
	~hkpRagdollLimitsDrawer();

	void drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler, int32_t id,
		int32_t tag);

	hkDisplayCone m_twistCone;
	hkDisplayCone m_planeCone1;
	hkDisplayCone m_planeCone2;
	hkDisplayPlane m_plane;
};
