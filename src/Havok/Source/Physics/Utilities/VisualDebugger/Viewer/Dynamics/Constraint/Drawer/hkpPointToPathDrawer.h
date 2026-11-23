#pragma once

#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintDrawer.h"

#include <stdint.h>

class hkDebugDisplayHandler;
class hkpConstraintInstance;

// Draws a point to path constraint. It adds no fields and has no vtable. hkpConstraintViewer::draw builds one for
// constraint type 3 and calls its drawConstraint.
class hkpPointToPathDrawer : public hkpConstraintDrawer {
public:
    // Havok SDK signature (hkpUtilities.lib, tied by reloc).
    void
    drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler, int32_t id, int32_t tag);
};
