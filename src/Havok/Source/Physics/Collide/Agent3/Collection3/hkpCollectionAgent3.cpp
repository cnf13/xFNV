#include "Havok/Source/Physics/Collide/Agent3/Collection3/hkpCollectionAgent3.h"

#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/List3/hkpListAgent3.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h"

// The Agent3Funcs constructor clears the flags and the five optional functions from m_updateFilterFunc on;
// initAgentFunc fills the rest. registerAgent3 copies the whole of f, so the bytes after m_reusePreviousEntry, never
// written here, land in the dispatcher's table, as the game does.
void hkpCollectionAgent3::registerAgent3(hkpCollisionDispatcher* dispatcher) {
	alignas(16) hkpCollisionDispatcher::Agent3Funcs f;
	f.m_isPredictive.m_bool = 0;
	f.m_ignoreSymmetricVersion.m_bool = 0;
	f.m_reusePreviousEntry.m_bool = 0;
	f.m_updateFilterFunc = nullptr;
	f.m_calcStatisticsFunc = nullptr;
	f.m_invalidateTimFunc = nullptr;
	f.m_warpTimeFunc = nullptr;
	f.m_sepNormalFunc = nullptr;
	hkpCollectionAgent3::initAgentFunc(f);
	dispatcher->registerAgent3(f, HK_SHAPE_CONVEX, HK_SHAPE_COLLECTION);
}

void hkpCollectionAgent3::initAgentFunc(hkpCollisionDispatcher::Agent3Funcs& f) {
	hkListAgent3::initAgentFunc(f);
	f.m_processFunc = &hkpCollectionAgent3::process;
}

// The body's shape is wrapped in a one-element list shape, which is then processed against the other body's
// collection by the collection-collection agent. The temporary collidable stands in for the body, over the list
// shape and the body's motion, and takes the body's bounding volume data when the body is a root. The list shape is
// released through its deleting destructor with no free flag afterwards.
hkpAgentData* hkpCollectionAgent3::process(const hkpAgent3ProcessInput& input, hkpAgentEntry* entry,
	hkpAgentData* agentData, hkVector4* separatingNormal, hkpProcessCollisionOutput& output) {
	HK_TIMER_BEGIN("Coll3", nullptr);

	alignas(16) uint8_t aInputStorage[sizeof(hkpAgent3ProcessInput)];
	hkpAgent3ProcessInput& rCopy = *reinterpret_cast<hkpAgent3ProcessInput*>(aInputStorage);
	hkpAgent3ProcessInput::CopyConstruct(&rCopy, input);
	rCopy.m_distAtT1 = input.m_distAtT1;
	rCopy.m_linearTimInfo = input.m_linearTimInfo;

	const hkpShape* pShape = input.m_bodyA->m_shape;
	alignas(16) uint8_t aListShapeStorage[sizeof(hkpListShape)];
	hkpListShape* pListShape =
		::new (aListShapeStorage) hkpListShape(&pShape, 1, hkpShapeContainer::REFERENCE_POLICY_IGNORE);

	// The collidable over the list shape.
	hkpCollidable kCollidable;
	kCollidable.m_shape = nullptr;
	kCollidable.m_shapeKey = 0xFFFFFFFF;
	kCollidable.m_motion.pState = nullptr;
	kCollidable.m_parent = nullptr;
	kCollidable.m_ownerOffset = 0;
	kCollidable.m_forceCollideOntoPpu = 0;
	kCollidable.m_shapeSizeOnSpu = 0;
	kCollidable.m_broadPhaseHandle.m_id = 0;
	kCollidable.m_broadPhaseHandle.m_type = 0;
	kCollidable.m_broadPhaseHandle.m_ownerOffset = hkpTypedBroadPhaseHandle::OFFSET_INVALID;
	kCollidable.m_broadPhaseHandle.m_objectQualityType = -1;
	kCollidable.m_broadPhaseHandle.m_collisionFilterInfo = 0;
	// Sets the handle's owner offset to the distance back to the collidable it belongs to.
	kCollidable.m_broadPhaseHandle.m_ownerOffset = static_cast<int8_t>(reinterpret_cast<const char*>(&kCollidable) -
		reinterpret_cast<const char*>(&kCollidable.m_broadPhaseHandle));
	kCollidable.m_allowedPenetrationDepth = -1.0f;

	const hkpCollidable* pBodyCollidable = static_cast<const hkpCollidable*>(input.m_bodyA);
	if (input.m_bodyA->m_parent == nullptr) {
		kCollidable.m_boundingVolumeData = pBodyCollidable->m_boundingVolumeData;
	}
	kCollidable.m_motion = input.m_bodyA->m_motion;
	kCollidable.m_parent = input.m_bodyA->m_parent;
	rCopy.m_overrideBodyA = input.m_bodyA;
	kCollidable.m_shape = pListShape;
	kCollidable.m_shapeKey = 0xFFFFFFFF;
	rCopy.m_bodyA = &kCollidable;

	hkpAgentData* pResult = hkpCollectionCollectionAgent3::process(rCopy, entry, agentData, separatingNormal, output);

	// Reads the heap for an array release whose branch is dead, as the game does.
	hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
	(void)pHeap;
	pListShape->m_childInfo.m_size = 0;
	pListShape->~hkpListShape();

	HK_TIMER_END();
	return pResult;
}
