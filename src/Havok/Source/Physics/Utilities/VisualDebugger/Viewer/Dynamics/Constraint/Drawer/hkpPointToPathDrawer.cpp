#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPointToPathDrawer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpParametricCurve.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPrimitiveDrawer.h"

// Draws the common frames and pivots, then the path as light blue lines between its consecutive points, each taken
// from the path's space (the reference body's frame times the constraint's second constraint space) to world space.
void hkpPointToPathDrawer::drawConstraint(
    hkpConstraintInstance* constraint,
    hkDebugDisplayHandler* displayHandler,
    int32_t id,
    int32_t tag
) {
    hkpPointToPathConstraintData* pData = static_cast<hkpPointToPathConstraintData*>(constraint->getDataRw());
    m_primitiveDrawer.setDisplayHandler(displayHandler);
    hkTransform refLocalToWorld;
    hkTransform attLocalToWorld;
    updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
    m_bodyBWPivot = refLocalToWorld.m_translation;
    pData->calcPivot(constraint->m_entities[0]->m_motion.m_motionState.m_transform, m_bodyAWPivot);
    drawBodyFrames(id, tag);
    drawPivots(id, tag);
    if (pData->m_path) {
        hkTransform pathToWorld;
        pathToWorld.setMul(refLocalToWorld, pData->m_transform_OS_KS[1]);
        hkpParametricCurve* pPath = pData->m_path;
        if (pPath) {
            alignas(16) hkArray<hkVector4> pathPoints;
            pPath->getPointsToDraw(pathPoints);
            for (int32_t i = 1; i < pathPoints.getSize(); i++) {
                // Copies each point to a temporary and transforms that copy.
                hkVector4 point;
                hkVector4 start;
                hkVector4 end;
                point = pathPoints.m_data[i];
                start.setTransformedPos(pathToWorld, point);
                point = pathPoints.m_data[i - 1];
                end.setTransformedPos(pathToWorld, point);
                displayHandler->displayLine(start, end, hkColor::rgbFromFloats(0.0f, 0.25f, 1.0f, 1.0f), id, tag);
            }
        }
    }
}
