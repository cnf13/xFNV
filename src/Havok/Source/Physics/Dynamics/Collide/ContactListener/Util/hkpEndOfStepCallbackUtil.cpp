#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/Util/hkpEndOfStepCallbackUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpCollisionEvent.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactPointEvent.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpDynamicsContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpSimpleConstraintContactMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpSimpleContactConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"

// The uids of the two entities of the collision's constraint, the first entity's first, compared lexicographically.
hkBool hkpEndOfStepCallbackUtil::Collision::operator<(const Collision& other) const {
	uint32_t uiUidA = m_mgr->m_constraint.getEntity(0)->m_uid;
	uint32_t uiUidB = m_mgr->m_constraint.getEntity(1)->m_uid;
	uint32_t uiOtherUidA = other.m_mgr->m_constraint.getEntity(0)->m_uid;
	uint32_t uiOtherUidB = other.m_mgr->m_constraint.getEntity(1)->m_uid;
	hkBool kResult;
	kResult.m_bool = (uiUidA < uiOtherUidA) || (uiUidA == uiOtherUidA && uiUidB < uiOtherUidB);
	return kResult;
}

// The contact points of the collision's manifold, last first, each one that is new or every one when the constraint
// asks for contact point callbacks, reported to listener as an end-of-step manifold event.
void hkpEndOfStepCallbackUtil::fireContactPointEventsForCollision(hkpSimpleConstraintContactMgr* mgr,
	hkpContactListener* listener, hkpCollisionEvent::CallbackSource source) {
	hkpRigidBody* pBodyA = static_cast<hkpRigidBody*>(mgr->getConstraintInstance()->m_entities[0]);
	hkpRigidBody* pBodyB = static_cast<hkpRigidBody*>(mgr->getConstraintInstance()->m_entities[1]);
	hkpSimpleContactConstraintAtom* pAtom = mgr->m_contactConstraintData.m_atom;
	const int32_t iNumContactPoints = pAtom->m_numContactPoints;
	hkpContactPointPropertiesStream* pProperties = pAtom->getContactPointPropertiesStream(0);
	const int32_t iStriding = pAtom->getContactPointPropertiesStriding();
	hkContactPoint* pContactPoint = pAtom->getContactPoints();
	const bool bFiringFullManifold = (mgr->getConstraintInstance()->getInternal()->m_callbackRequest &
										 hkpConstraintAtom::CALLBACK_REQUEST_CONTACT_POINT_CALLBACK) != 0;
	const int32_t iLast = iNumContactPoints - 1;
	for (int32_t i = iLast; i >= 0; --i) {
		hkpContactPointProperties* pProps = pProperties->asProperties();
		if ((pProps->m_flags & hkContactPointMaterial::CONTACT_IS_NEW) || bFiringFullManifold) {
			hkpContactPointEvent event(source, pBodyA, pBodyB, mgr);
			event.m_type = hkpContactPointEvent::TYPE_MANIFOLD_AT_END_OF_STEP;
			event.m_contactPoint = pContactPoint;
			event.m_contactPointProperties = pProps;
			event.m_firingCallbacksForFullManifold.m_bool = bFiringFullManifold;
			event.m_firstCallbackForFullManifold.m_bool = i == iLast;
			event.m_lastCallbackForFullManifold.m_bool = i == 0;
			event.m_separatingVelocity = nullptr;
			event.m_rotateNormal = nullptr;
			event.m_shapeKeyStorage = pProps->getStartOfExtendedUserData(pAtom);
			event.m_accumulators[0] = nullptr;
			event.m_accumulators[1] = nullptr;
			listener->contactPointCallback(event);
		}
		// The next properties, iStriding bytes on in the atom's untyped stream memory.
		// EXE-INLINED none: hkAddByteOffset<hkpContactPointPropertiesStream>
		pProperties =
			reinterpret_cast<hkpContactPointPropertiesStream*>(reinterpret_cast<char*>(pProperties) + iStriding);
		++pContactPoint;
	}
}

void hkpEndOfStepCallbackUtil::sortNewCollisions(hkpEndOfStepCallbackUtil::NewCollision* first, int32_t low,
	int32_t high, uint8_t cmpLess) {
	int32_t v3 = high;
	int32_t v4 = low;
	hkpEndOfStepCallbackUtil::NewCollision* v5 = first;
	while (1) {
		int32_t v15;
		int32_t v6 = v4;
		int32_t v7 = (v3 + v4) >> 1;
		hkpContactListener* m_listener = v5[v7].m_listener;
		hkpEndOfStepCallbackUtil::NewCollision* v9 = &v5[v7];
		int32_t v22 = v3;
		hkpSimpleConstraintContactMgr* m_mgr = v9->m_mgr;
		int32_t v21 = v6;
		hkpSimpleConstraintContactMgr* v29 = v9->m_mgr;
		hkpCollisionEvent::CallbackSource m_source = v9->m_source;
		int32_t m_sequenceNumber = v9->m_sequenceNumber;
		while (1) {
			uint32_t m_uid = m_mgr->m_constraint.m_entities[0]->getUid();
			uint32_t v11 = m_mgr->m_constraint.m_entities[1]->getUid();
			for (hkpEndOfStepCallbackUtil::NewCollision* i = &v5[v6];; ++i) {
				uint32_t v13 = i->m_mgr->m_constraint.m_entities[0]->getUid();
				uint32_t v14 = i->m_mgr->m_constraint.m_entities[1]->getUid();
				if (static_cast<uint64_t>((static_cast<uint64_t>(static_cast<uint32_t>(v13)) << 32) |
						static_cast<uint32_t>(v14)) >=
						static_cast<uint64_t>((static_cast<uint64_t>(static_cast<uint32_t>(m_uid)) << 32) |
							static_cast<uint32_t>(v11)) &&
					(v14 != v11 || (v13 != m_uid || i->m_sequenceNumber >= m_sequenceNumber))) {
					break;
				}
				++v21;
			}
			uint64_t v23 = static_cast<uint64_t>((static_cast<uint64_t>(static_cast<uint32_t>(m_uid)) << 32) |
				static_cast<uint32_t>(v11));
			v15 = v22;
			for (hkpEndOfStepCallbackUtil::NewCollision* j = &first[v22];; --j) {
				uint32_t v17 = j->m_mgr->m_constraint.m_entities[0]->getUid();
				uint32_t v18 = j->m_mgr->m_constraint.m_entities[1]->getUid();
				if (v23 >= static_cast<uint64_t>((static_cast<uint64_t>(static_cast<uint32_t>(v17)) << 32) |
							   static_cast<uint32_t>(v18)) &&
					(static_cast<uint32_t>(v23) != v18 || (m_uid != v17 || m_sequenceNumber >= j->m_sequenceNumber))) {
					break;
				}
				--v15;
			}
			v6 = v21;
			v5 = first;
			if (v15 < v21) {
				break;
			}
			if (v15 != v21) {
				hkpEndOfStepCallbackUtil::NewCollision* v19 = &first[v15];
				hkpSimpleConstraintContactMgr* v25 = v19->m_mgr;
				hkpContactListener* v26 = v19->m_listener;
				hkpCollisionEvent::CallbackSource v27 = v19->m_source;
				int32_t v28 = v19->m_sequenceNumber;
				v19->m_mgr = first[v21].m_mgr;
				hkpEndOfStepCallbackUtil::NewCollision* v20 = &first[v21];
				v19->m_listener = v20->m_listener;
				v19->m_source = v20->m_source;
				v19->m_sequenceNumber = v20->m_sequenceNumber;
				v20->m_mgr = v25;
				v20->m_listener = v26;
				v20->m_source = v27;
				v20->m_sequenceNumber = v28;
			}
			--v15;
			v6 = v21 + 1;
			v22 = v15;
			v21 = v6;
			if (v6 > v15) {
				break;
			}
			m_mgr = v29;
		}
		if (low < v15) {
			hkpEndOfStepCallbackUtil::sortNewCollisions(first, low, v15, cmpLess);
		}
		v3 = high;
		if (v6 >= high) {
			break;
		}
		v4 = v6;
		low = v6;
	}
}

// Drops from removedCollisions every collision the list also holds (the two cancel out) and copies the rest of the list
// into a new storage, which the list then takes over. The exe allocates that storage itself, through the router's
// allocator, rather than through hkArrayUtil::_reserve; the second growth goes through _reserve.
template <typename T>
void hkpEndOfStepCallbackUtil::removeCollisions(hkArray<Collision>& removedCollisions, hkArray<T>& list) {
	if (removedCollisions.m_size == 0)
		return;
	const int32_t kElementSize = static_cast<int32_t>(sizeof(T));
	const int32_t kOriginalRemovedSize = removedCollisions.m_size;
	FNVD_INLINED_LOCAL(hkArray<T>, filtered, std::nullopt, std::nullopt);
	filtered.m_data = nullptr;
	filtered.m_size = 0;
	filtered.m_capacityAndFlags = hkArrayBase<T>::DONT_DEALLOCATE_FLAG;
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	if (list.m_size != 0) {
		int32_t numBytes = kElementSize * list.m_size;
		filtered.m_data = static_cast<T*>(pHeap->bufAlloc(numBytes));
		int32_t numElements = numBytes / kElementSize;
		filtered.m_size = static_cast<uint32_t>(numElements);
		filtered.m_capacityAndFlags =
			numElements != 0 ? static_cast<uint32_t>(numElements) : hkArrayBase<T>::DONT_DEALLOCATE_FLAG;
	}
	if (list.m_size > 0) {
		T* pOut = filtered.m_data;
		for (uint32_t i = 0; i < list.m_size; ++i) {
			const T& kEntry = list.m_data[i];
			int32_t iFound = -1;
			for (uint32_t j = 0; j < removedCollisions.m_size; ++j) {
				hkBool kEqual;
				if (removedCollisions.m_data[j].equals(&kEqual, kEntry)->m_bool) {
					iFound = static_cast<int32_t>(j);
					break;
				}
			}
			if (iFound < 0) {
				*pOut = kEntry;
				++pOut;
			} else {
				// removeAt: the last collision moves into the freed slot.
				uint32_t uiLast = removedCollisions.m_size - 1;
				removedCollisions.m_size = uiLast;
				if (uiLast != static_cast<uint32_t>(iFound))
					removedCollisions.m_data[iFound] = removedCollisions.m_data[uiLast];
			}
		}
	}
	int32_t numKept = list.m_size + removedCollisions.m_size - kOriginalRemovedSize;
	hkMemoryAllocator* pGrowHeap = hkMemoryRouter::getInstance().m_heap;
	int32_t numCapacity = static_cast<int32_t>(filtered.m_capacityAndFlags & hkArrayBase<T>::CAPACITY_MASK);
	if (numCapacity < numKept) {
		int32_t numNewCapacity = 2 * numCapacity;
		if (numKept >= numNewCapacity)
			numNewCapacity = numKept;
		hkArrayUtil::_reserve(pGrowHeap, &filtered, numNewCapacity, kElementSize);
	}
	filtered.m_size = static_cast<uint32_t>(numKept);
	list.swap(filtered);
	hkMemoryAllocator* pFreeHeap = hkMemoryRouter::getInstance().m_heap;
	filtered.m_size = 0;
	if (filtered.m_capacityAndFlags >= 0)
		pFreeHeap->bufFree(filtered.m_data,
			kElementSize * static_cast<int32_t>(filtered.m_capacityAndFlags & hkArrayBase<T>::CAPACITY_MASK));
}

template void hkpEndOfStepCallbackUtil::removeCollisions<hkpEndOfStepCallbackUtil::NewCollision>(hkArray<Collision>&
																									 removedCollisions,
	hkArray<NewCollision>& list);
template void
hkpEndOfStepCallbackUtil::removeCollisions<hkpEndOfStepCallbackUtil::Collision>(hkArray<Collision>& removedCollisions,
	hkArray<Collision>& list);

// Merges the new collisions into the registered ones by Collision::operator<, a new one taken only when it orders
// strictly first (the registered one wins a tie), into a storage of exactly the combined size; the collisions take it
// over and the new list empties.
void hkpEndOfStepCallbackUtil::mergeCollisions(hkArray<NewCollision>& newCollisions, hkArray<Collision>& collisions) {
	// Both ends are computed on the addresses, in the exe's 32-bit arithmetic (`shl 4`, `lea [n*3]*4`), and compared
	// as addresses, so that clang does not fold the compares into tests of the counts.
	NewCollision* pNew = newCollisions.m_data;
	const uintptr_t uNewEnd =
		reinterpret_cast<uintptr_t>(pNew) + static_cast<uintptr_t>(newCollisions.m_size) * sizeof(NewCollision);
	if (reinterpret_cast<uintptr_t>(pNew) == uNewEnd)
		return;
	const int32_t kElementSize = static_cast<int32_t>(sizeof(Collision));
	int32_t numElements = newCollisions.m_size + collisions.m_size;
	FNVD_INLINED_LOCAL(hkArray<Collision>, merged, std::nullopt, std::nullopt);
	merged.m_data = nullptr;
	merged.m_size = 0;
	merged.m_capacityAndFlags = hkArrayBase<Collision>::DONT_DEALLOCATE_FLAG;
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	if (numElements != 0) {
		int32_t numBytes = kElementSize * numElements;
		merged.m_data = static_cast<Collision*>(pHeap->bufAlloc(numBytes));
		int32_t numAllocated = numBytes / kElementSize;
		merged.m_size = static_cast<uint32_t>(numAllocated);
		merged.m_capacityAndFlags =
			numAllocated != 0 ? static_cast<uint32_t>(numAllocated) : hkArrayBase<Collision>::DONT_DEALLOCATE_FLAG;
	}
	Collision* pOut = merged.m_data;
	Collision* pOld = collisions.m_data;
	const uintptr_t uOldEnd =
		reinterpret_cast<uintptr_t>(pOld) + static_cast<uintptr_t>(collisions.m_size) * sizeof(Collision);
	while (reinterpret_cast<uintptr_t>(pNew) < uNewEnd && reinterpret_cast<uintptr_t>(pOld) < uOldEnd) {
		// EXE-INLINED none: hkpEndOfStepCallbackUtil::Collision::operator<
		if (!(*pNew < *pOld)) {
			*pOut = *pOld;
			++pOld;
		} else {
			*pOut = *pNew;
			++pNew;
		}
		++pOut;
	}
	for (; reinterpret_cast<uintptr_t>(pNew) < uNewEnd; ++pNew) {
		*pOut = *pNew;
		++pOut;
	}
	for (; reinterpret_cast<uintptr_t>(pOld) < uOldEnd; ++pOld) {
		*pOut = *pOld;
		++pOut;
	}
	collisions.swap(merged);
	newCollisions.m_size = 0;
	hkMemoryAllocator* pFreeHeap = hkMemoryRouter::getInstance().m_heap;
	if (merged.m_capacityAndFlags >= 0)
		pFreeHeap->bufFree(merged.m_data,
			kElementSize * static_cast<int32_t>(merged.m_capacityAndFlags & hkArrayBase<Collision>::CAPACITY_MASK));
}

// Fires the callbacks of the step's collisions. The removed and the re-registered ones cancel out first; with
// deterministic order the new ones are sorted by uid and merged into the registered ones, otherwise they are appended.
// A collision whose master entity sits in an island with the integrate flags set is left to that island's job.
// The exe reaches it through the vtable of the hkpWorldPostSimulationListener subobject (+0x8).
void hkpEndOfStepCallbackUtil::postSimulationCallback(hkpWorld* world) {
	HK_TIMER_BEGIN("EndOfStepCbs", nullptr);
	removeCollisions(m_removedCollisions, m_newCollisions);
	removeCollisions(m_removedCollisions, m_collisions);
	if (m_deterministicOrder.m_bool) {
		if (m_newCollisions.m_size > 1)
			// The empty comparator's byte, as the exe writes it (`mov byte ptr [esp+0x14], 0`).
			sortNewCollisions(m_newCollisions.m_data, 0, m_newCollisions.m_size - 1, static_cast<uint8_t>(0));
		mergeCollisions(m_newCollisions, m_collisions);
	} else {
		int32_t numNew = m_newCollisions.m_size;
		hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
		int32_t numCollisions = m_collisions.m_size;
		int32_t numNeeded = numCollisions + numNew;
		int32_t numCapacity =
			static_cast<int32_t>(m_collisions.m_capacityAndFlags & hkArrayBase<Collision>::CAPACITY_MASK);
		if (numCapacity < numNeeded) {
			int32_t numNewCapacity = 2 * numCapacity;
			if (numNeeded < numNewCapacity)
				numNeeded = numNewCapacity;
			hkArrayUtil::_reserve(pHeap, &m_collisions, numNeeded, static_cast<int32_t>(sizeof(Collision)));
		}
		m_collisions.m_size += static_cast<uint32_t>(numNew);
		Collision* pDst = m_collisions.m_data + numCollisions;
		for (int32_t i = 0; i < m_newCollisions.m_size; ++i) {
			pDst[i].m_mgr = m_newCollisions.m_data[i].m_mgr;
			pDst[i].m_listener = m_newCollisions.m_data[i].m_listener;
			pDst[i].m_source = m_newCollisions.m_data[i].m_source;
		}
		m_newCollisions.m_size = 0;
	}
	for (int32_t i = 0; i < m_collisions.m_size; ++i) {
		Collision& collision = m_collisions.m_data[i];
		hkpSimulationIsland* pIsland = collision.m_mgr->getConstraintInstance()->getSimulationIsland();
		if (pIsland->m_activeMark != 0)
			fireContactPointEventsForCollision(collision.m_mgr, collision.m_listener, collision.m_source);
	}
	m_sequenceNumber = 0;
	HK_TIMER_END();
}

// The three arrays go with their members.
hkpEndOfStepCallbackUtil::~hkpEndOfStepCallbackUtil() {}

// The same manager, listener and source, stored in apResult.
hkBool* hkpEndOfStepCallbackUtil::Collision::equals(hkBool* apResult, const Collision& other) const {
	apResult->m_bool = m_mgr == other.m_mgr && m_listener == other.m_listener && m_source == other.m_source;
	return apResult;
}

// The three arrays start empty through their own constructors.
hkpEndOfStepCallbackUtil::hkpEndOfStepCallbackUtil() : m_sequenceNumber(0) {
	m_deterministicOrder = false;
}

void hkpEndOfStepCallbackUtil::addToWorld(hkpWorld* world) {
	if (this) {
		world->addWorldPostSimulationListener(this);
	} else {
		world->addWorldPostSimulationListener(0);
	}
}

void hkpEndOfStepCallbackUtil::removeFromWorld(hkpWorld* world) {
	if (this) {
		world->removeWorldPostSimulationListener(this);
	} else {
		world->removeWorldPostSimulationListener(0);
	}
}

// The Havok SDK's registerCollision: the collision appended to the new ones with the next sequence number.
void hkpEndOfStepCallbackUtil::registerCollision(hkpSimpleConstraintContactMgr* mgr, hkpContactListener* listener,
	hkpCollisionEvent::CallbackSource source) {
	hkArray<NewCollision>& newCollisions = m_newCollisions;
	// EXE-INLINED none: hkArray<hkpEndOfStepCallbackUtil::NewCollision, hkContainerHeapAllocator>::expandOne
	hkMemoryRouter& router = hkMemoryRouter::getInstance();
	if (newCollisions.m_size == (newCollisions.m_capacityAndFlags & hkArrayBase<NewCollision>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*router.m_heap, &newCollisions, sizeof(NewCollision));
	}
	NewCollision& collision = newCollisions.m_data[newCollisions.m_size++];
	collision.m_mgr = mgr;
	collision.m_listener = listener;
	collision.m_source = source;
	collision.m_sequenceNumber = m_sequenceNumber;
	m_sequenceNumber++;
}

// The Havok SDK's unregisterCollision: the collision appended to the removed ones.
void hkpEndOfStepCallbackUtil::unregisterCollision(hkpSimpleConstraintContactMgr* mgr, hkpContactListener* listener,
	hkpCollisionEvent::CallbackSource source) {
	hkArray<Collision>& removedCollisions = m_removedCollisions;
	// EXE-INLINED none: hkArray<hkpEndOfStepCallbackUtil::Collision, hkContainerHeapAllocator>::expandOne
	hkMemoryRouter& router = hkMemoryRouter::getInstance();
	if (removedCollisions.m_size == (removedCollisions.m_capacityAndFlags & hkArrayBase<Collision>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*router.m_heap, &removedCollisions, sizeof(Collision));
	}
	Collision& collision = removedCollisions.m_data[removedCollisions.m_size++];
	collision.m_mgr = mgr;
	collision.m_listener = listener;
	collision.m_source = source;
}
