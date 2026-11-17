#include "Havok/Source/Physics/Dynamics/World/Util/KdTree/hkpKdTreeWorldManager.h"

#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeUtils.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgentConfig.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Query/Collector/KdTreeCollector/hkpKdTreeCollidableRaycaster.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpTreeWorldManager.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Thread/Pool/hkJobThreadPool.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildInput.h"
#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildingUtils.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/Query/Collector/KdTreeCollector/hkpKdTreeWorldLinearCaster.h"
#include "Havok/Source/Physics/Collide/Util/KdTree/Mediator/hkpCollidableMediator.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"

hkpKdTreeWorldManager::hkpKdTreeWorldManager(hkpWorld* world, hkpKdTreeWorldManager::ObjectRemovedBehavior behav)
	: m_world(world), m_mustUpdateFromWorld(true) {
	m_objectRemovedBehavior = behav;
	m_type = hkpTreeWorldManager::MANAGER_KDTREE;
	m_world->addEntityListener(this);
	m_world->addPhantomListener(this);
}

hkpKdTreeWorldManager::~hkpKdTreeWorldManager() {
	m_world->removeEntityListener(this);
	m_world->removePhantomListener(this);
	delete m_kdTree;
}

void hkpKdTreeWorldManager::entityAddedCallback(hkpEntity* entity) {
	m_mustUpdateFromWorld.m_bool = 1;
}

// An entity leaving the world marks the tree dirty, or, when the manager was built to remove objects from the tree,
// takes the entity's collidable out of it (a tail jump into removeObjectFromTree in the exe).
void hkpKdTreeWorldManager::entityRemovedCallback(hkpEntity* entity) {
	if (m_objectRemovedBehavior.m_storage != MARK_TREE_AS_DIRTY) {
		removeObjectFromTree(entity);
		return;
	}
	m_mustUpdateFromWorld.m_bool = 1;
}

void hkpKdTreeWorldManager::entitySetMotionTypeCallback(hkpEntity* entity) {
	;
}

void hkpKdTreeWorldManager::phantomAddedCallback(hkpPhantom* phantom) {
	if (phantom->m_collidable.m_shape) {
		m_mustUpdateFromWorld.m_bool = 1;
	}
}

void hkpKdTreeWorldManager::phantomRemovedCallback(hkpPhantom* phantom) {
	if (phantom->m_collidable.m_shape) {
		if (m_objectRemovedBehavior.m_storage) {
			removeObjectFromTree(phantom);
		} else {
			m_mustUpdateFromWorld.m_bool = 1;
		}
	}
}

// The exe's HK_TIMER_BEGIN_LIST, HK_TIMER_SPLIT_LIST and HK_TIMER_END_LIST write into the stream's raw buffer, which
// the exe itself treats as untyped (hence the casts). The list starts empty (no storage, not ours), and the exe frees
// it inline. The last timer's rdtsc goes through the jobThreadPool argument's stack slot in the exe (its spill slot
// is reused); src's spill lands elsewhere.
void hkpKdTreeWorldManager::updateFromWorld(hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool) {
	HK_TIMER_BEGIN_LIST("kdTreeUpdate", "gather");
	FNVD_INLINED_LOCAL(hkArray<const hkpCollidable*>, collidables, std::nullopt, std::nullopt);
	collidables.m_data = nullptr;
	collidables.m_size = 0;
	collidables.m_capacityAndFlags = hkArrayBase<const hkpCollidable*>::DONT_DEALLOCATE_FLAG;
	gatherCollidablesFromWorld(collidables);
	HK_TIMER_SPLIT_LIST("rebuild");
	buildTree(m_kdTree, collidables, jobQueue, jobThreadPool);
	m_mustUpdateFromWorld.m_bool = 0;
	HK_TIMER_END_LIST();

	hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
	const int32_t capacityAndFlags = collidables.m_capacityAndFlags;
	collidables.m_size = 0;
	if (capacityAndFlags >= 0) {
		heap.bufFree(collidables.m_data,
			(capacityAndFlags & hkArrayBase<const hkpCollidable*>::CAPACITY_MASK) *
				static_cast<int32_t>(sizeof(const hkpCollidable*)));
	}
}

void hkpKdTreeWorldManager::castRay(const hkpWorldRayCastInput& input, const hkpCollisionFilter* filter,
	hkpWorldRayCastOutput& output) const {
	hkpWorldRayCastOutput* pOutput = &output;
	FNVD_INLINED_LOCAL(hkpKdTreeCollidableRaycaster, collector, hkpKdTreeCollidableRaycaster_vftable, std::nullopt);
	// MAYBE-INLINED 0xD0A39E: before the vptr store of hkpKdTreeCollidableRaycaster's inlined constructor
	collector.m_output = pOutput;
	// MAYBE-INLINED 0xD0A3AE: after the vptr store of hkpKdTreeCollidableRaycaster's inlined constructor
	collector.m_collector = 0;
	// MAYBE-INLINED 0xD0A3B2: after the vptr store of hkpKdTreeCollidableRaycaster's inlined constructor
	collector.m_input = &input;
	if (filter) {
		collector.m_filter = static_cast<const hkpRayCollidableFilter*>(filter);
		collector.m_collectionFilter = static_cast<const hkpRayShapeCollectionFilter*>(filter);
	} else {
		collector.m_filter = 0;
		collector.m_collectionFilter = 0;
	}
	// The exe passes the stack slot of its output pointer, dead by then, as the hidden result.
	hkBool result;
	hkKdTreeUtils::castRayIterative(&result, m_kdTree, input.m_from, input.m_to, &collector);
}

// Casts the ray through the kd-tree, the hits going to collector. The raycaster's output is a local the exe resets
// inline (hkpWorldRayCastOutput's constructor) and never reads back; the hkBool result is dropped.
void hkpKdTreeWorldManager::castRay(const hkpWorldRayCastInput& input, const hkpCollisionFilter* filter,
	hkpRayHitCollector& collector) const {
	// The exe's frame holds the raycaster just below the output, so the output is declared first.
	hkpWorldRayCastOutput kOutput;
	// The exe inlines the raycaster's constructor and calls no destructor.
	FNVD_CONSTRUCTED_LOCAL(hkpKdTreeCollidableRaycaster, kRaycaster, nullptr);
	kRaycaster.m_input = &input;
	kRaycaster.m_filter = filter;
	kRaycaster.m_collectionFilter = filter;
	kRaycaster.m_collector = &collector;
	kOutput.m_hitFraction = 1.0f;
	kOutput.m_extraInfo = -1;
	kOutput.m_shapeKeys[0] = static_cast<uint32_t>(-1);
	kRaycaster.m_output = &kOutput;
	kOutput.m_shapeKeyIndex = 0;
	kOutput.m_rootCollidable = nullptr;
	hkBool kResult;
	hkKdTreeUtils::castRayIterative(&kResult, m_kdTree, input.m_from, input.m_to, &kRaycaster);
}

void hkpKdTreeWorldManager::linearCast(const hkpCollidable* collA, const hkpLinearCastInput& input,
	const hkpCollidableCollidableFilter* filter, const hkpCollisionInput& collInput, hkpCollisionAgentConfig* config,
	hkpCdPointCollector& castCollector, hkpCdPointCollector* startPointCollector) {
	FNVD_INLINED_LOCAL(hkpKdTreeWorldLinearCaster, v9, hkpKdTreeWorldLinearCaster_vftable, std::nullopt);
	// MAYBE-INLINED 0xD0A4A8: before the vptr store of hkpKdTreeWorldLinearCaster's inlined constructor
	v9.m_shapeInput.m_maxExtraPenetration = 1.1920929e-07f;
	hkKdTree* m_kdTree = this->m_kdTree;
	// MAYBE-INLINED 0xD0A4D5: after the vptr store of hkpKdTreeWorldLinearCaster's inlined constructor
	v9.m_shapeInput.m_weldClosestPoints.m_storage = 0;
	v9.linearCast(m_kdTree, collA, input, filter, collInput, config, castCollector, startPointCollector);
}

void hkpKdTreeWorldManager::buildTree(hkKdTree*& tree,
	hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables, hkJobQueue* jobQueue,
	hkJobThreadPool* jobThreadPool) {
	if (jobQueue && jobThreadPool) {
		buildTreeDistributed(tree, collidables, jobQueue, jobThreadPool);
	} else {
		buildTreeSingleThreaded(tree, collidables);
	}
}

void hkpKdTreeWorldManager::gatherCollidablesFromWorld(hkArray<const hkpCollidable*, hkContainerHeapAllocator>&
		collidables) {
	collidables.m_size = 0;
	int32_t v4 = this->m_world->m_broadPhase->getNumObjects();
	hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t v6 = collidables.m_capacityAndFlags & hkArrayBase<const hkpCollidable*>::CAPACITY_MASK;
	if (v6 < v4) {
		int32_t v7 = 2 * v6;
		if (v4 < v7) {
			v4 = v7;
		}
		hkArrayUtil::_reserve(*m_heap, &collidables, v4, sizeof(const hkpCollidable*));
	}
	hkpWorld* m_world = this->m_world;
	int32_t v9 = 0;
	hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>* p_m_activeSimulationIslands =
		&m_world->m_activeSimulationIslands;
	if (p_m_activeSimulationIslands->m_size > 0) {
		do {
			hkpSimulationIsland* v11 = p_m_activeSimulationIslands->m_data[v9];
			for (int32_t i = 0; i < v11->m_entities.m_size; ++i) {
				collidables.m_data[collidables.m_size++] = &v11->m_entities.m_data[i]->m_collidable;
				v11 = p_m_activeSimulationIslands->m_data[v9];
			}
			++v9;
		} while (v9 < p_m_activeSimulationIslands->m_size);
	}
	hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>* p_m_inactiveSimulationIslands =
		&this->m_world->m_inactiveSimulationIslands;
	int32_t v14 = 0;
	if (this->m_world->m_inactiveSimulationIslands.m_size > 0) {
		do {
			hkpSimulationIsland* v15 = p_m_inactiveSimulationIslands->m_data[v14];
			for (int32_t j = 0; j < v15->m_entities.m_size; ++j) {
				collidables.m_data[collidables.m_size++] = &v15->m_entities.m_data[j]->m_collidable;
				v15 = p_m_inactiveSimulationIslands->m_data[v14];
			}
			++v14;
		} while (v14 < p_m_inactiveSimulationIslands->m_size);
	}
	hkArray<hkpPhantom*, hkContainerHeapAllocator>* p_m_phantoms = &this->m_world->m_phantoms;
	int32_t v18 = 0;
	if (this->m_world->m_phantoms.m_size > 0) {
		do {
			hkpCollidable* p_m_collidable = &p_m_phantoms->m_data[v18]->m_collidable;
			if (p_m_collidable->m_shape) {
				collidables.m_data[collidables.m_size++] = p_m_collidable;
			}
			++v18;
		} while (v18 < p_m_phantoms->m_size);
	}
	hkpWorld* v20 = this->m_world;
	hkpSimulationIsland* m_fixedIsland = v20->m_fixedIsland;
	int32_t v22 = 0;
	for (hkpRigidBody* a2 = v20->m_fixedRigidBody; v22 < m_fixedIsland->m_entities.m_size; ++v22) {
		hkpRigidBody* v23 = static_cast<hkpRigidBody*>(m_fixedIsland->m_entities.m_data[v22]);
		if (v23 != a2) {
			collidables.m_data[collidables.m_size++] = &v23->m_collidable;
		}
	}
}

// The object's collidable is a primitive of the tree: the exe adds 0x10 to the object (m_collidable, no getter call)
// and tail-jumps into the tree's invalidatePrimitive with the manager's m_kdTree.
void hkpKdTreeWorldManager::removeObjectFromTree(const hkpWorldObject* object) {
	m_kdTree->invalidatePrimitive(reinterpret_cast<hkPrimitiveId>(&object->m_collidable));
}

void hkpKdTreeWorldManager::buildTreeSingleThreaded(hkKdTree*& tree,
	hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables) {
	hkKdTree* newTree;
	hkKdTreeBuildInput::WorkingSet set;
	FNVD_CONSTRUCTED_LOCAL(hkpCollidableMediator, mediator, nullptr, collidables, m_world);
	hkKdTree* oldTree = tree;
	if (oldTree) {
		oldTree->~hkKdTree();
		hkMemoryRouter::getInstance().m_heap->blockFree(oldTree, sizeof(hkKdTree));
	}
	int32_t numPrimitives = mediator.hkpCollidableMediator::getNumPrimitives();
	FNVD_CONSTRUCTED_LOCAL(hkKdTreeCinfo, cinfo, nullptr);
	m_world->getBroadPhase()->getExtents(cinfo.m_aabb.m_min, cinfo.m_aabb.m_max);
	cinfo.m_numPrimitives = numPrimitives;
	cinfo.m_isDistributedBuild.m_bool = 0;
	hkKdTree* treeMemory = static_cast<hkKdTree*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkKdTree)));
	if (treeMemory) {
		newTree = ::new (treeMemory) hkKdTree(cinfo);
	} else {
		newTree = nullptr;
	}
	tree = newTree;
	HK_TIMER_BEGIN("buildTreeSingleThreaded", nullptr);
	hkKdTreeBuildingUtils::allocateBuffers(set, numPrimitives);
	hkKdTreeBuildingUtils::projectPrimitives(&mediator, 0, numPrimitives, set.m_entriesIn);
	hkKdTreeBuildingUtils::buildTree(set, tree);
	hkKdTreeBuildingUtils::deallocateBuffers(set, numPrimitives);
	HK_TIMER_END();
}

// Rebuilds the tree on the job queue: a coordinator job projects the collidables and spawns the distributed build,
// the thread pool and this thread process the queue, the semaphore waits for the last sub job, and the tree's nodes
// are finished here.
// KEEP: the coordinator job is 0x60 bytes, but addJob (0xD04B50) hands it to hkQueue::enqueue (0xD057A0), which
// copies a whole 0x80-byte JobQueueEntry (rep movsd, 0x20 dwords) into the queue: bytes nothing wrote reach memory
// outside the function (the job's +0x3 and +0x8..+0xF header padding, +0x44..+0x4F and +0x54..+0x5F, and the 0x20
// frame bytes after the job).
void hkpKdTreeWorldManager::buildTreeDistributed(hkKdTree*& tree,
	hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables, hkJobQueue* jobQueue,
	hkJobThreadPool* jobThreadPool) {
	hkKdTree* oldTree = tree;
	if (oldTree) {
		oldTree->~hkKdTree();
		hkMemoryRouter::getInstance().heap().blockFree(oldTree, sizeof(hkKdTree));
	}
	const int32_t numPrimitives = collidables.m_size;

	FNVD_CONSTRUCTED_LOCAL(hkKdTreeCinfo, cinfo, nullptr);
	cinfo.m_numPrimitives = numPrimitives;
	cinfo.m_isDistributedBuild.m_bool = 1;
	m_world->getBroadPhase()->getExtents(cinfo.m_aabb.m_min, cinfo.m_aabb.m_max);
	hkKdTree* newTree = static_cast<hkKdTree*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkKdTree)));
	tree = newTree ? ::new (newTree) hkKdTree(cinfo) : nullptr;

	hkKdTreeBuildInput::WorkingSet buffers;
	hkKdTreeBuildingUtils::allocateBuffers(buffers, numPrimitives);

	hkpCollisionQueryJobHeader* jobHeader = static_cast<hkpCollisionQueryJobHeader*>(hkMemoryRouter::getInstance()
			.heap()
			.blockAlloc(sizeof(hkpCollisionQueryJobHeader)));
	jobHeader->m_openJobs = 4;

	// The SDK's hkArray<int, hkContainerTempAllocator>(4), twice: the branches' depths and the sub jobs' sizes.
	// EXE-INLINED none: hkArray<int, hkContainerTempAllocator>::hkArray(int)
	using TempIntArray = hkArray<int32_t, hkContainerTempAllocator>;
	FNVD_INLINED_LOCAL(TempIntArray, branchDepths, std::nullopt, std::nullopt);
	{
		int32_t n = 4;
		branchDepths.m_data = hkMemoryRouter::getInstance().temp()._bufAlloc<int32_t>(n);
		branchDepths.m_size = 4;
		branchDepths.m_capacityAndFlags = n ? n : static_cast<int32_t>(hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG);
	}
	// EXE-INLINED none: hkArray<int, hkContainerTempAllocator>::hkArray(int)
	FNVD_INLINED_LOCAL(TempIntArray, subJobSizes, std::nullopt, std::nullopt);
	{
		int32_t n = 4;
		subJobSizes.m_data = hkMemoryRouter::getInstance().temp()._bufAlloc<int32_t>(n);
		subJobSizes.m_size = 4;
		subJobSizes.m_capacityAndFlags = n ? n : static_cast<int32_t>(hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG);
	}

	hkSemaphore* semaphore =
		static_cast<hkSemaphore*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkSemaphore)));
	semaphore = semaphore ? ::new (semaphore) hkSemaphore(0, 1000) : nullptr;

	// The SDK's constructors' header stores (their zeroed pointers the exe overwrites are left out).
	// EXE-INLINED none: hkpKdTreeBuildCoordinatorJob::hkpKdTreeBuildCoordinatorJob
	alignas(16) hkpKdTreeBuildCoordinatorJob job;
	job.m_jobSubType = hkpRayCastQueryJob::RAYCAST_QUERY_KD_TREE_BUILD_COORDINATOR_JOB;
	// EXE-INLINED none: hkEnum<E, S>::operator=
	job.m_jobType.m_storage = HK_JOB_TYPE_RAYCAST_QUERY;
	// EXE-INLINED none: hkEnum<E, S>::operator=
	job.m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_ENABLED;
	job.m_size = sizeof(hkpKdTreeBuildCoordinatorJob);
	job.m_threadAffinity = -1;
	job.m_semaphore = semaphore;
	job.m_sharedJobHeaderOnPpu = jobHeader;
	job.m_jobDoneFlag = nullptr;
	job.m_collisionInput = m_world->m_collisionInput;
	job.m_primitivesIn = buffers.m_entriesIn;
	job.m_primitivesOut = buffers.m_entriesOut;
	hkKdTree* builtTree = tree;
	job.m_treeOut = builtTree->m_nodes.m_data;
	job.m_projEntOut = builtTree->m_projectedEntries.m_data;
	job.m_branchDepthsOut = branchDepths.m_data;
	job.m_subJobSizes = subJobSizes.m_data;
	job.m_numPrimitives = builtTree->m_numPrimitives;
	job.m_nodeArraySize = builtTree->m_numRegularNodes + builtTree->m_numReservedEmptyNodes;
	job.m_numAvailableEmptyNodes = builtTree->m_numReservedEmptyNodes;
	job.m_collidables = collidables.m_data;

	HK_TIMER_BEGIN_LIST("buildTreeDistributed", "queue");
	// The exe calls the JobQueueEntry& overload (0xD04B50), which copies a whole entry from the job: the job is passed
	// as the queue's entry type through their common base hkJob.
	jobQueue->addJob(static_cast<hkJobQueue::JobQueueEntry&>(static_cast<hkJob&>(job)), hkJobQueue::JOB_HIGH_PRIORITY);
	HK_TIMER_SPLIT_LIST("start");
	jobThreadPool->processAllJobs(jobQueue, HK_JOB_TYPE_RAYCAST_QUERY);
	jobQueue->processAllJobs();
	HK_TIMER_SPLIT_LIST("waitForCompletion");
	jobThreadPool->waitForCompletion();
	HK_TIMER_SPLIT_LIST("waitSemaphore");
	semaphore->acquire();
	HK_TIMER_SPLIT_LIST("finishPpu");
	hkKdTreeBuildingUtils::completeDistributedBuild(tree, numPrimitives, job.m_branchDepthsOut, job.m_subJobSizes);
	HK_TIMER_END_LIST();

	hkKdTreeBuildingUtils::deallocateBuffers(buffers, numPrimitives);
	hkMemoryRouter::getInstance().heap().blockFree(jobHeader, sizeof(hkpCollisionQueryJobHeader));
	if (semaphore) {
		semaphore->~hkSemaphore();
		hkMemoryRouter::getInstance().heap().blockFree(semaphore, sizeof(hkSemaphore));
	}

	// The two arrays' destructors, in reverse order of construction.
	// EXE-INLINED none: hkArray<int, hkContainerTempAllocator>::~hkArray
	{
		hkMemoryAllocator& temp = hkMemoryRouter::getInstance().temp();
		if ((subJobSizes.m_capacityAndFlags & hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG) == 0) {
			temp._bufFree<int32_t>(subJobSizes.m_data,
				subJobSizes.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK);
		}
	}
	// EXE-INLINED none: hkArray<int, hkContainerTempAllocator>::~hkArray
	{
		hkMemoryAllocator& temp = hkMemoryRouter::getInstance().temp();
		if ((branchDepths.m_capacityAndFlags & hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG) == 0) {
			temp._bufFree<int32_t>(branchDepths.m_data,
				branchDepths.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK);
		}
	}
}
