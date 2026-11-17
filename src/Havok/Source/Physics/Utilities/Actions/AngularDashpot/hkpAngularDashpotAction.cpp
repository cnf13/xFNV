#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/SweptTransform/hkSweptTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpBinaryAction.h"
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
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Actions/Spring/hkpSpringAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "SDK/MSVC/rtti.h"

#include <bit>
#include <xmmintrin.h>

// The reflected members of hkpAngularDashpotAction.
using hkpAngularDashpotActionClass_MembersArray = const hkClassMember[3];
static hkpAngularDashpotActionClass_MembersArray hkpAngularDashpotActionClass_Members{
    {"rotation", nullptr, nullptr, hkClassMember::TYPE_QUATERNION, hkClassMember::TYPE_VOID, 0, 0, 0x20, nullptr},
    {"strength", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x30, nullptr},
    {"damping", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x34, nullptr}
};

hkClass hkpAngularDashpotActionClass(
    "hkpAngularDashpotAction",
    &hkpBinaryActionClass,
    64,
    nullptr,
    0,
    nullptr,
    0,
    hkpAngularDashpotActionClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpAngularDashpotActionTypeInfo = {
    "hkpAngularDashpotAction",
    finishLoadedObjecthkpAngularDashpotAction,
    cleanupLoadedObjecthkpAngularDashpotAction,
    getVtablehkpAngularDashpotAction(),
    &typeid(hkpAngularDashpotAction),
    sizeof(hkpAngularDashpotAction)
};

void cleanupLoadedObjecthkpAngularDashpotAction(void* p) {
    static_cast<hkpAngularDashpotAction*>(p)->~hkpAngularDashpotAction();
}

const void* getVtablehkpAngularDashpotAction() {
    return hkGetVtableOf<hkpAngularDashpotAction>();
}

hkpAngularDashpotAction::hkpAngularDashpotAction(hkpRigidBody* apEntityA, hkpRigidBody* apEntityB, hkUlong auiUserData)
  : hkpBinaryAction(apEntityA, apEntityB, auiUserData), m_strength(0.1f), m_damping(0.01f) {
    m_rotation.m_vec.m_quad = hkQuadReal0001;
}

void hkpAngularDashpotAction::applyAction(const hkStepInfo& stepInfo) {
    uint32_t v9;
    hkVector4 v10;
    hkQuaternion v11;
    hkQuaternion direction_4;
    hkpEntity* m_entityB = this->getEntityB();
    *reinterpret_cast<float*>(&v9) = stepInfo.m_deltaTime * 200.0;
    hkpEntity* m_entityA = this->getEntityA();
    v11.setMul(m_entityB->m_motion.m_motionState.m_sweptTransform.m_rotation1, m_rotation);
    direction_4.m_vec.m_quad = _mm_xor_ps(rdata_10C4C00, v11.m_vec.m_quad);
    v11.setMul(m_entityA->m_motion.m_motionState.m_sweptTransform.m_rotation1, direction_4);
    direction_4.m_vec.m_quad =
        _mm_sub_ps(m_entityA->m_motion.m_angularVelocity.m_quad, m_entityB->m_motion.m_angularVelocity.m_quad);
    float AngleSr = v11.getAngle();
    __m128 v5 = _mm_setzero_ps();
    if (AngleSr > 0.001f) {
        v5 = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(AngleSr), _mm_set_ss(AngleSr), 0), v11.m_vec.m_quad);
    }
    double v6 = *reinterpret_cast<float*>(&v9);
    *reinterpret_cast<float*>(&v9) = m_strength * *reinterpret_cast<float*>(&v9);
    __m128 v7 = _mm_set_ss(std::bit_cast<float>(v9));
    *reinterpret_cast<float*>(&v9) = v6 * m_damping;
    v10.m_quad = _mm_add_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v9)), _mm_set_ss(std::bit_cast<float>(v9)), 0),
            direction_4.m_vec.m_quad
        ),
        _mm_mul_ps(_mm_shuffle_ps(v7, v7, 0), v5)
    );
    m_entityB->activate();
    // Called through the motion's vtable.
    m_entityB->getMotion()->applyAngularImpulse(v10);
    v10.m_quad = _mm_xor_ps(v10.m_quad, hkQuadRealNegateMask);
    m_entityA->activate();
    m_entityA->getMotion()->applyAngularImpulse(v10);
}

// A copy of this action between two new entities (cast to rigid bodies), allocated with the size in its
// m_memSizeAndFlags as the class operator new does; null unless there are two entities and no phantoms.
hkpAction* hkpAngularDashpotAction::clone(
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& newEntities,
    const hkArray<hkpPhantom*, hkContainerHeapAllocator>& newPhantoms
) const {
    if (newEntities.m_size != 2 || newPhantoms.m_size) {
        return nullptr;
    }
    hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
    hkpAngularDashpotAction* pMemory =
        static_cast<hkpAngularDashpotAction*>(pRouter->m_heap->blockAlloc(sizeof(hkpAngularDashpotAction)));
    pMemory->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpAngularDashpotAction));
    hkpAngularDashpotAction* pAction = ::new (pMemory) hkpAngularDashpotAction(
        static_cast<hkpRigidBody*>(newEntities.m_data[0]), static_cast<hkpRigidBody*>(newEntities.m_data[1]), m_userData
    );
    pAction->m_rotation = m_rotation;
    pAction->m_strength = m_strength;
    pAction->m_damping = m_damping;
    return pAction;
}

const hkClass& hkpAngularDashpotAction::staticClass() {
    return hkpAngularDashpotActionClass;
}
