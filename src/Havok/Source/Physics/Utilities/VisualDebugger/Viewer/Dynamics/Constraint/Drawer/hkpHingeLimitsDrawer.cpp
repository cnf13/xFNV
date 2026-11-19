#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpHingeLimitsDrawer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplaySemiCircle.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/HingeLimits/hkpHingeLimitsData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintDrawer.h"

#include <xmmintrin.h>

hkpHingeLimitsDrawer::hkpHingeLimitsDrawer() : hkpConstraintDrawer(), m_angularLimit() {}

// The line lengths and the arc radius are floats; the factors 0.75, -0.75 and 0.5 are doubles. The lines' start points
// are copies of the pivots.
void hkpHingeLimitsDrawer::drawConstraint(
    hkpConstraintInstance* constraint,
    hkDebugDisplayHandler* displayHandler,
    int32_t id,
    int32_t tag
) {
    const hkpHingeLimitsData* hingeLimits = static_cast<const hkpHingeLimitsData*>(constraint->m_data);
    m_primitiveDrawer.setDisplayHandler(displayHandler);

    alignas(16) hkTransform refLocalToWorld;
    alignas(16) hkTransform attLocalToWorld;
    updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
    m_bodyBWPivot.setTransformedPos(refLocalToWorld, hkVector4Zero);
    m_bodyAWPivot.setTransformedPos(attLocalToWorld, hkVector4Zero);
    drawBodyFrames(id, tag);
    drawPivots(id, tag);

    const hkRotation& baseA = hingeLimits->m_atoms.m_rotations.m_rotationA;
    const hkRotation& baseB = hingeLimits->m_atoms.m_rotations.m_rotationB;
    alignas(16) hkVector4 axisInWorld;
    axisInWorld.setRotatedDir(m_RB, baseB.m_col0);
    alignas(16) hkVector4 perpInWorld;
    perpInWorld.setRotatedDir(m_RB, baseB.m_col2);

    // The error line between the pivots.
    displayHandler->displayLine(m_bodyAWPivot, m_bodyBWPivot, static_cast<int32_t>(hkColor::RED), id, tag);

    // The hinge axis through the B pivot.
    float lineLength = m_scale * m_lineLength;
    const float axisPlus = static_cast<float>(0.75 * lineLength);
    const float axisMinus = static_cast<float>(lineLength * -0.75);
    alignas(16) hkVector4 axisEnd;
    axisEnd.setAddMul4(m_bodyBWPivot, axisInWorld, hkSimdReal(axisPlus));
    alignas(16) hkVector4 axisStart;
    axisStart.setAddMul4(m_bodyBWPivot, axisInWorld, hkSimdReal(axisMinus));
    displayHandler->displayLine(axisStart, axisEnd, hkColor::rgbFromFloats(0.0f, 0.5f, 1.0f, 1.0f), id, tag);

    // The angular limit's arc.
    const float radius = m_arcRadius * m_scale;
    m_angularLimit.setParameters(
        radius, hingeLimits->m_atoms.m_angLimit.m_minAngle, hingeLimits->m_atoms.m_angLimit.m_maxAngle,
        m_numArcSegments, m_bodyBWPivot, axisInWorld, perpInWorld
    );

    alignas(16) hkArray<hkDisplayGeometry*> geometry;
    geometry.setSize(1);
    geometry.m_data[0] = &m_angularLimit;
    displayHandler->displayGeometry(geometry, -1, id, tag);

    // The limit's start direction from the B pivot.
    lineLength = m_scale * m_lineLength;
    const float perpLength = static_cast<float>(lineLength * 0.75);
    alignas(16) hkVector4 perpStart;
    perpStart.m_quad = m_bodyBWPivot.m_quad;
    alignas(16) hkVector4 perpEnd;
    perpEnd.setAddMul4(perpStart, perpInWorld, hkSimdReal(perpLength));
    displayHandler->displayLine(perpStart, perpEnd, static_cast<int32_t>(hkColor::YELLOW), id, tag);

    // Body A's direction from the A pivot, normalized.
    alignas(16) hkVector4 attInWorld;
    attInWorld.setRotatedDir(m_RA, baseA.m_col2);
    lineLength = m_scale * m_lineLength;
    const float attLength = static_cast<float>(lineLength * 0.5);
    attInWorld.normalize3();
    alignas(16) hkVector4 attStart;
    attStart.m_quad = m_bodyAWPivot.m_quad;
    alignas(16) hkVector4 attEnd;
    attEnd.setAddMul4(attStart, attInWorld, hkSimdReal(attLength));
    displayHandler->displayLine(attStart, attEnd, static_cast<int32_t>(hkColor::YELLOW), id, tag);
}
