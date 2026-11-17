#include "Havok/Source/Physics/Dynamics/World/Util/AabbTree/hkpAabbTreeWorldManager.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeUtils.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgentConfig.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Thread/Pool/hkJobThreadPool.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildingUtils.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Query/Collector/AabbTreeCollector/hkpAabbTreeCollidableRaycaster.h"
#include "Havok/Source/Physics/Collide/Query/Collector/KdTreeCollector/hkpKdTreeCollidableRaycaster.h"
#include "Havok/Source/Physics/Collide/Query/Collector/AabbTreeCollector/hkpAabbTreeWorldLinearCaster.h"
#include "Havok/Source/Physics/Collide/Query/Collector/KdTreeCollector/hkpKdTreeWorldLinearCaster.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Util/KdTree/Mediator/hkpCollidableMediator.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpIslandActivationListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobs.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpTreeWorldManager.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"

hkpAabbTreeWorldManager::hkpAabbTreeWorldManager(hkpWorld* world, hkpWorldCinfo::TreeUpdateType updateType,
	float aabbExpandDt)
	: m_world(world), m_kdTreeWorkingSet{nullptr, nullptr},
	  m_kdTreeJobpHeader(static_cast<hkpCollisionQueryJobHeader*>(hkMemoryRouter::getInstance()
			  .m_heap->blockAlloc(sizeof(hkpCollisionQueryJobHeader)))),
	  m_aabbExpandDt(aabbExpandDt), m_enableFullScan(false), m_activeListCheckStartIdx(0), m_numObjToCheckInAFrame(30),
	  m_treeIsClean(false), m_kdTreeSemaphore(new hkSemaphore(0, 1000)) {
	m_type = hkpTreeWorldManager::MANAGER_AABBTREE;
	if (updateType == hkpWorldCinfo::REBUILD_ACTIVE) {
		m_useInactiveTree = true;
	} else if (updateType == hkpWorldCinfo::REBUILD_ALL) {
		m_useInactiveTree = false;
	}
	m_world->addEntityListener(this);
	m_world->addPhantomListener(this);
	m_world->addIslandActivationListener(this);
	m_world->addWorldPostSimulationListener(this);
	if (m_useInactiveTree) {
		const hkVector4* pBitOffsetLow = &m_world->m_collisionInput->m_aabb32Info.m_bitOffsetLow;
		m_inactiveTree = new hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>(pBitOffsetLow[0], pBitOffsetLow[1],
			pBitOffsetLow[2]);
	}
	reset(this, nullptr);
}

// The active list and the transition queue free themselves.
hkpAabbTreeWorldManager::~hkpAabbTreeWorldManager() {
	m_world->removeEntityListener(this);
	m_world->removePhantomListener(this);
	m_world->removeIslandActivationListener(this);
	m_world->removeWorldPostSimulationListener(this);
	delete m_inactiveTree;
	delete m_kdTree;
	hkMemoryRouter::getInstance().m_heap->blockFree(m_kdTreeJobpHeader, sizeof(hkpCollisionQueryJobHeader));
	delete m_kdTreeSemaphore;
}

void hkpAabbTreeWorldManager::updateFromWorld(hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool) {
	HK_TIMER_BEGIN("TreeBuild", nullptr);
	m_treeIsClean = 1;
	if (!m_doneFirstBuild) {
		fullBuild(jobQueue, jobThreadPool);
		m_doneFirstBuild = 1;
	} else {
		incrementalBuild(jobQueue, jobThreadPool);
	}
	HK_TIMER_END();
}

// The SDK's: the ray through the active kd-tree and, when the inactive AABB tree is used, through that too, each
// under a split of the "CastRay" timer list.
void hkpAabbTreeWorldManager::castRay(const hkpWorldRayCastInput& input, const hkpCollisionFilter* filter,
	hkpWorldRayCastOutput& output) const {
	FNVD_CONSTRUCTED_LOCAL(hkpAabbTreeCollidableRaycaster, raycaster, nullptr);
	raycaster.m_output = &output;
	raycaster.m_input = &input;
	if (filter) {
		raycaster.m_filter = static_cast<const hkpRayCollidableFilter*>(filter);
		raycaster.m_collectionFilter = static_cast<const hkpRayShapeCollectionFilter*>(filter);
	} else {
		raycaster.m_filter = nullptr;
		raycaster.m_collectionFilter = nullptr;
	}
	HK_TIMER_BEGIN_LIST("CastRay", "ActiveTree");
	hkKdTree* kdTree = m_kdTree;
	if (kdTree) {
		FNVD_INLINED_LOCAL(hkpKdTreeCollidableRaycaster, collector, hkpKdTreeCollidableRaycaster_vftable, std::nullopt);
		collector.m_collector = nullptr;
		collector.m_output = &output;
		collector.m_input = &input;
		if (filter) {
			collector.m_filter = static_cast<const hkpRayCollidableFilter*>(filter);
			collector.m_collectionFilter = static_cast<const hkpRayShapeCollectionFilter*>(filter);
		} else {
			collector.m_filter = nullptr;
			collector.m_collectionFilter = nullptr;
		}
		hkBool result;
		hkKdTreeUtils::castRayIterative(&result, kdTree, input.m_from, input.m_to, &collector);
	}
	HK_TIMER_SPLIT_LIST("InactiveTree");
	if (m_useInactiveTree) {
		hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::castRay(m_inactiveTree, input.m_from,
			input.m_to, &raycaster, 1.0f);
	}
	HK_TIMER_END_LIST();
}

// Casts the ray through the active kd-tree (when there is one), then through the inactive AABB tree when it is in
// use, the hits going to collector. Each raycaster's output is a local the exe resets inline (hkpWorldRayCastOutput's
// constructor) and never reads back; the kd-tree cast's hkBool result is dropped.
void hkpAabbTreeWorldManager::castRay(const hkpWorldRayCastInput& input, const hkpCollisionFilter* filter,
	hkpRayHitCollector& collector) const {
	// Declared in the reverse of the exe's frame order (kd raycaster, AABB raycaster, AABB output, kd output, from
	// the bottom), which clang lays out top down.
	hkpWorldRayCastOutput kKdOutput;
	hkpWorldRayCastOutput kAabbOutput;
	// Raw storage: the exe calls the AABB raycaster's constructor, and calls no destructor. The exe's frame holds the
	// AABB raycaster 16-byte aligned.
	alignas(16) uint8_t aAabbStorage[sizeof(hkpAabbTreeCollidableRaycaster)];
	hkBool kResult;
	// The exe drops the constructor's return value and addresses the storage directly.
	::new (aAabbStorage) hkpAabbTreeCollidableRaycaster;
	hkpAabbTreeCollidableRaycaster& kAabbRaycaster = *reinterpret_cast<hkpAabbTreeCollidableRaycaster*>(aAabbStorage);
	kAabbRaycaster.m_input = &input;
	kAabbRaycaster.m_collector = &collector;
	kAabbRaycaster.m_filter = filter;
	kAabbRaycaster.m_collectionFilter = filter;
	kAabbOutput.m_hitFraction = 1.0f;
	kAabbOutput.m_extraInfo = -1;
	kAabbOutput.m_shapeKeys[0] = static_cast<uint32_t>(-1);
	kAabbRaycaster.m_output = &kAabbOutput;
	kAabbOutput.m_shapeKeyIndex = 0;
	kAabbOutput.m_rootCollidable = nullptr;
	if (m_kdTree) {
		// The exe inlines the raycaster's constructor and calls no destructor.
		FNVD_CONSTRUCTED_LOCAL(hkpKdTreeCollidableRaycaster, kKdRaycaster, nullptr);
		kKdRaycaster.m_input = &input;
		kKdRaycaster.m_collector = &collector;
		kKdRaycaster.m_filter = filter;
		kKdRaycaster.m_collectionFilter = filter;
		kKdOutput.m_hitFraction = 1.0f;
		kKdOutput.m_extraInfo = -1;
		kKdOutput.m_shapeKeys[0] = static_cast<uint32_t>(-1);
		kKdRaycaster.m_output = &kKdOutput;
		kKdOutput.m_shapeKeyIndex = 0;
		kKdOutput.m_rootCollidable = nullptr;
		hkKdTreeUtils::castRayIterative(&kResult, m_kdTree, input.m_from, input.m_to, &kKdRaycaster);
	}
	if (m_useInactiveTree) {
		hkAabbTreeQueryUtils<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::castRay(m_inactiveTree, input.m_from,
			input.m_to, &kAabbRaycaster, 1.0f);
	}
}

// The SDK's: the cast through the active kd-tree and, when the inactive AABB tree is used, through that too. Both
// casters' constructors are inlined down to their vptr stores and the two fields set here (the exe gives both the
// same stack slot); their destructors leave nothing.
void hkpAabbTreeWorldManager::linearCast(const hkpCollidable* collA, const hkpLinearCastInput& input,
	const hkpCollidableCollidableFilter* filter, const hkpCollisionInput& collInput, hkpCollisionAgentConfig* config,
	hkpCdPointCollector& castCollector, hkpCdPointCollector* startPointCollector) {
	hkKdTree* kdTree = m_kdTree;
	if (kdTree) {
		FNVD_INLINED_LOCAL(hkpKdTreeWorldLinearCaster, kdCaster, hkpKdTreeWorldLinearCaster_vftable, std::nullopt);
		kdCaster.m_shapeInput.m_maxExtraPenetration = 1.1920929e-07f;
		kdCaster.m_shapeInput.m_weldClosestPoints.m_storage = 0;
		kdCaster.linearCast(kdTree, collA, input, filter, collInput, config, castCollector, startPointCollector);
	}
	if (m_useInactiveTree) {
		FNVD_CONSTRUCTED_LOCAL(hkpAabbTreeWorldLinearCaster<hkAabbTreeData::hkAabbTreeAabb16>, aabbCaster, nullptr);
		aabbCaster.linearCast(m_inactiveTree, collA, input, filter, collInput, config, castCollector,
			startPointCollector);
	}
}

int32_t hkpAabbTreeWorldManager::calcWorkingBufferSize(int32_t maxObjects) {
	m_maxObjects = maxObjects;
	return 32 * maxObjects;
}

void hkpAabbTreeWorldManager::setWorkingBuffer(void* ptr) {
	m_kdTreeWorkingSet.m_entriesIn = static_cast<hkKdTreeProjectedEntry*>(ptr);
	m_kdTreeWorkingSet.m_entriesOut = static_cast<hkKdTreeProjectedEntry*>(ptr) + m_maxObjects;
}

void hkpAabbTreeWorldManager::entityAddedCallback(hkpEntity* entity) {
	m_mustUpdateActiveList = 1;
	m_treeIsClean = 0;
	pushBackToActiveList(&entity->m_collidable);
}

void hkpAabbTreeWorldManager::entityRemovedCallback(hkpEntity* entity) {
	removeObjectFromTree(entity);
}

void hkpAabbTreeWorldManager::entitySetMotionTypeCallback(hkpEntity* entity) {
	;
}

void hkpAabbTreeWorldManager::entityDeletedCallback(hkpEntity* entity) {
	removeObjectFromTree(entity);
}

void hkpAabbTreeWorldManager::phantomAddedCallback(hkpPhantom* phantom) {
	m_mustUpdateActiveList = 1;
	m_treeIsClean = 0;
	pushBackToActiveList(&phantom->m_collidable);
}

void hkpAabbTreeWorldManager::phantomRemovedCallback(hkpPhantom* phantom) {
	removeObjectFromTree(phantom);
}

// The same as islandDeactivatedCallback, queueing the entities as activated.
void hkpAabbTreeWorldManager::islandActivatedCallback(hkpSimulationIsland* island) {
	if (m_useInactiveTree) {
		m_mustUpdateActiveList = true;
		m_treeIsClean = false;
		for (int32_t i = 0; i < island->getEntities().getSize(); ++i) {
			// The exe reads the element inline, as above.
			hkpEntity* entity = island->getEntities().m_data[i];
			if (entity->m_collidable.m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY)
				queueActivated(&entity->m_collidable);
		}
	}
}

// The exe marks the active list for a rebuild and queues the island's own entities for the transition queue: an
// activated island's with 1, a deactivated island's with -1. Each entity is queued through its collidable, and only
// when its broad phase handle says it is a plain entity (an island's list holds only entities, but the exe tests the
// type anyway). The island's array is indexed and its size re-read each turn, as the exe does.
void hkpAabbTreeWorldManager::islandDeactivatedCallback(hkpSimulationIsland* island) {
	if (m_useInactiveTree) {
		m_mustUpdateActiveList = true;
		m_treeIsClean = false;
		for (int32_t i = 0; i < island->getEntities().getSize(); ++i) {
			// The exe reads the element inline (m_data plus the index), not through 0x629AB0.
			hkpEntity* entity = island->getEntities().m_data[i];
			if (entity->m_collidable.m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY)
				queueInactivated(&entity->m_collidable);
		}
	}
}

void hkpAabbTreeWorldManager::postSimulationCallback(hkpWorld* world) {
	;
}

// A moved inactive entity has to be looked at by the active list again; the exe leaves the tail call to the queue
// helper as a jump.
void hkpAabbTreeWorldManager::inactiveEntityMovedCallback(hkpEntity* entity) {
	if (m_useInactiveTree) {
		m_mustUpdateActiveList = true;
		m_treeIsClean = false;
		queueActivated(&entity->m_collidable);
	}
}

// The exe's HK_TIMER_BEGIN_LIST, HK_TIMER_SPLIT_LIST and HK_TIMER_END_LIST write into the stream's raw buffer, which
// the exe itself treats as untyped (hence the casts). The inactive list starts empty (no storage, not ours), and the
// exe clears and frees both lists inline. The last timer's rdtsc goes through the jobThreadPool argument's stack slot
// in the exe (its spill slot is reused); src's spill lands elsewhere. 0xFFFF is s_invalidIdx.
void hkpAabbTreeWorldManager::fullBuild(hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool) {
	FNVD_INLINED_LOCAL(hkArray<const hkpCollidable*>, inactiveList, std::nullopt, std::nullopt);
	inactiveList.m_data = nullptr;
	inactiveList.m_size = 0;
	inactiveList.m_capacityAndFlags = hkArrayBase<const hkpCollidable*>::DONT_DEALLOCATE_FLAG;
	HK_TIMER_BEGIN_LIST("ActiveTree", "Gather");
	m_activeList.m_size = 0;
	gatherActiveCollidablesFromWorld(m_activeList, m_world);
	if (!m_useInactiveTree)
		gatherInactiveCollidablesFromWorld(m_activeList, m_world);
	for (int32_t i = 0; i < m_activeList.m_size; ++i)
		setNode(m_activeList.m_data[i], 0xFFFF);
	HK_TIMER_SPLIT_LIST("Clear");
	HK_TIMER_SPLIT_LIST("Rebuild");
	buildActiveTree(m_activeList, jobQueue, jobThreadPool);
	HK_TIMER_END_LIST();
	if (m_useInactiveTree) {
		HK_TIMER_BEGIN_LIST("InactiveTree", "Gather");
		inactiveList.m_size = 0;
		gatherInactiveCollidablesFromWorld(inactiveList, m_world);
		HK_TIMER_SPLIT_LIST("Clear");
		m_inactiveTree->clear();
		HK_TIMER_SPLIT_LIST("Rebuild");
		buildInactiveTree(inactiveList, jobQueue, jobThreadPool);
		HK_TIMER_END_LIST();
	}
	clearTransitionQueue();

	hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
	const int32_t capacityAndFlags = inactiveList.m_capacityAndFlags;
	inactiveList.m_size = 0;
	if (capacityAndFlags >= 0) {
		heap.bufFree(inactiveList.m_data,
			(capacityAndFlags & hkArrayBase<const hkpCollidable*>::CAPACITY_MASK) *
				static_cast<int32_t>(sizeof(const hkpCollidable*)));
	}
}

void hkpAabbTreeWorldManager::buildActiveTree(hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables,
	hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool) {
	if (m_activeList.m_size) {
		buildKdTree(m_kdTree, collidables, m_world, jobQueue, jobThreadPool);
	}
}

// Converts the collidables' compressed AABBs (or, where a collidable's is invalid, its shape's AABB at its transform)
// and gathers the collidables in two stack arrays, builds the inactive tree over them and stores each collidable's
// leaf index as its tree node. The exe inlines the hkLocalArrays' constructors, setSize and destructors, the
// leaf array's constructor and destructor, and hkVector4Util::convertAabbFromUint32: written out here.
void hkpAabbTreeWorldManager::buildInactiveTree(hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables,
	hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool) {
	const int32_t numCollidables = collidables.m_size;
	hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* pTree = m_inactiveTree;
	const int32_t iCapacityAndFlags =
		static_cast<int32_t>(static_cast<uint32_t>(numCollidables) | hkArrayBase<hkAabb>::DONT_DEALLOCATE_FLAG);

	alignas(16) hkLocalArray<hkAabb> aabbs;
	aabbs.m_data = nullptr;
	aabbs.m_size = 0;
	aabbs.m_capacityAndFlags = hkArrayBase<hkAabb>::DONT_DEALLOCATE_FLAG;
	hkStackMemory& rStack0 = hkMemoryRouter::getInstance().m_stack;
	hkAabb* pAabbMemory = static_cast<hkAabb*>(static_cast<void*>(rStack0.m_next.m_storage));
	rStack0.m_next.m_storage += (numCollidables * static_cast<int32_t>(sizeof(hkAabb)) + 127) & ~127;
	aabbs.m_data = pAabbMemory;
	aabbs.m_capacityAndFlags = iCapacityAndFlags;
	aabbs.m_localMemory = pAabbMemory;
	// EXE-INLINED none: hkArray<hkAabb>::setSize
	hkMemoryAllocator* pHeap0 = hkMemoryRouter::getInstance().m_heap;
	const int32_t iCapacity0 = static_cast<int32_t>(aabbs.m_capacityAndFlags & hkArrayBase<hkAabb>::CAPACITY_MASK);
	if (iCapacity0 < numCollidables) {
		int32_t iRequest = 2 * iCapacity0;
		if (numCollidables >= iRequest) {
			iRequest = numCollidables;
		}
		hkArrayUtil::_reserve(*pHeap0, &aabbs, iRequest, sizeof(hkAabb));
	}
	aabbs.m_size = numCollidables;

	alignas(16) hkLocalArray<void*> userDatas;
	userDatas.m_data = nullptr;
	userDatas.m_size = 0;
	userDatas.m_capacityAndFlags = hkArrayBase<void*>::DONT_DEALLOCATE_FLAG;
	hkStackMemory& rStack1 = hkMemoryRouter::getInstance().m_stack;
	void** pUserDataMemory = static_cast<void**>(static_cast<void*>(rStack1.m_next.m_storage));
	rStack1.m_next.m_storage += (numCollidables * static_cast<int32_t>(sizeof(void*)) + 127) & ~127;
	userDatas.m_data = pUserDataMemory;
	userDatas.m_capacityAndFlags = iCapacityAndFlags;
	userDatas.m_localMemory = pUserDataMemory;
	// EXE-INLINED none: hkArray<void*>::setSize
	hkMemoryAllocator* pHeap1 = hkMemoryRouter::getInstance().m_heap;
	const int32_t iCapacity1 = static_cast<int32_t>(userDatas.m_capacityAndFlags & hkArrayBase<void*>::CAPACITY_MASK);
	if (iCapacity1 < numCollidables) {
		int32_t iRequest = 2 * iCapacity1;
		if (numCollidables >= iRequest) {
			iRequest = numCollidables;
		}
		hkArrayUtil::_reserve(*pHeap1, &userDatas, iRequest, sizeof(void*));
	}
	userDatas.m_size = numCollidables;

	HK_TIMER_BEGIN("prepare", nullptr);
	const hkpCollisionInput::Aabb32Info& rAabb32Info = m_world->m_collisionInput->m_aabb32Info;
	for (int32_t i = 0; i < numCollidables; ++i) {
		const hkpCollidable* pCollidable = collidables.m_data[i];
		const hkpCollidable::BoundingVolumeData& rVolume = pCollidable->m_boundingVolumeData;
		if (!rVolume.isValid()) {
			pCollidable->m_shape->getAabb(*pCollidable->m_motion.pMotion, 0.0f, aabbs.m_data[i]);
		} else {
			// EXE-INLINED none: hkVector4Util::convertAabbFromUint32
			hkAabb& rAabb = aabbs.m_data[i];
			rAabb.m_min.set(static_cast<float>(rVolume.m_min[0]), static_cast<float>(rVolume.m_min[1]),
				static_cast<float>(rVolume.m_min[2]), 0.0f);
			rAabb.m_max.set(static_cast<float>(rVolume.m_max[0]), static_cast<float>(rVolume.m_max[1]),
				static_cast<float>(rVolume.m_max[2]), 0.0f);
			hkVector4 invScale;
			invScale.setReciprocal3(rAabb32Info.m_bitScale);
			rAabb.m_min.mul4(invScale);
			rAabb.m_max.mul4(invScale);
			rAabb.m_min.sub4(rAabb32Info.m_bitOffsetLow);
			rAabb.m_max.sub4(rAabb32Info.m_bitOffsetLow);
		}
		userDatas.m_data[i] = const_cast<hkpCollidable*>(collidables.m_data[i]);
	}
	HK_TIMER_END();

	// EXE-INLINED none: hkArray<unsigned short>::hkArray
	using leafIdx_t = hkArray<uint16_t, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(leafIdx_t, leafIdx, std::nullopt, std::nullopt);
	leafIdx.m_data = nullptr;
	leafIdx.m_size = 0;
	leafIdx.m_capacityAndFlags = hkArrayBase<uint16_t>::DONT_DEALLOCATE_FLAG;
	HK_TIMER_BEGIN("build", nullptr);
	if (numCollidables > 0) {
		pTree->build(aabbs.m_data, userDatas.m_data, numCollidables, leafIdx);
	}
	HK_TIMER_END();
	for (int32_t i = 0; i < numCollidables; ++i) {
		hkpAabbTreeWorldManager::setNode(collidables.m_data[i], leafIdx.m_data[i]);
	}

	// EXE-INLINED none: hkArray<unsigned short>::~hkArray
	hkMemoryAllocator* pLeafHeap = hkMemoryRouter::getInstance().m_heap;
	leafIdx.m_size = 0;
	if ((leafIdx.m_capacityAndFlags & hkArrayBase<uint16_t>::DONT_DEALLOCATE_FLAG) == 0) {
		pLeafHeap->bufFree(leafIdx.m_data,
			static_cast<int32_t>((leafIdx.m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK) *
				sizeof(uint16_t)));
	}
	leafIdx.m_data = nullptr;
	leafIdx.m_capacityAndFlags = hkArrayBase<uint16_t>::DONT_DEALLOCATE_FLAG;
}

void hkpAabbTreeWorldManager::clearTransitionQueue() {
	m_mustUpdateActiveList = 0;
	m_transitionQueue.m_map.clear();
}

// Queues the collidable with 1 when the transition queue does not hold it yet.
void hkpAabbTreeWorldManager::queueActivated(const hkpCollidable* c) {
	int32_t iQueued;
	if (m_transitionQueue.get(c, &iQueued) == HK_FAILURE) {
		m_transitionQueue.insert(c, 1);
	}
}

// Queues the collidable with -1 when the transition queue does not hold it yet.
void hkpAabbTreeWorldManager::queueInactivated(const hkpCollidable* c) {
	int32_t iQueued;
	if (m_transitionQueue.get(c, &iQueued) == HK_FAILURE) {
		m_transitionQueue.insert(c, -1);
	}
}

bool hkpAabbTreeWorldManager::scanActiveListAndGetInactivated(hkArray<const hkpCollidable*, hkContainerHeapAllocator>&
		inactivatedOut) {
	if (m_enableFullScan) {
		for (int32_t i = m_activeList.m_size - 1; i >= 0; --i) {
			const hkpCollidable* pCollidable = m_activeList.m_data[i];
			if (pCollidable->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
				hkpEntity* pEntity = static_cast<hkpEntity*>(pCollidable->getOwner());
				if (pEntity && !pEntity->isActive().m_bool) {
					// EXE-INLINED none: hkArray::pushBackUnchecked
					inactivatedOut.m_data[inactivatedOut.m_size] = m_activeList.m_data[i];
					inactivatedOut.m_size++;
					m_activeList.removeAt(i);
				}
			}
		}
		return true;
	}
	const int32_t iLast = m_activeList.m_size - 1;
	m_activeListCheckStartIdx = (m_activeListCheckStartIdx < iLast) ? m_activeListCheckStartIdx : iLast;
	int32_t iNumChecked = 0;
	for (int32_t i = 0; i < m_numObjToCheckInAFrame; ++i) {
		const int32_t iIdx = m_activeListCheckStartIdx--;
		if (iIdx < 0) {
			m_activeListCheckStartIdx = m_activeList.m_size;
			break;
		}
		const hkpCollidable* pCollidable = m_activeList.m_data[iIdx];
		if (pCollidable->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
			hkpEntity* pEntity = static_cast<hkpEntity*>(pCollidable->getOwner());
			if (pEntity && !pEntity->isActive().m_bool) {
				// EXE-INLINED none: hkArray::pushBackUnchecked
				inactivatedOut.m_data[inactivatedOut.m_size] = m_activeList.m_data[iIdx];
				inactivatedOut.m_size++;
				m_activeList.removeAt(iIdx);
			}
		}
		++iNumChecked;
	}
	return iNumChecked == m_activeList.m_size;
}

void hkpAabbTreeWorldManager::pushBackToActiveList(const hkpCollidable* collidable) {
	if (m_useInactiveTree) {
		hkpAabbTreeWorldManager::setNode(collidable, 0xFFFF);
		hkArray<const hkpCollidable*, hkContainerHeapAllocator>* p_m_activeList = &m_activeList;
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		if (p_m_activeList->m_size ==
			(p_m_activeList->m_capacityAndFlags & hkArrayBase<const hkpCollidable*>::CAPACITY_MASK)) {
			hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_activeList, sizeof(const hkpCollidable*));
		}
		p_m_activeList->m_data[p_m_activeList->m_size++] = collidable;
	}
}

// KEEP (unregistered): the distributed build's coordinator job is a 0x60-byte local the exe hands to addJob
// (0xD04B50) as a JobQueueEntry; its enqueueInFront (0xD057F0) copies a whole 0x80-byte entry (rep movsd 0x20). The
// bytes it copies that nothing wrote reach the queue: the job's +0x3 (hkJob's padding03), +0x8..+0xF (hkJob's
// alignment padding), +0x44..+0x4F and +0x54..+0x5F (alignment padding after m_numAvailableEmptyNodes and
// m_collidables), and the 0x20 frame bytes past the job (+0x60..+0x7F; the entry's tail in src).
// From 32 collidables on, with a thread pool and a job queue, the tree is built by one coordinator job on the
// queue; otherwise it is built here from a mediator over the collidables. The depth and sub-job-size arrays are
// the SDK's hkArray<int>(4), constructed and destroyed inline.
void hkpAabbTreeWorldManager::buildKdTree(hkKdTree*& tree,
	hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables, hkpWorld* world, hkJobQueue* jobQueue,
	hkJobThreadPool* jobThreadPool) {
	if (jobThreadPool && jobQueue && collidables.getSize() >= 32) {
		HK_TIMER_BEGIN_LIST("BuildKdTreeMt", "Prepare");
		const int32_t numPrimitives = collidables.getSize();
		FNVD_CONSTRUCTED_LOCAL(hkKdTreeCinfo, cinfo, nullptr);
		cinfo.m_numPrimitives = numPrimitives;
		cinfo.m_isDistributedBuild.m_bool = 1;
		world->getBroadPhase()->getExtents(cinfo.m_aabb.m_min, cinfo.m_aabb.m_max);
		if (tree) {
			tree->reset(cinfo);
		} else {
			void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkKdTree));
			tree = pMemory ? ::new (pMemory) hkKdTree(cinfo) : nullptr;
		}
		m_kdTreeJobpHeader->m_openJobs = 4;

		// EXE-INLINED none: hkArray<int>::hkArray(int)
		FNVD_INLINED_LOCAL(hkArray<int32_t>, branchDepths, std::nullopt, std::nullopt);
		int32_t numBranchDepths = 4;
		branchDepths.m_data = hkMemoryRouter::getInstance().heap()._bufAlloc<int32_t>(numBranchDepths);
		branchDepths.m_size = numBranchDepths;
		if (numBranchDepths) {
			branchDepths.m_capacityAndFlags = numBranchDepths;
		} else {
			branchDepths.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
		}
		// EXE-INLINED none: hkArray<int>::hkArray(int)
		FNVD_INLINED_LOCAL(hkArray<int32_t>, subJobSizes, std::nullopt, std::nullopt);
		int32_t numSubJobSizes = 4;
		subJobSizes.m_data = hkMemoryRouter::getInstance().heap()._bufAlloc<int32_t>(numSubJobSizes);
		subJobSizes.m_size = numSubJobSizes;
		if (numSubJobSizes) {
			subJobSizes.m_capacityAndFlags = numSubJobSizes;
		} else {
			subJobSizes.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
		}

		// The queue takes the job as a whole entry (addJob's JobQueueEntry& overload, 0xD04B50), so the job is
		// written in place in the entry, as hkCpuKdTreeBuildCoordinatorJob reads it back.
		hkJobQueue::JobQueueEntry jobEntry;
		hkpKdTreeBuildCoordinatorJob& job = static_cast<hkpKdTreeBuildCoordinatorJob&>(static_cast<hkJob&>(jobEntry));
		// EXE-INLINED none: hkpKdTreeBuildCoordinatorJob::hkpKdTreeBuildCoordinatorJob
		job.m_jobSubType = hkpRayCastQueryJob::RAYCAST_QUERY_KD_TREE_BUILD_COORDINATOR_JOB;
		// EXE-INLINED none: hkEnum<E, S>::operator=
		job.m_jobType.m_storage = HK_JOB_TYPE_RAYCAST_QUERY;
		// EXE-INLINED none: hkEnum<E, S>::operator=
		job.m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_ENABLED;
		job.m_size = sizeof(hkpKdTreeBuildCoordinatorJob);
		job.m_threadAffinity = -1;
		job.m_semaphore = m_kdTreeSemaphore;
		job.m_sharedJobHeaderOnPpu = m_kdTreeJobpHeader;
		job.m_jobDoneFlag = nullptr;
		job.m_collisionInput = world->m_collisionInput;
		job.m_primitivesIn = m_kdTreeWorkingSet.m_entriesIn;
		job.m_primitivesOut = m_kdTreeWorkingSet.m_entriesOut;
		job.m_treeOut = tree->m_nodes.begin();
		job.m_projEntOut = tree->m_projectedEntries.begin();
		job.m_branchDepthsOut = branchDepths.m_data;
		job.m_subJobSizes = subJobSizes.m_data;
		job.m_numPrimitives = tree->m_numPrimitives;
		job.m_nodeArraySize = tree->m_numRegularNodes + tree->m_numReservedEmptyNodes;
		job.m_numAvailableEmptyNodes = tree->m_numReservedEmptyNodes;
		job.m_collidables = collidables.begin();

		HK_TIMER_SPLIT_LIST("Build");
		HK_TIMER_BEGIN_LIST("buildTreeDistributed", "queue");
		jobQueue->addJob(jobEntry, hkJobQueue::JOB_HIGH_PRIORITY);
		HK_TIMER_SPLIT_LIST("start");
		jobThreadPool->processAllJobs(jobQueue, HK_JOB_TYPE_RAYCAST_QUERY);
		jobQueue->processAllJobs();
		HK_TIMER_SPLIT_LIST("waitForCompletion");
		jobThreadPool->waitForCompletion();
		HK_TIMER_SPLIT_LIST("waitSemaphore");
		m_kdTreeSemaphore->acquire();
		HK_TIMER_SPLIT_LIST("finishPpu");
		hkKdTreeBuildingUtils::completeDistributedBuild(tree, numPrimitives, job.m_branchDepthsOut, job.m_subJobSizes);
		HK_TIMER_END_LIST();
		HK_TIMER_SPLIT_LIST("Finalize");
		HK_TIMER_END_LIST();

		// EXE-INLINED none: hkArray<int>::~hkArray
		{
			hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
			const int32_t capacityAndFlags = subJobSizes.m_capacityAndFlags;
			if (capacityAndFlags >= 0) {
				heap.bufFree(subJobSizes.m_data,
					(capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(int32_t)));
			}
		}
		// EXE-INLINED none: hkArray<int>::~hkArray
		{
			hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
			const int32_t capacityAndFlags = branchDepths.m_capacityAndFlags;
			if (capacityAndFlags >= 0) {
				heap.bufFree(branchDepths.m_data,
					(capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK) * static_cast<int32_t>(sizeof(int32_t)));
			}
		}
		return;
	}

	FNVD_CONSTRUCTED_LOCAL(hkpCollidableMediator, mediator, nullptr, collidables, world);
	const int32_t numPrimitives = mediator.hkpCollidableMediator::getNumPrimitives();
	FNVD_CONSTRUCTED_LOCAL(hkKdTreeCinfo, cinfo, nullptr);
	cinfo.m_numPrimitives = numPrimitives;
	cinfo.m_isDistributedBuild.m_bool = 0;
	world->getBroadPhase()->getExtents(cinfo.m_aabb.m_min, cinfo.m_aabb.m_max);
	if (tree) {
		tree->reset(cinfo);
	} else {
		void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkKdTree));
		tree = pMemory ? ::new (pMemory) hkKdTree(cinfo) : nullptr;
	}
	hkKdTreeBuildingUtils::projectPrimitives(&mediator, 0, numPrimitives, m_kdTreeWorkingSet.m_entriesIn);
	hkKdTreeBuildingUtils::buildTree(m_kdTreeWorkingSet, tree);
}

// The SDK's: without an inactive tree, gathers every collidable of the world and rebuilds the kd-tree; with one,
// moves the collidables found inactive from the active list into the inactive tree, the activated ones queued in
// m_transitionQueue out of it into the active list, and rebuilds the active tree. The exe inlines the hkLocalArray's
// constructor and destructor (written out here) and the map's iterator members.
void hkpAabbTreeWorldManager::incrementalBuild(hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool) {
	const int32_t numActive = m_activeList.m_size;
	alignas(16) hkLocalArray<const hkpCollidable*> inactivatedOut;
	inactivatedOut.m_size = 0;
	hkStackMemory& rStack = hkMemoryRouter::getInstance().m_stack;
	const hkpCollidable** pLocalMemory =
		static_cast<const hkpCollidable**>(static_cast<void*>(rStack.m_next.m_storage));
	rStack.m_next.m_storage += (numActive * static_cast<int32_t>(sizeof(const hkpCollidable*)) + 127) & ~127;
	inactivatedOut.m_data = pLocalMemory;
	inactivatedOut.m_capacityAndFlags = static_cast<int32_t>(static_cast<uint32_t>(numActive) |
		hkArrayBase<const hkpCollidable*>::DONT_DEALLOCATE_FLAG);
	inactivatedOut.m_localMemory = pLocalMemory;

	bool bInactivated = false;
	if (m_useInactiveTree) {
		HK_TIMER_BEGIN_LIST("IncrementalBuild", "Clear Tree");
		HK_TIMER_SPLIT_LIST("Find Inactivated");
		if (m_mustUpdateActiveList) {
			bInactivated = scanActiveListAndGetInactivated(inactivatedOut);
			HK_TIMER_SPLIT_LIST("Update Inactive Tree");
			hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* pTree = m_inactiveTree;
			for (int32_t i = 0; i < inactivatedOut.m_size; ++i) {
				const hkpCollidable* pCollidable = inactivatedOut.m_data[i];
				FNVD_INLINED_LOCAL(hkAabb, aabb, std::nullopt, std::nullopt);
				pCollidable->m_shape->getAabb(*pCollidable->m_motion.pMotion, 0.0f, aabb);
				HK_MONITOR_ADD_VALUE("Num of Added Nodes", 1.0f, HK_MONITOR_TYPE_INT);
				const uint16_t idx = pTree->add(aabb, const_cast<hkpCollidable*>(pCollidable));
				hkpAabbTreeWorldManager::setNode(inactivatedOut.m_data[i], idx);
			}
			m_mustUpdateActiveList = false;
		}
		HK_TIMER_SPLIT_LIST("Find Activated");
		hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* pTree = m_inactiveTree;
		for (int32_t it = m_transitionQueue.getIterator(); m_transitionQueue.isValid(it);
			it = m_transitionQueue.getNext(it)) {
			const hkpCollidable* pCollidable = m_transitionQueue.getKey(it);
			const int32_t iTransition = m_transitionQueue.getValue(it);
			const uint16_t nodeIdx = getNodeIdx(pCollidable);
			if (iTransition == 1 && nodeIdx != 0xFFFF &&
				pCollidable->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY) {
				HK_MONITOR_ADD_VALUE("Num of Removed Nodes", 1.0f, HK_MONITOR_TYPE_INT);
				pTree->remove(getNodeIdx(pCollidable));
				pushBackToActiveList(pCollidable);
			}
		}
		HK_TIMER_SPLIT_LIST("Update Active");
		buildActiveTree(m_activeList, jobQueue, jobThreadPool);
		HK_TIMER_END_LIST();
	} else {
		HK_TIMER_BEGIN_LIST("DynamicBuild", "Gather");
		m_activeList.m_size = 0;
		hkpAabbTreeWorldManager::gatherCollidablesFromWorld(m_activeList, m_world);
		HK_TIMER_SPLIT_LIST("Clear");
		HK_TIMER_SPLIT_LIST("Rebuild");
		buildKdTree(m_kdTree, m_activeList, m_world, jobQueue, jobThreadPool);
		HK_TIMER_END_LIST();
	}
	clearTransitionQueue();
	if (bInactivated) {
		m_mustUpdateActiveList = true;
	}
}

void hkpAabbTreeWorldManager::removeObjectFromTree(const hkpWorldObject* object) {
	if (m_useInactiveTree) {
		const hkpCollidable** m_data;
		uint16_t NodeIdx = getNodeIdx(&object->m_collidable);
		int32_t m_size = m_activeList.m_size;
		uint16_t v5 = NodeIdx;
		int32_t v6 = 0;
		if (m_size <= 0) {
			goto LABEL_8;
		}
		m_data = m_activeList.begin();
		while (*m_data != &object->m_collidable) {
			++v6;
			++m_data;
			if (v6 >= m_size) {
				goto LABEL_8;
			}
		}
		if (v6 == -1) {
		LABEL_8:;
			if (v5 != 0xFFFF) {
				m_inactiveTree->remove(v5);
			}
		} else {
			int32_t v8 = --m_activeList.m_size;
			if (v8 != v6) {
				m_activeList.m_data[v6] = m_activeList.m_data[v8];
			}
		}
	}
}

hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node*
hkpAabbTreeWorldManager::getNode(const hkpCollidable* collidable) {
	uint16_t NodeIdx = getNodeIdx(collidable);
	return static_cast<const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::NodeManager*>(&m_inactiveTree
																									   ->m_nodeManager)
		->getNode(NodeIdx);
}

// The SDK's form: the node index is kept in the owner's tree-data pointer.
uint16_t hkpAabbTreeWorldManager::getNodeIdx(const hkpCollidable* a1) const {
	return static_cast<uint16_t>(reinterpret_cast<uintptr_t>(static_cast<hkpWorldObject*>(a1->getOwner())
			->getTreeData()));
}

// The exe clears the array inline (a zero store to its size).
void hkpAabbTreeWorldManager::gatherCollidablesFromWorld(hkArray<const hkpCollidable*>& collidables,
	const hkpWorld* world) {
	collidables.m_size = 0;
	gatherActiveCollidablesFromWorld(collidables, world);
	gatherInactiveCollidablesFromWorld(collidables, world);
}

void hkpAabbTreeWorldManager::gatherActiveCollidablesFromWorld(hkArray<const hkpCollidable*>& collidables,
	const hkpWorld* world) {
	hkpSimulationIsland* v5;
	int32_t i;
	hkpCollidable* p_m_collidable;
	hkMemoryRouter* instance;
	int32_t j;
	hkpLinkedCollidable* v10;
	hkMemoryRouter* v11;
	int32_t v3 = 0;
	uint8_t v4 = (uint8_t)(((int32_t)world->m_activeSimulationIslands.m_size) <= 0);
	int32_t worlda = 0;
	if (!v4) {
		do {
			v5 = world->m_activeSimulationIslands.m_data[v3];
			for (i = 0; i < ((int32_t)v5->m_entities.m_size); ++i) {
				p_m_collidable = &v5->m_entities.m_data[i]->m_collidable;
				instance = &hkMemoryRouter::getInstance();
				if ((&collidables)->m_size == ((&collidables)->m_capacityAndFlags & 1073741823)) {
					hkArrayUtil::_reserveMore(*instance->m_heap, &collidables, sizeof(const hkpCollidable*));
				}
				v3 = worlda;
				(&collidables)->m_data[(&collidables)->m_size++] = p_m_collidable;
				v5 = world->m_activeSimulationIslands.m_data[worlda];
			}
			worlda = ++v3;
		} while (v3 < ((int32_t)world->m_activeSimulationIslands.m_size));
	}
	for (j = 0; j < ((int32_t)world->m_phantoms.m_size); ++j) {
		v10 = &world->m_phantoms.m_data[j]->m_collidable;
		if (v10->m_shape) {
			v11 = &hkMemoryRouter::getInstance();
			if ((&collidables)->m_size == ((&collidables)->m_capacityAndFlags & 1073741823)) {
				hkArrayUtil::_reserveMore(*v11->m_heap, &collidables, sizeof(const hkpCollidable*));
			}
			(&collidables)->m_data[(&collidables)->m_size++] = v10;
		}
	}
}

void hkpAabbTreeWorldManager::gatherInactiveCollidablesFromWorld(hkArray<const hkpCollidable*,
																	 hkContainerHeapAllocator>& collidables,
	const hkpWorld* world) {
	int32_t v3 = 0;
	bool v4 = world->m_inactiveSimulationIslands.m_size <= 0;
	int32_t worlda = 0;
	if (!v4) {
		do {
			hkpSimulationIsland* v5 = world->m_inactiveSimulationIslands.m_data[v3];
			for (int32_t i = 0; i < v5->m_entities.m_size; ++i) {
				hkpCollidable* p_m_collidable = &v5->m_entities.m_data[i]->m_collidable;
				hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
				if (collidables.m_size ==
					(collidables.m_capacityAndFlags & hkArrayBase<const hkpCollidable*>::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*Instance->m_heap, &collidables, sizeof(const hkpCollidable*));
				}
				v3 = worlda;
				collidables.m_data[collidables.m_size++] = p_m_collidable;
				v5 = world->m_inactiveSimulationIslands.m_data[worlda];
			}
			worlda = ++v3;
		} while (v3 < world->m_inactiveSimulationIslands.m_size);
	}
	hkpSimulationIsland* m_fixedIsland = world->m_fixedIsland;
	hkpRigidBody* m_fixedRigidBody = world->m_fixedRigidBody;
	int32_t v11 = 0;
	for (hkpRigidBody* worldb = m_fixedRigidBody; v11 < m_fixedIsland->m_entities.m_size; ++v11) {
		hkpRigidBody* v12 = static_cast<hkpRigidBody*>(m_fixedIsland->m_entities.m_data[v11]);
		if (v12 != worldb) {
			hkpCollidable* v13 = &v12->m_collidable;
			hkMemoryRouter* v14 = &hkMemoryRouter::getInstance();
			if (collidables.m_size ==
				(collidables.m_capacityAndFlags & hkArrayBase<const hkpCollidable*>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*v14->m_heap, &collidables, sizeof(const hkpCollidable*));
			}
			collidables.m_data[collidables.m_size++] = v13;
		}
	}
}

// The SDK keeps the node index in the owner's void* tree data: the integer is stored as the pointer's bits.
void hkpAabbTreeWorldManager::setNode(const hkpCollidable* collidable, uint16_t nodeIdx) {
	static_cast<hkpWorldObject*>(collidable->getOwner())
		->setTreeData(reinterpret_cast<void*>(static_cast<uintptr_t>(nodeIdx)));
}

void sub_D083C0() {
	;
}

void sub_D083D0() {
	;
}

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
int32_t reset(hkpAabbTreeWorldManager* apThis, [[maybe_unused]] void* apEdx) {
	apThis->m_activeList.m_size = (uint32_t)0;
	apThis->m_doneFirstBuild = (bool)0;
	apThis->m_mustUpdateActiveList = (bool)0;
	return 0;
}

int32_t hkpTreeWorldManager::calcWorkingBufferSize(int32_t maxObjects) {
	return 0;
}

// The exe has no copy (no vtable of the class): the mediators' destructors inline it, with nothing to free.
hkKdTreeBuildInput::Mediator::~Mediator() {}
