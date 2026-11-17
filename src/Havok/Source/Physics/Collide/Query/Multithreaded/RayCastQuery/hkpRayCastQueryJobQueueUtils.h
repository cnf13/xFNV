#pragma once

#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"

// The job queue callbacks of the ray cast query jobs (shape and world ray casts, k-d tree builds, ray bundles). It has
// no fields.
struct hkpRayCastQueryJobQueueUtils {
    // Does nothing on the PC.
    static void addKdTreeJob(hkJobQueue& queue, hkJobQueue::DynamicData* data, const hkJobQueue::JobQueueEntry& jobIn);
    // The queue's pop callback for the ray cast query jobs.
    static hkJobQueue::JobPopFuncResult popRayCastQueryJob(
        hkJobQueue& queue,
        hkJobQueue::DynamicData* data,
        hkJobQueue::JobQueueEntry& jobIn,
        hkJobQueue::JobQueueEntry& jobOut
    );
    // The queue's finish callback for the ray cast query jobs.
    static hkJobQueue::JobCreationStatus finishRayCastQueryJob(
        hkJobQueue& queue,
        hkJobQueue::DynamicData* data,
        const hkJobQueue::JobQueueEntry& jobIn,
        hkJobQueue::JobQueueEntryInput& newJobCreated
    );
    // Registers the eight ray cast query job functions and the pop and finish callbacks with queue for job type 3.
    static void registerWithJobQueue(hkJobQueue* queue);
};
