#pragma once

#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkJobQueue;
class hkJobThreadPool;

// The automatic ray bundling of the world ray cast jobs: splitToOctant, autoBundleFromList, autoBundleScanAll and
// similaritiy. The exe's copies are the functions src names hkpAutoRayBundleJob_sortRaysByOctant (0xE29B80),
// _bundleIndexedRays (0xE29CD0), _bundleOctantRays (0xE2A010) and _canBundleRays (0xE29920): their parameters are
// these members' (hkpCpuAutoRayBundleJob.h). Statics only: sizeof 1 in Havok's release hkpcollide.pdb.
class hkTreeBundleRayCastUtil {
public:
	// Havok SDK signature (hkpCollide.lib, tied by string).
	static void startWorldRayCastWithAutoBundleJobs(const hkVector4* rayFrom, const hkVector4* rayTo, int32_t numRays,
		float angleVariance, float distanceVariance2, hkpWorldRayCastJob* worldRayCastJob, hkJobQueue* jobQueue,
		hkJobThreadPool* jobThreadPool, hkpWorldRayCastOutput* resultBuffer, bool scanAllInJobs);
};
