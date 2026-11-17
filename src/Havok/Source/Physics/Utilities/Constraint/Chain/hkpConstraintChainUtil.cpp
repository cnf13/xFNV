#include "Havok/Source/Physics/Utilities/Constraint/Chain/hkpConstraintChainUtil.h"

#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/BallSocket/hkpBallSocketChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/Powered/hkpPoweredChainData.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Utilities/Constraint/Bilateral/hkpConstraintUtils.h"

// The SDK's buildPoweredChain: a powered chain over the bodies the ordered constraints link, each link with the
// constraint's pivots, the relative rotation of its bodies and its motors (the first motor standing in for a missing
// one, all three cloned on request); a constraint without pivots or motors, or not linked to the chain's last body,
// warns and gives null (cdecl). The exe realigns its frame: the pivots, the rotation, the buffer and the first two
// warnings' streams sit on 16-byte boundaries.
hkpConstraintChainInstance*
hkpConstraintChainUtil::buildPoweredChain(hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& constraints,
	hkBool cloneMotors) {
	hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
	hkpPoweredChainData* pDataMemory =
		static_cast<hkpPoweredChainData*>(pRouter->m_heap->blockAlloc(sizeof(hkpPoweredChainData)));
	pDataMemory->m_memSizeAndFlags = sizeof(hkpPoweredChainData);
	hkpPoweredChainData* pChainData = ::new (pDataMemory) hkpPoweredChainData;
	hkMemoryRouter* pRouter2 = &hkMemoryRouter::getInstance();
	hkpConstraintChainInstance* pInstanceMemory =
		static_cast<hkpConstraintChainInstance*>(pRouter2->m_heap->blockAlloc(sizeof(hkpConstraintChainInstance)));
	pInstanceMemory->m_memSizeAndFlags = sizeof(hkpConstraintChainInstance);
	hkpConstraintChainInstance* pChainInstance = ::new (pInstanceMemory) hkpConstraintChainInstance(pChainData);
	{
		// The chain starts at the first constraint's body that the second constraint does not share.
		hkpEntity* pFirstEntity = constraints.m_data[0]->m_entities[0];
		if (constraints.m_size > 1) {
			const hkpConstraintInstance* pSecond = constraints.m_data[1];
			if (pFirstEntity == pSecond->m_entities[0] || pFirstEntity == pSecond->m_entities[1]) {
				pFirstEntity = constraints.m_data[0]->m_entities[1];
			}
		}
		pChainInstance->addEntity(pFirstEntity);
	}
	alignas(16) char buf[512];
	for (int32_t c = 0; c < constraints.m_size; ++c) {
		hkVector4 pivotInA;
		hkVector4 pivotInB;
		if (hkpConstraintUtils::getConstraintPivots(constraints.m_data[c], pivotInA, pivotInB) == HK_FAILURE) {
			alignas(16) FNVD_INLINED_LOCAL(hkErrStream, stream, std::nullopt, std::nullopt);
			::new (&stream) hkErrStream(buf, 512);
			stream << "Not supported types of constraints used to build a chain!";
			hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAD88Du), buf,
				".\\Constraint\\Chain\\hkpConstraintChainUtil.cpp", 183);
			FNVD_DESTRUCT(hkOstream)(&stream);
			pChainData->removeReference();
			pChainInstance->removeReference();
			return nullptr;
		}
		hkpConstraintInstance** ppInstance = &constraints.m_data[c];
		if ((*ppInstance)->m_entities[0] !=
			pChainInstance->m_chainedEntities.m_data[pChainInstance->m_chainedEntities.m_size - 1]) {
			// Entity B is the chain's last body: the pivots swap.
			const hkVector4 pivotTmp = pivotInA;
			pivotInA = pivotInB;
			pivotInB = pivotTmp;
			if ((*ppInstance)->m_entities[1] !=
				pChainInstance->m_chainedEntities.m_data[pChainInstance->m_chainedEntities.m_size - 1]) {
				alignas(16) FNVD_INLINED_LOCAL(hkErrStream, stream, std::nullopt, std::nullopt);
				::new (&stream) hkErrStream(buf, 512);
				stream << "Constraints are not ordered properly ! Two consecutive constraint share no common "
						  "hkpEntity.";
				hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAD88Du), buf,
					".\\Constraint\\Chain\\hkpConstraintChainUtil.cpp", 194);
				FNVD_DESTRUCT(hkOstream)(&stream);
				pChainData->removeReference();
				pChainInstance->removeReference();
				return nullptr;
			}
		}
		hkQuaternion aTc;
		aTc.setInverseMul((*ppInstance)->m_entities[0]->m_motion.m_motionState.m_sweptTransform.m_rotation1,
			(*ppInstance)->m_entities[1]->m_motion.m_motionState.m_sweptTransform.m_rotation1);
		hkpConstraintMotor* pMotor0;
		hkpConstraintMotor* pMotor1;
		hkpConstraintMotor* pMotor2;
		hkpConstraintUtils::getConstraintMotors((*ppInstance)->getData(), pMotor0, pMotor1, pMotor2);
		if (!pMotor0) {
			FNVD_INLINED_LOCAL(hkErrStream, stream, std::nullopt, std::nullopt);
			::new (&stream) hkErrStream(buf, 512);
			stream << "No motors extracted from the original constraint.";
			hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA88D3u), buf,
				".\\Constraint\\Chain\\hkpConstraintChainUtil.cpp", 207);
			FNVD_DESTRUCT(hkOstream)(&stream);
			pChainData->removeReference();
			pChainInstance->removeReference();
			return nullptr;
		}
		if (!pMotor1) {
			pMotor1 = pMotor0;
		}
		if (!pMotor2) {
			pMotor2 = pMotor0;
		}
		if (cloneMotors.m_bool) {
			pMotor0 = pMotor0->clone();
			pMotor1 = pMotor1->clone();
			pMotor2 = pMotor2->clone();
		}
		pChainData->addConstraintInfoInBodySpace(pivotInA, pivotInB, aTc, pMotor0, pMotor1, pMotor2);
		hkpEntity* pEntityA = constraints.m_data[c]->m_entities[0];
		if (pEntityA == pChainInstance->m_chainedEntities.m_data[pChainInstance->m_chainedEntities.m_size - 1]) {
			pChainData->m_infos.m_data[pChainData->m_infos.m_size - 1].m_switchBodies.m_bool = true;
			pChainInstance->addEntity(constraints.m_data[c]->m_entities[1]);
		} else {
			pChainInstance->addEntity(pEntityA);
		}
	}
	pChainData->removeReference();
	return pChainInstance;
}

// The SDK's addConstraintToBallSocketChain: a link from the chain's last body to entityToAppend at the world point
// pivotWs, its pivot in each body's space, then entityToAppend appended. The bodies are the collidables'
// owners when they are entities, else null.
void hkpConstraintChainUtil::addConstraintToBallSocketChain(hkpConstraintChainInstance* instance,
	hkpEntity* entityToAppend, const hkVector4& pivotWs) {
	hkpBallSocketChainData* pData = static_cast<hkpBallSocketChainData*>(instance->getDataRw());
	const hkpCollidable& kCollidableA =
		instance->m_chainedEntities.m_data[instance->m_chainedEntities.m_size - 1]->m_collidable;
	// EXE-INLINED 0x4B59F0
	hkpRigidBody* pBodyA = kCollidableA.m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY
		? static_cast<hkpRigidBody*>(kCollidableA.getOwner())
		: nullptr;
	const hkpCollidable& kCollidableB = entityToAppend->m_collidable;
	// EXE-INLINED 0x4B59F0
	hkpRigidBody* pBodyB = kCollidableB.m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY
		? static_cast<hkpRigidBody*>(kCollidableB.getOwner())
		: nullptr;
	hkVector4 pivotInA;
	pivotInA.setTransformedInversePos(pBodyA->m_motion.m_motionState.m_transform, pivotWs);
	hkVector4 pivotInB;
	pivotInB.setTransformedInversePos(pBodyB->m_motion.m_motionState.m_transform, pivotWs);
	pData->addConstraintInfoInBodySpace(pivotInA, pivotInB);
	instance->addEntity(entityToAppend);
}

// The SDK's addConstraintToPoweredChain: as addConstraintToBallSocketChain, with the rotation of the appended body
// relative to the chain's last body and the three motors.
void hkpConstraintChainUtil::addConstraintToPoweredChain(hkpConstraintChainInstance* instance,
	hkpEntity* entityToAppend, const hkVector4& pivotWs, hkpConstraintMotor* motor0, hkpConstraintMotor* motor1,
	hkpConstraintMotor* motor2) {
	hkpPoweredChainData* pData = static_cast<hkpPoweredChainData*>(instance->getDataRw());
	const hkpCollidable& kCollidableA =
		instance->m_chainedEntities.m_data[instance->m_chainedEntities.m_size - 1]->m_collidable;
	// EXE-INLINED 0x4B59F0
	hkpRigidBody* pBodyA = kCollidableA.m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY
		? static_cast<hkpRigidBody*>(kCollidableA.getOwner())
		: nullptr;
	const hkpCollidable& kCollidableB = entityToAppend->m_collidable;
	// EXE-INLINED 0x4B59F0
	hkpRigidBody* pBodyB = kCollidableB.m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY
		? static_cast<hkpRigidBody*>(kCollidableB.getOwner())
		: nullptr;
	hkVector4 pivotInA;
	pivotInA.setTransformedInversePos(pBodyA->m_motion.m_motionState.m_transform, pivotWs);
	hkVector4 pivotInB;
	pivotInB.setTransformedInversePos(pBodyB->m_motion.m_motionState.m_transform, pivotWs);
	hkQuaternion aTb;
	aTb.setMulInverse(pBodyB->m_motion.m_motionState.m_sweptTransform.m_rotation1,
		pBodyA->m_motion.m_motionState.m_sweptTransform.m_rotation1);
	pData->addConstraintInfoInBodySpace(pivotInA, pivotInB, aTb, motor0, motor1, motor2);
	instance->addEntity(entityToAppend);
}

// The SDK's getAdjointEntities: for each constraint on entity, the other body goes to entitiesOut and the constraint
// to constraintsOut.
void hkpConstraintChainUtil::getAdjointEntities(const hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>&
													allConstraints,
	const hkpEntity* entity, hkArray<hkpEntity*, hkContainerHeapAllocator>& entitiesOut,
	hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& constraintsOut) {
	for (int32_t i = 0; i < allConstraints.m_size; ++i) {
		hkpConstraintInstance* pInstance = allConstraints.m_data[i];
		hkpEntity* pEntityA = pInstance->m_entities[0];
		if (pEntityA == entity || pInstance->m_entities[1] == entity) {
			hkpEntity* pOther =
				hkSelectOther<hkpEntity*>(const_cast<hkpEntity*>(entity), pEntityA, pInstance->m_entities[1]);
			{
				// EXE-INLINED none: hkArray<hkpEntity*>::pushBack
				hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
				if (entitiesOut.m_size == (entitiesOut.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*pRouter->m_heap, &entitiesOut, sizeof(hkpEntity*));
				}
				entitiesOut.m_data[entitiesOut.m_size] = pOther;
				entitiesOut.m_size += 1;
			}
			{
				// EXE-INLINED none: hkArray<hkpConstraintInstance*>::pushBack
				hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
				if (constraintsOut.m_size ==
					(constraintsOut.m_capacityAndFlags & hkArrayBase<hkpConstraintInstance*>::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*pRouter->m_heap, &constraintsOut, sizeof(hkpConstraintInstance*));
				}
				constraintsOut.m_data[constraintsOut.m_size] = pInstance;
				constraintsOut.m_size += 1;
			}
		}
	}
}

// The SDK's findConstraintLinkBetweenEntities: a breadth-first search from both end bodies at once, a step from each
// side in turn, over the bodies the constraints link; where the two searches meet, the bodies from entityA to entityB
// and the constraints between them go out. Each body gets an index (the map, in the order the constraints name the
// bodies) and a search record: the side that reached it (-1: none yet), the body it was reached from, the constraint
// it was reached through and its distance from its side's end. HK_FAILURE when the ends are the same body (with a
// warning) or no path joins them.
hkResult hkpConstraintChainUtil::findConstraintLinkBetweenEntities(const hkArray<hkpConstraintInstance*,
																	   hkContainerHeapAllocator>& allConstraints,
	hkpEntity* entityA, hkpEntity* entityB, hkArray<hkpEntity*, hkContainerHeapAllocator>& entitiesOut,
	hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& constraintsOut) {
	if (entityA == entityB) {
		char buf[512];
		FNVD_INLINED_LOCAL(hkErrStream, stream, std::nullopt, std::nullopt);
		::new (&stream) hkErrStream(buf, 512);
		stream << "Specify two different end bodies.";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA3BB3u), buf,
			".\\Constraint\\Chain\\hkpConstraintChainUtil.cpp", 250);
		FNVD_DESTRUCT(hkOstream)(&stream);
		return HK_FAILURE;
	}

	// Not named in the PDB or IDA; name chosen here. A body's search record (16 bytes in the exe).
	struct EntityInfo {
		int32_t m_side;
		int32_t m_parent;
		hkpConstraintInstance* m_constraint;
		int32_t m_depth;
	};
	using EntityIndexMap = hkPointerMap<hkpEntity*, int32_t>;
	using EntityIndexMapBase = hkUlongMapBase;
	using EntityInfoArray = hkInplaceArray<EntityInfo, 32>;
	using EntityQueues = hkInplaceArray<int32_t, 32>[2];
	using AdjointEntities = hkInplaceArray<hkpEntity*, 32>;
	using AdjointConstraints = hkInplaceArray<hkpConstraintInstance*, 32>;

	// EXE-INLINED none: hkArray<hkpEntity*>::hkArray
	FNVD_INLINED_LOCAL(hkArray<hkpEntity*>, entities, std::nullopt, std::nullopt);
	entities.m_data = nullptr;
	entities.m_size = 0;
	entities.m_capacityAndFlags = hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG;
	// EXE-INLINED 0x626560: hkMap<hkUlong, hkUlong>::hkMap(0), with its reserve (0x626600) inlined too
	FNVD_INLINED_LOCAL(EntityIndexMap, entityToIndex, std::nullopt, std::nullopt);
	entityToIndex.m_map.m_elem = nullptr;
	entityToIndex.m_map.m_numElems = 0;
	entityToIndex.m_map.m_hashMod = -1;
	static_cast<EntityIndexMapBase&>(entityToIndex.m_map).reserve(hkMemoryRouter::getInstance().heap(), 0);
	for (int32_t i = 0; i < allConstraints.m_size; ++i) {
		const hkpConstraintInstance* pConstraint = allConstraints.m_data[i];
		for (int32_t e = 0; e < 2; ++e) {
			if (!entityToIndex.isValid(entityToIndex.findKey(pConstraint->m_entities[e]))) {
				entityToIndex.insert(pConstraint->m_entities[e], entities.m_size);
				hkpEntity* pEntity = pConstraint->m_entities[e];
				// EXE-INLINED none: hkArray<hkpEntity*>::pushBack
				hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
				if (entities.m_size == entities.getCapacity()) {
					hkArrayUtil::_reserveMore(heap, &entities, sizeof(hkpEntity*));
				}
				entities.m_data[entities.m_size] = pEntity;
				entities.m_size++;
			}
		}
	}

	const int32_t numEntities = entities.m_size;
	// EXE-INLINED none: hkInplaceArray<EntityInfo, 32>::hkInplaceArray
	FNVD_INLINED_LOCAL(EntityInfoArray, info, std::nullopt, std::nullopt);
	info.m_data = info.m_storage;
	info.m_size = 0;
	info.m_capacityAndFlags = static_cast<int32_t>(32 | hkArrayBase<EntityInfo>::DONT_DEALLOCATE_FLAG);
	{
		// EXE-INLINED none: hkArray<EntityInfo>::setSize (the records have no constructor)
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		info._reserve(heap, numEntities);
		info.m_size = numEntities;
	}
	for (int32_t i = 0; i < info.m_size; ++i) {
		info.m_data[i].m_side = -1;
	}
	// The two sides' queues of body indices (side 0 searches from entityA, side 1 from entityB).
	FNVD_INLINED_LOCAL(EntityQueues, queues, std::nullopt, std::nullopt);
	for (int32_t k = 0; k < 2; ++k) {
		// EXE-INLINED none: hkInplaceArray<int, 32>::hkInplaceArray
		queues[k].m_data = queues[k].m_storage;
		queues[k].m_size = 0;
		queues[k].m_capacityAndFlags = static_cast<int32_t>(32 | hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG);
	}
	int32_t queuePos[2];
	queuePos[0] = -1;
	queuePos[1] = -1;
	const int32_t indexA = entityToIndex.getValue(entityToIndex.findKey(entityA));
	const int32_t indexB = entityToIndex.getValue(entityToIndex.findKey(entityB));
	info.m_data[indexA].m_side = 0;
	info.m_data[indexA].m_parent = -1;
	info.m_data[indexA].m_constraint = nullptr;
	info.m_data[indexA].m_depth = 0;
	info.m_data[indexB].m_side = 1;
	info.m_data[indexB].m_parent = -1;
	info.m_data[indexB].m_constraint = nullptr;
	info.m_data[indexB].m_depth = 0;
	{
		// EXE-INLINED none: hkArray<int>::pushBack
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		if (queues[0].m_size == queues[0].getCapacity()) {
			hkArrayUtil::_reserveMore(heap, &queues[0], sizeof(int32_t));
		}
		queues[0].m_data[queues[0].m_size] = indexA;
		queues[0].m_size++;
	}
	{
		// EXE-INLINED none: hkArray<int>::pushBack
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		if (queues[1].m_size == queues[1].getCapacity()) {
			hkArrayUtil::_reserveMore(heap, &queues[1], sizeof(int32_t));
		}
		queues[1].m_data[queues[1].m_size] = indexB;
		queues[1].m_size++;
	}

	// Where the searches meet: the length of each side's part of the path, its last body and the constraint between
	// the two parts.
	hkpConstraintInstance* pLink = nullptr;
	int32_t pathLength[2];
	int32_t meetIndex[2];
	// EXE-INLINED none: hkInplaceArray<hkpEntity*, 32>::hkInplaceArray
	FNVD_INLINED_LOCAL(AdjointEntities, adjEntities, std::nullopt, std::nullopt);
	adjEntities.m_data = adjEntities.m_storage;
	adjEntities.m_size = 0;
	adjEntities.m_capacityAndFlags = static_cast<int32_t>(32 | hkArrayBase<hkpEntity*>::DONT_DEALLOCATE_FLAG);
	// EXE-INLINED none: hkInplaceArray<hkpConstraintInstance*, 32>::hkInplaceArray
	FNVD_INLINED_LOCAL(AdjointConstraints, adjConstraints, std::nullopt, std::nullopt);
	adjConstraints.m_data = adjConstraints.m_storage;
	adjConstraints.m_size = 0;
	adjConstraints.m_capacityAndFlags =
		static_cast<int32_t>(32 | hkArrayBase<hkpConstraintInstance*>::DONT_DEALLOCATE_FLAG);
	int32_t found = 0;
	int32_t side = 0;
	bool exhausted = false;
	do {
		const int32_t pos = ++queuePos[side];
		const int32_t other = 1 - side;
		if (pos >= queues[side].m_size) {
			// This side has no body left to visit: no path joins the ends.
			exhausted = true;
			break;
		}
		const int32_t current = queues[side].m_data[pos];
		adjEntities.m_size = 0;
		adjConstraints.m_size = 0;
		getAdjointEntities(allConstraints, entities.m_data[current], adjEntities, adjConstraints);
		for (int32_t j = 0; j < adjEntities.m_size && !found; ++j) {
			const int32_t next = entityToIndex.getValue(entityToIndex.findKey(adjEntities.m_data[j]));
			const int32_t nextSide = info.m_data[next].m_side;
			if (nextSide == side) {
				continue;
			}
			if (nextSide == other) {
				pathLength[side] = info.m_data[current].m_depth + 1;
				meetIndex[side] = current;
				pathLength[other] = info.m_data[next].m_depth + 1;
				meetIndex[other] = next;
				found = 1;
				pLink = adjConstraints.m_data[j];
				break;
			}
			{
				// EXE-INLINED none: hkArray<int>::pushBack
				hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
				if (queues[side].m_size == queues[side].getCapacity()) {
					hkArrayUtil::_reserveMore(heap, &queues[side], sizeof(int32_t));
				}
				queues[side].m_data[queues[side].m_size] = next;
				queues[side].m_size++;
			}
			info.m_data[next].m_side = side;
			info.m_data[next].m_parent = queues[side].m_data[queuePos[side]];
			info.m_data[next].m_constraint = adjConstraints.m_data[j];
			info.m_data[next].m_depth = info.m_data[current].m_depth + 1;
		}
		side = other;
	} while (!found);

	if (!exhausted) {
		// The path: side 0's bodies back from where the searches met to entityA, then side 1's on to entityB; each
		// constraint sits between its two bodies, the last slot (side 1's end, which has none) dropped.
		const int32_t numPathEntities = pathLength[0] + pathLength[1];
		entitiesOut.m_size = 0;
		{
			// EXE-INLINED none: hkArray<hkpEntity*>::setSize
			hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
			entitiesOut._reserve(heap, numPathEntities);
			entitiesOut.m_size = numPathEntities;
		}
		constraintsOut.m_size = 0;
		{
			// EXE-INLINED none: hkArray<hkpConstraintInstance*>::setSize
			hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
			constraintsOut._reserve(heap, numPathEntities);
			constraintsOut.m_size = numPathEntities;
		}
		int32_t node = meetIndex[0];
		for (int32_t i = pathLength[0] - 1; i > 0; --i) {
			entitiesOut.m_data[i] = entities.m_data[node];
			constraintsOut.m_data[i - 1] = info.m_data[node].m_constraint;
			node = info.m_data[node].m_parent;
		}
		entitiesOut.m_data[0] = entities.m_data[node];
		constraintsOut.m_data[pathLength[0] - 1] = pLink;
		node = meetIndex[1];
		for (int32_t i = 0; i < pathLength[1]; ++i) {
			entitiesOut.m_data[pathLength[0] + i] = entities.m_data[node];
			constraintsOut.m_data[pathLength[0] + i] = info.m_data[node].m_constraint;
			node = info.m_data[node].m_parent;
		}
		constraintsOut.m_size--;
	}

	// EXE-INLINED none: hkInplaceArray<hkpConstraintInstance*, 32>::~hkInplaceArray
	adjConstraints.clearAndDeallocate();
	// EXE-INLINED none: hkInplaceArray<hkpEntity*, 32>::~hkInplaceArray
	adjEntities.clearAndDeallocate();
	for (int32_t k = 1; k >= 0; --k) {
		// EXE-INLINED none: hkInplaceArray<int, 32>::~hkInplaceArray
		queues[k].clearAndDeallocate();
	}
	{
		// EXE-INLINED none: hkInplaceArray<EntityInfo, 32>::~hkInplaceArray
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		info.m_size = 0;
		if ((info.m_capacityAndFlags & hkArrayBase<EntityInfo>::DONT_DEALLOCATE_FLAG) == 0) {
			heap.bufFree(info.m_data, info.getCapacity() * static_cast<int32_t>(sizeof(EntityInfo)));
		}
		info.m_data = nullptr;
		info.m_capacityAndFlags = hkArrayBase<EntityInfo>::DONT_DEALLOCATE_FLAG;
	}
	// EXE-INLINED 0x6264F0: hkMap<hkUlong, hkUlong>::~hkMap (its call of the empty 0xA29680, the operations'
	// destructor, is left out)
	static_cast<EntityIndexMapBase&>(entityToIndex.m_map).clearAndDeallocate(hkMemoryRouter::getInstance().heap());
	// EXE-INLINED none: hkArray<hkpEntity*>::~hkArray
	entities.clearAndDeallocate();
	return exhausted ? HK_FAILURE : HK_SUCCESS;
}

hkResult hkpConstraintChainUtil::addConstraintToChain(hkpConstraintInstance* newChildInstance,
	hkpConstraintChainInstance* chainInstance, hkpBallSocketChainData* chainData) {
	hkVector4 pivotInA;
	hkVector4 pivotInB;
	if (hkpConstraintUtils::getConstraintPivots(newChildInstance, pivotInA, pivotInB)) {
		return HK_FAILURE;
	}
	if (chainInstance->m_chainedEntities.m_size == 0) {
		chainInstance->addEntity(newChildInstance->getEntityA());
	}
	hkpEntity** ppChained = chainInstance->m_chainedEntities.m_data;
	hkpEntity* pLast = ppChained[chainInstance->m_chainedEntities.m_size - 1];
	hkpEntity* pEntityA = newChildInstance->getEntityA();
	if (pLast == pEntityA) {
		chainData->addConstraintInfoInBodySpace(pivotInA, pivotInB);
		chainInstance->addEntity(newChildInstance->getEntityB());
		return HK_SUCCESS;
	}
	hkpEntity* pEntityB = newChildInstance->getEntityB();
	if (pLast == pEntityB) {
		chainData->addConstraintInfoInBodySpace(pivotInB, pivotInA);
		chainInstance->addEntity(newChildInstance->getEntityA());
		return HK_SUCCESS;
	}
	hkpEntity* pFirst = ppChained[0];
	if (pFirst == pEntityA) {
		hkpBallSocketChainData::ConstraintInfo info;
		info.m_pivotInA = pivotInB;
		info.m_pivotInB = pivotInA;
		hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
		chainData->m_infos._insertAt(*pRouter->m_heap, 0, info);
		chainInstance->insertEntityAtFront(newChildInstance->getEntityB());
		return HK_SUCCESS;
	}
	if (pFirst != pEntityB) {
		return HK_FAILURE;
	}
	hkpBallSocketChainData::ConstraintInfo info;
	info.m_pivotInA = pivotInA;
	info.m_pivotInB = pivotInB;
	hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
	chainData->m_infos._insertAt(*pRouter->m_heap, 0, info);
	chainInstance->insertEntityAtFront(newChildInstance->getEntityA());
	return HK_SUCCESS;
}
