#pragma once

#include "Havok/Source/Common/Visualize/Shape/hkDisplaySemiCircle.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintDrawer.h"

#include <stdint.h>

class hkDebugDisplayHandler;
class hkpConstraintInstance;

// Draws a limited hinge constraint's angular limit. No vtable. hkpConstraintViewer::draw builds it for constraint type
// 2 (CONSTRAINT_TYPE_LIMITEDHINGE).
class hkpLimitedHingeDrawer : public hkpConstraintDrawer {
public:
    hkpLimitedHingeDrawer();

    // Havok SDK signature (hkpUtilities.lib, tied by reloc).
    void
    drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler, int32_t id, int32_t tag);

    hkDisplaySemiCircle m_angularLimit;
};
