#pragma once

#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobs.h"

class hkpBroadPhase;
class hkpCollidable;
struct hkpRootCdPoint;

// One world get-closest-points query: the collidable to query the world with and its result buffer.
struct hkpWorldGetClosestPointsCommand {
    const hkpCollidable* m_collidable;
    hkpRootCdPoint* m_results;
    int32_t m_resultsCapacity;
    uint32_t m_numResultsOut;
};

// Batches world get-closest-points commands against the world's spatial acceleration structures.
struct hkpWorldGetClosestPointsJob : public hkpCollisionQueryJob {
    enum : int32_t {
        MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK = 0x40,
    };

    // Clears each command's result count; the tasks are numCommands commands numCommandsPerTask at a time.
    hkpWorldGetClosestPointsJob(
        const hkpProcessCollisionInput* input,
        hkpCollisionQueryJobHeader* jobHeader,
        const hkpWorldGetClosestPointsCommand* commandArray,
        int32_t numCommands,
        const hkpBroadPhase* broadphase,
        float tolerance,
        hkSemaphore* semaphore,
        int32_t numCommandsPerTask = MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK
    )
      : hkpCollisionQueryJob(COLLISION_QUERY_WORLD_GET_CLOSEST_POINTS, sizeof(hkpWorldGetClosestPointsJob)) {
        for (int32_t i = 0; i < numCommands; i++) {
            const_cast<hkpWorldGetClosestPointsCommand&>(commandArray[i]).m_numResultsOut = 0;
        }
        m_collisionInput = input;
        m_sharedJobHeaderOnPpu = jobHeader;
        m_numCommandsPerTask = numCommandsPerTask;
        m_broadphase = broadphase;
        m_tolerance = tolerance;
        m_semaphore = semaphore;
        m_commandArray = commandArray;
        m_numCommands = numCommands;
        m_sharedJobHeaderOnPpu->m_openJobs = (numCommands - 1) / numCommandsPerTask + 1;
        m_aabbTreeInt16 = nullptr;
        m_kdTree = nullptr;
    }

    // Nothing to do on the PC.
    void setRunsOnSpuOrPpu();

    const hkpBroadPhase* m_broadphase;
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* m_aabbTreeInt16;
    const hkKdTree* m_kdTree;
    float m_tolerance;
    int32_t m_numCommandsPerTask;
    const hkpWorldGetClosestPointsCommand* m_commandArray;
    int32_t m_numCommands;

    // 4 bytes of padding: hkJob is 16-byte aligned, so the job's size rounds 0x3C up to 0x40.
};
