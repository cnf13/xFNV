#include "Havok/Source/Animation/Ragdoll/Utils/hkaRagdollUtils.h"

#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeletonUtils.h"
#include "Havok/Source/Animation/Ragdoll/Instance/hkaRagdollInstance.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
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
		const float fLengthSquared = _mm_cvtss_f32(_mm_add_ss(_mm_shuffle_ps(squares, squares, 0xAA),
			_mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares)));
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
	if (*r_eax->isApproximatelyEqual(&bIdentity, &hkQsTransformIdentity.m_translation.m_quad,
			std::bit_cast<uint32_t>(0.001f))) {
		return 0;
	}
	hkQsTransform bodyTransform;
	bodyTransform.setFromTransformNoScale(r_esi->m_motion.m_motionState.m_transform);
	// EXE-INLINED none: hkQsTransform::setMul
	hkQsTransform newTransform;
	hkVector4 extraTrans;
	extraTrans.setRotatedDir(bodyTransform.m_rotation, r_eax->m_translation);
	newTransform.m_translation.setAdd4(bodyTransform.m_translation, extraTrans);
	newTransform.m_rotation.setMul(bodyTransform.m_rotation, r_eax->m_rotation);
	newTransform.m_scale.setMul4(bodyTransform.m_scale, r_eax->m_scale);
	// EXE-INLINED none: hkQsTransform::setInverse
	hkQsTransform inverse;
	inverse.setInverse(*r_eax);
	const hkVector4 oldCenterOfMass = r_esi->m_motion.m_motionState.m_sweptTransform.m_centerOfMass1;

	hkBool bEnabled;
	hkError::s_instance->isEnabled(&bEnabled, 0x2FF8C16F);
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
			hkError::s_instance->message(hkError::MESSAGE_WARNING, -0x54457893, buf, ".\\Utils\\hkaRagdollUtils.cpp",
				308);
		}
	}
	return HK_SUCCESS;
}

void hkaRagdollUtils::destroySkeleton(hkaSkeleton* skeleton) {
	delete skeleton;
}

// Marks the descendants of splitBone, then runs two passes, each building an hkaRagdollInstance into the trunk or the
// branch reference.
void hkaRagdollUtils::splitRagdollInstanceShallow(const hkaRagdollInstance* original, int32_t splitBone,
	hkaRagdollInstance*& trunk, hkaRagdollInstance*& branch) {
	hkaRagdollInstance** ppOut[2] = {&trunk, &branch};
	bool bValues[2] = {false, true};
	const int32_t iNumBones = original->m_skeleton->m_bones.m_size;
	// EXE-INLINED none: hkArray<bool>::hkArray(int)
	FNVD_INLINED_LOCAL(hkArray<bool>, descendants, std::nullopt, std::nullopt);
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	descendants.m_size = static_cast<uint32_t>(iNumBones);
	if (iNumBones == 0) {
		descendants.m_data = nullptr;
		descendants.m_capacityAndFlags = hkArrayBase<bool>::DONT_DEALLOCATE_FLAG;
	} else {
		int32_t iNumBytes = iNumBones;
		descendants.m_data = static_cast<bool*>(pHeap->bufAlloc(iNumBytes));
		if (iNumBytes == 0) {
			descendants.m_capacityAndFlags = hkArrayBase<bool>::DONT_DEALLOCATE_FLAG;
		} else {
			descendants.m_capacityAndFlags = static_cast<uint32_t>(iNumBytes);
		}
	}
	hkaSkeletonUtils::markDescendants(original->m_skeleton, splitBone, descendants.m_data, true);
	for (int32_t i = 0; i < 2; ++i) {
		FNVD_INLINED_LOCAL(hkArray<hkpRigidBody*>, rigidBodies, std::nullopt, std::nullopt);
		rigidBodies.m_data = nullptr;
		rigidBodies.m_size = 0;
		rigidBodies.m_capacityAndFlags = hkArrayBase<hkpRigidBody*>::DONT_DEALLOCATE_FLAG;
		FNVD_INLINED_LOCAL(hkArray<hkpConstraintInstance*>, constraints, std::nullopt, std::nullopt);
		constraints.m_data = nullptr;
		constraints.m_size = 0;
		constraints.m_capacityAndFlags = hkArrayBase<hkpConstraintInstance*>::DONT_DEALLOCATE_FLAG;
		FNVD_INLINED_LOCAL(hkArray<int32_t>, boneToRigidBody, std::nullopt, std::nullopt);
		boneToRigidBody.m_data = nullptr;
		boneToRigidBody.m_size = 0;
		boneToRigidBody.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
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
					const int32_t iNewBody = rigidBodies.m_size;
					// EXE-INLINED none: hkArray<int>::pushBack
					hkMemoryRouter& rMapRouter = hkMemoryRouter::getInstance();
					if (boneToRigidBody.m_size ==
						(boneToRigidBody.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
						hkArrayUtil::_reserveMore(*rMapRouter.m_heap, &boneToRigidBody, sizeof(int32_t));
					}
					boneToRigidBody.m_data[boneToRigidBody.m_size] = iNewBody;
					boneToRigidBody.m_size += 1;
					// EXE-INLINED none: hkArray<hkpRigidBody*>::pushBack
					hkMemoryRouter& rBodyRouter = hkMemoryRouter::getInstance();
					if (rigidBodies.m_size ==
						(rigidBodies.m_capacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK)) {
						hkArrayUtil::_reserveMore(*rBodyRouter.m_heap, &rigidBodies, sizeof(hkpRigidBody*));
					}
					rigidBodies.m_data[rigidBodies.m_size] = pBody;
					rigidBodies.m_size += 1;
					if (b != splitBone && b != 0) {
						// EXE-INLINED none: hkArray<hkpConstraintInstance*>::pushBack
						hkMemoryRouter& rConstraintRouter = hkMemoryRouter::getInstance();
						if (constraints.m_size ==
							(constraints.m_capacityAndFlags & hkArrayBase<hkpConstraintInstance*>::CAPACITY_MASK)) {
							hkArrayUtil::_reserveMore(*rConstraintRouter.m_heap, &constraints,
								sizeof(hkpConstraintInstance*));
						}
						constraints.m_data[constraints.m_size] = pConstraint;
						constraints.m_size += 1;
					}
					continue;
				}
			}
			// EXE-INLINED none: hkArray<int>::pushBack
			hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
			if (boneToRigidBody.m_size == (boneToRigidBody.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*rRouter.m_heap, &boneToRigidBody, sizeof(int32_t));
			}
			boneToRigidBody.m_data[boneToRigidBody.m_size] = -1;
			boneToRigidBody.m_size += 1;
		}
		// EXE-INLINED 0x56D280
		void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkaRagdollInstance));
		static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkaRagdollInstance);
		*ppOut[i] = ::new (pMemory) hkaRagdollInstance(rigidBodies, constraints, original->m_skeleton, boneToRigidBody);
		// EXE-INLINED none: hkArray<int>::clearAndDeallocate
		hkMemoryAllocator* pMapHeap = hkMemoryRouter::getInstance().m_heap;
		boneToRigidBody.m_size = 0;
		if (!(boneToRigidBody.m_capacityAndFlags & hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG)) {
			pMapHeap->bufFree(boneToRigidBody.m_data,
				static_cast<int32_t>((boneToRigidBody.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK) *
					sizeof(int32_t)));
		}
		boneToRigidBody.m_data = nullptr;
		boneToRigidBody.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
		// EXE-INLINED none: hkArray<hkpConstraintInstance*>::clearAndDeallocate
		hkMemoryAllocator* pConstraintHeap = hkMemoryRouter::getInstance().m_heap;
		constraints.m_size = 0;
		if (!(constraints.m_capacityAndFlags & hkArrayBase<hkpConstraintInstance*>::DONT_DEALLOCATE_FLAG)) {
			pConstraintHeap->bufFree(constraints.m_data,
				static_cast<int32_t>((constraints.m_capacityAndFlags &
										 hkArrayBase<hkpConstraintInstance*>::CAPACITY_MASK) *
					sizeof(hkpConstraintInstance*)));
		}
		constraints.m_data = nullptr;
		constraints.m_capacityAndFlags = hkArrayBase<hkpConstraintInstance*>::DONT_DEALLOCATE_FLAG;
		// EXE-INLINED none: hkArray<hkpRigidBody*>::clearAndDeallocate
		hkMemoryAllocator* pBodyHeap = hkMemoryRouter::getInstance().m_heap;
		rigidBodies.m_size = 0;
		if (!(rigidBodies.m_capacityAndFlags & hkArrayBase<hkpRigidBody*>::DONT_DEALLOCATE_FLAG)) {
			pBodyHeap->bufFree(rigidBodies.m_data,
				static_cast<int32_t>((rigidBodies.m_capacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK) *
					sizeof(hkpRigidBody*)));
		}
		rigidBodies.m_data = nullptr;
		rigidBodies.m_capacityAndFlags = hkArrayBase<hkpRigidBody*>::DONT_DEALLOCATE_FLAG;
	}
	// EXE-INLINED none: hkArray<bool>::~hkArray
	hkMemoryAllocator* pFreeHeap = hkMemoryRouter::getInstance().m_heap;
	if (!(descendants.m_capacityAndFlags & hkArrayBase<bool>::DONT_DEALLOCATE_FLAG)) {
		pFreeHeap->bufFree(descendants.m_data,
			static_cast<int32_t>(descendants.m_capacityAndFlags & hkArrayBase<bool>::CAPACITY_MASK));
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
	case hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING:
		return true;
	default:
		if (abWarn) {
			alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
			char acBuffer[0x200];
			hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
			*pStream << "Unsupported type of constraint in rag doll";
			hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBABF3B), acBuffer,
				".\\Utils\\hkaRagdollUtils.cpp", 0x81);
			FNVD_DESTRUCT(hkOstream)(pStream);
		}
		return false;
	}
}

// With one constraint fewer than rigid bodies, records for each body the constraint whose first entity it is and that
// constraint's second body (its parent), failing where a body is missing or has a second such constraint; sorts the
// bodies by their depth in that hierarchy (a selection sort, swapping the order with the depths) and rewrites both
// arrays in that order, each body but the first followed by the constraint into it. The six hkLocalArrays
// (hkAllocateStack, setSize, hkDeallocateStack and the hkArray copy) are written out here.
hkResult hkaRagdollUtils::reorderForRagdoll(hkArray<hkpRigidBody*, hkContainerHeapAllocator>& rigidBodies,
	hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& constraints) {
	const int32_t numBodies = rigidBodies.m_size;
	const int32_t numConstraints = constraints.m_size;
	if (numConstraints != numBodies - 1) {
		char buf[512];
		hkErrStream kStream(buf, 512);
		kStream << "Expecting " << (numBodies - 1) << " constraints, not " << numConstraints
				<< ". Can't create rag doll";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA54BB), buf,
			".\\Utils\\hkaRagdollUtils.cpp", 324);
		return HK_FAILURE;
	}
	const int32_t iBodyBlockBytes = (numBodies * 4 + 127) & ~127;
	const int32_t iBodyCapacityAndFlags =
		static_cast<int32_t>(static_cast<uint32_t>(numBodies) | hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG);

	// EXE-INLINED none: hkLocalArray<int>::hkLocalArray
	FNVD_INLINED_LOCAL(hkLocalArray<int32_t>, constraintOfBody, std::nullopt, std::nullopt);
	constraintOfBody.m_data = nullptr;
	constraintOfBody.m_size = 0;
	constraintOfBody.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
	hkStackMemory& rStack0 = hkMemoryRouter::getInstance().m_stack;
	int32_t* pConstraintOfBodyMemory = static_cast<int32_t*>(static_cast<void*>(rStack0.m_next.m_storage));
	rStack0.m_next.m_storage += iBodyBlockBytes;
	constraintOfBody.m_data = pConstraintOfBodyMemory;
	constraintOfBody.m_capacityAndFlags = iBodyCapacityAndFlags;
	constraintOfBody.m_localMemory = pConstraintOfBodyMemory;
	// EXE-INLINED none: hkArray<int>::setSize(numBodies, 0)
	hkMemoryAllocator* pHeap0 = hkMemoryRouter::getInstance().m_heap;
	const int32_t iCapacity0 =
		static_cast<int32_t>(constraintOfBody.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK);
	if (iCapacity0 < numBodies) {
		int32_t iRequest = 2 * iCapacity0;
		if (numBodies >= iRequest) {
			iRequest = numBodies;
		}
		hkArrayUtil::_reserve(*pHeap0, &constraintOfBody, iRequest, sizeof(int32_t));
	}
	for (int32_t i = constraintOfBody.m_size; i < numBodies; ++i) {
		constraintOfBody.m_data[i] = 0;
	}
	constraintOfBody.m_size = numBodies;

	// EXE-INLINED none: hkLocalArray<int>::hkLocalArray
	FNVD_INLINED_LOCAL(hkLocalArray<int32_t>, order, std::nullopt, std::nullopt);
	order.m_data = nullptr;
	order.m_size = 0;
	order.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
	hkStackMemory& rStack1 = hkMemoryRouter::getInstance().m_stack;
	int32_t* pOrderMemory = static_cast<int32_t*>(static_cast<void*>(rStack1.m_next.m_storage));
	rStack1.m_next.m_storage += iBodyBlockBytes;
	order.m_data = pOrderMemory;
	order.m_capacityAndFlags = iBodyCapacityAndFlags;
	order.m_localMemory = pOrderMemory;
	order.m_size = numBodies;

	// EXE-INLINED none: hkLocalArray<int>::hkLocalArray
	FNVD_INLINED_LOCAL(hkLocalArray<int32_t>, parent, std::nullopt, std::nullopt);
	parent.m_data = nullptr;
	parent.m_size = 0;
	parent.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
	hkStackMemory& rStack2 = hkMemoryRouter::getInstance().m_stack;
	int32_t* pParentMemory = static_cast<int32_t*>(static_cast<void*>(rStack2.m_next.m_storage));
	rStack2.m_next.m_storage += iBodyBlockBytes;
	parent.m_data = pParentMemory;
	parent.m_capacityAndFlags = iBodyCapacityAndFlags;
	parent.m_localMemory = pParentMemory;
	// EXE-INLINED none: hkArray<int>::setSize(numBodies, -1)
	hkMemoryAllocator* pHeap2 = hkMemoryRouter::getInstance().m_heap;
	const int32_t iCapacity2 = static_cast<int32_t>(parent.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK);
	if (iCapacity2 < numBodies) {
		int32_t iRequest = 2 * iCapacity2;
		if (numBodies >= iRequest) {
			iRequest = numBodies;
		}
		hkArrayUtil::_reserve(*pHeap2, &parent, iRequest, sizeof(int32_t));
	}
	for (int32_t i = parent.m_size; i < numBodies; ++i) {
		parent.m_data[i] = -1;
	}
	parent.m_size = numBodies;

	bool bValid = true;
	for (int32_t i = 0; i < numConstraints; ++i) {
		hkpConstraintInstance* pConstraint = constraints.m_data[i];
		hkpRigidBody* pChildBody = pConstraint->getRigidBodyA();
		hkpRigidBody* pParentBody = pConstraint->getRigidBodyB();
		const int32_t iNumCandidates = rigidBodies.m_size;
		// EXE-INLINED none: hkArray<hkpRigidBody*>::indexOf
		int32_t iChild = -1;
		for (int32_t j = 0; j < iNumCandidates; ++j) {
			if (rigidBodies.m_data[j] == pChildBody) {
				iChild = j;
				break;
			}
		}
		// EXE-INLINED none: hkArray<hkpRigidBody*>::indexOf
		int32_t iParent = -1;
		for (int32_t j = 0; j < iNumCandidates; ++j) {
			if (rigidBodies.m_data[j] == pParentBody) {
				iParent = j;
				break;
			}
		}
		if (iChild < 0 || iParent < 0 || parent.m_data[iChild] != -1 || constraintOfBody.m_data[iChild] != 0) {
			bValid = false;
			break;
		}
		constraintOfBody.m_data[iChild] = i;
		parent.m_data[iChild] = iParent;
	}
	if (!bValid) {
		parent.~hkLocalArray<int32_t>();
		order.~hkLocalArray<int32_t>();
		constraintOfBody.~hkLocalArray<int32_t>();
		return HK_FAILURE;
	}

	{
		// EXE-INLINED none: hkLocalArray<int>::hkLocalArray
		FNVD_INLINED_LOCAL(hkLocalArray<int32_t>, depth, std::nullopt, std::nullopt);
		depth.m_data = nullptr;
		depth.m_size = 0;
		depth.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
		hkStackMemory& rStack3 = hkMemoryRouter::getInstance().m_stack;
		int32_t* pDepthMemory = static_cast<int32_t*>(static_cast<void*>(rStack3.m_next.m_storage));
		rStack3.m_next.m_storage += iBodyBlockBytes;
		depth.m_data = pDepthMemory;
		depth.m_capacityAndFlags = iBodyCapacityAndFlags;
		depth.m_localMemory = pDepthMemory;
		// EXE-INLINED none: hkArray<int>::setSize(numBodies, 0)
		hkMemoryAllocator* pHeap3 = hkMemoryRouter::getInstance().m_heap;
		const int32_t iCapacity3 = static_cast<int32_t>(depth.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK);
		if (iCapacity3 < numBodies) {
			int32_t iRequest = 2 * iCapacity3;
			if (numBodies >= iRequest) {
				iRequest = numBodies;
			}
			hkArrayUtil::_reserve(*pHeap3, &depth, iRequest, sizeof(int32_t));
		}
		for (int32_t i = depth.m_size; i < numBodies; ++i) {
			depth.m_data[i] = 0;
		}
		depth.m_size = numBodies;

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

		depth.~hkLocalArray<int32_t>();
	}

	// EXE-INLINED none: hkLocalArray<hkpRigidBody*>::hkLocalArray
	FNVD_INLINED_LOCAL(hkLocalArray<hkpRigidBody*>, newBodies, std::nullopt, std::nullopt);
	newBodies.m_data = nullptr;
	newBodies.m_size = 0;
	newBodies.m_capacityAndFlags = hkArrayBase<hkpRigidBody*>::DONT_DEALLOCATE_FLAG;
	hkStackMemory& rStack4 = hkMemoryRouter::getInstance().m_stack;
	hkpRigidBody** pNewBodiesMemory = static_cast<hkpRigidBody**>(static_cast<void*>(rStack4.m_next.m_storage));
	rStack4.m_next.m_storage += (numBodies * static_cast<int32_t>(sizeof(hkpRigidBody*)) + 127) & ~127;
	newBodies.m_data = pNewBodiesMemory;
	newBodies.m_capacityAndFlags = iBodyCapacityAndFlags;
	newBodies.m_localMemory = pNewBodiesMemory;
	newBodies.m_size = numBodies;

	// EXE-INLINED none: hkLocalArray<hkpConstraintInstance*>::hkLocalArray
	FNVD_INLINED_LOCAL(hkLocalArray<hkpConstraintInstance*>, newConstraints, std::nullopt, std::nullopt);
	newConstraints.m_data = nullptr;
	newConstraints.m_size = 0;
	newConstraints.m_capacityAndFlags = hkArrayBase<hkpConstraintInstance*>::DONT_DEALLOCATE_FLAG;
	hkStackMemory& rStack5 = hkMemoryRouter::getInstance().m_stack;
	hkpConstraintInstance** pNewConstraintsMemory =
		static_cast<hkpConstraintInstance**>(static_cast<void*>(rStack5.m_next.m_storage));
	rStack5.m_next.m_storage += (numConstraints * static_cast<int32_t>(sizeof(hkpConstraintInstance*)) + 127) & ~127;
	newConstraints.m_data = pNewConstraintsMemory;
	newConstraints.m_capacityAndFlags = static_cast<int32_t>(static_cast<uint32_t>(numConstraints) |
		hkArrayBase<hkpConstraintInstance*>::DONT_DEALLOCATE_FLAG);
	newConstraints.m_localMemory = pNewConstraintsMemory;
	newConstraints.m_size = numConstraints;

	for (int32_t i = 0; i < numBodies; ++i) {
		const int32_t iBody = order.m_data[i];
		newBodies.m_data[i] = rigidBodies.m_data[iBody];
		if (i > 0) {
			const int32_t iConstraint = constraintOfBody.m_data[iBody];
			if (iConstraint < 0) {
				newConstraints.m_data[i - 1] = nullptr;
			} else {
				newConstraints.m_data[i - 1] = constraints.m_data[iConstraint];
			}
		}
	}

	// EXE-INLINED none: hkArray<hkpRigidBody*>::operator=
	hkMemoryAllocator* pBodyHeap = hkMemoryRouter::getInstance().m_heap;
	const int32_t iBodiesCapacityAndFlags = rigidBodies.m_capacityAndFlags;
	if (static_cast<int32_t>(iBodiesCapacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK) < numBodies) {
		if (iBodiesCapacityAndFlags >= 0) {
			pBodyHeap->bufFree(rigidBodies.m_data,
				static_cast<int32_t>((iBodiesCapacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK) *
					sizeof(hkpRigidBody*)));
		}
		int32_t iBytes = numBodies * static_cast<int32_t>(sizeof(hkpRigidBody*));
		rigidBodies.m_data = static_cast<hkpRigidBody**>(pBodyHeap->bufAlloc(iBytes));
		rigidBodies.m_capacityAndFlags = iBytes / static_cast<int32_t>(sizeof(hkpRigidBody*));
	}
	rigidBodies.m_size = numBodies;
	for (int32_t i = 0; i < numBodies; ++i) {
		rigidBodies.m_data[i] = newBodies.m_data[i];
	}

	// EXE-INLINED none: hkArray<hkpConstraintInstance*>::operator=
	hkMemoryAllocator* pConstraintHeap = hkMemoryRouter::getInstance().m_heap;
	const int32_t iConstraintsCapacityAndFlags = constraints.m_capacityAndFlags;
	if (static_cast<int32_t>(iConstraintsCapacityAndFlags & hkArrayBase<hkpConstraintInstance*>::CAPACITY_MASK) <
		numConstraints) {
		if (iConstraintsCapacityAndFlags >= 0) {
			pConstraintHeap->bufFree(constraints.m_data,
				static_cast<int32_t>((iConstraintsCapacityAndFlags &
										 hkArrayBase<hkpConstraintInstance*>::CAPACITY_MASK) *
					sizeof(hkpConstraintInstance*)));
		}
		int32_t iBytes = numConstraints * static_cast<int32_t>(sizeof(hkpConstraintInstance*));
		constraints.m_data = static_cast<hkpConstraintInstance**>(pConstraintHeap->bufAlloc(iBytes));
		constraints.m_capacityAndFlags = iBytes / static_cast<int32_t>(sizeof(hkpConstraintInstance*));
	}
	constraints.m_size = numConstraints;
	for (int32_t i = 0; i < numConstraints; ++i) {
		constraints.m_data[i] = newConstraints.m_data[i];
	}

	newConstraints.~hkLocalArray<hkpConstraintInstance*>();
	newBodies.~hkLocalArray<hkpRigidBody*>();
	parent.~hkLocalArray<int32_t>();
	order.~hkLocalArray<int32_t>();
	constraintOfBody.~hkLocalArray<int32_t>();
	return HK_SUCCESS;
}

hkResult hkaRagdollUtils::reorderAndAlignForRagdoll(hkArray<hkpRigidBody*, hkContainerHeapAllocator>& rigidBodiesInOut,
	hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& constraintsInOut, hkBool warnOnAlignment) {
	return static_cast<hkResult>(hkaRagdollUtils::reorderForRagdoll(rigidBodiesInOut, constraintsInOut) ||
		alignChildrenToConstraints(constraintsInOut, warnOnAlignment) ||
		alignConstraintSpacesToChildren(constraintsInOut) != HK_SUCCESS);
}

// For each bone, the candidate rigid body of the bone's name (a warning for each further one of that name) and, for a
// bone with a parent, the candidate constraint from that body to the parent bone's body (a warning for each further
// one); the instance is built from the bodies, the constraints and the bone-to-body map. A bone without a name, a body
// without its constraint or no body at all is a warning and no instance.
hkaRagdollInstance* hkaRagdollUtils::createRagdollInstanceFromSkeleton(const hkaSkeleton* skeleton,
	const hkArrayBase<hkpRigidBody*>& candidateRBs, const hkArrayBase<hkpConstraintInstance*>& candidateConstraints) {
	const int32_t numBones = skeleton->m_bones.m_size;
	const int32_t iBoneBlockBytes = (numBones * 4 + 127) & ~127;
	const int32_t iBoneCapacityAndFlags =
		static_cast<int32_t>(static_cast<uint32_t>(numBones) | hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG);

	// EXE-INLINED none: hkLocalArray<hkpRigidBody*>::hkLocalArray
	FNVD_INLINED_LOCAL(hkLocalArray<hkpRigidBody*>, rigidBodies, std::nullopt, std::nullopt);
	rigidBodies.m_data = nullptr;
	rigidBodies.m_size = 0;
	rigidBodies.m_capacityAndFlags = hkArrayBase<hkpRigidBody*>::DONT_DEALLOCATE_FLAG;
	hkStackMemory& rStack0 = hkMemoryRouter::getInstance().m_stack;
	hkpRigidBody** pBodiesMemory = static_cast<hkpRigidBody**>(static_cast<void*>(rStack0.m_next.m_storage));
	rStack0.m_next.m_storage += (numBones * static_cast<int32_t>(sizeof(hkpRigidBody*)) + 127) & ~127;
	rigidBodies.m_data = pBodiesMemory;
	rigidBodies.m_capacityAndFlags = iBoneCapacityAndFlags;
	rigidBodies.m_localMemory = pBodiesMemory;

	// EXE-INLINED none: hkLocalArray<hkpConstraintInstance*>::hkLocalArray
	FNVD_INLINED_LOCAL(hkLocalArray<hkpConstraintInstance*>, constraints, std::nullopt, std::nullopt);
	constraints.m_data = nullptr;
	constraints.m_size = 0;
	constraints.m_capacityAndFlags = hkArrayBase<hkpConstraintInstance*>::DONT_DEALLOCATE_FLAG;
	hkStackMemory& rStack1 = hkMemoryRouter::getInstance().m_stack;
	hkpConstraintInstance** pConstraintsMemory =
		static_cast<hkpConstraintInstance**>(static_cast<void*>(rStack1.m_next.m_storage));
	rStack1.m_next.m_storage += ((numBones - 1) * static_cast<int32_t>(sizeof(hkpConstraintInstance*)) + 127) & ~127;
	constraints.m_capacityAndFlags = static_cast<int32_t>(static_cast<uint32_t>(numBones - 1) |
		hkArrayBase<hkpConstraintInstance*>::DONT_DEALLOCATE_FLAG);
	constraints.m_data = pConstraintsMemory;
	constraints.m_localMemory = pConstraintsMemory;

	// EXE-INLINED none: hkLocalArray<int>::hkLocalArray
	FNVD_INLINED_LOCAL(hkLocalArray<int32_t>, boneToRigidBody, std::nullopt, std::nullopt);
	boneToRigidBody.m_data = nullptr;
	boneToRigidBody.m_size = 0;
	boneToRigidBody.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
	hkStackMemory& rStack2 = hkMemoryRouter::getInstance().m_stack;
	int32_t* pMapMemory = static_cast<int32_t*>(static_cast<void*>(rStack2.m_next.m_storage));
	rStack2.m_next.m_storage += iBoneBlockBytes;
	boneToRigidBody.m_data = pMapMemory;
	boneToRigidBody.m_localMemory = pMapMemory;
	boneToRigidBody.m_capacityAndFlags = iBoneCapacityAndFlags;

	char buf[512];
	bool bFailed = false;
	for (int32_t i = 0; i < numBones; ++i) {
		const hkaBone& bone = skeleton->m_bones.m_data[i];
		const int16_t parentBone = skeleton->m_parentIndices.m_data[i];
		if (!bone.m_name.cString()) {
			hkErrStream kStream(buf, 512);
			kStream << "Some bones have no name - can't create rag doll";
			hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAE6A2), buf,
				".\\Utils\\hkaRagdollUtils.cpp", 675);
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
					hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA98AA), buf,
						".\\Utils\\hkaRagdollUtils.cpp", 619);
				} else {
					pBody = pCandidate;
				}
			}
		}
		if (!pBody) {
			// EXE-INLINED none: hkArray<int>::pushBack
			hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
			if (boneToRigidBody.m_size ==
				static_cast<int32_t>(boneToRigidBody.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*rRouter.m_heap, &boneToRigidBody, sizeof(int32_t));
			}
			boneToRigidBody.m_data[boneToRigidBody.m_size] = -1;
			boneToRigidBody.m_size += 1;
			continue;
		}
		// The new body's index is read before the push's router call.
		const int32_t iNewBody = rigidBodies.m_size;
		// EXE-INLINED none: hkArray<int>::pushBack
		hkMemoryRouter& rMapRouter = hkMemoryRouter::getInstance();
		if (boneToRigidBody.m_size ==
			static_cast<int32_t>(boneToRigidBody.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*rMapRouter.m_heap, &boneToRigidBody, sizeof(int32_t));
		}
		boneToRigidBody.m_data[boneToRigidBody.m_size] = iNewBody;
		boneToRigidBody.m_size += 1;
		// EXE-INLINED none: hkArray<hkpRigidBody*>::pushBack
		hkMemoryRouter& rBodyRouter = hkMemoryRouter::getInstance();
		if (rigidBodies.m_size ==
			static_cast<int32_t>(rigidBodies.m_capacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*rBodyRouter.m_heap, &rigidBodies, sizeof(hkpRigidBody*));
		}
		rigidBodies.m_data[rigidBodies.m_size] = pBody;
		rigidBodies.m_size += 1;
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
					hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA82FF), buf,
						".\\Utils\\hkaRagdollUtils.cpp", 657);
				} else {
					pConstraint = pCandidate;
				}
			}
		}
		if (!pConstraint) {
			hkErrStream kStream(buf, 512);
			kStream << "Couldn't find constraint for bone : " << bone.m_name;
			hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA9F9F), buf,
				".\\Utils\\hkaRagdollUtils.cpp", 664);
			bFailed = true;
			break;
		}
		// EXE-INLINED none: hkArray<hkpConstraintInstance*>::pushBack
		hkMemoryRouter& rConstraintRouter = hkMemoryRouter::getInstance();
		if (constraints.m_size ==
			static_cast<int32_t>(constraints.m_capacityAndFlags & hkArrayBase<hkpConstraintInstance*>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*rConstraintRouter.m_heap, &constraints, sizeof(hkpConstraintInstance*));
		}
		constraints.m_data[constraints.m_size] = pConstraint;
		constraints.m_size += 1;
	}

	hkaRagdollInstance* pInstance = nullptr;
	if (!bFailed) {
		if (rigidBodies.m_size == 0) {
			hkErrStream kStream(buf, 512);
			kStream << "Skeleton has no associated rigid bodies - can't create rag doll";
			hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAE6E4), buf,
				".\\Utils\\hkaRagdollUtils.cpp", 682);
		} else {
			// EXE-INLINED 0x56D280
			void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkaRagdollInstance));
			static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkaRagdollInstance);
			pInstance = ::new (pMemory) hkaRagdollInstance(rigidBodies, constraints, skeleton, boneToRigidBody);
		}
	}

	boneToRigidBody.~hkLocalArray<int32_t>();
	constraints.~hkLocalArray<hkpConstraintInstance*>();
	rigidBodies.~hkLocalArray<hkpRigidBody*>();
	return pInstance;
}

// Builds a new skeleton with a bone per rigid body. A bone's parent is the body the constraint before it attaches as
// its second entity, and its reference pose is the body's transform relative to the parent's (setMulInverseMul).
hkaSkeleton* hkaRagdollUtils::constructSkeletonForRagdoll(const hkArrayBase<hkpRigidBody*>& rigidBodies,
	const hkArrayBase<hkpConstraintInstance*>& constraints) {
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
		} while (apThis_8);
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
		} while (apThis_8a);
	}
	v3->m_bones.m_size = m_size;
	// EXE-INLINED none: hkArray<hkInt16>::setSize
	hkMemoryAllocator* m_heap_1 = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t m_size_6 = v3->m_parentIndices.m_capacityAndFlags & hkArrayBase<int16_t>::CAPACITY_MASK;
	if (m_size_6 < m_size) {
		int32_t m_size_5 = 2 * m_size_6;
		if (m_size >= m_size_5) {
			m_size_5 = m_size;
		}
		hkArrayUtil::_reserve(*m_heap_1, &v3->m_parentIndices, m_size_5, sizeof(int16_t));
	}
	v3->m_parentIndices.m_size = m_size;
	// EXE-INLINED none: hkArray<hkQsTransform>::setSize
	hkMemoryAllocator* m_heap_2 = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t m_size_8 = v3->m_referencePose.m_capacityAndFlags & hkArrayBase<hkQsTransform>::CAPACITY_MASK;
	if (m_size_8 < m_size) {
		int32_t m_size_7 = 2 * m_size_8;
		if (m_size >= m_size_7) {
			m_size_7 = m_size;
		}
		hkArrayUtil::_reserve(*m_heap_2, &v3->m_referencePose, m_size_7, sizeof(hkQsTransform));
	}
	v3->m_referencePose.m_size = m_size;
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
				v3->m_referencePose.m_data[apThis_8b]
					.setFromTransformNoScale(pBody->m_motion.m_motionState.m_transform);
			} else {
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
			} else {
				alignas(16) char buf[512];
				hkErrStream errStream(buf, 512);
				errStream.operator<<("Rigid Body name could not be found");
				hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBACC14), buf,
					".\\Utils\\hkaRagdollUtils.cpp", 558);
			}
			char m_bool = m_size_9 != 0;
			++apThis_8b;
			++m_size_9;
			pBone->m_lockTranslation.m_bool = m_bool;
		} while (m_size_9 < m_size_1);
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
	// hkpLimitedHingeConstraintData is not declared; its atoms start with the same hkpSetLocalTransformsConstraintAtom
	// at the same offset as the hinge's, so both are read through hkpHingeConstraintData.
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
		hkError::s_instance->message(hkError::MESSAGE_WARNING, -0x544540C5, buf, ".\\Utils\\hkaRagdollUtils.cpp", 180);
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
	// hkpLimitedHingeConstraintData is not declared; its atoms start with the same hkpSetLocalTransformsConstraintAtom
	// at the same offset as the hinge's, so both are read through hkpHingeConstraintData.
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
		hkError::s_instance->message(hkError::MESSAGE_WARNING, -0x544540C5, buf, ".\\Utils\\hkaRagdollUtils.cpp", 230);
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
	hkpGroupFilterUtil::disableCollisionsBetweenConstraintBodies(ragdoll->m_constraints.begin(),
		ragdoll->m_constraints.m_size, ++collisionFilter->m_nextFreeSystemGroup);
}

hkaRagdollInstance* hkaRagdollUtils::copyRagdollInstanceShallow(const hkaRagdollInstance* original) {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkaRagdollInstance* v2 = static_cast<hkaRagdollInstance*>(Instance->m_heap->blockAlloc(sizeof(hkaRagdollInstance)));
	// The native sizeof, since the heap frees by this value.
	v2->m_memSizeAndFlags = sizeof(hkaRagdollInstance);
	return ::new (v2) hkaRagdollInstance(original->m_rigidBodies, original->m_constraints, original->m_skeleton,
		original->m_boneToRigidBodyMap);
}
