#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"

class hkpProcessCollisionInput;

// The header the collision query jobs of one batch share: the number of the batch's jobs still open. The counter is
// declared 16-byte aligned, which makes the struct 16 bytes.
struct hkpCollisionQueryJobHeader {
    alignas(16) int32_t m_openJobs;
};

// The base of the collision query jobs (pair/world linear casts, closest points, MOPP AABB queries); the
// m_jobSubType byte is its hkpCollisionQueryJob::JobSubType.
struct hkpCollisionQueryJob : public hkJob {
    // The m_jobSubType values, which are also the indices of the job type's process function table.
    enum JobSubType : int32_t {
        COLLISION_QUERY_MOPP_AABB = 0x0,
        COLLISION_QUERY_PAIR_GET_CLOSEST_POINTS = 0x1,
        COLLISION_QUERY_WORLD_GET_CLOSEST_POINTS = 0x2,
        COLLISION_QUERY_PAIR_LINEAR_CAST = 0x3,
        COLLISION_QUERY_WORLD_LINEAR_CAST = 0x4,
        COLLISION_QUERY_KD_TREE_AABB_QUERY_JOB = 0x5,
        COLLISION_QUERY_AABB_TREE_AABB_QUERY_JOB = 0x6,
        COLLISION_QUERY_JOB_END = 0x7,
    };

    hkpCollisionQueryJob() = default;

    // Adds 1 to *m_jobDoneFlag (InterlockedExchangeAdd) when it is set and releases m_semaphore once when it is set.
    void atomicIncrementAndReleaseSemaphore() const;

protected:
    hkpCollisionQueryJob(JobSubType subType, uint16_t size) {
        m_jobSubType = static_cast<uint8_t>(subType);
        m_jobType = HK_JOB_TYPE_COLLISION_QUERY;
        m_jobSpuType = HK_JOB_SPU_TYPE_DISABLED;
        m_size = size;
        m_threadAffinity = -1;
        m_semaphore = nullptr;
        m_jobDoneFlag = nullptr;
        m_sharedJobHeaderOnPpu = nullptr;
        m_collisionInput = nullptr;
    }

public:
    // atomicIncrementAndReleaseSemaphore releases it through hkSemaphore::release.
    hkSemaphore* m_semaphore;
    hkpCollisionQueryJobHeader* m_sharedJobHeaderOnPpu;
    const hkpProcessCollisionInput* m_collisionInput;
    uint32_t* m_jobDoneFlag;
};

// One AABB query of a kd-tree AABB job: the box, the capacity of the caller's result buffer, the buffer and the
// number of hits written (SDK members; hkpcollide.pdb sizeof 48, hkAabb's 16-byte alignment rounding 0x2C up).
struct hkpKdTreeAabbCommand {
    hkAabb m_aabb;
    int32_t m_resultsCapacity;
    hkPrimitiveId* m_results;
    int32_t m_numResultsOut;
};

// The job that runs a batch of AABB queries against up to MAX_NUM_TREES kd-trees.
struct hkpKdTreeAabbJob : public hkpCollisionQueryJob {
    enum : int32_t {
        MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK = 256,
    };

    enum : int32_t {
        MAX_NUM_TREES = 4,
    };

    int32_t m_numCommandsPerTask;
    hkpKdTreeAabbCommand* m_aabbCommandArray;
    int32_t m_numAabbCommands;
    int32_t m_numTrees;
    const hkKdTree* m_trees[MAX_NUM_TREES];
};

// One AABB query of an AABB tree job: as hkpKdTreeAabbCommand, with the hits as the leaves' user data
// (hkpcollide.pdb sizeof 48).
struct hkpAabbTreeAabbCommand {
    hkAabb m_aabb;
    int32_t m_resultsCapacity;
    void** m_results;
    int32_t m_numResultsOut;
};

// The job that runs a batch of AABB queries against an AABB tree (float or 16-bit) or a kd-tree.
struct hkpAabbTreeAabbJob : public hkpCollisionQueryJob {
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* m_aabbTreeFp;
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* m_aabbTreeInt16;
    const hkKdTree* m_kdTree;
    hkpAabbTreeAabbCommand* m_aabbCommandArray;
    int32_t m_numAabbCommands;
};
