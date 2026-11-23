#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpSimpleContactConstraintData.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpSimpleContactConstraintInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/hkpConstraintQueryIn.h"
#include "Havok/Source/Physics/ConstraintSolver/SimpleConstraints/hkpSimpleConstraintUtil.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolve.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpCollisionEvent.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactPointEvent.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpSimpleConstraintContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Atom/hkpConstraintAtomUtil.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpContactImpulseLimitBreachedListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpDynamicsCpIdMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpSimpleContactConstraintUtil.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Response/hkpSimpleCollisionResponse.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityCallbackUtil.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldConstraintUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCallbackUtil.h"

hkpSimpleContactConstraintData::hkpSimpleContactConstraintData(
    hkpConstraintInstance* constraint,
    hkpRigidBody* bodyA,
    hkpRigidBody* bodyB
)
  : m_constraint(constraint) {
    const int32_t iKeysA = bodyA->m_numShapeKeysInContactPointProperties;
    const int32_t iKeysB = bodyB->m_numShapeKeysInContactPointProperties;
    m_atom = hkpSimpleContactConstraintAtomUtil::allocateAtom(
        1,
        iKeysA,
        iKeysB,
        static_cast<uint16_t>(0x3400 / (4 * (iKeysB + iKeysA) + 52))
    );
    m_atom->m_info.m_flags = 0;
    m_atom->m_info.m_index = 3;
    for (uint32_t& word : m_atom->m_info.m_data) {
        word = 0;
    }
    m_atomSize = m_atom->m_sizeOfAllAtoms;
}

// The atom's block freed; the compiler frees m_idMgrA's value array.
hkpSimpleContactConstraintData::~hkpSimpleContactConstraintData() {
    if (m_atom) {
        const int32_t iAtomSize = hkpSimpleContactConstraintAtomUtil::getAllocatedAtomSize(m_atom->m_sizeOfAllAtoms);
        hkMemoryRouter::getInstance().m_heap->blockFree(m_atom, iAtomSize);
    }
}

hkBool hkpSimpleContactConstraintData::isValid() const {
    return 1;
}

int32_t hkpSimpleContactConstraintData::getType() const {
    return 11;
}

void hkpSimpleContactConstraintData::getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const {
    infoOut.m_sizeOfExternalRuntime = 0;
    infoOut.m_numSolverResults = 0;
}

hkpSolverResults* hkpSimpleContactConstraintData::getSolverResults(void* runtime) {
    return nullptr;
}

void hkpSimpleContactConstraintData::getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const {
    hkpConstraintData::getConstraintInfoUtil(m_atom, m_atom->m_sizeOfAllAtoms, infoOut);
}

// Calls the collision response begin callback of each modifier atom of the constraint, outermost first; modifier
// types without one are passed over.
void hkpSimpleContactConstraintData::collisionResponseBeginCallback(
    const hkContactPoint& cp,
    hkpSimpleConstraintInfoInitInput& inA,
    hkpBodyVelocity& velA,
    hkpSimpleConstraintInfoInitInput& inB,
    hkpBodyVelocity& velB
) {
    hkpConstraintAtom* pAtom = m_constraint->m_constraintModifiers;
    if (!pAtom) {
        return;
    }
    while (pAtom->isModifierType()) {
        hkpModifierConstraintAtom* pModifier = static_cast<hkpModifierConstraintAtom*>(pAtom);
        switch (pModifier->m_type.m_storage) {
            case hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT:
                static_cast<hkpSoftContactModifierConstraintAtom*>(pModifier)
                    ->collisionResponseBeginCallback(cp, inA, velA, inB, velB);
                break;
            case hkpConstraintAtom::TYPE_MODIFIER_MASS_CHANGER:
                static_cast<hkpMassChangerModifierConstraintAtom*>(pModifier)
                    ->collisionResponseBeginCallback(cp, inA, velA, inB, velB);
                break;
            case hkpConstraintAtom::TYPE_MODIFIER_MOVING_SURFACE:
                static_cast<hkpMovingSurfaceModifierConstraintAtom*>(pModifier)
                    ->collisionResponseBeginCallback(cp, inA, velA, inB, velB);
                break;
            case hkpConstraintAtom::TYPE_MODIFIER_CENTER_OF_MASS_CHANGER:
                static_cast<hkpCenterOfMassChangerModifierConstraintAtom*>(pModifier)
                    ->collisionResponseBeginCallback(cp, inA, velA, inB, velB);
                break;
            default: break;
        }
        pAtom = pModifier->m_child;
    }
}

// The constraint's modifiers are collected outermost first, then called back innermost first, so that each undoes
// its collisionResponseBeginCallback change in the reverse order. Modifier types without an end callback are passed
// over.
void hkpSimpleContactConstraintData::collisionResponseEndCallback(
    const hkContactPoint& cp,
    float impulseApplied,
    hkpSimpleConstraintInfoInitInput& inA,
    hkpBodyVelocity& velA,
    hkpSimpleConstraintInfoInitInput& inB,
    hkpBodyVelocity& velB
) {
    hkpConstraintInstance* pConstraint = m_constraint;
    if (!pConstraint->m_constraintModifiers) {
        return;
    }
    using modifiers_t = hkInplaceArray<hkpModifierConstraintAtom*, 16>;
    modifiers_t modifiers;
    hkpConstraintAtom* pAtom = pConstraint->m_constraintModifiers;
    while (pAtom->isModifierType()) {
        hkpModifierConstraintAtom* pModifier = static_cast<hkpModifierConstraintAtom*>(pAtom);
        modifiers.pushBack(pModifier);
        pAtom = pModifier->m_child;
    }
    for (int32_t i = modifiers.m_size - 1; i >= 0; --i) {
        hkpModifierConstraintAtom* pModifier = modifiers.m_data[i];
        switch (pModifier->m_type.m_storage) {
            case hkpConstraintAtom::TYPE_MODIFIER_SOFT_CONTACT:
                static_cast<hkpSoftContactModifierConstraintAtom*>(pModifier)
                    ->collisionResponseEndCallback(cp, impulseApplied, inA, velA, inB, velB);
                break;
            case hkpConstraintAtom::TYPE_MODIFIER_MASS_CHANGER:
                static_cast<hkpMassChangerModifierConstraintAtom*>(pModifier)
                    ->collisionResponseEndCallback(cp, impulseApplied, inA, velA, inB, velB);
                break;
            case hkpConstraintAtom::TYPE_MODIFIER_MOVING_SURFACE:
                static_cast<hkpMovingSurfaceModifierConstraintAtom*>(pModifier)
                    ->collisionResponseEndCallback(cp, impulseApplied, inA, velA, inB, velB);
                break;
            case hkpConstraintAtom::TYPE_MODIFIER_CENTER_OF_MASS_CHANGER:
                static_cast<hkpCenterOfMassChangerModifierConstraintAtom*>(pModifier)
                    ->collisionResponseEndCallback(cp, impulseApplied, inA, velA, inB, velB);
                break;
            default: break;
        }
    }
}

// Returns the contact manager this data is embedded in: steps back by the offset of the manager's
// m_contactConstraintData (hkAddByteOffset of -HK_OFFSET_OF), over memory treated as raw bytes.
// KEEP: the caller hkSimpleContactConstraintData_fireCallbacks sets a byte before the call and stores it after without
// reloading it, relying on this function changing nothing but its return value.
hkpSimpleConstraintContactMgr* hkpSimpleContactConstraintData::getSimpleConstraintContactMgr() {
    return reinterpret_cast<hkpSimpleConstraintContactMgr*>(
        hkAddByteOffset(this, -static_cast<int32_t>(offsetof(hkpSimpleConstraintContactMgr, m_contactConstraintData)))
    );
}

// Adds a contact point at the end of the atom (expanding it, and telling the world when it moved) with fresh
// properties, grows the constraint's solver sizes by its schema and returns its new id.
uint16_t hkpSimpleContactConstraintData::allocateContactPoint(
    hkpConstraintOwner& constraintOwner,
    hkContactPoint** cpOut,
    hkpContactPointProperties** cpPropsOut
) {
    m_atom->m_info.m_flags |= hkpSimpleContactConstraintDataInfo::HK_FLAG_AREA_CHANGED;
    int32_t iNumContactPoints = m_atom->m_numContactPoints;
    hkpSimpleContactConstraintAtom* pOldAtom = m_atom;
    hkPadSpu<hkBool> atomReallocated;
    atomReallocated.m_storage.m_bool = 0;
    hkpSimpleContactConstraintAtom* pAtom = hkpSimpleContactConstraintAtomUtil::expandOne(pOldAtom, atomReallocated);
    m_atom = pAtom;
    hkpContactPointProperties* pProps = pAtom->getContactPointPropertiesStream(iNumContactPoints)->asProperties();
    pProps->m_impulseApplied = 0.0f;
    pProps->m_internalSolverData = 0.0f;
    pProps->m_flags = hkContactPointMaterial::CONTACT_IS_NEW;
    if (iNumContactPoints > 0) {
        hkpContactPointProperties* pPrevProps = pAtom->getContactPointPropertiesStream(iNumContactPoints - 1)
                                                    ->asProperties();
        if (!(pPrevProps->m_flags & hkContactPointMaterial::CONTACT_USES_SOLVER_PATH2) &&
            pPrevProps->m_maxImpulse.m_value == 0) {
            pProps->m_flags = hkContactPointMaterial::CONTACT_IS_NEW |
                hkContactPointMaterial::CONTACT_USES_SOLVER_PATH2;
        }
    }
    *cpOut = pAtom->getContactPoints() + iNumContactPoints;
    *cpPropsOut = pProps;

    hkpConstraintInfo delta;
    int32_t iNumAfter = iNumContactPoints + 1;
    int32_t iOdd = iNumAfter & 1;
    // The header, the points in pairs and a single one, and the 3D friction schema (hkpSolve.h's sizes).
    delta.m_maxSizeOfSchema = HK_SIZE_OF_JACOBIAN_PAIR_CONTACT_SCHEMA * (iNumAfter >> 1) +
        HK_SIZE_OF_JACOBIAN_SINGLE_CONTACT_SCHEMA * iOdd + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA +
        HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA;
    delta.m_numSolverResults = 0;
    delta.m_numSolverElemTemps = 0;
    int32_t iSizeOfSchemas = 0;
    if (iNumContactPoints == 1) {
        // The second point turns the 2D friction schema into the 3D one.
        iSizeOfSchemas = HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA - HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA;
        delta.m_numSolverResults = 1;
        delta.m_numSolverElemTemps = 1;
    }
    ++delta.m_numSolverResults;
    if (iOdd) {
        iSizeOfSchemas += 48;
    }
    else {
        iSizeOfSchemas += 64;
    }
    ++delta.m_numSolverElemTemps;
    delta.m_sizeOfSchemas = iSizeOfSchemas;
    constraintOwner.addConstraintInfo(m_constraint, delta);
    if (atomReallocated.m_storage.m_bool) {
        hkpWorldConstraintUtil::updateFatherOfMovedAtom(m_constraint, pOldAtom, m_atom, pAtom->m_sizeOfAllAtoms);
        m_atomSize = pAtom->m_sizeOfAllAtoms;
    }
    return m_idMgrA.newId(iNumContactPoints);
}

// Fires the contact point callbacks of the constraint's atom: the world's contact listeners and the bodies' own, for
// the given step. New points are then solved: by a single collision response when they
// separate faster than the solver's resting velocity and bounce enough, else through the solver's restitution data.
void hkSimpleContactConstraintData_fireCallbacks(
    hkpSimpleContactConstraintData* constraintData,
    const hkpConstraintQueryIn* in,
    hkpSimpleContactConstraintAtom* atom,
    hkpContactPointEvent::Type type
) {
    hkpConstraintInstance* pConstraint = in->m_constraintInstance;
    hkpRigidBody* pBodyB = static_cast<hkpRigidBody*>(pConstraint->m_entities[1]);
    hkpRigidBody* pBodyA = static_cast<hkpRigidBody*>(pConstraint->m_entities[0]);
    hkpWorld* pWorld = pBodyA->m_world;
    hkpVelocityAccumulator* pAccA = in->m_bodyA;
    hkpVelocityAccumulator* pAccB = in->m_bodyB;
    uint8_t bFullManifold = (pConstraint->getInternal()->m_callbackRequest &
                             hkpConstraintAtom::CALLBACK_REQUEST_CONTACT_POINT_CALLBACK) != 0;
    int32_t iStriding = atom->getContactPointPropertiesStriding();
    // The properties stream is the atom's untyped memory, stepped through by its striding.
    char* pPropsBytes = reinterpret_cast<char*>(atom->getContactPointPropertiesStream(0));
    int32_t iLast = atom->m_numContactPoints - 1;
    hkContactPoint* pCp = atom->getContactPoints();
    hkpContactPointProperties* pPrevProps = nullptr;
    for (int32_t i = iLast; i >= 0; --i) {
        hkpContactPointProperties* pProps = reinterpret_cast<hkpContactPointPropertiesStream*>(pPropsBytes)
                                                ->asProperties();
        if (pProps->m_flags & hkContactPointMaterial::CONTACT_IS_NEW) {
            float fSeparatingVelocity = hkpSimpleContactConstraintUtil::calculateSeparatingVelocity(
                pBodyA,
                pBodyB,
                in->m_bodyA->m_scratch0,
                in->m_bodyB->m_scratch0,
                pCp
            );
            hkpContactPointEvent
                event(hkpCollisionEvent::SOURCE_WORLD, pBodyA, pBodyB, constraintData->getSimpleConstraintContactMgr());
            event.m_type = type;
            event.m_contactPoint = pCp;
            event.m_contactPointProperties = pProps;
            event.m_firingCallbacksForFullManifold.m_bool = bFullManifold;
            event.m_firstCallbackForFullManifold.m_bool = i == iLast;
            event.m_lastCallbackForFullManifold.m_bool = i == 0;
            event.m_separatingVelocity = &fSeparatingVelocity;
            event.m_rotateNormal = nullptr;
            event.m_shapeKeyStorage = pProps->getStartOfExtendedUserData(atom);
            event.m_accumulators[0] = pAccA;
            event.m_accumulators[1] = pAccB;
            hkpWorldCallbackUtil::fireContactPointCallback(pWorld, event);
            event.m_source = hkpCollisionEvent::SOURCE_A;
            if (pBodyA->areContactListenersAdded()) {
                hkpEntityCallbackUtil::fireContactPointCallbackInternal(pBodyA, event);
            }
            event.m_source = hkpCollisionEvent::SOURCE_B;
            if (pBodyB->areContactListenersAdded()) {
                hkpEntityCallbackUtil::fireContactPointCallbackInternal(pBodyB, event);
            }
            if (i < iLast && pPrevProps->m_maxImpulse.m_value != 0) {
                pProps->m_flags &= ~hkContactPointMaterial::CONTACT_USES_SOLVER_PATH2;
            }

            if (pProps->m_flags & hkContactPointMaterial::CONTACT_IS_DISABLED) {
                pCp->m_separatingNormal(3) = 1.70141e+38f;
                pProps->m_impulseApplied = 0.0f;
                pProps->m_internalSolverData = 0.0f;
                pProps->m_internalDataA = 0.0f;
                pProps->m_flags &= ~hkContactPointMaterial::CONTACT_IS_NEW;
            }
            else {
                float fRestitution = static_cast<int32_t>(pProps->m_restitution) * 0.0078125;
                const hkpWorldDynamicsStepInfo* pStepInfo = static_cast<const hkpWorldDynamicsStepInfo*>(
                    pWorld->m_collisionInput->m_dynamicsInfo
                );
                if (fSeparatingVelocity < -pStepInfo->m_solverInfo.m_contactRestingVelocity && fRestitution > 0.3f) {
                    hkpSimpleConstraintUtilCollideParams params;
                    params.m_friction = pProps->m_friction;
                    params.m_restitution = fRestitution;
                    params.m_extraSeparatingVelocity = 0.0f;
                    params.m_extraSlope = 0.0f;
                    params.m_externalSeperatingVelocity = fSeparatingVelocity;
                    params.m_maxImpulse = HK_REAL_MAX;
                    if (pProps->m_maxImpulse.m_value) {
                        params.m_maxImpulse = pProps->m_maxImpulse;
                    }
                    // Starts clear (a departure): when the collide helper returns early (hkpProcessFlyingColors
                    // fails) it applies no impulse and leaves the flag unwritten, and the exe reads whatever the
                    // stack held.
                    params.m_contactImpulseLimitBreached.m_bool = 0;
                    hkpSimpleCollisionResponse::SolveSingleOutput2 output;
                    hkpSimpleCollisionResponse::solveSingleContact2(
                        constraintData,
                        *pCp,
                        params,
                        pBodyA,
                        pBodyB,
                        in->m_bodyA,
                        in->m_bodyB,
                        output
                    );
                    if (params.m_contactImpulseLimitBreached.m_bool) {
                        hkpContactImpulseLimitBreachedListenerInfo breachedContact;
                        breachedContact.set(in->m_constraintInstance, pProps, pCp, false);
                        hkpWorldCallbackUtil::fireContactImpulseLimitBreached(pWorld, &breachedContact, 1);
                    }
                    pProps->m_impulseApplied = 0.0f;
                    pProps->m_internalDataA = 0.0f;
                    pProps->m_flags &= ~hkContactPointMaterial::CONTACT_IS_NEW;
                }
                else {
                    float fInvMassA = pBodyA->m_motion.m_inertiaAndMassInv(3);
                    float fInvMassB = pBodyB->m_motion.m_inertiaAndMassInv(3);
                    float fSumInvMass = fInvMassB + fInvMassA;
                    float fMass = 1.0 / (fSumInvMass + 1.000000013351432e-10);
                    pProps->m_impulseApplied = fMass * -0.20000000298023224 * fSeparatingVelocity *
                        (1.0 + fRestitution);
                    float fScaled = fSeparatingVelocity * fRestitution * in->m_substepDeltaTime;
                    float fSolverData = fScaled * -1.2999999523162842;
                    pProps->m_internalSolverData = fSolverData;
                    if (fRestitution > 0.0f) {
                        pProps->m_internalDataA = fSolverData + pCp->m_separatingNormal(3);
                    }
                    else {
                        pProps->m_internalDataA = 0.0f;
                    }
                    pProps->m_flags &= ~hkContactPointMaterial::CONTACT_IS_NEW;
                }
            }
        }
        else if (bFullManifold) {
            hkpContactPointEvent
                event(hkpCollisionEvent::SOURCE_WORLD, pBodyA, pBodyB, constraintData->getSimpleConstraintContactMgr());
            event.m_type = type;
            event.m_contactPoint = pCp;
            event.m_contactPointProperties = pProps;
            event.m_firingCallbacksForFullManifold.m_bool = 1;
            event.m_firstCallbackForFullManifold.m_bool = i == iLast;
            event.m_lastCallbackForFullManifold.m_bool = i == 0;
            event.m_separatingVelocity = nullptr;
            event.m_rotateNormal = nullptr;
            event.m_shapeKeyStorage = pProps->getStartOfExtendedUserData(atom);
            event.m_accumulators[0] = pAccA;
            event.m_accumulators[1] = pAccB;
            hkpWorldCallbackUtil::fireContactPointCallback(pWorld, event);
            event.m_source = hkpCollisionEvent::SOURCE_A;
            if (pBodyA->areContactListenersAdded()) {
                hkpEntityCallbackUtil::fireContactPointCallbackInternal(pBodyA, event);
            }
            event.m_source = hkpCollisionEvent::SOURCE_B;
            if (pBodyB->areContactListenersAdded()) {
                hkpEntityCallbackUtil::fireContactPointCallbackInternal(pBodyB, event);
            }
            if (i < iLast && pPrevProps->m_maxImpulse.m_value != 0) {
                pProps->m_flags &= ~hkContactPointMaterial::CONTACT_USES_SOLVER_PATH2;
            }
        }
        ++pCp;
        pPrevProps = pProps;
        pPropsBytes += iStriding;
    }
    pConstraint->getInternal()->m_callbackRequest &= ~(
        hkpConstraintAtom::CALLBACK_REQUEST_NEW_CONTACT_POINT |
        hkpConstraintAtom::CALLBACK_REQUEST_CONTACT_POINT_CALLBACK
    );
}

int32_t hkpDynamicsCpIdMgr::newId(int32_t value) {
    int32_t result = m_values.m_size - 1;
    if (result >= 0) {
        uint8_t* m_data = m_values.begin();
        while (true) {
            if (m_data[result] == 0xFF) {
                m_data[result] = value;
                return result;
            }
            if (--result < 0) {
                break;
            }
        }
    }
    m_values.pushBack(static_cast<uint8_t>(value));
    return m_values.m_size - 1;
}
