#include "Havok/Source/Physics/Dynamics/Constraint/Util/hkpConstraintDataUtils.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/Null/hkpNullCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpClosestCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/BallAndSocket/hkpBallAndSocketConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Breakable/hkpBreakableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "xfnv/Compiler.h"

// The closest point between a shape and a point, asked of the world's collision dispatcher: the point is a sphere of
// the collision tolerance (0.01) placed at arPosition by an identity transform, the other body is arShape at
// hkTransformIdentity, the input is the world's collision input with an unlimited tolerance and the null filter, and
// the agent of the two shape types' entry collects the closest contact point. The result is the hit's shape key of
// the second body, or 0 when nothing was hit.
int32_t hkGetClosestPointOnShape(hkpWorld* apWorld, const hkpShape* apShape, const hkVector4& arPosition) {
    // The query sphere is constructed in raw local storage and deliberately never destroyed, as the game does.
    alignas(16) uint8_t sphereMemory[0x20];
    hkpSphereShape* pSphere = ::new (sphereMemory) hkpSphereShape(0.0099999998f);
    // The query sphere's transform is built from the identity rotation and the query point.
    const hkQuadReal zero = {0.0f, 0.0f, 0.0f, 0.0f};
    hkTransform transform;
    transform.m_rotation.m_col0.m_quad = zero;
    transform.m_rotation.m_col1.m_quad = zero;
    transform.m_rotation.m_col2.m_quad = zero;
    XFNV_M128_F32(transform.m_rotation.m_col0.m_quad, 0) = 1.0f;
    XFNV_M128_F32(transform.m_rotation.m_col1.m_quad, 1) = 1.0f;
    XFNV_M128_F32(transform.m_rotation.m_col2.m_quad, 2) = 1.0f;
    transform.m_translation = arPosition;
    hkpCdBody bodyA;
    bodyA.m_shape = pSphere;
    bodyA.m_shapeKey = 0xFFFFFFFF;
    bodyA.m_motion.pMotion = &transform;
    bodyA.m_parent = nullptr;
    hkpCdBody bodyB;
    bodyB.m_shape = apShape;
    bodyB.m_shapeKey = 0xFFFFFFFF;
    bodyB.m_motion.pMotion = &hkTransformIdentity;
    bodyB.m_parent = nullptr;
    // The collector is never destroyed (its destructor is empty).
    hkpClosestCdPointCollector collector;
    // The input is the world's, with the tolerance HK_REAL_HIGH and the null filter in place of the
    // world's.
    hkpCollisionInput input = *apWorld->m_collisionInput;
    input.m_tolerance = 1.70141e38f;
    alignas(16) hkpNullCollisionFilter nullFilter;
    input.m_filter = &nullFilter;
    hkpCollisionDispatcher* pDispatcher = apWorld->m_collisionDispatcher;
    const uint8_t uiAgentType = pDispatcher->m_agent2Types[pSphere->m_type][apShape->m_type];
    pDispatcher->m_agent2Func[uiAgentType].m_getClosestPointFunc(bodyA, bodyB, input, collector);
    if (collector.m_hitPoint.m_rootCollidableA != nullptr) {
        return collector.m_hitPoint.m_shapeKeyB;
    }
    return 0;
}

// A breakable constraint's wrapped data is read inline, a malleable one's through its getter; either way the switch
// runs again on the wrapped data. Ball and socket, stiff spring and pulley data start their atoms with the same
// hkpSetLocalTranslationsConstraintAtom, and hinge, limited hinge, prismatic, ragdoll, wheel and point to plane data
// with the same hkpSetLocalTransformsConstraintAtom, so each group is written through one of its classes. Other types
// have no pivot.
void hkpConstraintDataUtils::setPivot(hkpConstraintData* apData, const hkVector4& arPivot, int32_t aiIndex) {
    for (;;) {
        switch (apData->getType()) {
            case hkpConstraintData::CONSTRAINT_TYPE_BREAKABLE:
                apData = static_cast<hkpBreakableConstraintData*>(apData)->m_constraintData;
                continue;
            case hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE:
                apData = static_cast<hkpMalleableConstraintData*>(apData)->getWrappedConstraintData();
                continue;
            case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET:
            case hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING:
            case hkpConstraintData::CONSTRAINT_TYPE_PULLEY:
                (&static_cast<hkpBallAndSocketConstraintData*>(apData)->m_atoms.m_pivots.m_translationA)[aiIndex] =
                    arPivot;
                return;
            case hkpConstraintData::CONSTRAINT_TYPE_HINGE:
            case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE:
            case hkpConstraintData::CONSTRAINT_TYPE_PRISMATIC:
            case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL:
            case hkpConstraintData::CONSTRAINT_TYPE_WHEEL:
            case hkpConstraintData::CONSTRAINT_TYPE_POINTTOPLANE:
                (&static_cast<hkpLimitedHingeConstraintData*>(apData)->m_atoms.m_transforms.m_transformA)[aiIndex]
                    .m_translation = arPivot;
                return;
            default: return;
        }
    }
}

// Each pass starts by setting arPivot to identity: the rotation's columns zeroed, the diagonal set to 1.0f, then the
// translation zeroed. A malleable constraint's wrapped data comes from the const getter. The groups and the merged
// cases are setPivot's; a transform is copied whole, a pivot by a vector copy.
void hkpConstraintDataUtils::getPivotTransform(const hkpConstraintData* apData, int32_t aiIndex, hkTransform& arPivot) {
    for (;;) {
        const hkQuadReal zero = {0.0f, 0.0f, 0.0f, 0.0f};
        arPivot.m_rotation.m_col0.m_quad = zero;
        arPivot.m_rotation.m_col1.m_quad = zero;
        arPivot.m_rotation.m_col2.m_quad = zero;
        XFNV_M128_F32(arPivot.m_rotation.m_col0.m_quad, 0) = 1.0f;
        XFNV_M128_F32(arPivot.m_rotation.m_col1.m_quad, 1) = 1.0f;
        XFNV_M128_F32(arPivot.m_rotation.m_col2.m_quad, 2) = 1.0f;
        arPivot.m_translation.m_quad = zero;
        switch (apData->getType()) {
            case hkpConstraintData::CONSTRAINT_TYPE_BREAKABLE:
                apData = static_cast<const hkpBreakableConstraintData*>(apData)->m_constraintData;
                continue;
            case hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE:
                apData = static_cast<const hkpMalleableConstraintData*>(apData)->getWrappedConstraintData();
                continue;
            case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET:
            case hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING:
            case hkpConstraintData::CONSTRAINT_TYPE_PULLEY:
                arPivot.m_translation = (&static_cast<const hkpBallAndSocketConstraintData*>(apData)
                                              ->m_atoms.m_pivots.m_translationA)[aiIndex];
                return;
            case hkpConstraintData::CONSTRAINT_TYPE_HINGE:
            case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE:
            case hkpConstraintData::CONSTRAINT_TYPE_PRISMATIC:
            case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL:
            case hkpConstraintData::CONSTRAINT_TYPE_WHEEL:
            case hkpConstraintData::CONSTRAINT_TYPE_POINTTOPLANE: {
                const hkpLimitedHingeConstraintData* pTransformsData =
                    static_cast<const hkpLimitedHingeConstraintData*>(apData);
                arPivot.Assign((&pTransformsData->m_atoms.m_transforms.m_transformA)[aiIndex]);
                return;
            }
            default: return;
        }
    }
}

// The groups and the merged cases are setPivot's: a transform is copied through hkTransform::operator=, a pivot as the
// transform's translation by a vector copy.
void hkpConstraintDataUtils::setPivotTransform(hkpConstraintData* apData, const hkTransform& arPivot, int32_t aiIndex) {
    for (;;) {
        switch (apData->getType()) {
            case hkpConstraintData::CONSTRAINT_TYPE_BREAKABLE:
                apData = static_cast<hkpBreakableConstraintData*>(apData)->m_constraintData;
                continue;
            case hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE:
                apData = static_cast<hkpMalleableConstraintData*>(apData)->getWrappedConstraintData();
                continue;
            case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET:
            case hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING:
            case hkpConstraintData::CONSTRAINT_TYPE_PULLEY:
                (&static_cast<hkpBallAndSocketConstraintData*>(apData)->m_atoms.m_pivots.m_translationA)[aiIndex] =
                    arPivot.m_translation;
                return;
            case hkpConstraintData::CONSTRAINT_TYPE_HINGE:
            case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE:
            case hkpConstraintData::CONSTRAINT_TYPE_PRISMATIC:
            case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL:
            case hkpConstraintData::CONSTRAINT_TYPE_WHEEL:
            case hkpConstraintData::CONSTRAINT_TYPE_POINTTOPLANE: {
                hkpLimitedHingeConstraintData* pTransformsData = static_cast<hkpLimitedHingeConstraintData*>(apData);
                (&pTransformsData->m_atoms.m_transforms.m_transformA)[aiIndex].Assign(arPivot);
                return;
            }
            default: return;
        }
    }
}
