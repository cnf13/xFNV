#include "Havok/Source/Physics/Dynamics/World/BroadPhaseBorder/hkpBroadPhaseBorder.h"

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomOverlapListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldDeletionListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCinfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <bit>
#include <xmmintrin.h>

int32_t hkpBroadPhaseBorder_EntityCmpLess(const hkpEntity* a, const hkpEntity* b) {
	return a->m_uid < b->m_uid;
}

// The SDK's constructor: the border holds a reference to itself, registers as the world's deletion and post
// simulation listener, and puts six AABB phantoms, one per face of the broad phase's extents, into the world.
hkpBroadPhaseBorder::hkpBroadPhaseBorder(hkpWorld* world, hkpWorldCinfo::BroadPhaseBorderBehaviour type,
	hkBool postponeAndSortCallbacks)
	: m_world(world), m_type(type), m_postponeAndSortCallbacks(postponeAndSortCallbacks) {
	addReference();
	m_world->addWorldDeletionListener(this);
	m_world->addWorldPostSimulationListener(this);
	const hkVector4& kMin = m_world->m_broadPhaseExtents[0];
	const hkVector4& kMax = m_world->m_broadPhaseExtents[1];
	// Each face: an AABB from the extents with one axis collapsed to the face's side, (min, max) per row.
	const hkAabb akFaces[6] = {
		hkAabb(hkVector4(kMax.m_quad[0], kMin.m_quad[1], kMin.m_quad[2], 0.0f),
			hkVector4(kMax.m_quad[0], kMax.m_quad[1], kMax.m_quad[2], 0.0f)),
		hkAabb(hkVector4(kMin.m_quad[0], kMin.m_quad[1], kMin.m_quad[2], 0.0f),
			hkVector4(kMin.m_quad[0], kMax.m_quad[1], kMax.m_quad[2], 0.0f)),
		hkAabb(hkVector4(kMin.m_quad[0], kMax.m_quad[1], kMin.m_quad[2], 0.0f),
			hkVector4(kMax.m_quad[0], kMax.m_quad[1], kMax.m_quad[2], 0.0f)),
		hkAabb(hkVector4(kMin.m_quad[0], kMin.m_quad[1], kMin.m_quad[2], 0.0f),
			hkVector4(kMax.m_quad[0], kMin.m_quad[1], kMax.m_quad[2], 0.0f)),
		hkAabb(hkVector4(kMin.m_quad[0], kMin.m_quad[1], kMax.m_quad[2], 0.0f),
			hkVector4(kMax.m_quad[0], kMax.m_quad[1], kMax.m_quad[2], 0.0f)),
		hkAabb(hkVector4(kMin.m_quad[0], kMin.m_quad[1], kMin.m_quad[2], 0.0f),
			hkVector4(kMax.m_quad[0], kMax.m_quad[1], kMin.m_quad[2], 0.0f)),
	};
	for (int32_t i = 0; i < 6; ++i) {
		hkpAabbPhantom* phantom =
			new (hkReferencedObject::newObject(sizeof(hkpAabbPhantom))) hkpAabbPhantom(akFaces[i], 0);
		phantom->m_collidable.m_broadPhaseHandle.m_type = 3;
		phantom->addPhantomOverlapListener(this);
		m_phantoms[i] = phantom;
		world->addPhantom(phantom);
	}
}

// The SDK's destructor: each phantom's reference is dropped; the compiler frees the exiting entities' array.
hkpBroadPhaseBorder::~hkpBroadPhaseBorder() {
	for (int32_t i = 0; i < 6; ++i) {
		if (m_phantoms[i]) {
			m_phantoms[i]->removeReference();
			m_phantoms[i] = nullptr;
		}
	}
}

// The exe passes its body argument's own stack slot as removeEntity's result; src uses a local.
void hkpBroadPhaseBorder::maxPositionExceededCallback(hkpEntity* entity) {
	int32_t v3;
	hkpWorldCinfo::BroadPhaseBorderBehaviour m_type = this->m_type;
	if (m_type && (v3 = m_type - 1) != 0) {
		if (v3 == 1) {
			hkpWorld* m_world = this->m_world;
			if (entity->m_world == m_world) {
				hkBool result;
				m_world->removeEntity(&result, entity);
			}
		}
	} else {
		static_cast<hkpRigidBody*>(entity)->hkpRigidBody::setMotionType(hkpMotion::MOTION_FIXED,
			HK_ENTITY_ACTIVATION_DO_ACTIVATE, HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK);
	}
}

void hkpBroadPhaseBorder::deactivate() {
	hkpWorld* m_world = this->m_world;
	if (m_world) {
		hkpPhantom** m_phantoms = this->m_phantoms;
		m_world->removePhantomBatch(this->m_phantoms, 6);
		int32_t v4 = 6;
		do {
			(*m_phantoms)->removePhantomOverlapListener(this);
			(*m_phantoms)->removeReference();
			*m_phantoms++ = 0;
			--v4;
		} while (v4);
		worldDeletedCallback(this->m_world);
		this->m_world = 0;
	}
}

void hkpBroadPhaseBorder::collidableAddedCallback(const hkpCollidableAddedEvent& event) {
	const hkpCollidable* collidable = event.m_collidable;
	// The SDK's hkpGetRigidBody, inline: the owner of an entity's collidable.
	if (collidable->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
		hkpEntity* body = static_cast<hkpEntity*>(collidable->getOwner());
		if (body) {
			if (m_postponeAndSortCallbacks.m_bool) {
				// EXE-INLINED none: hkArray<hkpEntity*>::pushBack
				hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
				if (m_entitiesExitingBroadPhase.m_size ==
					(m_entitiesExitingBroadPhase.m_capacityAndFlags & hkArrayBase<hkpEntity*>::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*Instance->m_heap, &m_entitiesExitingBroadPhase, sizeof(hkpEntity*));
				}
				m_entitiesExitingBroadPhase.m_data[m_entitiesExitingBroadPhase.m_size++] = body;
				body->addReference();
			} else {
				maxPositionExceededCallback(body);
			}
		}
	}
}

void hkpBroadPhaseBorder::collidableRemovedCallback(const hkpCollidableRemovedEvent& event) {
	;
}

void hkpBroadPhaseBorder::postSimulationCallback(hkpWorld* world) {
	int32_t size = m_entitiesExitingBroadPhase.m_size;
	if (size) {
		// EXE-INLINED none: hkAlgorithm::quickSort
		if (size > 1) {
			hkAlgorithm::quickSortRecursive<hkpEntity*,
				bool (*)(const hkpEntity*, const hkpEntity*)>(m_entitiesExitingBroadPhase.m_data, 0, size - 1,
				&hkpBroadPhaseBorder_EntityCmpLessBool);
		}
		for (int32_t i = 0; i < m_entitiesExitingBroadPhase.m_size; ++i) {
			maxPositionExceededCallback(m_entitiesExitingBroadPhase.m_data[i]);
		}
		hkReferencedObject::removeReferences(m_entitiesExitingBroadPhase.m_data, m_entitiesExitingBroadPhase.m_size);
		m_entitiesExitingBroadPhase.m_size = 0;
	}
}

void hkpBroadPhaseBorder::worldDeletedCallback(hkpWorld* world) {
	hkpWorldPostSimulationListener* v3;
	m_world->removeWorldDeletionListener(this != nullptr ? static_cast<hkpWorldDeletionListener*>(this) : 0);
	if (this == nullptr) {
		v3 = 0;
	} else {
		v3 = static_cast<hkpWorldPostSimulationListener*>(this);
	}
	m_world->removeWorldPostSimulationListener(v3);
	removeReference();
}

void hkpBroadPhaseBorder::operator delete(void* apMemory) {
	hkReferencedObject::deleteObject(apMemory);
}
