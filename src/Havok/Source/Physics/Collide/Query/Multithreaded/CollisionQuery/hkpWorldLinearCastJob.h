#pragma once

#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpLinearCastInput.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobs.h"

class hkpBroadPhase;
class hkpCollidable;
struct hkpRootCdPoint;

// One world linear cast: the cast's input, the collidable it starts from and its result buffer.
struct hkpWorldLinearCastCommand {
    enum : int32_t {
        MAXIMUM_RESULTS_CAPACITY = 0x10,
    };

    hkpLinearCastInput m_input;
    const hkpCollidable* m_collidable;
    hkpRootCdPoint* m_results;
    int32_t m_resultsCapacity;
    int32_t m_numResultsOut;
};

// Batches world linear-cast commands against the world's spatial acceleration structures.
class hkpWorldLinearCastJob : public hkpCollisionQueryJob {
public:
    enum : int32_t {
        MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK = 0x40,
    };

    // The tasks are numCommands commands numCommandsPerTask at a time.
    hkpWorldLinearCastJob(
        const hkpProcessCollisionInput* input,
        hkpCollisionQueryJobHeader* jobHeader,
        const hkpWorldLinearCastCommand* commandArray,
        int32_t numCommands,
        const hkpBroadPhase* broadphase,
        hkSemaphore* semaphore,
        int32_t numCommandsPerTask = MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK
    )
      : hkpCollisionQueryJob(COLLISION_QUERY_WORLD_LINEAR_CAST, sizeof(hkpWorldLinearCastJob)) {
        m_collisionInput = input;
        m_sharedJobHeaderOnPpu = jobHeader;
        m_numCommandsPerTask = numCommandsPerTask;
        m_semaphore = semaphore;
        m_commandArray = commandArray;
        m_numCommands = numCommands;
        m_broadphase = broadphase;
        m_sharedJobHeaderOnPpu->m_openJobs = (numCommands - 1) / numCommandsPerTask + 1;
        m_aabbTree = nullptr;
        m_aabbTreeInt16 = nullptr;
        m_kdTree = nullptr;
    }

    // The class is 16-byte aligned through hkJob's HK_ALIGN16 m_jobSubType; there is no member after m_kdTree.
    void setRunsOnSpuOrPpu();

    int32_t m_numCommandsPerTask;
    const hkpWorldLinearCastCommand* m_commandArray;
    int32_t m_numCommands;
    const hkpBroadPhase* m_broadphase;
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* m_aabbTree;
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* m_aabbTreeInt16;
    const hkKdTree* m_kdTree;
};
