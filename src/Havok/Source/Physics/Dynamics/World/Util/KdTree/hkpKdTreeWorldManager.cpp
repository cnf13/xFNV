#include "Havok/Source/Physics/Dynamics/World/Util/KdTree/hkpKdTreeWorldManager.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Thread/Pool/hkJobThreadPool.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildInput.h"
#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildingUtils.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeUtils.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgentConfig.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/KdTreeCollector/hkpKdTreeCollidableRaycaster.h"
#include "Havok/Source/Physics/Collide/Query/Collector/KdTreeCollector/hkpKdTreeWorldLinearCaster.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"
#include "Havok/Source/Physics/Collide/Util/KdTree/Mediator/hkpCollidableMediator.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpTreeWorldManager.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

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
// takes the entity's collidable out of it.
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
        }
        else {
            m_mustUpdateFromWorld.m_bool = 1;
        }
    }
}

// HK_TIMER_BEGIN_LIST, HK_TIMER_SPLIT_LIST and HK_TIMER_END_LIST write into the stream's raw buffer, which is treated
// as untyped (hence the casts).
void hkpKdTreeWorldManager::updateFromWorld(hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool) {
    HK_TIMER_BEGIN_LIST("kdTreeUpdate", "gather");
    hkArray<const hkpCollidable*> collidables;
    gatherCollidablesFromWorld(collidables);
    HK_TIMER_SPLIT_LIST("rebuild");
    buildTree(m_kdTree, collidables, jobQueue, jobThreadPool);
    m_mustUpdateFromWorld.m_bool = 0;
    HK_TIMER_END_LIST();
}

void hkpKdTreeWorldManager::castRay(
    const hkpWorldRayCastInput& input,
    const hkpCollisionFilter* filter,
    hkpWorldRayCastOutput& output
) const {
    hkpWorldRayCastOutput* pOutput = &output;
    hkpKdTreeCollidableRaycaster collector;
    // MAYBE-INLINED: before the vptr store of hkpKdTreeCollidableRaycaster's inlined constructor
    collector.m_output = pOutput;
    // MAYBE-INLINED: after the vptr store of hkpKdTreeCollidableRaycaster's inlined constructor
    collector.m_collector = nullptr;
    // MAYBE-INLINED: after the vptr store of hkpKdTreeCollidableRaycaster's inlined constructor
    collector.m_input = &input;
    if (filter) {
        collector.m_filter = static_cast<const hkpRayCollidableFilter*>(filter);
        collector.m_collectionFilter = static_cast<const hkpRayShapeCollectionFilter*>(filter);
    }
    else {
        collector.m_filter = nullptr;
        collector.m_collectionFilter = nullptr;
    }
    hkKdTreeUtils::castRayIterative(m_kdTree, input.m_from, input.m_to, &collector);
}

// Casts the ray through the kd-tree, the hits going to collector. The raycaster's output is a local that is reset by
// hkpWorldRayCastOutput's constructor and never read back; the hkBool result is dropped.
void hkpKdTreeWorldManager::castRay(
    const hkpWorldRayCastInput& input,
    const hkpCollisionFilter* filter,
    hkpRayHitCollector& collector
) const {
    hkpWorldRayCastOutput kOutput;
    // The raycaster's destructor is deliberately never called.
    hkpKdTreeCollidableRaycaster kRaycaster;
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
    hkKdTreeUtils::castRayIterative(m_kdTree, input.m_from, input.m_to, &kRaycaster);
}

void hkpKdTreeWorldManager::linearCast(
    const hkpCollidable* collA,
    const hkpLinearCastInput& input,
    const hkpCollidableCollidableFilter* filter,
    const hkpCollisionInput& collInput,
    hkpCollisionAgentConfig* config,
    hkpCdPointCollector& castCollector,
    hkpCdPointCollector* startPointCollector
) {
    hkpKdTreeWorldLinearCaster v9;
    hkKdTree* m_kdTree = this->m_kdTree;
    // MAYBE-INLINED: after the vptr store of hkpKdTreeWorldLinearCaster's inlined constructor
    v9.m_shapeInput.m_weldClosestPoints.m_storage = 0;
    v9.linearCast(m_kdTree, collA, input, filter, collInput, config, castCollector, startPointCollector);
}

void hkpKdTreeWorldManager::buildTree(
    hkKdTree*& tree,
    hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables,
    hkJobQueue* jobQueue,
    hkJobThreadPool* jobThreadPool
) {
    if (jobQueue && jobThreadPool) {
        buildTreeDistributed(tree, collidables, jobQueue, jobThreadPool);
    }
    else {
        buildTreeSingleThreaded(tree, collidables);
    }
}

void hkpKdTreeWorldManager::gatherCollidablesFromWorld(
    hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables
) {
    collidables.m_size = 0;
    collidables.reserve(this->m_world->m_broadPhase->getNumObjects());
    hkpWorld* m_world = this->m_world;
    int32_t v9 = 0;
    hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>*
        p_m_activeSimulationIslands = &m_world->m_activeSimulationIslands;
    if (p_m_activeSimulationIslands->m_size > 0) {
        do {
            hkpSimulationIsland* v11 = p_m_activeSimulationIslands->m_data[v9];
            for (int32_t i = 0; i < v11->m_entities.m_size; ++i) {
                collidables.m_data[collidables.m_size++] = &v11->m_entities.m_data[i]->m_collidable;
                v11 = p_m_activeSimulationIslands->m_data[v9];
            }
            ++v9;
        }
        while (v9 < p_m_activeSimulationIslands->m_size);
    }
    hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>*
        p_m_inactiveSimulationIslands = &this->m_world->m_inactiveSimulationIslands;
    int32_t v14 = 0;
    if (this->m_world->m_inactiveSimulationIslands.m_size > 0) {
        do {
            hkpSimulationIsland* v15 = p_m_inactiveSimulationIslands->m_data[v14];
            for (int32_t j = 0; j < v15->m_entities.m_size; ++j) {
                collidables.m_data[collidables.m_size++] = &v15->m_entities.m_data[j]->m_collidable;
                v15 = p_m_inactiveSimulationIslands->m_data[v14];
            }
            ++v14;
        }
        while (v14 < p_m_inactiveSimulationIslands->m_size);
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
        }
        while (v18 < p_m_phantoms->m_size);
    }
    hkpWorld* v20 = this->m_world;
    hkpSimulationIsland* m_fixedIsland = v20->m_fixedIsland;
    int32_t v22 = 0;
    for (hkpRigidBody* fixedBody = v20->m_fixedRigidBody; v22 < m_fixedIsland->m_entities.m_size; ++v22) {
        hkpRigidBody* v23 = static_cast<hkpRigidBody*>(m_fixedIsland->m_entities.m_data[v22]);
        if (v23 != fixedBody) {
            collidables.m_data[collidables.m_size++] = &v23->m_collidable;
        }
    }
}

// The object's collidable is a primitive of the tree: its m_collidable address, taken without a getter call, is the
// primitive id passed to m_kdTree's invalidatePrimitive.
void hkpKdTreeWorldManager::removeObjectFromTree(const hkpWorldObject* object) {
    m_kdTree->invalidatePrimitive(reinterpret_cast<hkPrimitiveId>(&object->m_collidable));
}

void hkpKdTreeWorldManager::buildTreeSingleThreaded(
    hkKdTree*& tree,
    hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables
) {
    hkKdTreeBuildInput::WorkingSet set;
    hkpCollidableMediator mediator(collidables, m_world);
    delete tree;
    int32_t numPrimitives = mediator.hkpCollidableMediator::getNumPrimitives();
    hkKdTreeCinfo cinfo;
    m_world->getBroadPhase()->getExtents(cinfo.m_aabb.m_min, cinfo.m_aabb.m_max);
    cinfo.m_numPrimitives = numPrimitives;
    cinfo.m_isDistributedBuild.m_bool = 0;
    tree = new hkKdTree(cinfo);
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
// KEEP: the coordinator job is 96 bytes, but addJob hands it to hkQueue::enqueue, which copies a whole 128-byte
// JobQueueEntry into the queue: bytes nothing wrote reach memory outside the function (the job's +0x3 and +0x8..+0xF
// header padding, +0x44..+0x4F and +0x54..+0x5F, and the 32 frame bytes after the job).
void hkpKdTreeWorldManager::buildTreeDistributed(
    hkKdTree*& tree,
    hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables,
    hkJobQueue* jobQueue,
    hkJobThreadPool* jobThreadPool
) {
    delete tree;
    const int32_t numPrimitives = collidables.m_size;

    hkKdTreeCinfo cinfo;
    cinfo.m_numPrimitives = numPrimitives;
    cinfo.m_isDistributedBuild.m_bool = 1;
    m_world->getBroadPhase()->getExtents(cinfo.m_aabb.m_min, cinfo.m_aabb.m_max);
    tree = new hkKdTree(cinfo);

    hkKdTreeBuildInput::WorkingSet buffers;
    hkKdTreeBuildingUtils::allocateBuffers(buffers, numPrimitives);

    hkpCollisionQueryJobHeader* jobHeader = static_cast<hkpCollisionQueryJobHeader*>(
        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpCollisionQueryJobHeader))
    );
    jobHeader->m_openJobs = 4;

    // The branches' depths and the sub jobs' sizes.
    using TempIntArray = hkArray<int32_t, hkContainerTempAllocator>;
    TempIntArray branchDepths(4);
    TempIntArray subJobSizes(4);

    hkSemaphore* semaphore = new hkSemaphore(0, 1000);

    alignas(16) hkpKdTreeBuildCoordinatorJob job(jobHeader, m_world);
    job.m_semaphore = semaphore;
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
    // Calls the JobQueueEntry& overload, which copies a whole entry from the job: the job is passed as the queue's
    // entry type through their common base hkJob.
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
    delete semaphore;
}
