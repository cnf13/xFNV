#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPrismaticDrawer.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Prismatic/hkpPrismaticConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

#include <xmmintrin.h>

// Draws the pivots, the body frames, the line between the pivots, the shaft between its limits (a line of twice the
// drawing length centred on body B's pivot when the upper limit is the SDK's HK_REAL_MAX) and a short cross line at
// each limit.
void hkpPrismaticDrawer::drawConstraint(
    hkpConstraintInstance* constraint,
    hkDebugDisplayHandler* displayHandler,
    int32_t id,
    int32_t tag
) {
    using Atoms = hkpPrismaticConstraintData::Atoms;
    hkpPrismaticConstraintData* prismatic = static_cast<hkpPrismaticConstraintData*>(constraint->getDataRw());
    const Atoms& atoms = prismatic->m_atoms;
    m_primitiveDrawer.setDisplayHandler(displayHandler);
    hkTransform refLocalToWorld;
    hkTransform attLocalToWorld;
    updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
    m_bodyBWPivot.setTransformedPos(refLocalToWorld, atoms.m_transforms.m_transformB.getTranslation());
    m_bodyAWPivot.setTransformedPos(attLocalToWorld, atoms.m_transforms.m_transformA.getTranslation());
    drawPivots(id, tag);
    drawBodyFrames(id, tag);
    hkVector4 axis;
    axis.setRotatedDir(m_RB, atoms.m_transforms.m_transformB.getColumn(Atoms::AXIS_SHAFT));
    hkVector4 axisPerp;
    axisPerp.setRotatedDir(m_RB, atoms.m_transforms.m_transformB.getColumn(Atoms::AXIS_PERP_TO_SHAFT));
    displayHandler->displayLine(m_bodyAWPivot, m_bodyBWPivot, static_cast<int32_t>(hkColor::RED), id, tag);

    // The shaft: from the lower limit to the upper one, or the drawing length either way when it has no upper limit
    // (the double constant is HK_REAL_MAX, 3.40282e38f, widened).
    hkVector4 start;
    hkVector4 end;
    if (atoms.m_linLimit.m_max == 3.402820018375656e+38) {
        float length = getLineLengthForDraw();
        float negLength = -length;
        end.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(length), axis.m_quad), m_bodyBWPivot.m_quad);
        start.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(negLength), axis.m_quad), m_bodyBWPivot.m_quad);
    }
    else {
        float maxLimit = atoms.m_linLimit.m_max;
        float minLimit = atoms.m_linLimit.m_min;
        end.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(maxLimit), axis.m_quad), m_bodyBWPivot.m_quad);
        start.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(minLimit), axis.m_quad), m_bodyBWPivot.m_quad);
    }
    displayHandler->displayLine(start, end, hkColor::rgbFromFloats(0.0f, 0.5f, 1.0f, 1.0f), id, tag);

    // A cross line along the perpendicular axis at each limit, the lower limit's first.
    float minLimit = atoms.m_linLimit.m_min;
    float maxLimit = atoms.m_linLimit.m_max;
    {
        float length = getLineLengthForDraw();
        __m128 center = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(minLimit), axis.m_quad), m_bodyBWPivot.m_quad);
        float high = 0.5 * length;
        float low = length * -0.5;
        start.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(high), axisPerp.m_quad), center);
        end.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(low), axisPerp.m_quad), center);
        displayHandler->displayLine(start, end, static_cast<int32_t>(hkColor::WHITE), id, tag);
    }
    {
        float length = getLineLengthForDraw();
        __m128 center = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(maxLimit), axis.m_quad), m_bodyBWPivot.m_quad);
        float high = 0.5 * length;
        float low = length * -0.5;
        start.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(high), axisPerp.m_quad), center);
        end.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(low), axisPerp.m_quad), center);
        displayHandler->displayLine(start, end, static_cast<int32_t>(hkColor::WHITE), id, tag);
    }
}
