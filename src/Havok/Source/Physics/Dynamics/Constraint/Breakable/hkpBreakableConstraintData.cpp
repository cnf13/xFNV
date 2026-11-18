#include "Havok/Source/Physics/Dynamics/Constraint/Breakable/hkpBreakableConstraintData.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBuildJacobianFromAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpSimpleContactConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactPointEvent.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Util/hkpConstraintCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"

#include <cmath>
#include <xmmintrin.h>

hkpBreakableConstraintData::hkpBreakableConstraintData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag) {
    if (aFlag.m_finishing) {
        m_atoms.m_bridgeAtom.init(m_atoms.m_bridgeAtom.m_constraintData);
        m_atoms.m_bridgeAtom.init(this);
    }
}

hkpBreakableConstraintData::hkpBreakableConstraintData(hkpConstraintData* constraintData)
  : m_constraintData(constraintData), m_solverResultLimit(10.0f), m_removeWhenBroken(false),
    m_revertBackVelocityOnBreak(false) {
    m_atoms.m_bridgeAtom.m_type.m_storage = 1;
    if (m_constraintData) {
        hkpConstraintData::RuntimeInfo kInfo;
        m_constraintData->addReference();
        m_constraintData->getRuntimeInfo(static_cast<hkBool>(true), kInfo);
        m_childRuntimeSize = kInfo.m_sizeOfExternalRuntime;
        m_childNumSolverResults = kInfo.m_numSolverResults;
    }
    m_atoms.m_bridgeAtom.init(this);
}

// Releases the wrapped data's reference without a null test, as the game does.
hkpBreakableConstraintData::~hkpBreakableConstraintData() {
    m_constraintData->removeReference();
}

hkBool hkpBreakableConstraintData::isValid() const {
    return m_constraintData->isValid();
}

int32_t hkpBreakableConstraintData::getType() const {
    return 12;
}

void hkpBreakableConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    (&infoOut)->m_numSolverResults = (int32_t)m_childNumSolverResults;
    (&infoOut)->m_sizeOfExternalRuntime = m_childRuntimeSize + 52;
}

// The wrapped constraint's runtime follows the child's own bytes (m_childRuntimeSize in). While the constraint holds,
// the two bodies' linear and angular velocities are saved into it, and the wrapped constraint's constraint info decides
// the Jacobian builder: a contact first atom gets the contact builder, anything else the atoms builder. Once broken,
// the Jacobian is a nop and, when the constraint removes itself on breaking, its instance leaves its world.
// hkpWorld::removeConstraint zeroes the byte it is given, so a local byte is passed.
void hkpBreakableConstraintData::buildJacobian(const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out) {
    Runtime* runtime =
        reinterpret_cast<Runtime*>(static_cast<uint8_t*>(in.m_constraintRuntime.m_storage) + m_childRuntimeSize);
    if (runtime->m_isBroken.m_bool) {
        buildNopJacobian(in, out);
        if (m_removeWhenBroken.m_bool) {
            in.m_constraintInstance->m_entities[0]->m_world->removeConstraint(in.m_constraintInstance);
        }
        return;
    }
    const hkpVelocityAccumulator* bodyA = in.m_bodyA;
    const hkpVelocityAccumulator* bodyB = in.m_bodyB;
    runtime->m_linearVelcityA[0] = bodyA->m_linearVel.m_quad[0];
    runtime->m_linearVelcityA[1] = bodyA->m_linearVel.m_quad[1];
    runtime->m_linearVelcityA[2] = bodyA->m_linearVel.m_quad[2];
    runtime->m_linearVelcityB[0] = bodyB->m_linearVel.m_quad[0];
    runtime->m_linearVelcityB[1] = bodyB->m_linearVel.m_quad[1];
    runtime->m_linearVelcityB[2] = bodyB->m_linearVel.m_quad[2];
    runtime->m_angularVelcityA[0] = bodyA->m_angularVel.m_quad[0];
    runtime->m_angularVelcityA[1] = bodyA->m_angularVel.m_quad[1];
    runtime->m_angularVelcityA[2] = bodyA->m_angularVel.m_quad[2];
    runtime->m_angularVelcityB[0] = bodyB->m_angularVel.m_quad[0];
    runtime->m_angularVelcityB[1] = bodyB->m_angularVel.m_quad[1];
    runtime->m_angularVelcityB[2] = bodyB->m_angularVel.m_quad[2];
    hkpConstraintData::ConstraintInfo kInfo;
    m_constraintData->getConstraintInfo(kInfo);
    if (kInfo.m_atoms->m_type.m_storage == hkpConstraintAtom::TYPE_CONTACT) {
        hkBool bContact = {true};
        hkSimpleContactConstraintDataBuildJacobian(
            static_cast<hkpSimpleContactConstraintAtom*>(kInfo.m_atoms), in, bContact, out
        );
    }
    else {
        hkSolverBuildJacobianFromAtomsNotContact(kInfo.m_atoms, kInfo.m_sizeOfAllAtoms, in, out);
    }
}

hkBool hkpBreakableConstraintData::isBuildJacobianCallbackRequired() const {
    return true;
}

// The wrapped constraint's solver results head the runtime, this class's Runtime follows m_childRuntimeSize bytes in.
// While the constraint holds, the squared impulses are summed, the sum rounded to float per step. Past the squared
// limit (only a strictly greater sum breaks) the constraint breaks with the sum's root, and with
// m_revertBackVelocityOnBreak each body's velocities move back toward the saved ones: saved * (1 - f) + current * f, f
// = (1 / force) * limit, each step rounded to float. The saved vectors' w is 0.0f. All saved values are loaded before
// the first store. The impulses are cleared on every path.
void hkpBreakableConstraintData::buildJacobianCallback(const hkpConstraintQueryIn& in) {
    hkpSolverResults* results = static_cast<hkpSolverResults*>(in.m_constraintRuntime.m_storage);
    // The runtime is untyped solver memory (void*); the SDK's getRuntime offsets it by the child's size.
    Runtime* runtime =
        reinterpret_cast<Runtime*>(static_cast<uint8_t*>(in.m_constraintRuntime.m_storage) + m_childRuntimeSize);
    const int32_t numResults = m_childNumSolverResults;
    if (!runtime->m_isBroken.m_bool) {
        float sumImpulseSq = 0.0f;
        for (int32_t i = 0; i < numResults; i++) {
            float impulse = results[i].m_impulseApplied;
            sumImpulseSq = impulse * impulse + sumImpulseSq;
        }
        if (sumImpulseSq > m_solverResultLimit * m_solverResultLimit) {
            float currentForce = std::sqrt(sumImpulseSq);
            setBroken(in.m_constraintInstance, static_cast<hkBool>(true), currentForce);
            if (m_revertBackVelocityOnBreak.m_bool) {
                float invForce = 1.0f / currentForce;
                float factor = invForce * m_solverResultLimit;
                hkpVelocityAccumulator* bodyA = in.m_bodyA;
                hkpVelocityAccumulator* bodyB = in.m_bodyB;
                hkVector4 linVelA;
                linVelA.set(
                    runtime->m_linearVelcityA[0], runtime->m_linearVelcityA[1], runtime->m_linearVelcityA[2], 0.0f
                );
                hkVector4 linVelB;
                linVelB.set(
                    runtime->m_linearVelcityB[0], runtime->m_linearVelcityB[1], runtime->m_linearVelcityB[2], 0.0f
                );
                hkVector4 angVelA;
                angVelA.set(
                    runtime->m_angularVelcityA[0], runtime->m_angularVelcityA[1], runtime->m_angularVelcityA[2], 0.0f
                );
                hkVector4 angVelB;
                angVelB.set(
                    runtime->m_angularVelcityB[0], runtime->m_angularVelcityB[1], runtime->m_angularVelcityB[2], 0.0f
                );
                const __m128 kT = _mm_set1_ps(factor);
                // EXE-INLINED 0x5DBF70
                bodyA->m_linearVel.m_quad = _mm_add_ps(
                    _mm_mul_ps(linVelA.m_quad, _mm_sub_ps(hkQuadReal1111_0, kT)),
                    _mm_mul_ps(bodyA->m_linearVel.m_quad, kT)
                );
                // EXE-INLINED 0x5DBF70
                bodyB->m_linearVel.m_quad = _mm_add_ps(
                    _mm_mul_ps(linVelB.m_quad, _mm_sub_ps(hkQuadReal1111_0, kT)),
                    _mm_mul_ps(bodyB->m_linearVel.m_quad, kT)
                );
                // EXE-INLINED 0x5DBF70
                bodyA->m_angularVel.m_quad = _mm_add_ps(
                    _mm_mul_ps(angVelA.m_quad, _mm_sub_ps(hkQuadReal1111_0, kT)),
                    _mm_mul_ps(bodyA->m_angularVel.m_quad, kT)
                );
                // EXE-INLINED 0x5DBF70
                bodyB->m_angularVel.m_quad = _mm_add_ps(
                    _mm_mul_ps(angVelB.m_quad, _mm_sub_ps(hkQuadReal1111_0, kT)),
                    _mm_mul_ps(kT, bodyB->m_angularVel.m_quad)
                );
            }
        }
    }
    for (int32_t i = 0; i < numResults; i++) {
        results[i].m_impulseApplied = 0.0f;
    }
}

// The wrapped constraint's info, with this constraint's bridge atom as its atoms: the SDK's
// const_cast<hkpConstraintAtom*>(m_atoms.getAtoms()) and m_atoms.getSizeOfAllAtoms() (the bridge atom alone).
void hkpBreakableConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const {
    m_constraintData->getConstraintInfo(info);
    info.m_atoms = const_cast<hkpBridgeConstraintAtom*>(&m_atoms.m_bridgeAtom);
    info.m_sizeOfAllAtoms = sizeof(hkpBridgeAtoms);
}

// The broken and repaired branches each build the event type that matches them.
void hkpBreakableConstraintData::setBroken(hkpConstraintInstance* instance, hkBool broken, float currentForce) {
    // The instance's runtime is untyped solver memory (void*); the SDK's getRuntime offsets it by the child's size.
    Runtime* pRuntime =
        reinterpret_cast<Runtime*>(static_cast<uint8_t*>(instance->m_internal->m_runtime) + m_childRuntimeSize);
    // Compares the raw value of broken against m_isBroken normalised to 0 or 1.
    if (broken.m_bool == (pRuntime->m_isBroken.m_bool != 0)) {
        return;
    }
    pRuntime->m_isBroken.m_bool = broken.m_bool;
    hkConstraintInternal* pInternal = instance->m_internal;
    // Bit 4 of the callback request: the constraint needs its build-Jacobian callback while it holds.
    if (broken.m_bool == 0) {
        pInternal->m_callbackRequest |= 4;
    }
    else {
        pInternal->m_callbackRequest &= 0xFB;
    }
    hkpWorld* pWorld = static_cast<hkpSimulationIsland*>(instance->m_owner)->m_world;
    if (broken.m_bool != 0) {
        hkpConstraintBrokenEvent kEvent;
        kEvent.m_world = pWorld;
        kEvent.m_constraintInstance = instance;
        kEvent.m_eventSource = &hkpBreakableConstraintDataClass;
        kEvent.m_eventSourceDetails = nullptr;
        kEvent.m_actualImpulse = currentForce;
        kEvent.m_impulseLimit = m_solverResultLimit;
        if (instance->m_listeners.m_size != 0) {
            hkpConstraintCallbackUtil::fireConstraintBroken(kEvent);
        }
        hkpWorldCallbackUtil::fireConstraintBroken(pWorld, kEvent);
    }
    else {
        hkpConstraintRepairedEvent kEvent;
        kEvent.m_world = pWorld;
        kEvent.m_constraintInstance = instance;
        kEvent.m_eventSource = &hkpBreakableConstraintDataClass;
        kEvent.m_eventSourceDetails = nullptr;
        if (instance->m_listeners.m_size != 0) {
            hkpConstraintCallbackUtil::fireConstraintRepaired(kEvent);
        }
        hkpWorldCallbackUtil::fireConstraintRepaired(pWorld, kEvent);
    }
}

// Runs in Havok's heap context. The new constraint is referenced before the old one is released (a null new one is not
// referenced but is still stored and queried); the two child words are stored from the runtime info with the solver
// results first, and the bridge atom is constructed over m_atoms.m_bridgeAtom, pointing at this constraint.
void hkpBreakableConstraintData::SetWrappedConstraintData(hkpConstraintData* apData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (apData) {
        apData->addReference();
    }
    if (m_constraintData) {
        m_constraintData->removeReference();
    }
    m_constraintData = apData;
    hkpConstraintData::RuntimeInfo kInfo;
    hkBool bWantRuntime = {true};
    m_constraintData->getRuntimeInfo(bWantRuntime, kInfo);
    m_childNumSolverResults = static_cast<uint16_t>(kInfo.m_numSolverResults);
    m_childRuntimeSize = static_cast<uint16_t>(kInfo.m_sizeOfExternalRuntime);
    m_atoms.m_bridgeAtom.init(this);
}

void hkpBreakableConstraintData::SetWrappedConstraintData_0(hkpConstraintData* apData) {
    if (apData) {
        apData->addReference();
    }
    hkpConstraintData* m_constraintData = this->m_constraintData;
    if (m_constraintData) {
        m_constraintData->removeReference();
    }
    this->m_constraintData = apData;
    if (apData) {
        hkpConstraintData::RuntimeInfo v5;
        this->m_constraintData->getRuntimeInfo(static_cast<hkBool>(true), v5);
        uint16_t m_numSolverResults = v5.m_numSolverResults;
        m_childRuntimeSize = v5.m_sizeOfExternalRuntime;
        m_childNumSolverResults = m_numSolverResults;
    }
}

// The SDK's hkBeginConstraints with no solver results (stride 8).
void hkpBreakableConstraintData::buildNopJacobian(const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out) {
    hkBeginConstraints(&in, &out, nullptr, 8);
}
