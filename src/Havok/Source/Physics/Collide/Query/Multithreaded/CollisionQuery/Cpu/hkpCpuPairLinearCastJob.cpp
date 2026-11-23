#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/Cpu/hkpCpuPairLinearCastJob.h"

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgentConfig.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpFixedBufferCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpPairLinearCastJob.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

// The SDK's body: two fixed buffer collectors over 16-point stack buffers, one for the cast's points and one for the
// start points, run each command's linear cast function; then each buffer's points go to the command's result
// buffers.
void hkCpuPairLinearCastImplementation(
    const hkpPairLinearCastJob& pairLinearCastJob,
    const hkpProcessCollisionInput& collisionInput,
    hkpPairLinearCastCommand* commandsBase,
    int32_t numCommands
) {
    hkpRootCdPoint pointsBuffer[hkpPairLinearCastCommand::MAXIMUM_RESULTS_CAPACITY];
    hkpRootCdPoint startPointsBuffer[hkpPairLinearCastCommand::MAXIMUM_RESULTS_CAPACITY];
    hkpFixedBufferCdPointCollector startPointCollector(
        startPointsBuffer,
        hkpPairLinearCastCommand::MAXIMUM_RESULTS_CAPACITY
    );

    hkpCollisionAgentConfig config;
    config.m_iterativeLinearCastEarlyOutDistance = pairLinearCastJob.m_iterativeLinearCastEarlyOutDistance;
    config.m_iterativeLinearCastMaxIterations = pairLinearCastJob.m_iterativeLinearCastMaxIterations;

    // Only these fields of the input are set; the agents get the rest as the stack holds it.
    hkpLinearCastCollisionInput input;
    input.m_dispatcher = collisionInput.m_dispatcher;
    input.m_weldClosestPoints.m_storage = 0;
    input.m_tolerance = pairLinearCastJob.m_tolerance;
    input.m_filter = collisionInput.m_filter;
    input.m_config = &config;

    hkpPairLinearCastCommand* command = commandsBase;
    for (int32_t remaining = numCommands; remaining > 0; remaining--, command++) {
        const hkpCollidable* collA = command->m_collidableA;
        const hkpShape* shapeA = collA->m_shape;
        const hkpShape* shapeB = command->m_collidableB->m_shape;

        input.m_path.setSub4(command->m_to, command->m_from);
        input.m_cachedPathLength = input.m_path.length3();

        // The collectors for this command.
        hkpFixedBufferCdPointCollector collector(pointsBuffer, command->m_resultsCapacity);
        input.m_tolerance = pairLinearCastJob.m_tolerance;
        input.m_maxExtraPenetration = pairLinearCastJob.m_maxExtraPenetration;
        hkpRootCdPoint* startPointResults = command->m_startPointResults;
        if (startPointResults) {
            startPointCollector.~hkpFixedBufferCdPointCollector();
            ::new (&startPointCollector)
                hkpFixedBufferCdPointCollector(startPointsBuffer, command->m_startPointResultsCapacity);
        }

        hkpCollisionDispatcher* dispatcher = collisionInput.m_dispatcher;
        dispatcher->m_agent2Func[dispatcher->m_agent2Types[shapeA->m_type.m_storage][shapeB->m_type.m_storage]]
            .m_linearCastFunc(
                *collA,
                *command->m_collidableB,
                input,
                collector,
                startPointResults ? &startPointCollector : nullptr
            );

        // Only the low 16 bits of the point counts are used for the counts stored and copied.
        const int32_t numHits = static_cast<uint16_t>(collector.m_numPoints.m_storage);
        command->m_numResultsOut = numHits;
        command->m_startPointNumResultsOut = static_cast<uint16_t>(startPointCollector.m_numPoints.m_storage);
        if (numHits > 0) {
            hkString::memCpy16NonEmpty(
                command->m_results,
                pointsBuffer,
                numHits * static_cast<int32_t>(sizeof(hkpRootCdPoint) / 16)
            );
        }
        if (command->m_startPointResults && command->m_startPointNumResultsOut > 0) {
            hkString::memCpy16NonEmpty(
                command->m_startPointResults,
                startPointsBuffer,
                command->m_startPointNumResultsOut * static_cast<int32_t>(sizeof(hkpRootCdPoint) / 16)
            );
        }
    }
}

// The job's commands run between the timer macros, then the queue hands out the next job.
hkJobQueue::JobStatus hkCpuPairLinearCastJob(hkJobQueue& jobQueue, hkJobQueue::JobQueueEntry& nextJobOut) {
    HK_TIMER_BEGIN("CollQueryPairLinearCast", nullptr);
    // The queue entry holds the job: its hkJob base is the job's.
    const hkpPairLinearCastJob& job = static_cast<const hkpPairLinearCastJob&>(static_cast<const hkJob&>(nextJobOut));
    // The job keeps its commands const; the implementation writes their result counts.
    hkCpuPairLinearCastImplementation(
        job,
        *job.m_collisionInput,
        const_cast<hkpPairLinearCastCommand*>(job.m_commandArray),
        job.m_numCommands
    );
    HK_TIMER_END();
    return jobQueue.finishJobAndGetNextJob(&nextJobOut, nextJobOut, hkJobQueue::WAIT_FOR_NEXT_JOB);
}
