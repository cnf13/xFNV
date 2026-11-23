#include "Havok/Source/Physics/Dynamics/Collide/hkpResponseModifier.h"

#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpDynamicsContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldConstraintUtil.h"
#include "SDK/Windows/winbase.h"
#include "xfnv/Compiler.h"

#include <xmmintrin.h>

// The SDK: the contact's constraint gets a mass changer atom (made and added under constraintOwner when it has
// none), which takes factorA for whichever of its entities is bodyA. bodyB is not read.
void hkpResponseModifier::setInvMassScalingForContact(
    hkpDynamicsContactMgr* manager,
    hkpRigidBody* bodyA,
    hkpRigidBody* bodyB,
    hkpConstraintOwner& constraintOwner,
    const hkVector4& factorA,
    const hkVector4& factorB
) {
    hkpConstraintInstance* pInstance = manager->getConstraintInstance();
    if (!pInstance) {
        return;
    }
    constraintOwner.checkAccessRw();
    HK_TIMER_BEGIN("SetMassChang", nullptr);
    hkpMassChangerModifierConstraintAtom* pAtom = static_cast<hkpMassChangerModifierConstraintAtom*>(
        hkpWorldConstraintUtil::findModifier(pInstance, hkpConstraintAtom::TYPE_MODIFIER_MASS_CHANGER)
    );
    if (!pAtom) {
        pAtom = new hkpMassChangerModifierConstraintAtom;
        hkpWorldConstraintUtil::addModifier(pInstance, constraintOwner, pAtom);
    }
    if (bodyA == pInstance->getEntityA()) {
        pAtom->m_factorA = factorA;
        pAtom->m_factorB = factorB;
    }
    else {
        pAtom->m_factorA = factorB;
        pAtom->m_factorB = factorA;
    }
    HK_TIMER_END();
}

// The SDK: a new mass changer atom gets factor for body and one for the other entity; an existing one only has
// body's factor replaced, and when both factors' w are then zero both w become one.
void hkpResponseModifier::setInvMassScalingForContact(
    hkpDynamicsContactMgr* manager,
    hkpRigidBody* body,
    hkpConstraintOwner& constraintOwner,
    const hkVector4& factor
) {
    hkpConstraintInstance* pInstance = manager->getConstraintInstance();
    if (!pInstance) {
        return;
    }
    constraintOwner.checkAccessRw();
    HK_TIMER_BEGIN("SetMassChang", nullptr);
    hkpMassChangerModifierConstraintAtom* pAtom = static_cast<hkpMassChangerModifierConstraintAtom*>(
        hkpWorldConstraintUtil::findModifier(pInstance, hkpConstraintAtom::TYPE_MODIFIER_MASS_CHANGER)
    );
    if (!pAtom) {
        pAtom = new hkpMassChangerModifierConstraintAtom;
        hkpWorldConstraintUtil::addModifier(pInstance, constraintOwner, pAtom);
        if (body == pInstance->getEntityA()) {
            pAtom->m_factorA = factor;
            pAtom->m_factorB.setAll(1.0f);
        }
        else {
            pAtom->m_factorA.setAll(1.0f);
            pAtom->m_factorB = factor;
        }
    }
    else {
        if (body == pInstance->getEntityA()) {
            pAtom->m_factorA = factor;
        }
        else {
            pAtom->m_factorB = factor;
        }
        // A NaN w is not zero.
        if (0.0f == XFNV_M128_F32(pAtom->m_factorA.m_quad, 3) && 0.0f == XFNV_M128_F32(pAtom->m_factorB.m_quad, 3)) {
            XFNV_M128_F32(pAtom->m_factorA.m_quad, 3) = 1.0f;
            XFNV_M128_F32(pAtom->m_factorB.m_quad, 3) = 1.0f;
        }
    }
    HK_TIMER_END();
}

// As setInvMassScalingForContact with a centre of mass changer atom. Reuses the mass changer's timer name. bodyB is not
// read.
void hkpResponseModifier::setCenterOfMassDisplacementForContact(
    hkpDynamicsContactMgr* manager,
    hkpRigidBody* bodyA,
    hkpRigidBody* bodyB,
    hkpConstraintOwner& constraintOwner,
    const hkVector4& displacementA,
    const hkVector4& displacementB
) {
    hkpConstraintInstance* pInstance = manager->getConstraintInstance();
    if (!pInstance) {
        return;
    }
    constraintOwner.checkAccessRw();
    HK_TIMER_BEGIN("SetMassChang", nullptr);
    hkpCenterOfMassChangerModifierConstraintAtom* pAtom = static_cast<hkpCenterOfMassChangerModifierConstraintAtom*>(
        hkpWorldConstraintUtil::findModifier(pInstance, hkpConstraintAtom::TYPE_MODIFIER_CENTER_OF_MASS_CHANGER)
    );
    if (!pAtom) {
        pAtom = new hkpCenterOfMassChangerModifierConstraintAtom;
        hkpWorldConstraintUtil::addModifier(pInstance, constraintOwner, pAtom);
    }
    if (bodyA == pInstance->getEntityA()) {
        pAtom->m_displacementA = displacementA;
        pAtom->m_displacementB = displacementB;
    }
    else {
        pAtom->m_displacementA = displacementB;
        pAtom->m_displacementB = displacementA;
    }
    HK_TIMER_END();
}

// The SDK: unless the constraint already has one, an ignore modifier atom is made and added under ostr, which
// switches the constraint off.
void hkpResponseModifier::disableConstraint(hkpConstraintInstance* instance, hkpConstraintOwner& constraintOwner) {
    constraintOwner.checkAccessRw();
    HK_TIMER_BEGIN("DsblConstr", nullptr);
    if (!hkpWorldConstraintUtil::findModifier(instance, hkpConstraintAtom::TYPE_MODIFIER_IGNORE_CONSTRAINT)) {
        hkpIgnoreModifierConstraintAtom* pAtom = new hkpIgnoreModifierConstraintAtom;
        hkpWorldConstraintUtil::addModifier(instance, constraintOwner, pAtom);
    }
    HK_TIMER_END();
}

// The SDK: the constraint's ignore modifier is removed and deleted, and the solver results of its runtime, when it
// has one, are cleared.
void hkpResponseModifier::enableConstraint(hkpConstraintInstance* instance, hkpConstraintOwner& constraintOwner) {
    constraintOwner.checkAccessRw();
    HK_TIMER_BEGIN("EnblConstr", nullptr);
    hkpWorldConstraintUtil::removeAndDeleteModifier(
        instance,
        constraintOwner,
        hkpConstraintAtom::TYPE_MODIFIER_IGNORE_CONSTRAINT
    );
    hkpSolverResults* pResults = static_cast<hkpSolverResults*>(instance->getInternal()->m_runtime);
    if (pResults) {
        hkpConstraintData::RuntimeInfo kInfo;
        instance->getDataRw()->getRuntimeInfo(hkBool(true), kInfo);
        int32_t iNumResults = kInfo.m_numSolverResults;
        for (int32_t i = 0; i < iNumResults; i++) {
            pResults[i].m_impulseApplied = 0.0f;
            pResults[i].m_internalSolverData = 0.0f;
        }
    }
    HK_TIMER_END();
}

// The SDK: the contact's constraint gets a moving surface modifier atom (made and added under constraintOwner when it
// has none) holding velWorld, negated when body is the constraint's first entity.
void hkpResponseModifier::setSurfaceVelocity(
    hkpDynamicsContactMgr* manager,
    hkpRigidBody* body,
    hkpConstraintOwner& constraintOwner,
    const hkVector4& velWorld
) {
    hkpConstraintInstance* instance = manager->getConstraintInstance();
    if (!instance) {
        return;
    }
    constraintOwner.checkAccessRw();
    HK_TIMER_BEGIN("SetSurfVel", nullptr);
    hkVector4 velocity = velWorld;
    if (instance->m_entities[0] == body) {
        velocity.m_quad = _mm_xor_ps(hkQuadRealNegateMask, velocity.m_quad);
    }
    hkpModifierConstraintAtom* pFound = hkpWorldConstraintUtil::findModifier(
        instance,
        hkpConstraintAtom::TYPE_MODIFIER_MOVING_SURFACE
    );
    if (pFound) {
        static_cast<hkpMovingSurfaceModifierConstraintAtom*>(pFound)->m_velocity.m_quad = velocity.m_quad;
    }
    else {
        hkpMovingSurfaceModifierConstraintAtom* pAtom = new hkpMovingSurfaceModifierConstraintAtom;
        pAtom->m_velocity.m_quad = velocity.m_quad;
        hkpWorldConstraintUtil::addModifier(instance, constraintOwner, pAtom);
    }
    HK_TIMER_END();
}

// The SDK: the contact's constraint gets a soft contact modifier atom (made and added under constraintOwner when it has
// none) holding the impulse fraction and the maximum acceleration. bodyA and bodyB are not read.
void hkpResponseModifier::setImpulseScalingForContact(
    hkpDynamicsContactMgr* manager,
    hkpRigidBody* bodyA,
    hkpRigidBody* bodyB,
    hkpConstraintOwner& constraintOwner,
    float usedImpulseFraction,
    float maxAcceleration
) {
    hkpConstraintInstance* instance = manager->getConstraintInstance();
    if (!instance) {
        return;
    }
    constraintOwner.checkAccessRw();
    HK_TIMER_BEGIN("SetSoftContact", nullptr);
    hkpModifierConstraintAtom* pFound = hkpWorldConstraintUtil::findModifier(
        instance,
        hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT
    );
    if (pFound) {
        hkpSoftContactModifierConstraintAtom* pSoft = static_cast<hkpSoftContactModifierConstraintAtom*>(pFound);
        float fTau = usedImpulseFraction;
        pSoft->m_tau = fTau;
        float fMaxAcceleration = maxAcceleration;
        pSoft->m_maxAcceleration = fMaxAcceleration;
    }
    else {
        hkpSoftContactModifierConstraintAtom* pAtom = new hkpSoftContactModifierConstraintAtom;
        float fTau = usedImpulseFraction;
        pAtom->m_tau = fTau;
        float fMaxAcceleration = maxAcceleration;
        pAtom->m_maxAcceleration = fMaxAcceleration;
        hkpWorldConstraintUtil::addModifier(instance, constraintOwner, pAtom);
    }
    HK_TIMER_END();
}

// The SDK: the contact's constraint loses its moving surface modifier atom. body is not read.
void hkpResponseModifier::clearSurfaceVelocity(
    hkpDynamicsContactMgr* manager,
    hkpConstraintOwner& constraintOwner,
    hkpRigidBody* body
) {
    hkpConstraintInstance* instance = manager->getConstraintInstance();
    if (!instance) {
        return;
    }
    constraintOwner.checkAccessRw();
    HK_TIMER_BEGIN("ClrSurfVel", nullptr);
    hkpWorldConstraintUtil::removeAndDeleteModifier(
        instance,
        constraintOwner,
        hkpConstraintAtom::TYPE_MODIFIER_MOVING_SURFACE
    );
    HK_TIMER_END();
}
