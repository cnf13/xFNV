#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobs.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"

class hkpBroadPhase;
class hkpCollidable;
class hkpProcessCollisionInput;

// The base of the ray cast query jobs; the m_jobSubType byte is its JobSubType. The SDK's m_sharedJobHeaderOnPpu is a
// hkpRayCastQueryJobHeader, its typedef of hkpCollisionQueryJobHeader.
struct hkpRayCastQueryJob : public hkJob {
    // The m_jobSubType values.
    enum JobSubType : int32_t {
        RAYCAST_QUERY_SHAPE_RAYCAST,
        RAYCAST_QUERY_WORLD_RAYCAST,
        RAYCAST_QUERY_KD_TREE_BUILD_FAST,
        RAYCAST_QUERY_KD_TREE_BUILD_FAST_DISTRIBUTED_JOB,
        RAYCAST_QUERY_KD_TREE_BUILD_FAST_DISTRIBUTED_SUBJOB,
        RAYCAST_QUERY_KD_TREE_BUILD_SETUP_JOB,
        RAYCAST_QUERY_KD_TREE_BUILD_COORDINATOR_JOB,
        RAYCAST_QUERY_WORLD_RAYCAST_WITH_AUTOBUNDLE,
        RAYCAST_QUERY_JOB_END
    };

    // Adds 1 to *m_jobDoneFlag (InterlockedExchangeAdd) when it is set and releases m_semaphore once when it is set.
    void atomicIncrementAndReleaseSemaphore() const;

    // hkSemaphoreBusyWait, which the PC build typedefs to hkSemaphore.
    hkSemaphore* m_semaphore;
    hkpCollisionQueryJobHeader* m_sharedJobHeaderOnPpu;
    uint32_t* m_jobDoneFlag;
    const hkpProcessCollisionInput* m_collisionInput;
};

// One shape ray cast: the ray, the filter and size of the collidables to test, and the results buffer it fills in.
// The strides and offsets hkCpuShapeRayCastJob reads: the ray input is a 0x30-byte copy, the commands are
// 0x50 bytes apart and m_useCollector is the byte at 0x4C.
struct hkpShapeRayCastCommand {
    // The SDK's limits (hkpRayCastQueryJobs.h).
    enum : int32_t {
        MAXIMUM_RESULTS_CAPACITY = 96,
    };

    enum : int32_t {
        MAXIMUM_NUM_COLLIDABLES = 64,
    };

    hkpShapeRayCastInput m_rayInput;
    uint32_t m_filterType;
    int32_t m_filterSize;
    const hkpCollidable** m_collidables;
    int32_t m_numCollidables;
    hkpWorldRayCastOutput* m_results;
    int32_t m_resultsCapacity;
    int32_t m_numResultsOut;
    hkBool m_useCollector;
};

// A job casting the rays of m_numCommands shape ray cast commands, m_numCommandsPerTask of them per task.
struct hkpShapeRayCastJob : public hkpRayCastQueryJob {
    enum : int32_t { MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK = 90 };

    // Nothing to do on the PC.
    void setRunsOnSpuOrPpu();

    int32_t m_numCommandsPerTask;
    const hkpShapeRayCastCommand* m_commandArray;
    int32_t m_numCommands;
};

// One world ray cast: the ray, the results buffer it fills in and whether it stops at the first hit.
struct hkpWorldRayCastCommand {
    hkpWorldRayCastInput m_rayInput;
    hkpWorldRayCastOutput* m_results;
    int32_t m_resultsCapacity;
    int32_t m_numResultsOut;
    hkBool m_useCollector;
    hkBool m_stopAfterFirstHit;
};

// Up to four world rays cast as one bundle: the rays, how many are in use, whether a ray stops at its first hit, each
// ray's results buffer and hit count, and whether the hits go to a collector.
struct hkpWorldRayCastBundleCommand {
    hkpWorldRayCastInput m_rayInput[4];
    int32_t m_numActiveRays;
    hkBool m_stopAfterFirstHit;
    hkpWorldRayCastOutput* m_results[4];
    alignas(16) int32_t m_numResultsOut[4];
    hkBool m_useCollector;
};

// A job casting world rays (single and bundled commands) through the broadphase, an AABB tree or a k-d tree.
struct hkpWorldRayCastJob : public hkpRayCastQueryJob {
    enum : int32_t { MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK = 64 };

    int32_t m_numCommandsPerTask;
    const hkpWorldRayCastCommand* m_commandArray;
    int32_t m_numCommands;
    const hkpWorldRayCastBundleCommand* m_bundleCommandArray;
    int32_t m_numBundleCommands;
    const hkpBroadPhase* m_broadphase;
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* m_aabbTreeFp;
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* m_aabbTreeInt16;
    const hkKdTree* m_kdTree;
};

// A k-d tree ray bundle cast: the four rays, whether a ray stops at its first hit, and the four results it fills in.
// m_results is the pointer getHitFractions reads the hit fractions through.
struct hkKdTreeRayCastBundleCommand {
    hkpWorldRayCastInput m_rayInput[4];
    hkBool m_stopAfterFirstHit;
    hkpWorldRayCastOutput* m_results;
    alignas(16) int32_t m_numResultsOut[4];
};

// A job building a k-d tree over m_numCollidables collidables in one task.
struct hkpKdTreeBuildJobFast : public hkpRayCastQueryJob {
    int32_t m_numCollidables;
    hkKdTreeNode* m_treeOut;
    hkKdTreeProjectedEntry* m_projEntOut;
    int32_t* m_treeDepthOut;
    int32_t m_nodeArraySize;
    int32_t m_numAvailableEmptyNodes;
    hkKdTreeProjectedEntry* m_entriesIn;
    hkKdTreeProjectedEntry* m_entriesOut;
};

// A k-d tree build split over sub jobs: the primitives' two buffers, the tree written, the branches' depths and the
// sub jobs' sizes.
struct hkpKdTreeBuildDistributedJob : public hkpRayCastQueryJob {
    hkKdTreeProjectedEntry* m_primitivesIn;
    hkKdTreeProjectedEntry* m_primitivesOut;
    hkKdTreeNode* m_treeOut;
    hkKdTreeProjectedEntry* m_projEntOut;
    int32_t* m_branchDepthsOut;
    int32_t* m_subJobSizes;
    int32_t m_numPrimitives;
    int32_t m_nodeArraySize;
    int32_t m_numAvailableEmptyNodes;
};

// The distributed build's first job: it projects the world's collidables and spawns the build.
struct hkpKdTreeBuildCoordinatorJob : public hkpKdTreeBuildDistributedJob {
    const hkpCollidable** m_collidables;
};

// Projects the collidables [m_startIdx, m_endIdx) into m_entriesOut; the last one to finish spawns the distributed
// build when m_spawnBuildDistributedJob is set.
struct hkpKdTreeBuildSetupJob : public hkpKdTreeBuildDistributedJob {
    const hkpCollidable** m_collidables;
    hkKdTreeProjectedEntry* m_entriesOut;
    int32_t m_startIdx;
    int32_t m_endIdx;
    int32_t m_spawnBuildDistributedJob;
};

// One branch of a distributed k-d tree build: its node, its primitives' range in the two buffers and their bounds.
struct hkpKdTreeBuildDistributedSubJob : public hkpRayCastQueryJob {
    hkKdTreeNode* m_treeOut;
    hkKdTreeProjectedEntry* m_projEntOut;
    int32_t m_currentNodeIdx;
    hkKdTreeProjectedEntry* m_primitivesIn;
    hkKdTreeProjectedEntry* m_primitivesOut;
    int32_t m_startPrimIdx;
    int32_t m_endPrimIdx;
    hkKdTreeProjectedEntry m_bounds;
    int32_t* m_branchDepthOut;
    int32_t m_nodeArraySize;
    int32_t m_numAvailableEmptyNodes;
};

// A job that sorts the rays [m_rayFrom, m_rayTo) of one octant (or the listed m_rayIndices) into single and bundled
// commands and casts them.
struct hkpWorldRayCastWithAutoBundleJob : public hkpRayCastQueryJob {
    int32_t* m_rayIndices;
    int32_t m_octantIdx;
    int32_t m_numIndices;
    const hkVector4* m_rayFrom;
    const hkVector4* m_rayTo;
    int32_t m_numRays;
    float m_angleVariance;
    float m_distanceVariance2;
    hkpWorldRayCastOutput* m_resultBuffer;
    hkpWorldRayCastCommand* m_singleCommands;
    int32_t m_numSingleCommands;
    hkpWorldRayCastBundleCommand* m_bundledCommands;
    int32_t m_numBundledCommands;
    const hkpProcessCollisionInput* m_collisionInput;
    hkpCollisionQueryJobHeader* m_jobHeader;
    // hkSemaphoreBusyWait, which the PC build typedefs to hkSemaphore.
    hkSemaphore* m_semaphore;
    const hkpBroadPhase* m_broadPhase;
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint16_t>* m_aabbTreeFp;
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>* m_aabbTreeInt16;
    const hkKdTree* m_kdTree;
};
