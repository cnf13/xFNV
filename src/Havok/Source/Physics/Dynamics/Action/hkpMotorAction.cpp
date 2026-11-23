#include "Havok/Source/Physics/Dynamics/Action/hkpMotorAction.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"

#include <xmmintrin.h>

// The member table of hkpMotorAction's class record, as the Havok SDK's class file has it.
using hkpMotorActionClass_MembersArray = const hkClassMember[4];
static hkpMotorActionClass_MembersArray hkpMotorActionClass_Members{
    {"axis", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr},
    {"spinRate", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x30, nullptr},
    {"gain", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x34, nullptr},
    {"active", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x38, nullptr}
};

hkClass hkpMotorActionClass(
    "hkpMotorAction",
    &hkpUnaryActionClass,
    64,
    nullptr,
    0,
    nullptr,
    0,
    hkpMotorActionClass_Members,
    4,
    nullptr,
    nullptr,
    0,
    0
);

hkTypeInfo hkpMotorActionTypeInfo = {
    "hkpMotorAction",
    finishLoadedObjecthkpMotorAction,
    cleanupLoadedObjecthkpMotorAction,
    getVtablehkpMotorAction(),
    &typeid(hkpMotorAction),
    sizeof(hkpMotorAction)
};

// The SDK's constructor: the unary base on body, the members from the arguments, active, and the axis normalized.
hkpMotorAction::hkpMotorAction(hkpRigidBody* body, const hkVector4& axis, float spinRate, float gain)
  : hkpUnaryAction(body, 0), m_axis(axis), m_spinRate(spinRate), m_gain(gain) {
    m_active.m_bool = true;
    m_axis.normalize4();
}

// The torque about the axis that drives the body's angular velocity along it towards m_spinRate, scaled by m_gain,
// taken through the body's local inertia into world space and applied over the step.
void hkpMotorAction::applyAction(const hkStepInfo& stepInfo) {
    if (m_active.m_bool) {
        hkpEntity* pEntity = m_entity;
        hkpMotion* pMotion = pEntity->getMotion();
        hkVector4 currentAngVel;
        currentAngVel.setRotatedInverseDir(pMotion->m_motionState.m_transform.m_rotation, pMotion->m_angularVelocity);
        float fCurrentRate = m_axis.dot3(currentAngVel);
        float fTorque = m_spinRate - fCurrentRate;
        fTorque = fTorque * m_gain;
        hkVector4 torque;
        torque.setMul4(fTorque, m_axis);
        hkMatrix3 inertia;
        pMotion->getInertiaLocal(inertia);
        torque.setMul3(inertia, torque);
        torque.setRotatedDir(pMotion->m_motionState.m_transform.m_rotation, torque);
        float fDeltaTime = stepInfo.m_deltaTime;
        pEntity->activate();
        pMotion->applyTorque(fDeltaTime, torque);
    }
}

hkpAction* hkpMotorAction::clone(
    const hkArray<hkpEntity*, hkContainerHeapAllocator>& newEntities,
    const hkArray<hkpPhantom*, hkContainerHeapAllocator>& newPhantoms
) const {
    if (newEntities.m_size != 1 || newPhantoms.m_size) {
        return nullptr;
    }
    hkpMotorAction*
        pResult = new hkpMotorAction(static_cast<hkpRigidBody*>(newEntities.m_data[0]), m_axis, m_spinRate, m_gain);
    pResult->m_active.m_bool = m_active.m_bool;
    pResult->m_userData = m_userData;
    return pResult;
}

const hkClass& hkpMotorAction::staticClass() {
    return hkpMotorActionClass;
}
