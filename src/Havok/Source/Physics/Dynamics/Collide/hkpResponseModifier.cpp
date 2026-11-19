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

#include <x86intrin.h>
#include <xmmintrin.h>

// The monitor stream's timer names: the begin command ("Tt" + the name) and the end command.
using TimerName3 = const char[3];

using TimerName15 = const char[15];
static TimerName15 s_szSetMassChangTimer{"TtSetMassChang"};

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
    // EXE-INLINED none: HK_TIMER_BEGIN
    hkMonitorStream* pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
    if (pStream->m_end < pStream->m_capacityMinus16) {
        // The stream's buffer is raw memory.
        auto* pBegin = reinterpret_cast<hkMonitorStream::TimerCommand*>(pStream->m_end);
        pBegin->m_commandAndMonitor = s_szSetMassChangTimer;
        pBegin->m_time0 = static_cast<uint32_t>(__rdtsc());
        pStream->m_end = reinterpret_cast<char*>(pBegin + 1);
    }
    hkpMassChangerModifierConstraintAtom* pAtom = static_cast<hkpMassChangerModifierConstraintAtom*>(
        hkpWorldConstraintUtil::findModifier(pInstance, hkpConstraintAtom::TYPE_MODIFIER_MASS_CHANGER)
    );
    if (!pAtom) {
        // EXE-INLINED none: hkpMassChangerModifierConstraintAtom::operator new and constructor (the type and the
        // atom size only)
        void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpMassChangerModifierConstraintAtom));
        if (pMemory) {
            pAtom = static_cast<hkpMassChangerModifierConstraintAtom*>(pMemory);
            pAtom->m_type.m_storage = hkpConstraintAtom::TYPE_MODIFIER_MASS_CHANGER;
            pAtom->m_modifierAtomSize = sizeof(hkpMassChangerModifierConstraintAtom);
        }
        else {
            pAtom = nullptr;
        }
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
    // EXE-INLINED none: HK_TIMER_END
    pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
    if (pStream->m_end < pStream->m_capacityMinus16) {
        auto* pEnd = reinterpret_cast<hkMonitorStream::TimerCommand*>(pStream->m_end);
        pEnd->m_commandAndMonitor = "Et";
        pEnd->m_time0 = static_cast<uint32_t>(__rdtsc());
        pStream->m_end = reinterpret_cast<char*>(pEnd + 1);
    }
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
    // EXE-INLINED none: HK_TIMER_BEGIN
    hkMonitorStream* pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
    if (pStream->m_end < pStream->m_capacityMinus16) {
        // The stream's buffer is raw memory.
        auto* pBegin = reinterpret_cast<hkMonitorStream::TimerCommand*>(pStream->m_end);
        pBegin->m_commandAndMonitor = s_szSetMassChangTimer;
        pBegin->m_time0 = static_cast<uint32_t>(__rdtsc());
        pStream->m_end = reinterpret_cast<char*>(pBegin + 1);
    }
    hkpMassChangerModifierConstraintAtom* pAtom = static_cast<hkpMassChangerModifierConstraintAtom*>(
        hkpWorldConstraintUtil::findModifier(pInstance, hkpConstraintAtom::TYPE_MODIFIER_MASS_CHANGER)
    );
    if (!pAtom) {
        // EXE-INLINED none: hkpMassChangerModifierConstraintAtom::operator new and constructor (the type and the
        // atom size only)
        void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpMassChangerModifierConstraintAtom));
        if (pMemory) {
            pAtom = static_cast<hkpMassChangerModifierConstraintAtom*>(pMemory);
            pAtom->m_type.m_storage = hkpConstraintAtom::TYPE_MODIFIER_MASS_CHANGER;
            pAtom->m_modifierAtomSize = sizeof(hkpMassChangerModifierConstraintAtom);
        }
        else {
            pAtom = nullptr;
        }
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
        if (0.0f == pAtom->m_factorA.m_quad[3] && 0.0f == pAtom->m_factorB.m_quad[3]) {
            pAtom->m_factorA.m_quad[3] = 1.0f;
            pAtom->m_factorB.m_quad[3] = 1.0f;
        }
    }
    // EXE-INLINED none: HK_TIMER_END
    pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
    if (pStream->m_end < pStream->m_capacityMinus16) {
        auto* pEnd = reinterpret_cast<hkMonitorStream::TimerCommand*>(pStream->m_end);
        pEnd->m_commandAndMonitor = "Et";
        pEnd->m_time0 = static_cast<uint32_t>(__rdtsc());
        pStream->m_end = reinterpret_cast<char*>(pEnd + 1);
    }
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
    // EXE-INLINED none: HK_TIMER_BEGIN
    hkMonitorStream* pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
    if (pStream->m_end < pStream->m_capacityMinus16) {
        // The stream's buffer is raw memory.
        auto* pBegin = reinterpret_cast<hkMonitorStream::TimerCommand*>(pStream->m_end);
        pBegin->m_commandAndMonitor = s_szSetMassChangTimer;
        pBegin->m_time0 = static_cast<uint32_t>(__rdtsc());
        pStream->m_end = reinterpret_cast<char*>(pBegin + 1);
    }
    hkpCenterOfMassChangerModifierConstraintAtom* pAtom = static_cast<hkpCenterOfMassChangerModifierConstraintAtom*>(
        hkpWorldConstraintUtil::findModifier(pInstance, hkpConstraintAtom::TYPE_MODIFIER_CENTER_OF_MASS_CHANGER)
    );
    if (!pAtom) {
        // EXE-INLINED none: hkpCenterOfMassChangerModifierConstraintAtom::operator new and constructor (the type and
        // the atom size only)
        void* pMemory =
            hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpCenterOfMassChangerModifierConstraintAtom));
        if (pMemory) {
            pAtom = static_cast<hkpCenterOfMassChangerModifierConstraintAtom*>(pMemory);
            pAtom->m_type.m_storage = hkpConstraintAtom::TYPE_MODIFIER_CENTER_OF_MASS_CHANGER;
            pAtom->m_modifierAtomSize = sizeof(hkpCenterOfMassChangerModifierConstraintAtom);
        }
        else {
            pAtom = nullptr;
        }
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
    // EXE-INLINED none: HK_TIMER_END
    pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
    if (pStream->m_end < pStream->m_capacityMinus16) {
        auto* pEnd = reinterpret_cast<hkMonitorStream::TimerCommand*>(pStream->m_end);
        pEnd->m_commandAndMonitor = "Et";
        pEnd->m_time0 = static_cast<uint32_t>(__rdtsc());
        pStream->m_end = reinterpret_cast<char*>(pEnd + 1);
    }
}

// The SDK: unless the constraint already has one, an ignore modifier atom is made and added under ostr, which
// switches the constraint off.
void hkpResponseModifier::disableConstraint(hkpConstraintInstance* instance, hkpConstraintOwner& constraintOwner) {
    constraintOwner.checkAccessRw();
    HK_TIMER_BEGIN("DsblConstr", nullptr);
    if (!hkpWorldConstraintUtil::findModifier(instance, hkpConstraintAtom::TYPE_MODIFIER_IGNORE_CONSTRAINT)) {
        // EXE-INLINED none: hkpIgnoreModifierConstraintAtom::operator new and constructor (the type and the atom
        // size only)
        void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpIgnoreModifierConstraintAtom));
        hkpIgnoreModifierConstraintAtom* pAtom;
        if (pMemory) {
            pAtom = static_cast<hkpIgnoreModifierConstraintAtom*>(pMemory);
            pAtom->m_type.m_storage = hkpConstraintAtom::TYPE_MODIFIER_IGNORE_CONSTRAINT;
            pAtom->m_modifierAtomSize = sizeof(hkpIgnoreModifierConstraintAtom);
        }
        else {
            pAtom = nullptr;
        }
        hkpWorldConstraintUtil::addModifier(instance, constraintOwner, pAtom);
    }
    HK_TIMER_END();
}

// The SDK: the constraint's ignore modifier is removed and deleted, and the solver results of its runtime, when it
// has one, are cleared.
void hkpResponseModifier::enableConstraint(hkpConstraintInstance* instance, hkpConstraintOwner& a2) {
    a2.checkAccessRw();
    HK_TIMER_BEGIN("EnblConstr", nullptr);
    hkpWorldConstraintUtil::removeAndDeleteModifier(instance, a2, hkpConstraintAtom::TYPE_MODIFIER_IGNORE_CONSTRAINT);
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
    hkpModifierConstraintAtom* pFound =
        hkpWorldConstraintUtil::findModifier(instance, hkpConstraintAtom::TYPE_MODIFIER_MOVING_SURFACE);
    if (pFound) {
        static_cast<hkpMovingSurfaceModifierConstraintAtom*>(pFound)->m_velocity.m_quad = velocity.m_quad;
    }
    else {
        // EXE-INLINED none: hkpMovingSurfaceModifierConstraintAtom::operator new and constructor (the type and the
        // atom size only)
        void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpMovingSurfaceModifierConstraintAtom));
        hkpMovingSurfaceModifierConstraintAtom* pAtom;
        if (pMemory) {
            pAtom = static_cast<hkpMovingSurfaceModifierConstraintAtom*>(pMemory);
            pAtom->m_type.m_storage = hkpConstraintAtom::TYPE_MODIFIER_MOVING_SURFACE;
            pAtom->m_modifierAtomSize = sizeof(hkpMovingSurfaceModifierConstraintAtom);
        }
        else {
            pAtom = nullptr;
        }
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
    hkpModifierConstraintAtom* pFound =
        hkpWorldConstraintUtil::findModifier(instance, hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT);
    if (pFound) {
        hkpSoftContactModifierConstraintAtom* pSoft = static_cast<hkpSoftContactModifierConstraintAtom*>(pFound);
        float fTau = usedImpulseFraction;
        pSoft->m_tau = fTau;
        float fMaxAcceleration = maxAcceleration;
        pSoft->m_maxAcceleration = fMaxAcceleration;
    }
    else {
        // EXE-INLINED none: hkpSoftContactModifierConstraintAtom::operator new and constructor (the type, the atom
        // size and the SDK's default tau and maximum acceleration)
        void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpSoftContactModifierConstraintAtom));
        hkpSoftContactModifierConstraintAtom* pAtom;
        if (pMemory) {
            pAtom = static_cast<hkpSoftContactModifierConstraintAtom*>(pMemory);
            pAtom->m_tau = 0.1f;
            pAtom->m_type.m_storage = hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT;
            pAtom->m_maxAcceleration = 20.0f;
            pAtom->m_modifierAtomSize = sizeof(hkpSoftContactModifierConstraintAtom);
        }
        else {
            pAtom = nullptr;
        }
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
        instance, constraintOwner, hkpConstraintAtom::TYPE_MODIFIER_MOVING_SURFACE
    );
    HK_TIMER_END();
}
