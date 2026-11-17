#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobs.h"

class hkpCollidable;
struct hkpRootCdPoint;

// One pair linear cast: the two collidables, the cast's start and end, and the result buffers for the cast and for
// its start points.
struct hkpPairLinearCastCommand {
    enum : int32_t {
        MAXIMUM_RESULTS_CAPACITY = 0x10,
    };

    hkVector4 m_from;
    hkVector4 m_to;
    const hkpCollidable* m_collidableA;
    const hkpCollidable* m_collidableB;
    hkpRootCdPoint* m_results;
    int32_t m_resultsCapacity;
    int32_t m_numResultsOut;
    hkpRootCdPoint* m_startPointResults;
    int32_t m_startPointResultsCapacity;
    int32_t m_startPointNumResultsOut;
};

// Batches pair linear-cast commands between two collidables.
struct hkpPairLinearCastJob : public hkpCollisionQueryJob {
    enum : int32_t {
        MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK = 0x80,
    };

    // Nothing to do on the PC.
    void setRunsOnSpuOrPpu();

    float m_tolerance;
    float m_maxExtraPenetration;
    float m_iterativeLinearCastEarlyOutDistance;
    int32_t m_iterativeLinearCastMaxIterations;
    int32_t m_numCommandsPerTask;
    const hkpPairLinearCastCommand* m_commandArray;
    int32_t m_numCommands;

    // 4 bytes of padding: hkJob is 16-byte aligned, so the job's size rounds 0x3C up to 0x40.
};
