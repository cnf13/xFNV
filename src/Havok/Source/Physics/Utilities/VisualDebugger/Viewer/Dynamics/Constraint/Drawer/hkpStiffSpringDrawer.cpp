#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpStiffSpringDrawer.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/StiffSpring/hkpStiffSpringConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

#include <xmmintrin.h>

// Draws the pivots, the body frames, the spring's rest length from body A's pivot towards body B's and the rest of
// the way to body B's pivot in red.
void hkpStiffSpringDrawer::drawConstraint(
    hkpConstraintInstance* constraint,
    hkDebugDisplayHandler* displayHandler,
    int32_t id,
    int32_t tag
) {
    hkpStiffSpringConstraintData* spring = static_cast<hkpStiffSpringConstraintData*>(constraint->getDataRw());
    m_primitiveDrawer.setDisplayHandler(displayHandler);
    hkTransform refLocalToWorld;
    hkTransform attLocalToWorld;
    updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
    m_bodyBWPivot.setTransformedPos(refLocalToWorld, spring->m_atoms.m_pivots.m_translationB);
    m_bodyAWPivot.setTransformedPos(attLocalToWorld, spring->m_atoms.m_pivots.m_translationA);
    drawPivots(id, tag);
    drawBodyFrames(id, tag);
    float restLength = spring->m_atoms.m_spring.m_length;
    hkVector4 dir;
    dir.m_quad = _mm_sub_ps(m_bodyBWPivot.m_quad, m_bodyAWPivot.m_quad);
    dir.normalize3();
    hkVector4 restEnd;
    restEnd.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(restLength), dir.m_quad), m_bodyAWPivot.m_quad);
    displayHandler->displayLine(m_bodyAWPivot, restEnd, hkColor::rgbFromFloats(0.0f, 0.5f, 1.0f, 1.0f), id, tag);
    displayHandler->displayLine(restEnd, m_bodyBWPivot, static_cast<int32_t>(hkColor::RED), id, tag);
}
