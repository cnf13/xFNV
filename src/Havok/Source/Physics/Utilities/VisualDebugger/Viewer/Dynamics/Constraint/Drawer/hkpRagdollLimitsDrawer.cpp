#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpRagdollLimitsDrawer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/RagdollLimits/hkpRagdollLimitsData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

#include <cmath>

// The vector code is written as SSE lane operations with intrinsics, each product's operands in a fixed order. The
// normalize3 Newton step's constants 0.0, 3.0 and 0.5 are the SDK's. The draw sizes and the cone angles are floats;
// pi/2 is the double 1.5707963705062866. The first two geometry arrays get clearAndDeallocate (their data and capacity
// reset), the last only its destructor.
void hkpRagdollLimitsDrawer::drawConstraint(
    hkpConstraintInstance* constraint,
    hkDebugDisplayHandler* displayHandler,
    int32_t id,
    int32_t tag
) {
    const hkpRagdollLimitsData* limits = static_cast<const hkpRagdollLimitsData*>(constraint->m_data);
    m_primitiveDrawer.setDisplayHandler(displayHandler);

    alignas(16) hkTransform refLocalToWorld;
    alignas(16) hkTransform attLocalToWorld;
    updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
    m_bodyBWPivot.setTransformedPos(refLocalToWorld, hkVector4Zero);
    m_bodyAWPivot.setTransformedPos(attLocalToWorld, hkVector4Zero);
    drawPivots(id, tag);

    const hkRotation& baseA = limits->m_atoms.m_rotations.m_rotationA;
    const hkRotation& baseB = limits->m_atoms.m_rotations.m_rotationB;
    alignas(16) hkVector4 refTwistAxisWorld;
    refTwistAxisWorld.setRotatedDir(m_RB, baseB.m_col0);

    alignas(16) hkVector4 refCrossedWorld;
    refCrossedWorld.setCross(baseB.m_col0, baseB.m_col1);
    refCrossedWorld.normalize3();
    refCrossedWorld.setRotatedDir(m_RB, refCrossedWorld);
    alignas(16) hkVector4 refPlaneNormalWorld;
    refPlaneNormalWorld.setRotatedDir(m_RB, baseB.m_col1);

    alignas(16) hkVector4 cross;
    cross.setCross(baseA.m_col1, baseA.m_col0);
    cross.normalize3();
    alignas(16) hkVector4 attCrossedWorld;
    attCrossedWorld.setRotatedDir(m_RA, cross);
    alignas(16) hkVector4 attTwistAxisWorld;
    attTwistAxisWorld.setRotatedDir(m_RA, baseA.m_col0);

    float arrowSize = getArrowSizeForDraw();
    m_primitiveDrawer.displayArrow(
        m_bodyBWPivot,
        refTwistAxisWorld,
        refCrossedWorld,
        static_cast<int32_t>(0xFF008000),
        arrowSize,
        id,
        tag
    );
    arrowSize = getArrowSizeForDraw();
    const int32_t yellow = hkColor::rgbFromChars(0xFF, 0xFF, 0x00, 0xFF);
    m_primitiveDrawer.displayArrow(m_bodyAWPivot, attTwistAxisWorld, attCrossedWorld, yellow, arrowSize, id, tag);

    float coneSize = getConeSizeForDraw();
    m_twistCone.setParameters(
        limits->m_atoms.m_coneLimit.m_maxAngle,
        coneSize,
        m_numArcSegments,
        refTwistAxisWorld,
        m_bodyBWPivot
    );

    const float planeSize = getPlaneSizeForDraw();
    alignas(16) hkVector4 extent;
    extent.setAll(planeSize);
    m_plane.setParameters(refPlaneNormalWorld, refCrossedWorld, m_bodyBWPivot, extent);

    const float planeMinAngle = limits->m_atoms.m_planesLimit.m_minAngle;
    coneSize = getConeSizeForDraw();
    const float absMaxAngle = std::fabsf(limits->m_atoms.m_planesLimit.m_maxAngle);
    float coneAngle = static_cast<float>(1.5707963705062866 - absMaxAngle);
    m_planeCone1.setParameters(coneAngle, coneSize, m_numArcSegments, refPlaneNormalWorld, m_bodyBWPivot);

    coneSize = getConeSizeForDraw();
    refPlaneNormalWorld.setNeg4(refPlaneNormalWorld);
    const float absMinAngle = std::fabsf(planeMinAngle);
    coneAngle = static_cast<float>(1.5707963705062866 - absMinAngle);
    m_planeCone2.setParameters(coneAngle, coneSize, m_numArcSegments, refPlaneNormalWorld, m_bodyBWPivot);

    alignas(16) hkArray<hkDisplayGeometry*> twistGeometry;
    alignas(16) hkArray<hkDisplayGeometry*> coneGeometries;
    coneGeometries.m_data = nullptr;
    coneGeometries.m_size = 0;
    coneGeometries.m_capacityAndFlags = hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG;
    alignas(16) hkArray<hkDisplayGeometry*> planeGeometry;
    planeGeometry.m_data = nullptr;
    planeGeometry.m_size = 0;
    planeGeometry.m_capacityAndFlags = hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG;

    twistGeometry.setSize(1);
    planeGeometry.setSize(1);
    coneGeometries.setSize(2);

    twistGeometry.m_data[0] = &m_twistCone;
    coneGeometries.m_data[0] = &m_planeCone1;
    coneGeometries.m_data[1] = &m_planeCone2;
    planeGeometry.m_data[0] = &m_plane;
    displayHandler->displayGeometry(twistGeometry, static_cast<int32_t>(0xFFFFFF00), id, tag);
    displayHandler->displayGeometry(coneGeometries, static_cast<int32_t>(0xFFFF0000), id, tag);
    const int32_t magenta = hkColor::rgbFromChars(0xFF, 0x00, 0xFF, 0xFF);
    displayHandler->displayGeometry(planeGeometry, magenta, id, tag);

    planeGeometry.clearAndDeallocate();
    coneGeometries.clearAndDeallocate();
}
