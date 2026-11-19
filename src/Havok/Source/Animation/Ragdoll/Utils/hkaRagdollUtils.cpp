#include "Havok/Source/Animation/Ragdoll/Utils/hkaRagdollUtils.h"

#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeletonUtils.h"
#include "Havok/Source/Animation/Ragdoll/Instance/hkaRagdollInstance.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Filter/Group/hkpGroupFilter.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Transform/hkpTransformShape.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/BallAndSocket/hkpBallAndSocketConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Hinge/hkpHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Prismatic/hkpPrismaticConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Ragdoll/hkpRagdollConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/StiffSpring/hkpStiffSpringConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/Collide/Filter/GroupFilter/hkpGroupFilterUtil.h"
#include "Havok/Source/Physics/Utilities/Collide/ShapeUtils/CollapseTransform/hkpTransformCollapseUtil.h"

// Havok SDK signature (hkaRagdoll.lib, tied by reloc).
static hkResult alignChildrenToConstraints(const hkArrayBase<hkpConstraintInstance*>& a1, hkBool a2);
// Called only by alignChildrenToConstraints, with a bone's hkQsTransform and the bone's rigid body: when the transform
// is not the identity it wraps the body's shape in an hkpTransformShape, collapses it, moves the body by the transform
// (motion transform and centre of mass) and returns 0.
static int32_t moveRigidBodyPivot(hkQsTransform* r_eax, hkpRigidBody* r_esi);

// For each supported constraint whose pivot in its first body is not at that body's origin (squared length above 1e-6),
// moves the body's shape and transform so that the pivot becomes its origin (moveRigidBodyPivot). HK_FAILURE when a
// constraint's type is unsupported or a move failed. The hkBool is never read. The squared length is summed z + (y +
// x), as the game does.
static hkResult alignChildrenToConstraints(const hkArrayBase<hkpConstraintInstance*>& constraints, hkBool a2) {
    hkResult result = HK_SUCCESS;
    const int32_t iCount = constraints.m_size;
    for (int32_t i = 0; i < iCount; ++i) {
        hkpConstraintInstance* pConstraint = constraints.m_data[i];
        if (!hkaRagdollUtils::isConstraintSupported(pConstraint, true)) {
            result = HK_FAILURE;
            continue;
        }
        const hkVector4 pivot = *hkaRagdollUtils::_accessConstraintPivotA(pConstraint);
        hkpRigidBody* pBody = pConstraint->getRigidBodyA();
        hkQsTransform childFromConstraint;
        childFromConstraint.m_rotation.setIdentity();
        childFromConstraint.m_scale.m_quad = _mm_set1_ps(1.0f);
        childFromConstraint.m_translation = pivot;
        const hkQuadReal squares =
            _mm_mul_ps(childFromConstraint.m_translation.m_quad, childFromConstraint.m_translation.m_quad);
        const float fLengthSquared = _mm_cvtss_f32(_mm_add_ss(
            _mm_shuffle_ps(squares, squares, 0xAA), _mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares)
        ));
        if (0.000001f < fLengthSquared && moveRigidBodyPivot(&childFromConstraint, pBody)) {
            result = HK_FAILURE;
        }
    }
    return result;
}

// Unless the transform is the identity (within 0.001), wraps the body's shape in an hkpTransformShape of the
// transform's inverse and collapses it (with hkError's warning 0x2FF8C16F off meanwhile, then restored), moves the body
// to its old transform times the transform and, unless it is fixed, keeps its centre of mass where it was in world
// space. Returns 0. Reads the old centre of mass before the shape changes.
static int32_t moveRigidBodyPivot(hkQsTransform* r_eax, hkpRigidBody* r_esi) {
    bool bIdentity;
    if (*r_eax->isApproximatelyEqual(
            &bIdentity, &hkQsTransformIdentity.m_translation.m_quad, std::bit_cast<uint32_t>(0.001f)
        )) {
        return 0;
    }
    hkQsTransform bodyTransform;
    bodyTransform.setFromTransformNoScale(r_esi->m_motion.m_motionState.m_transform);
    hkQsTransform newTransform;
    newTransform.setMul(bodyTransform, *r_eax);
    hkQsTransform inverse;
    inverse.setInverse(*r_eax);
    const hkVector4 oldCenterOfMass = r_esi->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1;

    hkBool bEnabled = hkError::s_instance->isEnabled(0x2FF8C16F);
    hkError::s_instance->setEnabled(0x2FF8C16F, false);
    const hkpShape* pShape = r_esi->m_collidable.m_shape;
    hkTransform transform;
    inverse.copyToTransformNoScale(transform);
    void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpTransformShape));
    static_cast<hkpTransformShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpTransformShape);
    hkpTransformShape* pTransformShape = ::new (pMemory) hkpTransformShape(pShape, transform);
    r_esi->setShape(pTransformShape);
    pTransformShape->removeReference();
    alignas(16) hkpTransformCollapseUtil::Options options;
    alignas(16) hkpTransformCollapseUtil::Results results;
    hkpTransformCollapseUtil::collapseTransforms(r_esi, options, results);
    hkError::s_instance->setEnabled(0x2FF8C16F, bEnabled);

    newTransform.copyToTransformNoScale(transform);
    r_esi->setTransform(transform);
    if (r_esi->m_motion.m_type.m_storage != hkpMotion::MOTION_FIXED) {
        hkVector4 centerOfMassLocal;
        centerOfMassLocal.setTransformedInversePos(transform, oldCenterOfMass);
        r_esi->setCenterOfMassLocal(centerOfMassLocal);
    }
    return 0;
}

// Havok SDK: for each supported constraint, moves its pivot onto the first body's origin: pivot A becomes zero and
// pivot B the first body's world position in the second body's space. Warns for every stiff spring; always
// HK_SUCCESS.
hkResult alignConstraintSpacesToChildren(const hkArrayBase<hkpConstraintInstance*>& constraints) {
    const int32_t iCount = constraints.m_size;
    for (int32_t i = 0; i < iCount; ++i) {
        hkpConstraintInstance* pConstraint = constraints.m_data[i];
        if (hkaRagdollUtils::isConstraintSupported(pConstraint, true)) {
            const hkVector4 pivotInWorld =
                pConstraint->getRigidBodyA()->m_motion.m_motionState.m_transform.m_translation;
            hkpRigidBody* pBodyB = pConstraint->getRigidBodyB();
            hkaRagdollUtils::_accessConstraintPivotA(pConstraint)->m_quad = _mm_setzero_ps();
            hkaRagdollUtils::_accessConstraintPivotB(pConstraint)
                ->setTransformedInversePos(pBodyB->m_motion.m_motionState.m_transform, pivotInWorld);
        }
        if (pConstraint->getDataRw()->getType() == hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING) {
            alignas(16) char buf[512];
            hkErrStream apThis(buf, 512);
            apThis.operator<<("Unusual : Aligning constraint spaces of a stiff spring.");
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, -0x54457893, buf, ".\\Utils\\hkaRagdollUtils.cpp", 308
            );
        }
    }
    return HK_SUCCESS;
}

void hkaRagdollUtils::destroySkeleton(hkaSkeleton* skeleton) {
    delete skeleton;
}

// Marks the descendants of splitBone, then runs two passes, each building an hkaRagdollInstance into the trunk or the
// branch reference.
void hkaRagdollUtils::splitRagdollInstanceShallow(
    const hkaRagdollInstance* original,
    int32_t splitBone,
    hkaRagdollInstance*& trunk,
    hkaRagdollInstance*& branch
) {
    hkaRagdollInstance** ppOut[2] = {&trunk, &branch};
    bool bValues[2] = {false, true};
    hkArray<bool> descendants(original->m_skeleton->m_bones.m_size);
    hkaSkeletonUtils::markDescendants(original->m_skeleton, splitBone, descendants.m_data, true);
    for (int32_t i = 0; i < 2; ++i) {
        hkArray<hkpRigidBody*> rigidBodies;
        hkArray<hkpConstraintInstance*> constraints;
        hkArray<int32_t> boneToRigidBody;
        const bool bValue = bValues[i];
        for (int32_t b = 0; b < original->m_skeleton->m_bones.m_size; ++b) {
            hkpRigidBody* pBody = nullptr;
            if (descendants.m_data[b] == bValue) {
                const int32_t iBody = original->m_boneToRigidBodyMap.m_data[b];
                if (iBody >= 0) {
                    pBody = original->m_rigidBodies.m_data[iBody];
                }
                // The constraint into body iBody is constraint iBody - 1.
                const int32_t iConstraint = iBody - 1;
                hkpConstraintInstance* pConstraint = nullptr;
                if (iConstraint >= 0) {
                    pConstraint = original->m_constraints.m_data[iConstraint];
                }
                if (pBody) {
                    // The new body's index is read before the push's router call.
                    boneToRigidBody.pushBack(rigidBodies.getSize());
                    rigidBodies.pushBack(pBody);
                    if (b != splitBone && b != 0) {
                        constraints.pushBack(pConstraint);
                    }
                    continue;
                }
            }
            boneToRigidBody.pushBack(-1);
        }
        // EXE-INLINED 0x56D280
        void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkaRagdollInstance));
        static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkaRagdollInstance);
        *ppOut[i] = ::new (pMemory) hkaRagdollInstance(rigidBodies, constraints, original->m_skeleton, boneToRigidBody);
    }
}

void hkaRagdollUtils::destroyRagdollInstanceShallow(hkaRagdollInstance* ragdoll) {
    delete ragdoll;
}

// KEEP (unregistered): the exe passes apConstraint in eax (the compiler's own convention for this file-static helper;
// abWarn is the only stack argument, plain ret), and src has no registration form for a register argument. Called
// from 0xCE9DB0 and 0xCE9E90 with abWarn set.
// Ball and socket, hinge, limited hinge, prismatic, ragdoll and stiff spring constraints are supported; any other
// type is reported as a warning when abWarn is set.
bool hkaRagdollUtils::isConstraintSupported(const hkpConstraintInstance* apConstraint, bool abWarn) {
    switch (apConstraint->getData()->getType()) {
        case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET:
        case hkpConstraintData::CONSTRAINT_TYPE_HINGE:
        case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE:
        case hkpConstraintData::CONSTRAINT_TYPE_PRISMATIC:
        case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL:
        case hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING: return true;
        default:
            if (abWarn) {
                alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
                char acBuffer[0x200];
                hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
                *pStream << "Unsupported type of constraint in rag doll";
                hkError::s_instance->message(
                    hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBABF3B), acBuffer,
                    ".\\Utils\\hkaRagdollUtils.cpp", 0x81
                );
                pStream->~hkErrStream();
            }
            return false;
    }
}

// With one constraint fewer than rigid bodies, records for each body the constraint whose first entity it is and that
// constraint's second body (its parent), failing where a body is missing or has a second such constraint; sorts the
// bodies by their depth in that hierarchy (a selection sort, swapping the order with the depths) and rewrites both
// arrays in that order, each body but the first followed by the constraint into it. The hkLocalArrays' setSize with a
// fill value is written out here.
hkResult hkaRagdollUtils::reorderForRagdoll(
    hkArray<hkpRigidBody*, hkContainerHeapAllocator>& rigidBodies,
    hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& constraints
) {
    const int32_t numBodies = rigidBodies.m_size;
    const int32_t numConstraints = constraints.m_size;
    if (numConstraints != numBodies - 1) {
        char buf[512];
        hkErrStream kStream(buf, 512);
        kStream << "Expecting " << (numBodies - 1) << " constraints, not " << numConstraints
                << ". Can't create rag doll";
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA54BB), buf, ".\\Utils\\hkaRagdollUtils.cpp", 324
        );
        return HK_FAILURE;
    }
    hkLocalArray<int32_t> constraintOfBody(numBodies);
    constraintOfBody.setSize(numBodies, 0);

    hkLocalArray<int32_t> order(numBodies);
    order.m_size = numBodies;

    hkLocalArray<int32_t> parent(numBodies);
    parent.setSize(numBodies, -1);

    bool bValid = true;
    for (int32_t i = 0; i < numConstraints; ++i) {
        hkpConstraintInstance* pConstraint = constraints.m_data[i];
        hkpRigidBody* pChildBody = pConstraint->getRigidBodyA();
        hkpRigidBody* pParentBody = pConstraint->getRigidBodyB();
        const int32_t iChild = rigidBodies.indexOf(pChildBody);
        const int32_t iParent = rigidBodies.indexOf(pParentBody);
        if (iChild < 0 || iParent < 0 || parent.m_data[iChild] != -1 || constraintOfBody.m_data[iChild] != 0) {
            bValid = false;
            break;
        }
        constraintOfBody.m_data[iChild] = i;
        parent.m_data[iChild] = iParent;
    }
    if (!bValid) {
        return HK_FAILURE;
    }

    {
        hkLocalArray<int32_t> depth(numBodies);
        depth.setSize(numBodies, 0);

        for (int32_t b = 0; b < numBodies; ++b) {
            for (int32_t p = parent.m_data[b]; p != -1; p = parent.m_data[p]) {
                ++depth.m_data[b];
            }
        }
        for (int32_t b = 0; b < numBodies; ++b) {
            order.m_data[b] = b;
        }
        for (int32_t i = 0; i < numBodies - 1; ++i) {
            for (int32_t j = i + 1; j < numBodies; ++j) {
                const int32_t iDepth = depth.m_data[i];
                if (iDepth > depth.m_data[j]) {
                    depth.m_data[i] = depth.m_data[j];
                    depth.m_data[j] = iDepth;
                    const int32_t iBody = order.m_data[i];
                    order.m_data[i] = order.m_data[j];
                    order.m_data[j] = iBody;
                }
            }
        }
    }

    hkLocalArray<hkpRigidBody*> newBodies(numBodies);
    newBodies.m_size = numBodies;

    hkLocalArray<hkpConstraintInstance*> newConstraints(numConstraints);
    newConstraints.m_size = numConstraints;

    for (int32_t i = 0; i < numBodies; ++i) {
        const int32_t iBody = order.m_data[i];
        newBodies.m_data[i] = rigidBodies.m_data[iBody];
        if (i > 0) {
            const int32_t iConstraint = constraintOfBody.m_data[iBody];
            if (iConstraint < 0) {
                newConstraints.m_data[i - 1] = nullptr;
            }
            else {
                newConstraints.m_data[i - 1] = constraints.m_data[iConstraint];
            }
        }
    }

    rigidBodies = newBodies;
    constraints = newConstraints;

    return HK_SUCCESS;
}

hkResult hkaRagdollUtils::reorderAndAlignForRagdoll(
    hkArray<hkpRigidBody*, hkContainerHeapAllocator>& rigidBodiesInOut,
    hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& constraintsInOut,
    hkBool warnOnAlignment
) {
    return static_cast<hkResult>(
        hkaRagdollUtils::reorderForRagdoll(rigidBodiesInOut, constraintsInOut) ||
        alignChildrenToConstraints(constraintsInOut, warnOnAlignment) ||
        alignConstraintSpacesToChildren(constraintsInOut) != HK_SUCCESS
    );
}

// For each bone, the candidate rigid body of the bone's name (a warning for each further one of that name) and, for a
// bone with a parent, the candidate constraint from that body to the parent bone's body (a warning for each further
// one); the instance is built from the bodies, the constraints and the bone-to-body map. A bone without a name, a body
// without its constraint or no body at all is a warning and no instance.
hkaRagdollInstance* hkaRagdollUtils::createRagdollInstanceFromSkeleton(
    const hkaSkeleton* skeleton,
    const hkArrayBase<hkpRigidBody*>& candidateRBs,
    const hkArrayBase<hkpConstraintInstance*>& candidateConstraints
) {
    const int32_t numBones = skeleton->m_bones.m_size;
    hkLocalArray<hkpRigidBody*> rigidBodies(numBones);

    hkLocalArray<hkpConstraintInstance*> constraints(numBones - 1);

    hkLocalArray<int32_t> boneToRigidBody(numBones);

    char buf[512];
    bool bFailed = false;
    for (int32_t i = 0; i < numBones; ++i) {
        const hkaBone& bone = skeleton->m_bones.m_data[i];
        const int16_t parentBone = skeleton->m_parentIndices.m_data[i];
        if (!bone.m_name.cString()) {
            hkErrStream kStream(buf, 512);
            kStream << "Some bones have no name - can't create rag doll";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAE6A2), buf, ".\\Utils\\hkaRagdollUtils.cpp", 675
            );
            bFailed = true;
            break;
        }
        hkpRigidBody* pBody = nullptr;
        for (int32_t c = 0; c < candidateRBs.m_size; ++c) {
            hkpRigidBody* pCandidate = candidateRBs.m_data[c];
            const char* pName = pCandidate->m_name.cString();
            if (pName && hkString::strCmp(pName, bone.m_name.cString()) == 0) {
                if (pBody) {
                    hkErrStream kStream(buf, 512);
                    kStream << "Some RBS have duplicated names : " << pName;
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA98AA), buf,
                        ".\\Utils\\hkaRagdollUtils.cpp", 619
                    );
                }
                else {
                    pBody = pCandidate;
                }
            }
        }
        if (!pBody) {
            boneToRigidBody.pushBack(-1);
            continue;
        }
        // The new body's index is read before the push's router call.
        boneToRigidBody.pushBack(rigidBodies.getSize());
        rigidBodies.pushBack(pBody);
        if (parentBone == -1) {
            continue;
        }
        hkpRigidBody* pParentBody = rigidBodies.m_data[boneToRigidBody.m_data[parentBone]];
        hkpConstraintInstance* pConstraint = nullptr;
        for (int32_t c = 0; c < candidateConstraints.m_size; ++c) {
            hkpConstraintInstance* pCandidate = candidateConstraints.m_data[c];
            hkpEntity* pEntityA = pCandidate->m_entities[0];
            hkpEntity* pEntityB = pCandidate->m_entities[1];
            if (pEntityA == pBody && pEntityB == pParentBody) {
                if (pConstraint) {
                    hkErrStream kStream(buf, 512);
                    kStream << "Found more than one constraint between rigid bodies " << pEntityA->m_name.cString()
                            << " and " << pEntityB->m_name.cString();
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA82FF), buf,
                        ".\\Utils\\hkaRagdollUtils.cpp", 657
                    );
                }
                else {
                    pConstraint = pCandidate;
                }
            }
        }
        if (!pConstraint) {
            hkErrStream kStream(buf, 512);
            kStream << "Couldn't find constraint for bone : " << bone.m_name;
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA9F9F), buf, ".\\Utils\\hkaRagdollUtils.cpp", 664
            );
            bFailed = true;
            break;
        }
        constraints.pushBack(pConstraint);
    }

    hkaRagdollInstance* pInstance = nullptr;
    if (!bFailed) {
        if (rigidBodies.m_size == 0) {
            hkErrStream kStream(buf, 512);
            kStream << "Skeleton has no associated rigid bodies - can't create rag doll";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAE6E4), buf, ".\\Utils\\hkaRagdollUtils.cpp", 682
            );
        }
        else {
            // EXE-INLINED 0x56D280
            void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkaRagdollInstance));
            static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkaRagdollInstance);
            pInstance = ::new (pMemory) hkaRagdollInstance(rigidBodies, constraints, skeleton, boneToRigidBody);
        }
    }

    return pInstance;
}

// Builds a new skeleton with a bone per rigid body. A bone's parent is the body the constraint before it attaches as
// its second entity, and its reference pose is the body's transform relative to the parent's (setMulInverseMul).
hkaSkeleton* hkaRagdollUtils::constructSkeletonForRagdoll(
    const hkArrayBase<hkpRigidBody*>& rigidBodies,
    const hkArrayBase<hkpConstraintInstance*>& constraints
) {
    // EXE-INLINED 0x56D280
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkaSkeleton* v3 = static_cast<hkaSkeleton*>(Instance->m_heap->blockAlloc(sizeof(hkaSkeleton)));
    v3->m_memSizeAndFlags = sizeof(hkaSkeleton);
    ::new (v3) hkaSkeleton;
    int32_t m_size = rigidBodies.m_size;
    int32_t m_size_1 = m_size;
    // EXE-INLINED none: hkArray<hkaBone>::setSize
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t m_size_3 = v3->m_bones.m_capacityAndFlags & hkArrayBase<hkaBone>::CAPACITY_MASK;
    if (m_size_3 < m_size) {
        int32_t m_size_2 = 2 * m_size_3;
        if (m_size >= m_size_2) {
            m_size_2 = m_size;
        }
        hkArrayUtil::_reserve(*m_heap, &v3->m_bones, m_size_2, sizeof(hkaBone));
    }
    hkaBone* pDeadBone = &v3->m_bones.m_data[m_size];
    if (v3->m_bones.m_size - m_size > 0) {
        int32_t apThis_8 = v3->m_bones.m_size - m_size;
        do {
            pDeadBone->m_name.~hkStringPtr();
            ++pDeadBone;
            --apThis_8;
        }
        while (apThis_8);
    }
    int32_t m_size_4 = v3->m_bones.m_size;
    hkaBone* v10 = &v3->m_bones.m_data[m_size_4];
    if (m_size - m_size_4 > 0) {
        int32_t apThis_8a = m_size - m_size_4;
        do {
            if (v10) {
                ::new (&v10->m_name) hkStringPtr;
                v10->m_lockTranslation.m_bool = 0;
            }
            ++v10;
            --apThis_8a;
        }
        while (apThis_8a);
    }
    v3->m_bones.m_size = m_size;
    v3->m_parentIndices.setSize(m_size);
    v3->m_referencePose.setSize(m_size);
    int32_t m_size_9 = 0;
    if (m_size > 0) {
        int32_t apThis_8b = 0;
        do {
            int32_t m_size_11 = -1;
            if (m_size_9) {
                int32_t m_size_10 = 0;
                if (m_size_9 > 0) {
                    hkpRigidBody** m_data = rigidBodies.m_data;
                    while (*m_data != constraints.m_data[m_size_9 - 1]->m_entities[1]) {
                        ++m_size_10;
                        ++m_data;
                        if (m_size_10 >= m_size_9) {
                            goto LABEL_30;
                        }
                    }
                    m_size_11 = m_size_10;
                }
            }
        LABEL_30:;
            v3->m_parentIndices.m_data[m_size_9] = static_cast<int16_t>(m_size_11);
            hkpRigidBody* pBody = rigidBodies.m_data[m_size_9];
            hkaBone* pBone = &v3->m_bones.m_data[m_size_9];
            if (m_size_11 < 0) {
                v3->m_referencePose.m_data[apThis_8b].setFromTransformNoScale(
                    pBody->m_motion.m_motionState.m_transform
                );
            }
            else {
                hkpRigidBody* pParentBody = rigidBodies.m_data[m_size_11];
                hkQsTransform childWorld;
                childWorld.setFromTransformNoScale(pBody->m_motion.m_motionState.m_transform);
                hkQsTransform parentWorld;
                parentWorld.setFromTransformNoScale(pParentBody->m_motion.m_motionState.m_transform);
                v3->m_referencePose.m_data[apThis_8b].setMulInverseMul(parentWorld, childWorld);
            }
            const char* s = pBody->m_name.cString();
            if (s) {
                pBone->m_name = s;
            }
            else {
                alignas(16) char buf[512];
                hkErrStream errStream(buf, 512);
                errStream.operator<<("Rigid Body name could not be found");
                hkError::s_instance->message(
                    hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBACC14), buf, ".\\Utils\\hkaRagdollUtils.cpp",
                    558
                );
            }
            char m_bool = m_size_9 != 0;
            ++apThis_8b;
            ++m_size_9;
            pBone->m_lockTranslation.m_bool = m_bool;
        }
        while (m_size_9 < m_size_1);
    }
    if (v3->m_bones.m_size > 0) {
        v3->m_name.operator=(v3->m_bones.begin()->m_name);
    }
    return v3;
}

// The pivot of apConstraint in its first body's space: the ball-and-socket's and stiff spring's translation A, the
// translation of the hinge's, limited hinge's, prismatic's and ragdoll's transform A; null, with a warning, for any
// other type. Writable: alignConstraintSpacesToChildren zeroes it.
hkVector4* hkaRagdollUtils::_accessConstraintPivotA(hkpConstraintInstance* apConstraint) {
    hkVector4* result;
    hkpConstraintData* pData = apConstraint->getDataRw();
    switch (pData->getType()) {
        case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET: {
            result = &static_cast<hkpBallAndSocketConstraintData*>(pData)->m_atoms.m_pivots.m_translationA;
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING: {
            result = &static_cast<hkpStiffSpringConstraintData*>(pData)->m_atoms.m_pivots.m_translationA;
            break;
        }
        // hkpLimitedHingeConstraintData is not declared; its atoms start with the same
        // hkpSetLocalTransformsConstraintAtom at the same offset as the hinge's, so both are read through
        // hkpHingeConstraintData.
        case hkpConstraintData::CONSTRAINT_TYPE_HINGE:
        case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE: {
            result = &static_cast<hkpHingeConstraintData*>(pData)->m_atoms.m_transforms.m_transformA.m_translation;
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_PRISMATIC: {
            result = &static_cast<hkpPrismaticConstraintData*>(pData)->m_atoms.m_transforms.m_transformA.m_translation;
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL: {
            result = &static_cast<hkpRagdollConstraintData*>(pData)->m_atoms.m_transforms.m_transformA.m_translation;
            break;
        }
        default: {
            char buf[512];
            hkErrStream apThis(buf, 512);
            apThis.operator<<("Unsupported type of constraint in rag doll");
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, -0x544540C5, buf, ".\\Utils\\hkaRagdollUtils.cpp", 180
            );
            result = nullptr;
            break;
        }
    }
    return result;
}

// The pivot of apConstraint in its second body's space: the ball-and-socket's translation B, the translation of the
// hinge's, limited hinge's, prismatic's and ragdoll's transform B; null, with a warning, for any other type. For a
// stiff spring it deliberately returns translation A, as _accessConstraintPivotA does and as the game does.
hkVector4* hkaRagdollUtils::_accessConstraintPivotB(hkpConstraintInstance* apConstraint) {
    hkVector4* result;
    hkpConstraintData* pData = apConstraint->getDataRw();
    switch (pData->getType()) {
        case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET: {
            result = &static_cast<hkpBallAndSocketConstraintData*>(pData)->m_atoms.m_pivots.m_translationB;
            break;
        }
        // hkpLimitedHingeConstraintData is not declared; its atoms start with the same
        // hkpSetLocalTransformsConstraintAtom at the same offset as the hinge's, so both are read through
        // hkpHingeConstraintData.
        case hkpConstraintData::CONSTRAINT_TYPE_HINGE:
        case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE: {
            result = &static_cast<hkpHingeConstraintData*>(pData)->m_atoms.m_transforms.m_transformB.m_translation;
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_PRISMATIC: {
            result = &static_cast<hkpPrismaticConstraintData*>(pData)->m_atoms.m_transforms.m_transformB.m_translation;
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL: {
            result = &static_cast<hkpRagdollConstraintData*>(pData)->m_atoms.m_transforms.m_transformB.m_translation;
            break;
        }
        case hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING: {
            result = &static_cast<hkpStiffSpringConstraintData*>(pData)->m_atoms.m_pivots.m_translationA;
            break;
        }
        default: {
            char buf[512];
            hkErrStream apThis(buf, 512);
            apThis.operator<<("Unsupported type of constraint in rag doll.");
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, -0x544540C5, buf, ".\\Utils\\hkaRagdollUtils.cpp", 230
            );
            result = nullptr;
            break;
        }
    }
    return result;
}

void hkaRagdollUtils::setCollisionLayer(hkaRagdollInstance* ragdoll, int32_t layer, hkpGroupFilter* collisionFilter) {
    hkpRigidBody* v4;
    for (int32_t i = 0; i < ragdoll->m_rigidBodies.m_size;
         v4->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo = layer) {
        v4 = ragdoll->m_rigidBodies.m_data[i++];
    }
    hkpGroupFilterUtil::disableCollisionsBetweenConstraintBodies(
        ragdoll->m_constraints.begin(), ragdoll->m_constraints.m_size, ++collisionFilter->m_nextFreeSystemGroup
    );
}

hkaRagdollInstance* hkaRagdollUtils::copyRagdollInstanceShallow(const hkaRagdollInstance* original) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkaRagdollInstance* v2 = static_cast<hkaRagdollInstance*>(Instance->m_heap->blockAlloc(sizeof(hkaRagdollInstance)));
    // The native sizeof, since the heap frees by this value.
    v2->m_memSizeAndFlags = sizeof(hkaRagdollInstance);
    return ::new (v2) hkaRagdollInstance(
        original->m_rigidBodies, original->m_constraints, original->m_skeleton, original->m_boneToRigidBodyMap
    );
}
