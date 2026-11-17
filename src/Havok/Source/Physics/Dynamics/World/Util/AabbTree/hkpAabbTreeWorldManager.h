#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Common/Internal/KdTree/Build/hkKdTreeBuildInput.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpIslandActivationListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpTreeWorldManager.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCinfo.h"

#include <stdint.h>

class hkJobQueue;
class hkJobThreadPool;
class hkpCollidable;
struct hkpCollisionQueryJobHeader;
class hkpWorld;
class hkpWorldObject;
class hkSemaphore;

// The tree world manager that keeps the world's collidables in AABB trees (an active and an inactive one), updated from
// the world's entity, phantom, island and post simulation callbacks.
class hkpAabbTreeWorldManager : public hkpTreeWorldManager,
                                public hkpEntityListener,
                                public hkpPhantomListener,
                                public hkpIslandActivationListener,
                                public hkpWorldPostSimulationListener {
public:
    enum : int32_t {
        NUM_DISTRIBUTED_BUILD_THRESHOLD = 0x20,
    };

    // The manager of world's AABB trees.
    hkpAabbTreeWorldManager(hkpWorld* world, hkpWorldCinfo::TreeUpdateType updateType, float aabbExpandDt);
    ~hkpAabbTreeWorldManager() override;

    void updateFromWorld(hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool) override;
    bool isUpToDate() const override {
        return m_treeIsClean;
    }
    void setUpToDate(bool upToDate) override {
        m_treeIsClean = upToDate;
    }
    void castRay(
        const hkpWorldRayCastInput& input,
        const hkpCollisionFilter* filter,
        hkpWorldRayCastOutput& output
    ) const override;
    void castRay(
        const hkpWorldRayCastInput& input,
        const hkpCollisionFilter* filter,
        hkpRayHitCollector& collector
    ) const override;
    void linearCast(
        const hkpCollidable* collA,
        const hkpLinearCastInput& input,
        const hkpCollidableCollidableFilter* filter,
        const hkpCollisionInput& collInput,
        hkpCollisionAgentConfig* config,
        hkpCdPointCollector& castCollector,
        hkpCdPointCollector* startPointCollector
    ) override;
    int32_t calcWorkingBufferSize(int32_t maxObjects) override;
    void setWorkingBuffer(void* ptr) override;
    // hkpEntityListener's vtable at +0x14.
    void entityAddedCallback(hkpEntity* entity) override;
    // hkpEntityListener's vtable at +0x14.
    void entityRemovedCallback(hkpEntity* entity) override;
    // hkpEntityListener's vtable at +0x14.
    void entitySetMotionTypeCallback(hkpEntity* entity) override;
    // hkpEntityListener's vtable at +0x14.
    void entityDeletedCallback(hkpEntity* entity) override;
    // hkpPhantomListener's vtable at +0x18.
    void phantomAddedCallback(hkpPhantom* phantom) override;
    // hkpPhantomListener's vtable at +0x18.
    void phantomRemovedCallback(hkpPhantom* phantom) override;
    // hkpIslandActivationListener's vtable at +0x1c.
    void islandActivatedCallback(hkpSimulationIsland* island) override;
    // hkpIslandActivationListener's vtable at +0x1c.
    void islandDeactivatedCallback(hkpSimulationIsland* island) override;
    // hkpWorldPostSimulationListener's vtable at +0x20.
    void postSimulationCallback(hkpWorld* world) override;
    // hkpWorldPostSimulationListener's vtable at +0x20.
    void inactiveEntityMovedCallback(hkpEntity* entity) override;

protected:
    // Gathers the active collidables (and the inactive ones too without an inactive tree), resets their tree nodes
    // and builds the active tree; with an inactive tree, gathers the inactive collidables and rebuilds that tree too.
    void fullBuild(hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool);
    // Builds the active tree from collidables when the active list is not empty.
    void
    buildActiveTree(hkArray<const hkpCollidable*>& collidables, hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool);
    // Builds the inactive tree from collidables.
    void
    buildInactiveTree(hkArray<const hkpCollidable*>& collidables, hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool);
    // Clears m_mustUpdateActiveList and empties m_transitionQueue.
    void clearTransitionQueue();
    // Adds collidable to m_transitionQueue with 1, but only when the queue does not hold it yet: looks the key up with
    // the map's get and inserts when it is absent.
    void queueActivated(const hkpCollidable* c);
    // The same with -1, which objIsInactive reads.
    void queueInactivated(const hkpCollidable* c);

public:
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    bool scanActiveListAndGetInactivated(hkArray<const hkpCollidable*, hkContainerHeapAllocator>& inactivatedOut);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    void pushBackToActiveList(const hkpCollidable* collidable);
    // Builds tree over collidables, distributed over the job queue from 32 collidables on.
    void buildKdTree(
        hkKdTree*& tree,
        hkArray<const hkpCollidable*, hkContainerHeapAllocator>& collidables,
        hkpWorld* world,
        hkJobQueue* jobQueue,
        hkJobThreadPool* jobThreadPool
    );
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc,string).
    void incrementalBuild(hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    void removeObjectFromTree(const hkpWorldObject* object);
    hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>::Node* getNode(const hkpCollidable* collidable);
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    uint16_t getNodeIdx(const hkpCollidable* a1) const;

    // Several bases declare one: DeletingDestruct's free, the Havok heap's block of m_memSizeAndFlags.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }
    // Empties collidables, then appends the world's active and inactive collidables.
    static void gatherCollidablesFromWorld(hkArray<const hkpCollidable*>& collidables, const hkpWorld* world);
    // Appends the collidables of the active islands' entities and of the phantoms that have
    // a shape.
    static void gatherActiveCollidablesFromWorld(hkArray<const hkpCollidable*>& collidables, const hkpWorld* world);
    // Appends the collidables of the inactive islands' entities and of the fixed island's
    // entities except the world's fixed body.
    static void gatherInactiveCollidablesFromWorld(hkArray<const hkpCollidable*>& collidables, const hkpWorld* world);

protected:
    // Stores idx as the tree node of the collidable's owner.
    static void setNode(const hkpCollidable* collidable, uint16_t nodeIdx);

public:
    bool m_useInactiveTree;
    uint8_t padding25[3];
    hkpWorld* m_world;
    bool m_doneFirstBuild;
    uint8_t padding2D[3];
    hkKdTreeBuildInput::WorkingSet m_kdTreeWorkingSet;
    int32_t m_maxObjects;
    hkpCollisionQueryJobHeader* m_kdTreeJobpHeader;
    float m_aabbExpandDt;
    hkArray<const hkpCollidable*> m_activeList;
    hkPointerMap<const hkpCollidable*, int32_t> m_transitionQueue;
    bool m_enableFullScan;
    uint8_t padding5D[3];
    int32_t m_activeListCheckStartIdx;
    int32_t m_numObjToCheckInAFrame;
    bool m_treeIsClean;
    bool m_mustUpdateActiveList;
    uint8_t padding6A[2];
    hkSemaphore* m_kdTreeSemaphore;
};

void sub_D083C0();
void sub_D083D0();
int32_t reset(hkpAabbTreeWorldManager* apThis, [[maybe_unused]] void* apEdx);
