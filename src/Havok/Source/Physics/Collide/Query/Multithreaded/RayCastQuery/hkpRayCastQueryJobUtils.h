#pragma once

#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeUtils.h"
#include "Reconstruction.hpp"

class hkKdTree;
struct hkKdTreeProjectedEntry;
class hkSemaphore;
// The SPU build's software cache; the PDB has no definition of it either.
class hkSpu4WayCache;
class hkpCollidable;
class hkpCollisionFilter;
class hkpFixedBufferRayHitCollector;
struct hkpKdTreeBuildSetupJob;
struct hkpWorldRayCastBundleCommand;
struct hkpWorldRayCastCommand;
struct hkpWorldRayCastInput;
struct hkpWorldRayCastOutput;

void sub_E2ABD0();
void sub_E2ABE0();

// The kd-tree jobs' setup helpers: initializeBuildSetupJobs (splits the collidables over the hkpKdTreeBuildSetupJob
// jobs hkCpuKdTreeBuildSetupJob 0xDF8800 runs) and makeBundleFromInputs. Statics only: sizeof 1 in Havok's release
// hkpcollide.pdb; the game PDB has both functions.
class hkKdTreeRayCastJobUtil {
public:
	// Splits numCollidables collidables over numJobs setup jobs (no more jobs than collidables): job 0 gets the
	// semaphore, the collidables, the output entries and the first share, each further job a copy of job 0 with the
	// next share, the last one up to numCollidables (cdecl, ret 0, 0xE2AA70). Havok SDK name and signature; the exe
	// leaves jobsOut in eax, which its one caller, hkCpuKdTreeBuildCoordinatorJob (0xDF8570), does not read.
	static void initializeBuildSetupJobs(const hkpCollidable** collidablesPtr, int32_t numCollidables,
		hkKdTreeProjectedEntry* entries, hkSemaphore* semaphore, int32_t numJobs, hkpKdTreeBuildSetupJob* jobsOut);
	// The four rays' from and to points of inputs, transposed into two bundles, one ray per lane (cdecl, ret 0,
	// 0xE2AB00). The exe leaves fromBundle in eax; no caller reads it.
	static void makeBundleFromInputs(hkpWorldRayCastInput* inputs, hkKdTreeUtils::RayBundle& fromBundle,
		hkKdTreeUtils::RayBundle& toBundle);
};

// The ray cast jobs' tree casts (the SDK's and PDB's hkTreeRayCastJobUtil, hkpRayCastQueryJobUtils.cpp).
class hkTreeRayCastJobUtil {
public:
	// Casts command's ray through kdTree (hkKdTreeUtils::castRayIterative) into a stack
	// hkCpuWorldRayCastCollector with filter; when the command's input asks for a collector (+0x3C), the hits go to
	// fixedBufferHitCollector, built there on the command's results, else to the command's results (cdecl, ret 0,
	// 0xE2ABF0). The three caches and the output array are the SPU build's: the exe reads only the first four
	// arguments, but its one caller, hkCpuWorldRayCastJob (0xDF8DF0), pushes all eight (the last four null) and pops
	// 0x20 bytes.
	static void castRayKdTree(const hkKdTree* kdTree, hkpWorldRayCastCommand* command, const hkpCollisionFilter* filter,
		hkpFixedBufferRayHitCollector* fixedBufferHitCollector, hkSpu4WayCache* collidableCache,
		hkSpu4WayCache* shapeCache, hkSpu4WayCache* transformCache, hkpWorldRayCastOutput* resultLocalArray);
	// Casts command's up to four rays as one bundle through kdTrees (hkKdTreeUtils::castRayBundleIterative) into a
	// stack hkKdTreeCpuBroadphaseBundleCastCollector with filter, on copies of the command's results that start at
	// the fractions of earlyHitFractionInOut, copies the results and hit counts back and leaves each ray's hit
	// fraction in earlyHitFractionInOut (cdecl, ret 0, 0xE2ACF0). The caches, the output array and the count are the
	// SPU build's: the exe reads only the first four arguments, but its one caller, hkCpuWorldRayCastJob
	// (0xDF8DF0), pushes all nine (the last five null) and pops 0x24 bytes.
	static void castRayBundleKdTree(const hkKdTree* kdTree, hkpWorldRayCastBundleCommand* command,
		const hkpCollisionFilter* filter, hkVector4& earlyHitFractionInOut, hkSpu4WayCache* collidableCache,
		hkSpu4WayCache* shapeCache, hkSpu4WayCache* transformCache, hkpWorldRayCastOutput* resultLocalArray,
		int32_t* numResultsInOut);
};
