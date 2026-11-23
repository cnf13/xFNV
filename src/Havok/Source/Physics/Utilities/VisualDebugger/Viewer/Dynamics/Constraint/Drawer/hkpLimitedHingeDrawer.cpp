#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpLimitedHingeDrawer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

#include <xmmintrin.h>

// The vector code is written as SSE lane operations with intrinsics, keeping each operation's operand order. The line
// lengths and the arc radius are floats; the factors 0.75, -0.75 and 0.5 and the limit range's bound 1e14 are doubles.
// The lines' start points are copies of the pivots.
void hkpLimitedHingeDrawer::drawConstraint(
    hkpConstraintInstance* constraint,
    hkDebugDisplayHandler* displayHandler,
    int32_t id,
    int32_t tag
) {
    const hkpLimitedHingeConstraintData* limitedHinge = static_cast<const hkpLimitedHingeConstraintData*>(
        constraint->m_data
    );
    m_primitiveDrawer.setDisplayHandler(displayHandler);

    const hkTransform& baseA = limitedHinge->m_atoms.m_transforms.m_transformA;
    const hkTransform& baseB = limitedHinge->m_atoms.m_transforms.m_transformB;
    alignas(16) hkTransform refLocalToWorld;
    alignas(16) hkTransform attLocalToWorld;
    updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
    m_bodyBWPivot.setTransformedPos(refLocalToWorld, baseB.m_translation);
    m_bodyAWPivot.setTransformedPos(attLocalToWorld, baseA.m_translation);
    drawBodyFrames(id, tag);
    drawPivots(id, tag);

    alignas(16) hkVector4 axisInWorld;
    axisInWorld.setRotatedDir(m_RB, baseB.m_rotation.m_col0);
    alignas(16) hkVector4 perpInWorld;
    perpInWorld.setRotatedDir(m_RB, baseB.m_rotation.m_col2);

    // The error line between the pivots.
    displayHandler->displayLine(m_bodyAWPivot, m_bodyBWPivot, static_cast<int32_t>(hkColor::RED), id, tag);

    // Body B's hinge axis through the B pivot.
    float lineLength = getLineLengthForDraw();
    const float axisPlus = static_cast<float>(0.75 * lineLength);
    const float axisMinus = static_cast<float>(lineLength * -0.75);
    alignas(16) hkVector4 axisEnd;
    axisEnd.setAddMul4(m_bodyBWPivot, axisInWorld, axisPlus);
    alignas(16) hkVector4 axisStart;
    axisStart.setAddMul4(m_bodyBWPivot, axisInWorld, axisMinus);
    displayHandler->displayLine(axisStart, axisEnd, hkColor::rgbFromFloats(0.0f, 0.5f, 1.0f, 1.0f), id, tag);

    // Body A's hinge axis through the A pivot.
    alignas(16) hkVector4 attAxisInWorld;
    attAxisInWorld.setRotatedDir(m_RA, baseA.m_rotation.m_col0);
    lineLength = getLineLengthForDraw();
    const float attPlus = static_cast<float>(0.75 * lineLength);
    const float attMinus = static_cast<float>(lineLength * -0.75);
    alignas(16) hkVector4 attAxisEnd;
    attAxisEnd.setAddMul4(m_bodyAWPivot, attAxisInWorld, attPlus);
    alignas(16) hkVector4 attAxisStart;
    attAxisStart.setAddMul4(m_bodyAWPivot, attAxisInWorld, attMinus);
    displayHandler->displayLine(attAxisStart, attAxisEnd, hkColor::rgbFromFloats(0.0f, 0.5f, 1.0f, 1.0f), id, tag);

    // The angular limit's arc, where the limit is on and its range below 1e14.
    if (limitedHinge->m_atoms.m_angLimit.m_isEnabled) {
        const float maxAngle = limitedHinge->m_atoms.m_angLimit.m_maxAngle;
        const float minAngle = limitedHinge->m_atoms.m_angLimit.m_minAngle;
        if (maxAngle - minAngle < 100000000376832.0) {
            const float radius = m_arcRadius * m_scale;
            m_angularLimit
                .setParameters(radius, minAngle, maxAngle, m_numArcSegments, m_bodyBWPivot, axisInWorld, perpInWorld);

            alignas(16) hkArray<hkDisplayGeometry*> geometry;
            geometry.setSize(1);
            geometry.m_data[0] = &m_angularLimit;
            displayHandler->displayGeometry(geometry, -1, id, tag);
        }
    }

    // The limit's start direction from the B pivot.
    lineLength = getLineLengthForDraw();
    const float perpLength = static_cast<float>(lineLength * 0.5);
    alignas(16) hkVector4 perpStart;
    perpStart.m_quad = m_bodyBWPivot.m_quad;
    alignas(16) hkVector4 perpEnd;
    perpEnd.setAddMul4(perpStart, perpInWorld, perpLength);
    displayHandler->displayLine(perpStart, perpEnd, static_cast<int32_t>(hkColor::YELLOW), id, tag);

    // Body A's direction from the A pivot, normalized.
    alignas(16) hkVector4 attInWorld;
    attInWorld.setRotatedDir(m_RA, baseA.m_rotation.m_col2);
    lineLength = getLineLengthForDraw();
    const float attLength = static_cast<float>(lineLength * 0.75);
    attInWorld.normalize3();
    alignas(16) hkVector4 attStart;
    attStart.m_quad = m_bodyAWPivot.m_quad;
    alignas(16) hkVector4 attEnd;
    attEnd.setAddMul4(attStart, attInWorld, attLength);
    displayHandler->displayLine(attStart, attEnd, static_cast<int32_t>(hkColor::YELLOW), id, tag);
}
