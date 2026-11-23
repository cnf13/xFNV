#pragma once

#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobs.h"

class hkpCollidable;
struct hkpRootCdPoint;

// One pair get-closest-points query: the two collidables, the result buffer and where this command's results start in
// the job's shared buffer. It is declared 16-byte aligned, which pads its members to the command stride.
struct alignas(16) hkpPairGetClosestPointsCommand {
    enum : int32_t {
        MAXIMUM_RESULTS_CAPACITY = 0x10,
    };

    const hkpCollidable* m_collidableA;
    const hkpCollidable* m_collidableB;
    hkpRootCdPoint* m_results;
    int32_t m_resultsCapacity;
    int32_t m_numResultsOut;
    uint32_t* m_indexIntoSharedResults;
};

// Batches pair get-closest-points commands: each command finds the closest points of two collidables within
// m_tolerance.
struct hkpPairGetClosestPointsJob : public hkpCollisionQueryJob {
    enum : int32_t {
        MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK = 0x100,
    };

    // Does nothing on the PC.
    void setRunsOnSpuOrPpu();

    float m_tolerance;
    int32_t m_numCommandsPerTask;
    const hkpPairGetClosestPointsCommand* m_commandArray;
    int32_t m_numCommands;
};
