#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpClosestCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// The member table of hkpMouseSpringAction's class record, as the Havok SDK's class file has it.
using hkpMouseSpringActionClass_MembersArray = const hkClassMember[8];
static hkpMouseSpringActionClass_MembersArray hkpMouseSpringActionClass_Members{
    {"positionInRbLocal",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x20,
     nullptr},
    {"mousePositionInWorld",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x30,
     nullptr},
    {"springDamping", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x40, nullptr},
    {"springElasticity",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x44,
     nullptr},
    {"maxRelativeForce",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_REAL},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x48,
     nullptr},
    {"objectDamping", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4C, nullptr},
    {"shapeKey", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x50, nullptr},
    {"applyCallbacks",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x54,
     nullptr}
};

hkClass hkpMouseSpringActionClass(
    "hkpMouseSpringAction",
    &hkpUnaryActionClass,
    96,
    nullptr,
    0,
    nullptr,
    0,
    hkpMouseSpringActionClass_Members,
    8,
    nullptr,
    nullptr,
    0,
    0
);

hkTypeInfo hkpMouseSpringActionTypeInfo = {
    "hkpMouseSpringAction",
    finishLoadedObjecthkpMouseSpringAction,
    cleanupLoadedObjecthkpMouseSpringAction,
    getVtablehkpMouseSpringAction(),
    &typeid(hkpMouseSpringAction),
    sizeof(hkpMouseSpringAction)
};

void cleanupLoadedObjecthkpMouseSpringAction(void* p) {
    static_cast<hkpMouseSpringAction*>(p)->~hkpMouseSpringAction();
}

// The unary action on body, the default spring parameters, an empty callback array, zero positions and no shape key.
hkpMouseSpringAction::hkpMouseSpringAction(hkpRigidBody* body)
  : hkpUnaryAction(body, 0), m_springDamping(0.5f), m_springElasticity(0.3f), m_maxRelativeForce(250.0f),
    m_objectDamping(0.95f), m_shapeKey(HK_INVALID_SHAPE_KEY) {
    m_positionInRbLocal.m_quad = _mm_setzero_ps();
    m_mousePositionInWorld.m_quad = _mm_setzero_ps();
    m_name = "hkpMouseSpringAction";
}

// The spring's parameters and a copy of the callbacks. With rb in a world, a unit sphere at the mouse position is
// tested against rb for the closest point, and m_shapeKey takes the hit's shape key on rb's side.
hkpMouseSpringAction::hkpMouseSpringAction(
    const hkVector4& positionInRbLocal,
    const hkVector4& mousePositionInWorld,
    const float springDamping,
    const float springElasticity,
    const float objectDamping,
    hkpRigidBody* rb,
    const hkArray<ApplyCallback, hkContainerHeapAllocator>* appliedCallbacks
)
  : hkpUnaryAction(rb, 0), m_positionInRbLocal(positionInRbLocal), m_mousePositionInWorld(mousePositionInWorld),
    m_springDamping(springDamping), m_springElasticity(springElasticity), m_maxRelativeForce(250.0f),
    m_objectDamping(objectDamping), m_shapeKey(HK_INVALID_SHAPE_KEY) {
    if (appliedCallbacks) {
        m_applyCallbacks
            .copyFromArray(hkContainerHeapAllocator().get(&m_applyCallbacks), *appliedCallbacks, hkTypeIsPod());
    }
    hkpWorld* pWorld = rb->m_world;
    if (pWorld) {
        hkpSphereShape sphere(1.0f);
        hkTransform transform;
        static_cast<hkMatrix3&>(transform.m_rotation) = hkMatrix3::getIdentity();
        transform.m_translation.m_quad = mousePositionInWorld.m_quad;
        hkpCollidable collidable(&sphere, &transform, 0);
        hkpCollisionDispatcher::GetClosestPointsFunc
            getClosestPoints = pWorld->m_collisionDispatcher->getGetClosestPointsFunc(
                static_cast<hkpShapeType>(sphere.m_type.m_storage),
                static_cast<hkpShapeType>(rb->m_collidable.m_shape->m_type.m_storage)
            );
        hkpCollisionInput input = *pWorld->m_collisionInput;
        hkpClosestCdPointCollector collector;
        XFNV_M128_F32(collector.m_hitPoint.m_contact.m_separatingNormal.m_quad, 3) = HK_REAL_MAX;
        collector.m_earlyOutDistance = HK_REAL_MAX;
        collector.m_hitPoint.m_rootCollidableA = nullptr;
        getClosestPoints(collidable, rb->m_collidable, input, collector);
        if (collector.m_hitPoint.m_rootCollidableA) {
            m_shapeKey = collector.m_hitPoint.m_shapeKeyB;
        }
    }
    m_name = "hkpMouseSpringAction";
}

// The callback array's storage goes with m_applyCallbacks.
hkpMouseSpringAction::~hkpMouseSpringAction() {}

// Pulls the body's point m_positionInRbLocal towards m_mousePositionInWorld: an impulse through the effective mass
// matrix at that point, clamped to m_maxRelativeForce, after damping the body's velocities by m_objectDamping.
void hkpMouseSpringAction::applyAction(const hkStepInfo& stepInfo) {
    hkpRigidBody* pRb = static_cast<hkpRigidBody*>(m_entity);

    alignas(16) hkVector4 pointWs;
    pointWs.setTransformedPos(pRb->m_motion.m_motionState.m_transform, m_positionInRbLocal);
    alignas(16) hkVector4 posError;
    posError.setSub4(pointWs, m_mousePositionInWorld);

    hkpMotion* pMotion;
    if (pRb->getStoredDynamicMotion()) {
        pMotion = pRb->getStoredDynamicMotion();
    }
    else {
        pMotion = &pRb->m_motion;
    }
    float massInv = pMotion->getMassInv();

    alignas(16) hkVector4 rel;
    rel.setSub4(pointWs, pRb->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1);
    alignas(16) hkMatrix3 rhat;
    rhat.setCrossSkewSymmetric(rel);

    alignas(16) hkMatrix3 inertiaInv;
    pMotion->getInertiaInvWorld(inertiaInv);

    // The mass matrix: massInv on the diagonal, zero elsewhere.
    alignas(16) hkMatrix3 mass;
    mass.m_col0.m_quad = hkQuadReal{0.0f, 0.0f, 0.0f, 0.0f};
    XFNV_M128_F32(mass.m_col0.m_quad, 0) = massInv;
    mass.m_col1.m_quad = hkQuadReal{0.0f, 0.0f, 0.0f, 0.0f};
    XFNV_M128_F32(mass.m_col1.m_quad, 1) = massInv;
    mass.m_col2.m_quad = hkQuadReal{0.0f, 0.0f, 0.0f, 0.0f};
    XFNV_M128_F32(mass.m_col2.m_quad, 2) = massInv;
    {
        alignas(16) hkMatrix3 temp;
        temp.setMul(rhat, inertiaInv);
        alignas(16) hkMatrix3 temp2;
        temp2.setMul(temp, rhat);
        mass.sub(temp2);
    }
    if (mass.invert(1e-07f) != 0) {
        return;
    }

    alignas(16) hkVector4 linVel = pRb->getLinearVelocity();
    linVel.mul4(m_objectDamping);
    pRb->setLinearVelocity(linVel);
    alignas(16) hkVector4 angVel = pRb->getAngularVelocity();
    angVel.mul4(m_objectDamping);
    pRb->setAngularVelocity(angVel);

    alignas(16) hkVector4 pointVel;
    pRb->getPointVelocity(pointWs, pointVel);

    // deltaV = elasticity / dt * posError + springDamping * pointVel.
    float elasticity = m_springElasticity * stepInfo.m_invDeltaTime;
    alignas(16) hkVector4 deltaV;
    float springDamping = m_springDamping;
    XFNV_M128_F32(deltaV.m_quad, 0) = springDamping * XFNV_M128_F32(pointVel.m_quad, 0) +
        elasticity * XFNV_M128_F32(posError.m_quad, 0);
    XFNV_M128_F32(deltaV.m_quad, 1) = springDamping * XFNV_M128_F32(pointVel.m_quad, 1) +
        elasticity * XFNV_M128_F32(posError.m_quad, 1);
    XFNV_M128_F32(deltaV.m_quad, 2) = springDamping * XFNV_M128_F32(pointVel.m_quad, 2) +
        elasticity * XFNV_M128_F32(posError.m_quad, 2);
    XFNV_M128_F32(deltaV.m_quad, 3) = springDamping * XFNV_M128_F32(pointVel.m_quad, 3) +
        elasticity * XFNV_M128_F32(posError.m_quad, 3);

    alignas(16) hkVector4 impulse;
    impulse.setMul3(mass, deltaV);
    impulse.setNeg4(impulse);
    float impulseLen2 = impulse.lengthSquared3();

    float maxImpulse = pMotion->getMass();
    maxImpulse = stepInfo.m_deltaTime * maxImpulse * m_maxRelativeForce;
    if (impulseLen2 > maxImpulse * maxImpulse) {
        float len = std::sqrt(impulseLen2);
        float factor = 1.0f / len;
        factor = factor * maxImpulse;
        XFNV_M128_F32(impulse.m_quad, 0) = factor * XFNV_M128_F32(impulse.m_quad, 0);
        XFNV_M128_F32(impulse.m_quad, 1) = factor * XFNV_M128_F32(impulse.m_quad, 1);
        XFNV_M128_F32(impulse.m_quad, 2) = factor * XFNV_M128_F32(impulse.m_quad, 2);
        XFNV_M128_F32(impulse.m_quad, 3) = factor * XFNV_M128_F32(impulse.m_quad, 3);
    }

    pRb->activate();
    hkpMotion* pRbMotion = &pRb->m_motion;
    pRbMotion->applyPointImpulse(impulse, pointWs);

    for (int32_t i = 0; i < m_applyCallbacks.m_size; ++i) {
        m_applyCallbacks.m_data[i](this, stepInfo, impulse);
    }
}

void hkpMouseSpringAction::entityRemovedCallback(hkpEntity* entity) {
    hkpUnaryAction::entityRemovedCallback(entity);
    m_entity = nullptr;
}

// A copy of this action on the one new entity (cast to a rigid body); null unless there is one entity and no phantoms.
hkpAction* hkpMouseSpringAction::clone(
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& newEntities,
    const hkArray<hkpPhantom*, hkContainerHeapAllocator>& newPhantoms
) const {
    if (newEntities.m_size != 1 || newPhantoms.m_size) {
        return nullptr;
    }
    hkpMouseSpringAction* pAction = new hkpMouseSpringAction(static_cast<hkpRigidBody*>(newEntities.m_data[0]));
    pAction->m_positionInRbLocal.m_quad = m_positionInRbLocal.m_quad;
    pAction->m_mousePositionInWorld.m_quad = m_mousePositionInWorld.m_quad;
    pAction->m_springDamping = m_springDamping;
    pAction->m_springElasticity = m_springElasticity;
    pAction->m_maxRelativeForce = m_maxRelativeForce;
    pAction->m_objectDamping = m_objectDamping;
    pAction->m_userData = m_userData;
    pAction->m_shapeKey = m_shapeKey;
    pAction->m_applyCallbacks = m_applyCallbacks;
    return pAction;
}

void* hkpMouseSpringAction::GetEntity() {
    return m_entity;
}

void hkpMouseSpringAction::setMousePosition(const hkVector4& mousePositionInWorld) {
    if ((_mm_movemask_ps(_mm_cmple_ps(
             _mm_and_ps(_mm_sub_ps(mousePositionInWorld.m_quad, m_mousePositionInWorld.m_quad), hkVector4::absMask),
             _mm_shuffle_ps(
                 _mm_set_ss(std::bit_cast<float>(0x3A83126Fu)),
                 _mm_set_ss(std::bit_cast<float>(0x3A83126Fu)),
                 0
             )
         )) &
         7) != 7) {
        hkpEntity* m_entity = this->m_entity;
        if (m_entity && m_entity->m_world) {
            m_entity->activate();
        }
    }
    m_mousePositionInWorld.m_quad = mousePositionInWorld.m_quad;
}

const hkClass& hkpMouseSpringAction::staticClass() {
    return hkpMouseSpringActionClass;
}
