#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"

#include "Havok/Source/Common/Base/Algorithm/UnionFind/hkUnionFind.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalBuffer.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnSector.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/hkpAgentMachineUtil.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Setup/hkpConstraintSolverSetup.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

hkClass hkpSimulationIslandClass(
    "hkpSimulationIsland",
    &hkReferencedObjectClass,
    108,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    nullptr,
    0,
    0
);

// hkpConstraintOwner, hkMultiThreadCheck and hkpAgentNnTrack have no constructor, so their fields are set here.
hkpSimulationIsland::hkpSimulationIsland(hkpWorld* apWorld)
  : m_world(apWorld), m_numConstraints(0), m_dirtyListIndex(0xFFFF), m_splitCheckRequested(0), m_sparseEnabled(0),
    m_actionListCleanupNeeded(0), m_allowIslandLocking(0), m_isInActiveIslandsArray(1), m_activeMark(1),
    m_tryToIncreaseIslandSizeMark(0), m_inIntegrateJob(0), m_timeSinceLastHighFrequencyCheck(0.0f),
    m_timeSinceLastLowFrequencyCheck(0.0f), m_timeOfDeactivation(-10.0f) {
    m_multiThreadCheck.m_threadId = hkMultiThreadCheck::UNMARKED;
    m_multiThreadCheck.m_markCount = 0x8000;
    m_agentTrack.m_bytesUsedInLastSector = HK_AGENT3_SECTOR_SIZE;
    m_constraintInfo.m_maxSizeOfSchema = 0;
    m_constraintInfo.m_sizeOfSchemas = 0;
    m_constraintInfo.m_numSolverResults = 0;
    m_constraintInfo.m_numSolverElemTemps = 0;
}

// The arrays and the agent track's sectors free themselves.
hkpSimulationIsland::~hkpSimulationIsland() {}

void hkpSimulationIsland::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    int32_t m_capacityAndFlags = m_entities.m_capacityAndFlags;
    if (m_capacityAndFlags >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t AllocatedSize = hkStatisticsCollector::getAllocatedSize(
            m_entities.m_data,
            static_cast<int32_t>(sizeof(*m_entities.m_data)) * m_capacityAndFlags
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "EntityPtrs",
            m_entities.m_data,
            static_cast<int32_t>(sizeof(*m_entities.m_data)) * m_entities.m_size,
            AllocatedSize
        );
    }
    int32_t v7 = 0;
    for (int32_t i = 0; v7 < m_entities.m_size; i = v7) {
        hkpEntity* v8 = m_entities.m_data[v7];
        collector->addReferencedObject("Entity", v8, 0);
        collector->pushDir("Constraints");
        if ((v8->m_constraintsMaster.m_capacityAndFlags & hkSmallArray<hkConstraintInternal>::DONT_DEALLOCATE_FLAG) ==
                0 ||
            collector->m_includeUnmanagedArrayMemory) {
            collector->addChunk(
                hkStatisticsCollector::MEM_ARRAY,
                "ConMstPtr",
                v8->m_constraintsMaster.begin(),
                static_cast<int32_t>(sizeof(*v8->m_constraintsMaster.begin())) * v8->m_constraintsMaster.m_size,
                static_cast<int32_t>(sizeof(*v8->m_constraintsMaster.begin())) *
                    (v8->m_constraintsMaster.m_capacityAndFlags & hkSmallArray<hkConstraintInternal>::CAPACITY_MASK)
            );
        }
        int32_t collectora = 0;
        if (v8->m_constraintsMaster.m_size) {
            int32_t v9 = 0;
            do {
                hkpConstraintInstance* m_constraint = v8->m_constraintsMaster.m_data[v9].m_constraint;
                if ((m_constraint->m_memSizeAndFlags & hkReferencedObject::MASK_MEMSIZE) != 0) {
                    collector->addReferencedObject("ConInstance", m_constraint, 0);
                }
                hkpConstraintData* m_data = v8->m_constraintsMaster.m_data[v9].m_constraint->getDataRw();
                if ((m_data->m_memSizeAndFlags & hkReferencedObject::MASK_MEMSIZE) != 0) {
                    collector->addReferencedObject("ConData", m_data, 0);
                }
                ++v9;
                ++collectora;
            }
            while (collectora < v8->m_constraintsMaster.m_size);
            v7 = i;
        }
        collector->popDir();
        ++v7;
    }
    int32_t v12 = m_actions.m_capacityAndFlags;
    if (v12 >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t v13 = hkStatisticsCollector::getAllocatedSize(
            m_actions.m_data,
            static_cast<int32_t>(sizeof(*m_actions.m_data)) * v12
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "ActionPtrs",
            m_actions.m_data,
            static_cast<int32_t>(sizeof(*m_actions.m_data)) * m_actions.m_size,
            v13
        );
    }
    for (hkpAction* action : m_actions) {
        collector->addReferencedObject("Actions", action, 0);
    }
    hkpAgentMachineUtil::calcNnStatisticsContactMgrsOnly(m_agentTrack, collector);
    collector->pushDir("CollAgents");
    hkpAgentMachineUtil::calcNnStatistics(m_agentTrack, *m_world->m_collisionInput, collector);
    collector->popDir();
}

// Hands the constraint to the world of its first entity.
void hkpSimulationIsland::addConstraintToCriticalLockedIsland(hkpConstraintInstance* constraint) {
    hkpWorldOperationUtil::addConstraintToCriticalLockedIsland(constraint->m_entities[0]->m_world, constraint);
}

void hkpSimulationIsland::removeConstraintFromCriticalLockedIsland(hkpConstraintInstance* constraint) {
    hkpWorldOperationUtil::removeConstraintFromCriticalLockedIsland(constraint->m_entities[0]->m_world, constraint);
}

void hkpSimulationIsland::addCallbackRequest(hkpConstraintInstance* constraint, int32_t request) {
    constraint->getInternal()->m_callbackRequest |= request;
}

// Joins the island's entities in checkConnectivityOut by their collision partners, the constraints they master and
// their actions, skipping fixed bodies; true as soon as they form one group, else whether they do at the end.
bool hkpSimulationIsland::isFullyConnected(hkUnionFind& checkConnectivityOut) {
    for (int32_t e = 0; e < m_entities.m_size; e++) {
        hkpEntity* entity = m_entities.m_data[e];
        hkArray<hkpLinkedCollidable::CollisionEntry>& collisionEntries = entity->m_collidable.m_collisionEntries;
        for (const hkpLinkedCollidable::CollisionEntry& entry : collisionEntries) {
            hkpEntity* otherEntity = static_cast<hkpEntity*>(entry.m_partner->getOwner());
            if (!otherEntity->isFixed().m_bool) {
                checkConnectivityOut.addEdge(e, otherEntity->m_storageIndex);
                if (checkConnectivityOut.isOneGroup().m_bool) {
                    return true;
                }
            }
        }
    }
    for (hkpEntity* entity : m_entities) {
        for (const hkConstraintInternal& con : entity->m_constraintsMaster) {
            if (!con.m_entities[0]->isFixed().m_bool && !con.m_entities[1]->isFixed().m_bool) {
                checkConnectivityOut.addEdge(con.m_entities[0]->m_storageIndex, con.m_entities[1]->m_storageIndex);
                if (checkConnectivityOut.isOneGroup().m_bool) {
                    return true;
                }
            }
        }
    }
    hkInplaceArray<hkpEntity*, 10, hkContainerHeapAllocator> actionEntities;
    for (hkpAction* action : m_actions) {
        if (action) {
            actionEntities.clear();
            action->getEntities(actionEntities);
            // The first unfixed entity is joined with every later unfixed one.
            int32_t firstUnfixed = -1;
            int32_t k;
            for (k = 0; k < actionEntities.m_size && firstUnfixed == -1; k++) {
                if (!actionEntities.m_data[k]->isFixed().m_bool) {
                    firstUnfixed = k;
                }
            }
            for (; k < actionEntities.m_size; k++) {
                if (!actionEntities.m_data[k]->isFixed().m_bool) {
                    checkConnectivityOut.addEdge(
                        actionEntities.m_data[firstUnfixed]->m_storageIndex,
                        actionEntities.m_data[k]->m_storageIndex
                    );
                    if (checkConnectivityOut.isOneGroup().m_bool) {
                        return true;
                    }
                }
            }
        }
    }
    return checkConnectivityOut.isOneGroup().m_bool;
}

void hkpSimulationIsland::internalRemoveEntity(hkpEntity* entity) {
    m_entities.m_data[entity->m_storageIndex] = m_entities.m_data[m_entities.m_size - 1];
    m_entities.m_data[entity->m_storageIndex]->m_storageIndex = entity->m_storageIndex;
    --m_entities.m_size;
    entity->m_simulationIsland = nullptr;
    entity->m_storageIndex = static_cast<uint16_t>(-1);
    m_splitCheckRequested = 1;
}

// Havok SDK.
void hkpSimulationIsland::internalAddEntity(hkpEntity* entity) {
    entity->m_simulationIsland = this;
    entity->m_storageIndex = m_entities.m_size;
    m_entities.pushBack(entity);
}

// Appends the action to the island's list and sets its island.
void hkpSimulationIsland::addAction(hkpAction* act) {
    m_actions.pushBack(act);
    act->m_island = this;
}

// Removes the action from the island's list.
void hkpSimulationIsland::removeAction(hkpAction* act) {
    int32_t m_size = m_actions.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
        v3 = -1;
    }
    else {
        hkpAction** m_data = static_cast<hkArrayBase<hkpAction*>&>(m_actions).begin();
        while (*m_data != act) {
            ++v3;
            ++m_data;
            if (v3 >= m_size) {
                v3 = -1;
                break;
            }
        }
    }
    m_actions.m_data[v3] = 0;
    act->m_island = nullptr;
    m_splitCheckRequested = 1;
    m_actionListCleanupNeeded = 1;
}

// The SDK's multithreading check: empty, as the game's Havok has no multithreading debug checks.
void hkpSimulationIsland::markForWrite() {}

void hkpSimulationIsland::mergeConstraintInfo(hkpSimulationIsland& other) {
    int32_t m_maxSizeOfSchema;
    m_maxSizeOfSchema = other.m_constraintInfo.m_maxSizeOfSchema;
    if (m_constraintInfo.m_maxSizeOfSchema > m_maxSizeOfSchema) {
        m_maxSizeOfSchema = m_constraintInfo.m_maxSizeOfSchema;
    }
    m_constraintInfo.m_maxSizeOfSchema = m_maxSizeOfSchema;
    m_constraintInfo.m_sizeOfSchemas += other.m_constraintInfo.m_sizeOfSchemas;
    m_constraintInfo.m_numSolverResults += other.m_constraintInfo.m_numSolverResults;
    m_constraintInfo.m_numSolverElemTemps += other.m_constraintInfo.m_numSolverElemTemps;
}

// Whether the island's entities form one connected set: a union-find over a parent buffer from the stack allocator. The
// buffer's m_debugSize is deliberately left unwritten; nothing reads it.
bool hkpSimulationIsland::isFullyConnected() {
    hkLocalBuffer<int32_t> parents(m_entities.m_size, nullptr);
    hkUnionFind checkConnectivity(&parents, m_entities.m_size);
    bool bResult = this->isFullyConnected(checkConnectivity);
    return bResult;
}

// Empty in the release build (the SDK's checks are debug-only).
void hkpSimulationIsland::isValid() {
    ;
}

// calcBufferSize's code again (hkpConstraintSolverSetup.cpp).
int32_t calcBufferSizeForSolve(hkpSimulationIsland& island) {
    alignas(16) hkpBuildJacobianTaskHeader taskHeader;
    const int32_t size = hkpConstraintSolverSetup::calcBufferOffsetsForSolve(island, nullptr, 0, taskHeader);
    return size;
}

// Clears the island's split request and splits it, adding the new islands to newSplitIslands. The old-entities array is
// constructed inline and deliberately never destroyed: nothing is freed after the split.
void hkCpuSplitSimulationIslandJobImpl(
    hkpSimulationIsland* island,
    hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>& newSplitIslands
) {
    island->m_splitCheckRequested = 0;
    using oldEntities_t = hkArray<hkpEntity*, hkContainerHeapAllocator>;
    alignas(oldEntities_t) uint8_t oldEntitiesStorage[sizeof(oldEntities_t)];
    oldEntities_t& oldEntities = *::new (static_cast<void*>(oldEntitiesStorage)) oldEntities_t;
    island->markForWrite();
    hkpWorldOperationUtil::splitSimulationIsland(island, island->getWorld(), newSplitIslands, &oldEntities);
}
