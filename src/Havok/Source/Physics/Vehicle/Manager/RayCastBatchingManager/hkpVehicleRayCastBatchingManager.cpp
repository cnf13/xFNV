#include "Havok/Source/Physics/Vehicle/Manager/RayCastBatchingManager/hkpVehicleRayCastBatchingManager.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJob.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpCollisionQueryJobs.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/RayCastQuery/hkpRayCastQueryJobs.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Vehicle/Manager/hkpVehicleCastBatchingManager.h"
#include "Havok/Source/Physics/Vehicle/Manager/hkpVehicleManager.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/RayCast/hkpVehicleRayCastWheelCollide.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/hkpVehicleWheelCollide.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"

static void finishLoadedObjecthkpVehicleRayCastBatchingManager(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpVehicleRayCastBatchingManager();
static void cleanupLoadedObjecthkpVehicleRayCastBatchingManager(void* p);

hkClass hkpVehicleRayCastBatchingManagerClass(
    "hkpVehicleRayCastBatchingManager",
    &hkpVehicleCastBatchingManagerClass,
    24,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    0,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpVehicleRayCastBatchingManagerTypeInfo = {
    "hkpVehicleRayCastBatchingManager",
    finishLoadedObjecthkpVehicleRayCastBatchingManager,
    cleanupLoadedObjecthkpVehicleRayCastBatchingManager,
    getVtablehkpVehicleRayCastBatchingManager(),
    &typeid(hkpVehicleRayCastBatchingManager),
    sizeof(hkpVehicleRayCastBatchingManager)
};

// The type info's finish-loaded-object callback for hkpVehicleRayCastBatchingManager: runs its finish constructor.
static void finishLoadedObjecthkpVehicleRayCastBatchingManager(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkpVehicleRayCastBatchingManager(flag);
    }
}

static const void* getVtablehkpVehicleRayCastBatchingManager() {
    return hkGetVtableOf<hkpVehicleRayCastBatchingManager>();
}

static void cleanupLoadedObjecthkpVehicleRayCastBatchingManager(void* p) {
    static_cast<hkpVehicleRayCastBatchingManager*>(p)->~hkpVehicleRayCastBatchingManager();
}

// The buffer's pieces: a command and an output per wheel, one index byte per vehicle and a header per job, each rounded
// up to 16 bytes.
int32_t hkpVehicleRayCastBatchingManager::getBufferSize(int32_t numJobs) {
    const size_t numWheels = static_cast<size_t>(m_totalNumWheels);
    return static_cast<int32_t>(
        ((static_cast<size_t>(m_registeredVehicles.m_size) + 15) & ~size_t(15)) +
        ((sizeof(hkpCollisionQueryJobHeader) * static_cast<size_t>(numJobs) + 15) & ~size_t(15)) +
        ((sizeof(hkpShapeRayCastCommand) * numWheels + 15) & ~size_t(15)) +
        ((sizeof(hkpWorldRayCastOutput) * numWheels + 15) & ~size_t(15))
    );
}

// The SDK's form: the vehicles' wheel collides build their ray cast commands into the buffer's command and output
// storage (each vehicle's command count in its index byte), then the commands are split into at most numJobs shape
// ray cast jobs of nearly equal size, each added to the queue at low priority. Returns the number of jobs added.
// KEEP: partcopy. Of the 0x30-byte shape ray cast job the exe writes bytes 0x0-0x2, 0x4-0x7 and 0x10-0x2B only;
// hkJobQueue::addJob (0xD04B90) copies all m_size bytes into the queue, so the job's bytes 0x3, 0x8-0xF and
// 0x2C-0x2F reach it unwritten.
int32_t hkpVehicleRayCastBatchingManager::buildAndAddCastJobs(
    const hkpWorld* world,
    int32_t filterSize,
    int32_t numJobs,
    hkJobQueue* jobQueue,
    hkSemaphore* semaphore,
    void* buffer
) {
    HK_TIMER_BEGIN("BuildAndAddRaycastJobs", nullptr);
    const int32_t numVehicles = m_registeredVehicles.m_size;
    const hkpCollisionFilter* filter = world->m_collisionFilter;
    alignas(16) RaycastBatch raycastBatch;
    getRaycastBatchFromBuffer(buffer, numJobs, raycastBatch);

    HK_TIMER_BEGIN("BuildCommands", nullptr);
    int32_t numCommands = 0;
    {
        hkpShapeRayCastCommand* commandStorage = raycastBatch.m_commandStorage;
        hkpWorldRayCastOutput* outputStorage = raycastBatch.m_outputStorage;
        for (int32_t i = 0; i < numVehicles; ++i) {
            hkpVehicleInstance* const vehicle = m_registeredVehicles.m_data[i];
            hkpVehicleRayCastWheelCollide* wheelCollide =
                static_cast<hkpVehicleRayCastWheelCollide*>(vehicle->m_wheelCollide);
            const int32_t numCommandsAdded =
                wheelCollide->buildRayCastCommands(vehicle, filter, filterSize, commandStorage, outputStorage);
            if (numCommandsAdded > 0) {
                numCommands += numCommandsAdded;
                raycastBatch.m_index[i] = static_cast<uint8_t>(numCommandsAdded);
                commandStorage += numCommandsAdded;
                outputStorage += numCommandsAdded;
            }
            else {
                raycastBatch.m_index[i] = 0;
            }
        }
    }
    HK_TIMER_END();

    // EXE-INLINED none: hkMath::min2<int>
    numJobs = (numJobs < numCommands) ? numJobs : numCommands;
    const int32_t minCommandsPerJob = numCommands / numJobs;
    const int32_t extra = numCommands % numJobs;
    {
        const hkpShapeRayCastCommand* commandStorage = raycastBatch.m_commandStorage;
        for (int32_t i = 0; i < numJobs; ++i) {
            const int32_t numCommandsInJob = minCommandsPerJob + (i < extra ? 1 : 0);
            hkpCollisionQueryJobHeader* jobHeader = &raycastBatch.m_jobHeaders[i];
            // EXE-INLINED none: hkpShapeRayCastJob::hkpShapeRayCastJob
            hkpShapeRayCastJob job;
            job.m_jobSubType = hkpRayCastQueryJob::RAYCAST_QUERY_SHAPE_RAYCAST;
            job.m_jobType = HK_JOB_TYPE_RAYCAST_QUERY;
            job.m_size = sizeof(hkpShapeRayCastJob);
            job.m_threadAffinity = -1;
            job.m_semaphore = semaphore;
            job.m_sharedJobHeaderOnPpu = jobHeader;
            job.m_jobDoneFlag = nullptr;
            job.m_collisionInput = world->m_collisionInput;
            job.m_numCommandsPerTask = hkpShapeRayCastJob::MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK;
            job.m_commandArray = commandStorage;
            job.m_numCommands = numCommandsInJob;
            job.m_jobSpuType = HK_JOB_SPU_TYPE_ENABLED;
            jobHeader->m_openJobs =
                (numCommandsInJob - 1) / hkpShapeRayCastJob::MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK + 1;
            job.setRunsOnSpuOrPpu();
            jobQueue->addJob(job, hkJobQueue::JOB_LOW_PRIORITY);
            commandStorage += numCommandsInJob;
        }
    }
    HK_TIMER_END();
    return numJobs;
}

// Each vehicle's wheels take their collision output from their ray cast's result where the vehicle cast rays (its index
// byte) and the command hit something, else the no-hit output; then the vehicle is stepped. The outputs go into a local
// array of 16 from the thread's stack allocator. The wheel count is read as an unsigned byte and the wheel index is
// kept as a signed byte.
void hkpVehicleRayCastBatchingManager::stepVehiclesUsingCastResults(
    const hkStepInfo& stepInfo,
    int32_t numJobs,
    void* buffer
) {
    HK_TIMER_BEGIN("DoVehicles", nullptr);
    const int32_t numVehicles = m_registeredVehicles.m_size;
    RaycastBatch raycastBatch;
    getRaycastBatchFromBuffer(buffer, numJobs, raycastBatch);
    const hkpShapeRayCastCommand* command = raycastBatch.m_commandStorage;

    hkLocalArray<hkpVehicleWheelCollide::CollisionDetectionWheelOutput> cdInfo(16);

    for (int32_t i = 0; i < numVehicles; ++i) {
        hkpVehicleInstance* vehicle = m_registeredVehicles.m_data[i];
        const int32_t numWheels = static_cast<uint8_t>(vehicle->m_data->m_numWheels);
        // EXE-INLINED none: hkArray<hkpVehicleWheelCollide::CollisionDetectionWheelOutput>::setSize
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        const int32_t capacity = static_cast<int32_t>(
            cdInfo.m_capacityAndFlags &
            hkArrayBase<hkpVehicleWheelCollide::CollisionDetectionWheelOutput>::CAPACITY_MASK
        );
        if (capacity < numWheels) {
            int32_t request = 2 * capacity;
            if (numWheels >= request) {
                request = numWheels;
            }
            hkArrayUtil::_reserve(
                pHeap, &cdInfo, request, sizeof(hkpVehicleWheelCollide::CollisionDetectionWheelOutput)
            );
        }
        cdInfo.m_size = static_cast<uint32_t>(numWheels);

        // Reads the wheel count again after setSize.
        const int32_t numWheelsToCollide = static_cast<uint8_t>(vehicle->m_data->m_numWheels);
        hkpVehicleRayCastWheelCollide* wheelCollide =
            static_cast<hkpVehicleRayCastWheelCollide*>(vehicle->m_wheelCollide);
        for (int8_t j = 0; j < numWheelsToCollide; ++j) {
            const int32_t hasCommand = raycastBatch.m_index[i];
            if (hasCommand && command->m_numResultsOut) {
                wheelCollide->getCollisionOutputFromCastResult(
                    vehicle, static_cast<uint8_t>(j), *command->m_results, cdInfo.m_data[j]
                );
            }
            else {
                wheelCollide->getCollisionOutputWithoutHit(vehicle, static_cast<uint8_t>(j), cdInfo.m_data[j]);
            }
            wheelCollide->wheelCollideCallback(vehicle, static_cast<uint8_t>(j), cdInfo.m_data[j]);
            if (hasCommand) {
                ++command;
            }
        }
        m_registeredVehicles.m_data[i]->stepVehicleUsingWheelCollideOutput(stepInfo, cdInfo.m_data);
    }
    HK_TIMER_END();
}

// The buffer's pieces in getBufferSize's order: the commands and the outputs per wheel, then one index byte per
// vehicle, then the job headers, each 16-byte aligned.
void hkpVehicleRayCastBatchingManager::getRaycastBatchFromBuffer(
    void* raycastBuffer,
    int32_t numJobs,
    RaycastBatch& raycastBatch
) {
    const size_t numWheels = static_cast<size_t>(m_totalNumWheels);
    const size_t numVehicles = static_cast<size_t>(m_registeredVehicles.m_size);
    uint8_t* pBase = static_cast<uint8_t*>(raycastBuffer);
    uint8_t* v6 = pBase + ((sizeof(hkpShapeRayCastCommand) * numWheels + 15) & ~size_t(15));
    raycastBatch.m_outputStorage = reinterpret_cast<hkpWorldRayCastOutput*>(v6);
    uint8_t* result = v6 + ((sizeof(hkpWorldRayCastOutput) * numWheels + 15) & ~size_t(15));
    raycastBatch.m_commandStorage = reinterpret_cast<hkpShapeRayCastCommand*>(pBase);
    raycastBatch.m_jobHeaders =
        reinterpret_cast<hkpCollisionQueryJobHeader*>(result + ((numVehicles + 15) & ~size_t(15)));
    raycastBatch.m_index = result;
}

const hkClass& hkpVehicleRayCastBatchingManager::staticClass() {
    return hkpVehicleRayCastBatchingManagerClass;
}

void hkpShapeRayCastJob::setRunsOnSpuOrPpu() {}
