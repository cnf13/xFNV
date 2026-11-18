#include "Havok/Source/Physics/Utilities/Constraint/Bilateral/hkpConstraintUtils.h"

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/BallAndSocket/hkpBallAndSocketConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Hinge/hkpHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Ragdoll/hkpRagdollConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/HingeLimits/hkpHingeLimitsData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/RagdollLimits/hkpRagdollLimitsData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

#include <xmmintrin.h>

// Copies the atoms r_eax into r_esi, then takes a reference on the copied angular motor. Used by
// hkpConstraintUtils::convertToPowered.
static void
CopyAtomsAddMotorRef(hkpLimitedHingeConstraintData::Atoms* r_esi, const hkpLimitedHingeConstraintData::Atoms* r_eax);

// The atoms are copied from r_eax (operator=), then a reference is taken on the copied angular motor.
static void
CopyAtomsAddMotorRef(hkpLimitedHingeConstraintData::Atoms* r_esi, const hkpLimitedHingeConstraintData::Atoms* r_eax) {
    r_esi->operator=(*r_eax);
    hkpConstraintMotor* pMotor = r_esi->m_angMotor.m_motor;
    if (pMotor) {
        pMotor->addReference();
    }
}

// When the constraint's pivots in world space are further apart than maxAllowedError, the child body (A) is moved to
// the point relativeDisplacementTowardsParent of the way from the parent's (B) pivot to the parent's centre and given
// the parent's velocities; a child pivot off the body's origin only warns. The hkBool result goes through apResult,
// which is returned.
hkBool* hkpConstraintUtils::checkAndFixConstraint(
    hkBool* apResult,
    const hkpConstraintInstance* constraint,
    float maxAllowedError,
    float relativeDisplacementTowardsParent
) {
    hkVector4 pivotA;
    hkVector4 pivotB;
    if (getConstraintPivots(constraint, pivotA, pivotB) != HK_SUCCESS) {
        apResult->m_bool = false;
        return apResult;
    }
    // EXE-INLINED 0x630100: hkVector4::lengthSquared3
    const __m128 pivotSq = _mm_mul_ps(pivotA.m_quad, pivotA.m_quad);
    const float fPivotLenSq = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(pivotSq, pivotSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(pivotSq, pivotSq, 0x55), pivotSq))
    );
    if (fPivotLenSq > 1e-06f) {
        alignas(16) char buf[512];
        alignas(16) hkErrStream stream(buf, 512);
        stream << "Pivot of child rigid body (A) is expected to be aligned with the constraint at setup time.";
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA5DFFu), buf,
            ".\\Constraint\\Bilateral\\hkpConstraintUtils.cpp", 285
        );
    }
    hkpEntity* pParent = constraint->m_entities[1];
    hkpRigidBody* pChild = static_cast<hkpRigidBody*>(constraint->m_entities[0]);
    hkVector4 pivotBWorld;
    pivotBWorld.setTransformedPos(pParent->m_motion.m_motionState.m_transform, pivotB);
    hkVector4 pivotAWorld;
    pivotAWorld.setTransformedPos(pChild->m_motion.m_motionState.m_transform, pivotA);
    // EXE-INLINED 0x630100: hkVector4::lengthSquared3
    const __m128 diff = _mm_sub_ps(pivotBWorld.m_quad, pivotAWorld.m_quad);
    const __m128 diffSq = _mm_mul_ps(diff, diff);
    const float fDistSq = _mm_cvtss_f32(
        _mm_add_ss(_mm_shuffle_ps(diffSq, diffSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(diffSq, diffSq, 0x55), diffSq))
    );
    // The squared error stays on the x87 stack.
    if (!(fDistSq > maxAllowedError * maxAllowedError)) {
        apResult->m_bool = false;
        return apResult;
    }
    const __m128 t = _mm_set1_ps(relativeDisplacementTowardsParent);
    hkVector4 newPosition;
    newPosition.m_quad = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, t), pivotBWorld.m_quad),
        _mm_mul_ps(pParent->m_motion.m_motionState.m_transform.m_translation.m_quad, t)
    );
    pChild->setPosition(newPosition);
    pChild->setLinearVelocity(pParent->m_motion.m_linearVelocity);
    pChild->setAngularVelocity(pParent->m_motion.m_angularVelocity);
    apResult->m_bool = true;
    return apResult;
}

// A limited hinge's or ragdoll's data is copied into new data of its type, with constraintMotor set on its motors and
// the motors enabled as asked, in a new instance on the same entities with the same priority and name; any other type
// warns twice and gives null. Reads the entities before the data's type.
hkpConstraintInstance* hkpConstraintUtils::convertToPowered(
    const hkpConstraintInstance* originalConstraint,
    hkpConstraintMotor* constraintMotor,
    hkBool enableMotors
) {
    hkpEntity* pEntityA = originalConstraint->m_entities[0];
    hkpEntity* pEntityB = originalConstraint->m_entities[1];
    hkpConstraintData* pOldData = originalConstraint->getDataRw();
    hkpConstraintData* pNewData;
    hkpConstraintInstance* pNewConstraint;
    switch (pOldData->getType()) {
        case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE: {
            hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
            hkpLimitedHingeConstraintData* pMemory = static_cast<hkpLimitedHingeConstraintData*>(
                pRouter->m_heap->blockAlloc(sizeof(hkpLimitedHingeConstraintData))
            );
            pMemory->m_memSizeAndFlags = sizeof(hkpLimitedHingeConstraintData);
            hkpLimitedHingeConstraintData* pNew = ::new (pMemory) hkpLimitedHingeConstraintData;
            CopyAtomsAddMotorRef(&pNew->m_atoms, &static_cast<hkpLimitedHingeConstraintData*>(pOldData)->m_atoms);
            pNew->setMotor(constraintMotor);
            pNew->setMotorActive(nullptr, enableMotors);
            hkMemoryRouter* pRouter2 = &hkMemoryRouter::getInstance();
            hkpConstraintInstance* pInstance =
                static_cast<hkpConstraintInstance*>(pRouter2->m_heap->blockAlloc(sizeof(hkpConstraintInstance)));
            pInstance->m_memSizeAndFlags = sizeof(hkpConstraintInstance);
            pNewConstraint =
                ::new (pInstance) hkpConstraintInstance(pEntityA, pEntityB, pNew, originalConstraint->getPriority());
            pNewData = pNew;
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL: {
            hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
            hkpRagdollConstraintData* pMemory =
                static_cast<hkpRagdollConstraintData*>(pRouter->m_heap->blockAlloc(sizeof(hkpRagdollConstraintData)));
            pMemory->m_memSizeAndFlags = sizeof(hkpRagdollConstraintData);
            hkpRagdollConstraintData* pNew = ::new (pMemory) hkpRagdollConstraintData;
            pNew->m_atoms.CopyAndAddMotorRefs(static_cast<hkpRagdollConstraintData*>(pOldData)->m_atoms);
            pNew->setTwistMotor(constraintMotor);
            pNew->setPlaneMotor(constraintMotor);
            pNew->setConeMotor(constraintMotor);
            pNew->setMotorsActive(nullptr, enableMotors);
            hkMemoryRouter* pRouter2 = &hkMemoryRouter::getInstance();
            hkpConstraintInstance* pInstance =
                static_cast<hkpConstraintInstance*>(pRouter2->m_heap->blockAlloc(sizeof(hkpConstraintInstance)));
            pInstance->m_memSizeAndFlags = sizeof(hkpConstraintInstance);
            pNewConstraint =
                ::new (pInstance) hkpConstraintInstance(pEntityA, pEntityB, pNew, originalConstraint->getPriority());
            pNewData = pNew;
            break;
        }
        default: {
            XFNV_INLINED_LOCAL(hkErrStream, stream, std::nullopt, std::nullopt);
            char buf[512];
            ::new (&stream) hkErrStream(buf, 512);
            stream << "Cannot convert constraint \"" << originalConstraint->getName() << "\" to a powered constraint.";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA1B34u), buf,
                ".\\Constraint\\Bilateral\\hkpConstraintUtils.cpp", 111
            );
            stream.~hkErrStream();
            ::new (&stream) hkErrStream(buf, 512);
            stream << "Only limited hinges and ragdoll constraints can be powered.";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA1B34u), buf,
                ".\\Constraint\\Bilateral\\hkpConstraintUtils.cpp", 112
            );
            stream.~hkErrStream();
            return nullptr;
        }
    }
    pNewData->removeReference();
    pNewConstraint->m_name = originalConstraint->getName();
    return pNewConstraint;
}

// The SDK's getConstraintMotors: a limited hinge's angular motor, or a ragdoll's twist, cone and plane motors; any
// other type gets three nulls, a warning and HK_FAILURE.
hkResult hkpConstraintUtils::getConstraintMotors(
    const hkpConstraintData* constraintData,
    hkpConstraintMotor*& motor0,
    hkpConstraintMotor*& motor1,
    hkpConstraintMotor*& motor2
) {
    switch (constraintData->getType()) {
        case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE: {
            motor0 = static_cast<const hkpLimitedHingeConstraintData*>(constraintData)->m_atoms.m_angMotor.m_motor;
            motor1 = nullptr;
            motor2 = nullptr;
            return HK_SUCCESS;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL: {
            const hkpRagdollConstraintData* pRagdoll = static_cast<const hkpRagdollConstraintData*>(constraintData);
            motor0 = pRagdoll->getTwistMotor();
            motor1 = pRagdoll->getConeMotor();
            motor2 = pRagdoll->getPlaneMotor();
            return HK_SUCCESS;
        }
        default: {
            char buf[512];
            motor2 = nullptr;
            motor1 = nullptr;
            motor0 = nullptr;
            hkErrStream stream(buf, 512);
            stream << "This type of constraint does not have motors";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAE233u), buf,
                ".\\Constraint\\Bilateral\\hkpConstraintUtils.cpp", 260
            );
            return HK_FAILURE;
        }
    }
}

// The pivots in each body's space of a ball and socket, hinge, limited hinge or ragdoll constraint; any other type
// warns and gives HK_FAILURE.
hkResult hkpConstraintUtils::getConstraintPivots(
    const hkpConstraintInstance* constraint,
    hkVector4& pivotInAOut,
    hkVector4& pivotInBOut
) {
    hkpConstraintData* pData = constraint->getDataRw();
    switch (pData->getType()) {
        case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET: {
            const hkpBallAndSocketConstraintData* pBall = static_cast<const hkpBallAndSocketConstraintData*>(pData);
            pivotInAOut.m_quad = pBall->m_atoms.m_pivots.m_translationA.m_quad;
            pivotInBOut.m_quad = pBall->m_atoms.m_pivots.m_translationB.m_quad;
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_HINGE: {
            const hkpHingeConstraintData* pHinge = static_cast<const hkpHingeConstraintData*>(pData);
            pivotInAOut.m_quad = pHinge->m_atoms.m_transforms.m_transformA.m_translation.m_quad;
            pivotInBOut.m_quad = pHinge->m_atoms.m_transforms.m_transformB.m_translation.m_quad;
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE: {
            const hkpLimitedHingeConstraintData* pHinge = static_cast<const hkpLimitedHingeConstraintData*>(pData);
            pivotInAOut.m_quad = pHinge->m_atoms.m_transforms.m_transformA.m_translation.m_quad;
            pivotInBOut.m_quad = pHinge->m_atoms.m_transforms.m_transformB.m_translation.m_quad;
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL: {
            const hkpRagdollConstraintData* pRagdoll = static_cast<const hkpRagdollConstraintData*>(pData);
            pivotInAOut.m_quad = pRagdoll->m_atoms.m_transforms.m_transformA.m_translation.m_quad;
            pivotInBOut.m_quad = pRagdoll->m_atoms.m_transforms.m_transformB.m_translation.m_quad;
            break;
        }
        default: {
            alignas(16) char buf[512];
            hkErrStream stream(buf, 512);
            stream << "Unsupported type of constraint in prepareSystemForRagdoll()";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBABF3Bu), buf,
                ".\\Constraint\\Bilateral\\hkpConstraintUtils.cpp", 226
            );
            return HK_FAILURE;
        }
    }
    return HK_SUCCESS;
}

// For a hinge or limited hinge only: weights each body by the sum of its local inverse inertia's yz and zz, yy and
// zy entries in the frame of its constraint space (over its inverse mass when that exceeds the float epsilon), then
// slides each pivot along its hinge axis (column 0 of its rotation) so that both pivots' distances from their centres
// of mass along the axis are shared out by those weights.
void hkpConstraintUtils::setHingePivotToOptimalPosition(hkpConstraintInstance* constraint) {
    hkpConstraintData* data = constraint->getDataRw();
    const int32_t type = data->getType();
    if (type != hkpConstraintData::CONSTRAINT_TYPE_HINGE && type != hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE) {
        return;
    }
    hkpRigidBody* bodyA = constraint->getRigidBodyA();
    hkpRigidBody* bodyB = constraint->getRigidBodyB();
    // Both data classes hold their transforms atom first in their atoms, at the same offset.
    hkpSetLocalTransformsConstraintAtom& transforms = type == hkpConstraintData::CONSTRAINT_TYPE_HINGE
        ? static_cast<hkpHingeConstraintData*>(data)->m_atoms.m_transforms
        : static_cast<hkpLimitedHingeConstraintData*>(data)->m_atoms.m_transforms;

    hkMatrix3 inertiaA;
    bodyA->getInertiaInvLocal(inertiaA);
    hkRotation rotationA = transforms.m_transformA.getRotation();
    rotationA.transpose();
    inertiaA.changeBasis(rotationA);
    const float inertiaA2 = inertiaA.m_col2(2) + inertiaA.m_col2(1);
    const float inertiaA1 = inertiaA.m_col1(2) + inertiaA.m_col1(1);
    float weightA = inertiaA2 + inertiaA1;
    // EXE-INLINED 0x458700
    if (_mm_cvtss_f32(_mm_shuffle_ps(
            bodyA->getRigidMotion()->m_inertiaAndMassInv.m_quad, bodyA->getRigidMotion()->m_inertiaAndMassInv.m_quad,
            0xFF
        )) > 1.1920929e-07f) {
        // EXE-INLINED 0x458700
        weightA = weightA /
            _mm_cvtss_f32(_mm_shuffle_ps(
                bodyA->getRigidMotion()->m_inertiaAndMassInv.m_quad,
                bodyA->getRigidMotion()->m_inertiaAndMassInv.m_quad, 0xFF
            ));
    }

    hkMatrix3 inertiaB;
    bodyB->getInertiaInvLocal(inertiaB);
    hkRotation rotationB = transforms.m_transformB.getRotation();
    rotationB.transpose();
    inertiaB.changeBasis(rotationB);
    const float inertiaB2 = inertiaB.m_col2(2) + inertiaB.m_col2(1);
    const float inertiaB1 = inertiaB.m_col1(2) + inertiaB.m_col1(1);
    float weightB = inertiaB2 + inertiaB1;
    // EXE-INLINED 0x458700
    if (_mm_cvtss_f32(_mm_shuffle_ps(
            bodyB->getRigidMotion()->m_inertiaAndMassInv.m_quad, bodyB->getRigidMotion()->m_inertiaAndMassInv.m_quad,
            0xFF
        )) > 1.1920929e-07f) {
        // EXE-INLINED 0x458700
        weightB = weightB /
            _mm_cvtss_f32(_mm_shuffle_ps(
                bodyB->getRigidMotion()->m_inertiaAndMassInv.m_quad,
                bodyB->getRigidMotion()->m_inertiaAndMassInv.m_quad, 0xFF
            ));
    }

    const float inverseSum = 1.0f / (weightA + weightB);

    // Each pivot's distance from its centre of mass along its axis.
    const hkVector4& centerA = bodyA->getRigidMotion()->getCenterOfMassLocal();
    const hkVector4& axisA = transforms.m_transformA.getRotation().getColumn(0);
    hkVector4 relA;
    relA.setSub4(transforms.m_transformA.getTranslation(), centerA);
    const hkQuadReal productA = _mm_mul_ps(axisA.m_quad, relA.m_quad);
    // EXE-INLINED 0x62B0A0
    const float distanceA = _mm_cvtss_f32(_mm_add_ss(
        _mm_shuffle_ps(productA, productA, 0xAA), _mm_add_ss(_mm_shuffle_ps(productA, productA, 0x55), productA)
    ));
    const hkVector4& axisB = transforms.m_transformB.getRotation().getColumn(0);
    hkVector4 relB;
    relB.setSub4(transforms.m_transformB.getTranslation(), bodyB->getRigidMotion()->getCenterOfMassLocal());
    const hkQuadReal productB = _mm_mul_ps(axisB.m_quad, relB.m_quad);
    // EXE-INLINED 0x62B0A0
    const float distanceB = _mm_cvtss_f32(_mm_add_ss(
        _mm_shuffle_ps(productB, productB, 0xAA), _mm_add_ss(_mm_shuffle_ps(productB, productB, 0x55), productB)
    ));

    const float difference = distanceA - distanceB;
    const float shiftB = weightB * difference * inverseSum;
    const float moveA = shiftB - distanceA;
    const float shiftA = weightA * -difference * inverseSum;
    const float moveB = shiftA - distanceB;

    transforms.m_transformA.getTranslation().m_quad =
        _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps(moveA), axisA.m_quad), relA.m_quad), centerA.m_quad);
    transforms.m_transformB.getTranslation().m_quad = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(_mm_set1_ps(moveB), axisB.m_quad), relB.m_quad),
        bodyB->getRigidMotion()->getCenterOfMassLocal().m_quad
    );
}

hkpConstraintInstance* hkpConstraintUtils::convertToLimits(hkpConstraintInstance* originalConstraint) {
    const hkpConstraintData* pOldData = originalConstraint->getData();
    hkpConstraintData* pNewData;
    switch (pOldData->getType()) {
        case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE: {
            const hkpLimitedHingeConstraintData* pOld = static_cast<const hkpLimitedHingeConstraintData*>(pOldData);
            hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
            hkpHingeLimitsData* pMemory =
                static_cast<hkpHingeLimitsData*>(pRouter->m_heap->blockAlloc(sizeof(hkpHingeLimitsData)));
            pMemory->m_memSizeAndFlags = sizeof(hkpHingeLimitsData);
            hkpHingeLimitsData* pNew = ::new (pMemory) hkpHingeLimitsData;
            pNew->m_atoms.m_rotations.m_rotationA = pOld->m_atoms.m_transforms.m_transformA.getRotation();
            pNew->m_atoms.m_rotations.m_rotationB = pOld->m_atoms.m_transforms.m_transformB.getRotation();
            pNew->m_atoms.m_angLimit = pOld->m_atoms.m_angLimit;
            pNew->m_atoms.m_2dAng = pOld->m_atoms.m_2dAng;
            pNewData = pNew;
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL: {
            const hkpRagdollConstraintData* pOld = static_cast<const hkpRagdollConstraintData*>(pOldData);
            hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
            hkpRagdollLimitsData* pMemory =
                static_cast<hkpRagdollLimitsData*>(pRouter->m_heap->blockAlloc(sizeof(hkpRagdollLimitsData)));
            pMemory->m_memSizeAndFlags = sizeof(hkpRagdollLimitsData);
            hkpRagdollLimitsData* pNew = ::new (pMemory) hkpRagdollLimitsData;
            pNew->m_atoms.m_rotations.m_rotationA = pOld->m_atoms.m_transforms.m_transformA.getRotation();
            pNew->m_atoms.m_rotations.m_rotationB = pOld->m_atoms.m_transforms.m_transformB.getRotation();
            pNew->m_atoms.m_twistLimit = pOld->m_atoms.m_twistLimit;
            pNew->m_atoms.m_planesLimit = pOld->m_atoms.m_planesLimit;
            pNew->m_atoms.m_coneLimit = pOld->m_atoms.m_coneLimit;
            pNew->setConeLimitStabilization(static_cast<char>(pOld->m_atoms.m_coneLimit.m_memOffsetToAngleOffset != 0));
            pNewData = pNew;
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_HINGE_LIMITS:
        case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL_LIMITS:
            originalConstraint->addReference();
            return originalConstraint;
        default: return nullptr;
    }
    hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
    hkpConstraintInstance* pMemory =
        static_cast<hkpConstraintInstance*>(pRouter->m_heap->blockAlloc(sizeof(hkpConstraintInstance)));
    pMemory->m_memSizeAndFlags = sizeof(hkpConstraintInstance);
    hkpConstraintInstance* pNewConstraint = ::new (pMemory) hkpConstraintInstance(
        originalConstraint->getEntityA(), originalConstraint->getEntityB(), pNewData, originalConstraint->getPriority()
    );
    pNewData->removeReference();
    return pNewConstraint;
}
