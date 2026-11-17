#pragma once

#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"

#include <stdint.h>

class hkJobQueue;
class hkJobThreadPool;

// The automatic ray bundling of the world ray cast jobs: splitToOctant, autoBundleFromList, autoBundleScanAll and
// similaritiy. Statics only.
class hkTreeBundleRayCastUtil {
public:
    // Havok SDK signature (hkpCollide.lib, tied by string).
    static void startWorldRayCastWithAutoBundleJobs(
        const hkVector4* rayFrom,
        const hkVector4* rayTo,
        int32_t numRays,
        float angleVariance,
        float distanceVariance2,
        hkpWorldRayCastJob* worldRayCastJob,
        hkJobQueue* jobQueue,
        hkJobThreadPool* jobThreadPool,
        hkpWorldRayCastOutput* resultBuffer,
        bool scanAllInJobs
    );
};
