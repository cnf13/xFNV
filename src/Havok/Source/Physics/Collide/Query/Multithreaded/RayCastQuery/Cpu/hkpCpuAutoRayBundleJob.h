#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"

struct hkpWorldRayCastBundleCommand;
struct hkpWorldRayCastCommand;

// Up to four rays the job casts as one bundle: their indices and how
// many there are (the 20-byte element of the job's local array).
struct hkpAutoRayBundle {
	int32_t m_rayIndices[4];
	int32_t m_numRays;
};

// Writes each ray's unit direction and squared length, then sorts the ray indices into eight arrays by the signs of the
// direction's x, y and z, counting each.
void hkpAutoRayBundleJob_sortRaysByOctant(const hkVector4* from, const hkVector4* to, int32_t numRays,
	hkVector4* directionsOut, hkVector4* lengthsSquaredOut, hkArray<int32_t>* octantRaysOut, int32_t* octantCountsOut);
// Groups the listed rays into bundles of up to four close rays of similar direction, then writes the commands.
void hkpAutoRayBundleJob_bundleIndexedRays(const int32_t* rayIndices, int32_t numIndices, const hkVector4* from,
	const hkVector4* to, int32_t numRays, float maxAngle, float maxDistanceFactor, hkpWorldRayCastOutput* outputs,
	hkpWorldRayCastCommand* commandsOut, int32_t* numCommandsOut, hkpWorldRayCastBundleCommand* bundleCommandsOut,
	int32_t* numBundleCommandsOut);
// The same grouping for the rays whose direction lies in octant.
void hkpAutoRayBundleJob_bundleOctantRays(int32_t octant, const hkVector4* from, const hkVector4* to, int32_t numRays,
	float maxAngle, float maxDistanceFactor, hkpWorldRayCastOutput* outputs, hkpWorldRayCastCommand* commandsOut,
	int32_t* numCommandsOut, hkpWorldRayCastBundleCommand* bundleCommandsOut, int32_t* numBundleCommandsOut);
// Whether the second ray may join the first's bundle: the directions' dot product is at least minCos and both the start
// points and the end points are within maxDistanceSquared of each other.
bool hkpAutoRayBundleJob_canBundleRays(const hkVector4& from0, const hkVector4& to0, const hkVector4& direction0,
	const hkVector4& from1, const hkVector4& to1, const hkVector4& direction1, float minCos, float maxDistanceSquared);
// Writes a single-ray command for each bundle of
// one ray and a bundle command for each larger one, and their counts; returns the single-ray count, or 0 when there
// are no bundles.
int32_t hkpAutoRayBundleJob_writeCommands(const hkArray<hkpAutoRayBundle>& bundles, const hkVector4* from,
	const hkVector4* to, hkpWorldRayCastOutput* outputs, hkpWorldRayCastCommand* commandsOut, int32_t* numCommandsOut,
	hkpWorldRayCastBundleCommand* bundleCommandsOut, int32_t* numBundleCommandsOut);
// The SDK's job function: bundles the job's rays into single-ray and bundle commands.
hkJobQueue::JobStatus hkCpuAutoRayBundleJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut);
int64_t time64(int64_t* apTime);
int32_t getCurrentTime();
