#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/Cpu/hkpCpuPairGetClosestPointsJob.h"

#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpFixedBufferCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpPairGetClosestPointsJob.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "SDK/Windows/winbase.h"

// The SDK's body: one fixed buffer collector over a 16-point stack buffer runs each command's get-closest-points
// function, then the points go to the command's result buffer, or to its slice of the shared buffer the command's
// counter hands out (given back when the slice would overrun the capacity).
void hkCpuPairGetClosestPointsImplementation(
    const hkpPairGetClosestPointsJob& pairGetClosestPointsJob,
    const hkpProcessCollisionInput& collisionInput,
    hkpPairGetClosestPointsCommand* commandsBase,
    int32_t numCommands
) {
    hkpRootCdPoint pointsBuffer[hkpPairGetClosestPointsCommand::MAXIMUM_RESULTS_CAPACITY];

    // Only these three fields of the input are set; the agents get the rest as the stack holds it.
    hkpCollisionInput input;
    input.m_tolerance = pairGetClosestPointsJob.m_tolerance;
    input.m_dispatcher = collisionInput.m_dispatcher;
    input.m_filter = collisionInput.m_filter;

    hkpPairGetClosestPointsCommand* command = commandsBase;
    for (int32_t remaining = numCommands; remaining > 0; remaining--, command++) {
        const hkpCollidable* collA = command->m_collidableA;
        const hkpShape* shapeA = collA->m_shape;
        const hkpCollidable* collB = command->m_collidableB;
        const hkpShape* shapeB = collB->m_shape;

        hkpFixedBufferCdPointCollector collector(pointsBuffer, command->m_resultsCapacity);
        hkpCollisionDispatcher* dispatcher = collisionInput.m_dispatcher;

        dispatcher->m_agent2Func[dispatcher->m_agent2Types[shapeA->m_type.m_storage][shapeB->m_type.m_storage]]
            .m_getClosestPointFunc(*collA, *collB, input, collector);

        hkpRootCdPoint* dest;
        if (command->m_indexIntoSharedResults) {
            const int32_t numPoints = collector.m_numPoints.m_storage;
            const int32_t index = static_cast<int32_t>(
                hkCriticalSection::atomicExchangeAdd(command->m_indexIntoSharedResults, numPoints)
            );
            if (index + numPoints > command->m_resultsCapacity) {
                hkCriticalSection::atomicExchangeAdd(command->m_indexIntoSharedResults, -numPoints);
                continue;
            }
            dest = command->m_results + index;
        }
        else {
            dest = command->m_results;
        }
        if (dest) {
            // Only the point count's low 16 bits are used for the count that is stored and copied.
            const int32_t numHits = static_cast<uint16_t>(collector.m_numPoints.m_storage);
            command->m_numResultsOut = numHits;
            if (numHits > 0) {
                hkString::memCpy16NonEmpty(
                    dest,
                    pointsBuffer,
                    numHits * static_cast<int32_t>(sizeof(hkpRootCdPoint) / 16)
                );
            }
        }
    }
}

// The job's commands run between the timer macros, then the queue hands out the next job.
hkJobQueue::JobStatus hkCpuPairGetClosestPointsJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    HK_TIMER_BEGIN("CollQueryPairGetClosestPoints", nullptr);
    // The queue entry holds the job: its hkJob base is the job's.
    const hkpPairGetClosestPointsJob& job = static_cast<const hkpPairGetClosestPointsJob&>(
        static_cast<const hkJob&>(nextJobOut)
    );
    // The job keeps its commands const; the implementation writes their result counts.
    hkCpuPairGetClosestPointsImplementation(
        job,
        *job.m_collisionInput,
        const_cast<hkpPairGetClosestPointsCommand*>(job.m_commandArray),
        job.m_numCommands
    );
    HK_TIMER_END();
    return jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}
