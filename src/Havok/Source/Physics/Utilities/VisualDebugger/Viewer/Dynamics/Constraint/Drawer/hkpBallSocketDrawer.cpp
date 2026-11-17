#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpBallSocketDrawer.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/BallAndSocket/hkpBallAndSocketConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

void hkpBallSocketDrawer::drawConstraint(
    const hkpConstraintInstance* constraint,
    hkDebugDisplayHandler* displayHandler,
    int32_t id,
    int32_t tag
) {
    const hkpBallAndSocketConstraintData* bsData =
        static_cast<const hkpBallAndSocketConstraintData*>(constraint->getData());
    m_primitiveDrawer.setDisplayHandler(displayHandler);
    hkTransform refLocalToWorld;
    hkTransform attLocalToWorld;
    updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
    m_bodyBWPivot.setTransformedPos(refLocalToWorld, bsData->m_atoms.m_pivots.m_translationB);
    m_bodyAWPivot.setTransformedPos(attLocalToWorld, bsData->m_atoms.m_pivots.m_translationA);
    drawPivots(id, tag);
    drawBodyFrames(id, tag);
    displayHandler->displayLine(m_bodyAWPivot, m_bodyBWPivot, static_cast<int32_t>(hkColor::RED), id, tag);
}
