#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpRagdollDrawer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayCone.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayPlane.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Ragdoll/hkpRagdollConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

#include <cmath>

// The plane and the cones go with their members.
hkpRagdollDrawer::~hkpRagdollDrawer() {}

// The vector code is written as lane operations with intrinsics, keeping each product's operands in the game's order.
// The normalize3 Newton step's constants 0.0, 3.0 and 0.5 are the SDK's. The draw sizes and the cone angles are floats;
// pi/2 is the double 1.5707963705062866. The axis indices are bytes of the limit atoms, read again for each use. The
// first two geometry arrays get the SDK's clearAndDeallocate (their data and capacity reset), the last only its
// destructor.
void hkpRagdollDrawer::drawConstraint(
    hkpConstraintInstance* constraint,
    hkDebugDisplayHandler* displayHandler,
    int32_t id,
    int32_t tag
) {
    const hkpRagdollConstraintData* ragdoll = static_cast<const hkpRagdollConstraintData*>(constraint->m_data);
    m_primitiveDrawer.setDisplayHandler(displayHandler);

    alignas(16) hkTransform refLocalToWorld;
    alignas(16) hkTransform attLocalToWorld;
    updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
    const hkpRagdollConstraintData::Atoms& atoms = ragdoll->m_atoms;
    m_bodyBWPivot.setTransformedPos(refLocalToWorld, atoms.m_transforms.m_transformB.m_translation);
    m_bodyAWPivot.setTransformedPos(attLocalToWorld, atoms.m_transforms.m_transformA.m_translation);
    drawPivots(id, tag);

    const hkRotation& baseA = atoms.m_transforms.m_transformA.m_rotation;
    const hkRotation& baseB = atoms.m_transforms.m_transformB.m_rotation;
    alignas(16) hkVector4 refTwistAxisWorld;
    refTwistAxisWorld.setRotatedDir(m_RB, baseB.getColumn(atoms.m_twistLimit.m_twistAxis));

    alignas(16) hkVector4 refCrossedWorld;
    refCrossedWorld.setCross(
        baseB.getColumn(atoms.m_planesLimit.m_twistAxisInA), baseB.getColumn(atoms.m_planesLimit.m_refAxisInB)
    );
    refCrossedWorld.normalize3();
    refCrossedWorld.setRotatedDir(m_RB, refCrossedWorld);
    alignas(16) hkVector4 refPlaneNormalWorld;
    refPlaneNormalWorld.setRotatedDir(m_RB, baseB.getColumn(atoms.m_planesLimit.m_refAxisInB));

    alignas(16) hkVector4 cross;
    cross.setCross(
        baseA.getColumn(atoms.m_planesLimit.m_refAxisInB), baseA.getColumn(atoms.m_planesLimit.m_twistAxisInA)
    );
    cross.normalize3();
    alignas(16) hkVector4 attCrossedWorld;
    attCrossedWorld.setRotatedDir(m_RA, cross);
    alignas(16) hkVector4 attTwistAxisWorld;
    attTwistAxisWorld.setRotatedDir(m_RA, baseA.getColumn(atoms.m_twistLimit.m_twistAxis));

    // EXE-INLINED none: hkpConstraintDrawer::getArrowSizeForDraw
    float arrowSize = m_scale * m_arrowSize;
    m_primitiveDrawer.displayArrow(
        m_bodyBWPivot, refTwistAxisWorld, refCrossedWorld, static_cast<int32_t>(0xFF008000), arrowSize, id, tag
    );
    // EXE-INLINED none: hkpConstraintDrawer::getArrowSizeForDraw
    arrowSize = m_scale * m_arrowSize;
    const int32_t yellow = hkColor::rgbFromChars(0xFF, 0xFF, 0x00, 0xFF);
    m_primitiveDrawer.displayArrow(m_bodyAWPivot, attTwistAxisWorld, attCrossedWorld, yellow, arrowSize, id, tag);

    // EXE-INLINED none: hkpConstraintDrawer::getConeSizeForDraw
    float coneSize = m_scale * m_coneSize;
    m_twistCone.setParameters(
        atoms.m_coneLimit.m_maxAngle, coneSize, m_numArcSegments, refTwistAxisWorld, m_bodyBWPivot
    );

    // EXE-INLINED none: hkpConstraintDrawer::getPlaneSizeForDraw (the exe multiplies the plane size by the scale)
    const float planeSize = m_planeSize * m_scale;
    alignas(16) hkVector4 extent;
    extent.setAll(planeSize);
    m_plane.setParameters(refPlaneNormalWorld, refCrossedWorld, m_bodyBWPivot, extent);

    const float planeMinAngle = atoms.m_planesLimit.m_minAngle;
    // EXE-INLINED none: hkpConstraintDrawer::getConeSizeForDraw
    coneSize = m_scale * m_coneSize;
    const float absMaxAngle = std::fabsf(atoms.m_planesLimit.m_maxAngle);
    float coneAngle = static_cast<float>(1.5707963705062866 - absMaxAngle);
    m_planeCone1.setParameters(coneAngle, coneSize, m_numArcSegments, refPlaneNormalWorld, m_bodyBWPivot);

    // EXE-INLINED none: hkpConstraintDrawer::getConeSizeForDraw
    coneSize = m_scale * m_coneSize;
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
