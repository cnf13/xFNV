#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Util/hkTreeBundleRaycastUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Thread/Pool/hkJobThreadPool.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/Cpu/hkpCpuAutoRayBundleJob.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"

// KEEP (partcopy:): addJob (0xD04B90) copies each job whole (m_size, 0x70 bytes) into the queue, and the exe writes
// neither the job's byte 3 (hkJob's padding03), bytes 8-0xF (hkJob's tail), 0x1C (the base's m_collisionInput), 0x48
// (m_numSingleCommands) nor 0x50 (m_numBundledCommands); the octant-list jobs never write 0x24 (m_octantIdx) either
// (an uninitialised read). With scanAllInJobs, eight jobs scan all rays, one octant each; else the rays are sorted into
// the eight octants on this thread and each non-empty octant's list becomes a job. The arrays' constructors and
// destructors are inlined (their sizes are never stored); the two 8-int arrays are allocated and freed unused. The
// float parameters are x87 copies into the job (0xE2A413, 0xE2A77B).
void hkTreeBundleRayCastUtil::startWorldRayCastWithAutoBundleJobs(const hkVector4* rayFrom, const hkVector4* rayTo,
	int32_t numRays, float angleVariance, float distanceVariance2, hkpWorldRayCastJob* worldRayCastJob,
	hkJobQueue* jobQueue, hkJobThreadPool* jobThreadPool, hkpWorldRayCastOutput* resultBuffer, bool scanAllInJobs) {
	hkpWorldRayCastWithAutoBundleJob job;
	if (scanAllInJobs) {
		HK_TIMER_BEGIN_LIST("AutoJob", "AddJobs");
		// The job holds the world job's const commands as non-const pointers.
		hkpWorldRayCastCommand* singleCommands = const_cast<hkpWorldRayCastCommand*>(worldRayCastJob->m_commandArray);
		hkpWorldRayCastBundleCommand* bundledCommands =
			const_cast<hkpWorldRayCastBundleCommand*>(worldRayCastJob->m_bundleCommandArray);
		for (int32_t octant = 0; octant < 8; ++octant) {
			// EXE-INLINED none: hkpWorldRayCastWithAutoBundleJob::hkpWorldRayCastWithAutoBundleJob
			job.m_threadAffinity = -1;
			job.m_angleVariance = angleVariance;
			job.m_distanceVariance2 = distanceVariance2;
			job.m_size = static_cast<uint16_t>(sizeof(hkpWorldRayCastWithAutoBundleJob));
			job.m_numRays = numRays;
			job.m_rayFrom = rayFrom;
			job.m_rayTo = rayTo;
			job.m_singleCommands = singleCommands;
			job.m_resultBuffer = resultBuffer;
			job.m_bundledCommands = bundledCommands;
			job.m_jobHeader = worldRayCastJob->m_sharedJobHeaderOnPpu;
			job.m_collisionInput = worldRayCastJob->m_collisionInput;
			job.m_semaphore = worldRayCastJob->m_semaphore;
			job.m_aabbTreeFp = worldRayCastJob->m_aabbTreeFp;
			job.m_broadPhase = worldRayCastJob->m_broadphase;
			job.m_aabbTreeInt16 = worldRayCastJob->m_aabbTreeInt16;
			job.m_jobSubType = hkpRayCastQueryJob::RAYCAST_QUERY_WORLD_RAYCAST_WITH_AUTOBUNDLE;
			job.m_jobType.m_storage = HK_JOB_TYPE_RAYCAST_QUERY;
			job.hkpRayCastQueryJob::m_semaphore = nullptr;
			job.m_jobDoneFlag = nullptr;
			job.m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_ENABLED;
			job.m_sharedJobHeaderOnPpu = nullptr;
			job.m_rayIndices = nullptr;
			job.m_octantIdx = octant;
			job.m_numIndices = 0;
			job.m_kdTree = worldRayCastJob->m_kdTree;
			jobQueue->addJob(job, hkJobQueue::JOB_LOW_PRIORITY);
			singleCommands += numRays;
			bundledCommands += numRays;
		}
		jobThreadPool->processAllJobs(jobQueue, HK_JOB_TYPE_MAX);
		HK_TIMER_SPLIT_LIST("WaitThreads");
		jobThreadPool->waitForCompletion();
		HK_TIMER_END_LIST();
		worldRayCastJob->m_semaphore->acquire();
		return;
	}

	HK_TIMER_BEGIN("CPUSplit", nullptr);
	// EXE-INLINED none: hkArray<hkVector4>::hkArray(int)
	FNVD_INLINED_LOCAL(hkArray<hkVector4>, directions, std::nullopt, std::nullopt);
	{
		hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
		if (numRays) {
			int32_t numBytes = numRays * static_cast<int32_t>(sizeof(hkVector4));
			directions.m_data = static_cast<hkVector4*>(heap.bufAlloc(numBytes));
			directions.m_capacityAndFlags = numBytes / static_cast<int32_t>(sizeof(hkVector4));
		} else {
			directions.m_data = nullptr;
			directions.m_capacityAndFlags = 0;
		}
		if (!directions.m_capacityAndFlags) {
			directions.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
		}
		directions.m_size = numRays;
	}
	// EXE-INLINED none: hkArray<hkVector4>::hkArray(int)
	FNVD_INLINED_LOCAL(hkArray<hkVector4>, lengthsSquared, std::nullopt, std::nullopt);
	{
		hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
		if (numRays) {
			int32_t numBytes = numRays * static_cast<int32_t>(sizeof(hkVector4));
			lengthsSquared.m_data = static_cast<hkVector4*>(heap.bufAlloc(numBytes));
			lengthsSquared.m_capacityAndFlags = numBytes / static_cast<int32_t>(sizeof(hkVector4));
		} else {
			lengthsSquared.m_data = nullptr;
			lengthsSquared.m_capacityAndFlags = 0;
		}
		if (!lengthsSquared.m_capacityAndFlags) {
			lengthsSquared.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
		}
		lengthsSquared.m_size = numRays;
	}
	// EXE-INLINED none: hkArray<hkArray<int32_t>>::hkArray(int) (each element's constructor too)
	using octantRays_t = hkArray<hkArray<int32_t>>;
	FNVD_INLINED_LOCAL(octantRays_t, octantRays, std::nullopt, std::nullopt);
	{
		hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
		int32_t numBytes = 8 * static_cast<int32_t>(sizeof(hkArray<int32_t>));
		octantRays.m_data = static_cast<hkArray<int32_t>*>(heap.bufAlloc(numBytes));
		const int32_t count = numBytes / static_cast<int32_t>(sizeof(hkArray<int32_t>));
		octantRays.m_size = count;
		octantRays.m_capacityAndFlags = count;
		if (!count) {
			octantRays.m_capacityAndFlags = hkArrayBase<hkArray<int32_t>>::DONT_DEALLOCATE_FLAG;
		}
		hkArray<int32_t>* element = octantRays.m_data;
		for (int32_t n = count; n > 0; --n, ++element) {
			if (element) {
				element->m_data = nullptr;
				element->m_size = 0;
				element->m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
			}
		}
	}
	alignas(16) int32_t octantCounts[8];
	hkpAutoRayBundleJob_sortRaysByOctant(rayFrom, rayTo, numRays, directions.m_data, lengthsSquared.m_data,
		octantRays.m_data, octantCounts);
	// EXE-INLINED none: hkArray<int32_t>::hkArray(int)
	FNVD_INLINED_LOCAL(hkArray<int32_t>, unusedA, std::nullopt, std::nullopt);
	{
		hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
		int32_t numBytes = 8 * static_cast<int32_t>(sizeof(int32_t));
		unusedA.m_data = static_cast<int32_t*>(heap.bufAlloc(numBytes));
		unusedA.m_capacityAndFlags = numBytes / static_cast<int32_t>(sizeof(int32_t));
		if (!unusedA.m_capacityAndFlags) {
			unusedA.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
		}
		unusedA.m_size = 8;
	}
	// EXE-INLINED none: hkArray<int32_t>::hkArray(int)
	FNVD_INLINED_LOCAL(hkArray<int32_t>, unusedB, std::nullopt, std::nullopt);
	{
		hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
		int32_t numBytes = 8 * static_cast<int32_t>(sizeof(int32_t));
		unusedB.m_data = static_cast<int32_t*>(heap.bufAlloc(numBytes));
		unusedB.m_capacityAndFlags = numBytes / static_cast<int32_t>(sizeof(int32_t));
		if (!unusedB.m_capacityAndFlags) {
			unusedB.m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
		}
		unusedB.m_size = 8;
	}

	// The job holds the world job's const commands as non-const pointers.
	hkpWorldRayCastCommand* singleCommands = const_cast<hkpWorldRayCastCommand*>(worldRayCastJob->m_commandArray);
	hkpWorldRayCastBundleCommand* bundledCommands =
		const_cast<hkpWorldRayCastBundleCommand*>(worldRayCastJob->m_bundleCommandArray);
	hkArray<int32_t>* octant = octantRays.m_data;
	for (int32_t remaining = 8; remaining != 0; --remaining, ++octant) {
		if (!octant->m_size) {
			continue;
		}
		// EXE-INLINED none: hkpWorldRayCastWithAutoBundleJob::hkpWorldRayCastWithAutoBundleJob
		job.m_threadAffinity = -1;
		job.m_angleVariance = angleVariance;
		job.m_distanceVariance2 = distanceVariance2;
		job.m_size = static_cast<uint16_t>(sizeof(hkpWorldRayCastWithAutoBundleJob));
		job.m_rayFrom = rayFrom;
		job.m_rayIndices = octant->m_data;
		job.m_numIndices = octant->m_size;
		job.m_singleCommands = singleCommands;
		job.m_rayTo = rayTo;
		job.m_resultBuffer = resultBuffer;
		job.m_jobHeader = worldRayCastJob->m_sharedJobHeaderOnPpu;
		job.m_bundledCommands = bundledCommands;
		job.m_collisionInput = worldRayCastJob->m_collisionInput;
		job.m_aabbTreeFp = worldRayCastJob->m_aabbTreeFp;
		job.m_semaphore = worldRayCastJob->m_semaphore;
		job.m_broadPhase = worldRayCastJob->m_broadphase;
		job.m_jobSubType = hkpRayCastQueryJob::RAYCAST_QUERY_WORLD_RAYCAST_WITH_AUTOBUNDLE;
		job.m_jobType.m_storage = HK_JOB_TYPE_RAYCAST_QUERY;
		job.hkpRayCastQueryJob::m_semaphore = nullptr;
		job.m_jobDoneFlag = nullptr;
		job.m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_ENABLED;
		job.m_sharedJobHeaderOnPpu = nullptr;
		job.m_numRays = numRays;
		job.m_aabbTreeInt16 = worldRayCastJob->m_aabbTreeInt16;
		job.m_kdTree = worldRayCastJob->m_kdTree;
		jobQueue->addJob(job, hkJobQueue::JOB_LOW_PRIORITY);
		singleCommands += numRays;
		bundledCommands += numRays;
	}
	HK_TIMER_END();

	HK_TIMER_BEGIN("AutoBundleJobs", nullptr);
	jobThreadPool->processAllJobs(jobQueue, HK_JOB_TYPE_MAX);
	jobThreadPool->waitForCompletion();
	worldRayCastJob->m_semaphore->acquire();
	HK_TIMER_END();

	// EXE-INLINED none: hkArray<int32_t>::~hkArray
	{
		hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
		if (unusedB.m_capacityAndFlags >= 0) {
			heap.bufFree(unusedB.m_data,
				(unusedB.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK) *
					static_cast<int32_t>(sizeof(int32_t)));
		}
	}
	// EXE-INLINED none: hkArray<int32_t>::~hkArray
	{
		hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
		if (unusedA.m_capacityAndFlags >= 0) {
			heap.bufFree(unusedA.m_data,
				(unusedA.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK) *
					static_cast<int32_t>(sizeof(int32_t)));
		}
	}
	// EXE-INLINED none: hkArray<hkArray<int32_t>>::~hkArray (each element's destructor too)
	{
		hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
		hkArray<int32_t>* element = octantRays.m_data;
		for (int32_t n = octantRays.m_size; n > 0; --n, ++element) {
			hkMemoryAllocator& elementHeap = *hkMemoryRouter::getInstance().m_heap;
			element->m_size = 0;
			if (element->m_capacityAndFlags >= 0) {
				elementHeap.bufFree(element->m_data,
					(element->m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK) *
						static_cast<int32_t>(sizeof(int32_t)));
			}
			element->m_data = nullptr;
			element->m_capacityAndFlags = hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
		}
		if (octantRays.m_capacityAndFlags >= 0) {
			heap.bufFree(octantRays.m_data,
				(octantRays.m_capacityAndFlags & hkArrayBase<hkArray<int32_t>>::CAPACITY_MASK) *
					static_cast<int32_t>(sizeof(hkArray<int32_t>)));
		}
	}
	// EXE-INLINED none: hkArray<hkVector4>::~hkArray
	{
		hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
		if (lengthsSquared.m_capacityAndFlags >= 0) {
			heap.bufFree(lengthsSquared.m_data,
				(lengthsSquared.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) *
					static_cast<int32_t>(sizeof(hkVector4)));
		}
	}
	// EXE-INLINED none: hkArray<hkVector4>::~hkArray
	{
		hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
		if (directions.m_capacityAndFlags >= 0) {
			heap.bufFree(directions.m_data,
				(directions.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) *
					static_cast<int32_t>(sizeof(hkVector4)));
		}
	}
}
