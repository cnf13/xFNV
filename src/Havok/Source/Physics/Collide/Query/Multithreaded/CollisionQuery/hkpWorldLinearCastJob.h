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
