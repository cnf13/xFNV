#pragma once

#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeUtils.h"

class hkKdTree;
struct hkKdTreeProjectedEntry;
class hkpCollidable;
class hkpCollisionFilter;
class hkpFixedBufferRayHitCollector;
struct hkpKdTreeBuildSetupJob;
struct hkpWorldRayCastBundleCommand;
struct hkpWorldRayCastCommand;
struct hkpWorldRayCastInput;
struct hkpWorldRayCastOutput;
class hkSemaphore;
// The SPU build's software cache; only forward-declared.
class hkSpu4WayCache;

// The kd-tree jobs' setup helpers: initializeBuildSetupJobs (splits the collidables over the hkpKdTreeBuildSetupJob
// jobs hkCpuKdTreeBuildSetupJob runs) and makeBundleFromInputs. Statics only.
class hkKdTreeRayCastJobUtil {
public:
    // Splits numCollidables collidables over numJobs setup jobs (no more jobs than collidables): job 0 gets the
    // semaphore, the collidables, the output entries and the first share, each further job a copy of job 0 with the
    // next share, the last one up to numCollidables.
    static void initializeBuildSetupJobs(
        const hkpCollidable** collidablesPtr,
        int32_t numCollidables,
        hkKdTreeProjectedEntry* entries,
        hkSemaphore* semaphore,
        int32_t numJobs,
        hkpKdTreeBuildSetupJob* jobsOut
    );
    // The four rays' from and to points of inputs, transposed into two bundles, one ray per lane.
    static void makeBundleFromInputs(
        hkpWorldRayCastInput* inputs,
        hkKdTreeUtils::RayBundle& fromBundle,
        hkKdTreeUtils::RayBundle& toBundle
    );
};

// The ray cast jobs' tree casts.
class hkTreeRayCastJobUtil {
public:
    // Casts command's ray through kdTree (hkKdTreeUtils::castRayIterative) into a stack hkCpuWorldRayCastCollector with
    // filter; when the command's input asks for a collector, the hits go to fixedBufferHitCollector, built there on the
    // command's results, else to the command's results. The three caches and the output array are for the SPU build:
    // only the first four arguments are read, and the one caller, hkCpuWorldRayCastJob, passes null for the last four.
    static void castRayKdTree(
        const hkKdTree* kdTree,
        hkpWorldRayCastCommand* command,
        const hkpCollisionFilter* filter,
        hkpFixedBufferRayHitCollector* fixedBufferHitCollector,
        hkSpu4WayCache* collidableCache,
        hkSpu4WayCache* shapeCache,
        hkSpu4WayCache* transformCache,
        hkpWorldRayCastOutput* resultLocalArray
    );
    // Casts command's up to four rays as one bundle through kdTrees (hkKdTreeUtils::castRayBundleIterative) into a
    // stack hkKdTreeCpuBroadphaseBundleCastCollector with filter, on copies of the command's results that start at the
    // fractions of earlyHitFractionInOut, copies the results and hit counts back and leaves each ray's hit fraction in
    // earlyHitFractionInOut. The caches, the output array and the count are the SPU build's: only the first four
    // arguments are read, and the one caller, hkCpuWorldRayCastJob, passes the last five as null.
    static void castRayBundleKdTree(
        const hkKdTree* kdTree,
        hkpWorldRayCastBundleCommand* command,
        const hkpCollisionFilter* filter,
        hkVector4& earlyHitFractionInOut,
        hkSpu4WayCache* collidableCache,
        hkSpu4WayCache* shapeCache,
        hkSpu4WayCache* transformCache,
        hkpWorldRayCastOutput* resultLocalArray,
        int32_t* numResultsInOut
    );
};
