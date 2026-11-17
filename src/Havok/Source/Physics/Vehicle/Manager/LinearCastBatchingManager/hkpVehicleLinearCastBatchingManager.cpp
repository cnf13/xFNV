#include "Havok/Source/Physics/Vehicle/Manager/LinearCastBatchingManager/hkpVehicleLinearCastBatchingManager.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Query/Multithreaded/CollisionQuery/hkpPairLinearCastJob.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Vehicle/Manager/hkpVehicleCastBatchingManager.h"
#include "Havok/Source/Physics/Vehicle/Manager/hkpVehicleManager.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/LinearCast/hkpVehicleLinearCastWheelCollide.h"
#include "Havok/Source/Physics/Vehicle/WheelCollide/hkpVehicleWheelCollide.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"

static void finishLoadedObjecthkpVehicleLinearCastBatchingManager(void* p, int32_t finishing);
// Havok SDK signature (hkpVehicle.lib, tied by reloc,target).
static const void* getVtablehkpVehicleLinearCastBatchingManager();
static void cleanupLoadedObjecthkpVehicleLinearCastBatchingManager(void* p);

hkClass hkpVehicleLinearCastBatchingManagerClass(
    "hkpVehicleLinearCastBatchingManager",
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
hkTypeInfo hkpVehicleLinearCastBatchingManagerTypeInfo = {
    "hkpVehicleLinearCastBatchingManager",
    finishLoadedObjecthkpVehicleLinearCastBatchingManager,
    cleanupLoadedObjecthkpVehicleLinearCastBatchingManager,
    getVtablehkpVehicleLinearCastBatchingManager(),
    &typeid(hkpVehicleLinearCastBatchingManager),
    sizeof(hkpVehicleLinearCastBatchingManager)
};

static void finishLoadedObjecthkpVehicleLinearCastBatchingManager(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpVehicleLinearCastBatchingManager(kFlag);
    }
}

static const void* getVtablehkpVehicleLinearCastBatchingManager() {
    return hkGetVtableOf<hkpVehicleLinearCastBatchingManager>();
}

static void cleanupLoadedObjecthkpVehicleLinearCastBatchingManager(void* p) {
    static_cast<hkpVehicleLinearCastBatchingManager*>(p)->~hkpVehicleLinearCastBatchingManager();
}

// The buffer's pieces: a collidable per wheel, a command and an output per command and a header per job, each rounded
// up to 16 bytes.
int32_t hkpVehicleLinearCastBatchingManager::getBufferSize(int32_t numJobs) {
    const size_t numCommands = static_cast<size_t>(getTotalNumCommands());
    return static_cast<int32_t>(
        ((sizeof(hkpCollisionQueryJobHeader) * static_cast<size_t>(numJobs) + 15) & ~size_t(15)) +
        ((sizeof(hkpPairLinearCastCommand) * numCommands + 15) & ~size_t(15)) +
        ((sizeof(hkpRootCdPoint) * numCommands + 15) & ~size_t(15)) +
        ((sizeof(hkpCollidable) * static_cast<size_t>(m_totalNumWheels) + 15) & ~size_t(15))
    );
}

// Builds every registered vehicle's linear-cast commands into the buffer, then splits them into at most numJobs pair
// linear-cast jobs (no more jobs than commands, the remainder spread one each over the first jobs) and queues them
// at low priority. Returns the number of jobs queued. filterSize is not read.
// KEEP: partcopy. Of the 0x40-byte hkpPairLinearCastJob the exe writes every field but leaves byte 0x3, bytes
// 0x8-0xF (hkJob's alignment padding) and bytes 0x3C-0x3F (the job's tail padding) unwritten; hkJobQueue::addJob
// (0xD04B90) copies all m_size bytes into the queue, so they reach it unwritten.
int32_t hkpVehicleLinearCastBatchingManager::buildAndAddCastJobs(
    const hkpWorld* world,
    int32_t filterSize,
    int32_t numJobs,
    hkJobQueue* jobQueue,
    hkSemaphore* semaphore,
    void* buffer
) {
    HK_TIMER_BEGIN("BuildAndAddLinearCastJobs", nullptr);
    const hkpCollisionFilter* filter = world->m_collisionFilter;
    const int32_t numVehicles = m_registeredVehicles.m_size;
    alignas(16) LinearCastBatch batch;
    getLinearCastBatchFromBuffer(buffer, numJobs, batch);
    HK_TIMER_BEGIN("BuildCommands", nullptr);
    int32_t totalNumCommands = 0;
    {
        hkpCollidable* collidableStorage = batch.m_collidableStorage;
        hkpPairLinearCastCommand* commandStorage = batch.m_commandStorage;
        hkpRootCdPoint* outputStorage = batch.m_outputStorage;
        for (int32_t i = 0; i < numVehicles; ++i) {
            hkpVehicleInstance* vehicle = m_registeredVehicles.m_data[i];
            const hkpVehicleLinearCastWheelCollide* wheelCollide =
                static_cast<const hkpVehicleLinearCastWheelCollide*>(vehicle->m_wheelCollide);
            const int32_t numCommands = wheelCollide->buildLinearCastCommands(
                vehicle, filter, collidableStorage, commandStorage, outputStorage
            );
            totalNumCommands += numCommands;
            commandStorage += numCommands;
            outputStorage += numCommands;
            // Zero-extends m_numWheels here.
            collidableStorage += static_cast<uint8_t>(vehicle->m_data->m_numWheels);
        }
    }
    HK_TIMER_END();

    if (numJobs >= totalNumCommands) {
        numJobs = totalNumCommands;
    }
    const int32_t numCommandsPerJob = totalNumCommands / numJobs;
    const int32_t numJobsWithExtraCommand = totalNumCommands % numJobs;
    hkpPairLinearCastCommand* commandStorage = batch.m_commandStorage;
    hkpCollisionQueryJobHeader* jobHeader = batch.m_jobHeaders;
    for (int32_t i = 0; i < numJobs; ++i) {
        const int32_t numCommands = numCommandsPerJob + (i < numJobsWithExtraCommand ? 1 : 0);
        // This fills the job as its constructor does, sets the shared job header's open job count, one per task of up
        // to MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK commands, and calls setRunsOnSpuOrPpu.
        // EXE-INLINED none: hkpPairLinearCastJob::hkpPairLinearCastJob
        hkpPairLinearCastJob job;
        job.m_jobSubType = hkpCollisionQueryJob::COLLISION_QUERY_PAIR_LINEAR_CAST;
        // EXE-INLINED none: hkEnum<E, S>::operator=
        job.m_jobType.m_storage = HK_JOB_TYPE_COLLISION_QUERY;
        job.m_jobSpuType.m_storage = HK_JOB_SPU_TYPE_DISABLED;
        job.m_size = sizeof(hkpPairLinearCastJob);
        job.m_threadAffinity = -1;
        job.m_semaphore = semaphore;
        job.m_sharedJobHeaderOnPpu = jobHeader;
        job.m_collisionInput = world->m_collisionInput;
        job.m_jobDoneFlag = nullptr;
        job.m_tolerance = 0.0f;
        job.m_maxExtraPenetration = 1.1920929e-07f;
        job.m_iterativeLinearCastEarlyOutDistance = 0.01f;
        job.m_iterativeLinearCastMaxIterations = 10;
        job.m_numCommandsPerTask = hkpPairLinearCastJob::MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK;
        job.m_commandArray = commandStorage;
        job.m_numCommands = numCommands;
        jobHeader->m_openJobs = (numCommands - 1) / hkpPairLinearCastJob::MAXIMUM_NUMBER_OF_COMMANDS_PER_TASK + 1;
        job.setRunsOnSpuOrPpu();
        jobQueue->addJob(job, hkJobQueue::JOB_LOW_PRIORITY);
        commandStorage += numCommands;
        ++jobHeader;
    }
    HK_TIMER_END();
    return numJobs;
}

// Each vehicle's wheels take their collision output from the nearest hit of their linear casts, else the no-hit output,
// and step the command pointer past their commands; then the vehicle is stepped. The outputs go into a local array of
// 16 from the thread's stack allocator.
void hkpVehicleLinearCastBatchingManager::stepVehiclesUsingCastResults(
    const hkStepInfo& stepInfo,
    int32_t numJobs,
    void* buffer
) {
    HK_TIMER_BEGIN("DoVehicles", nullptr);
    const int32_t numVehicles = m_registeredVehicles.m_size;
    LinearCastBatch linearCastBatch;
    getLinearCastBatchFromBuffer(buffer, numJobs, linearCastBatch);
    const hkpPairLinearCastCommand* command = linearCastBatch.m_commandStorage;

    FNVD_INLINED_LOCAL(
        hkLocalArray<hkpVehicleWheelCollide::CollisionDetectionWheelOutput>, cdInfo, std::nullopt, std::nullopt
    );
    cdInfo.m_data = nullptr;
    cdInfo.m_size = 0;
    cdInfo.m_capacityAndFlags =
        hkArrayBase<hkpVehicleWheelCollide::CollisionDetectionWheelOutput>::DONT_DEALLOCATE_FLAG;
    // EXE-INLINED none: hkAllocateStack<hkpVehicleWheelCollide::CollisionDetectionWheelOutput>
    hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
    void* pBlock = stack.m_next.m_storage;
    stack.m_next.m_storage =
        static_cast<char*>(pBlock) + 16 * sizeof(hkpVehicleWheelCollide::CollisionDetectionWheelOutput);
    hkpVehicleWheelCollide::CollisionDetectionWheelOutput* pLocal =
        static_cast<hkpVehicleWheelCollide::CollisionDetectionWheelOutput*>(pBlock);
    cdInfo.m_data = pLocal;
    cdInfo.m_capacityAndFlags =
        16u | hkArrayBase<hkpVehicleWheelCollide::CollisionDetectionWheelOutput>::DONT_DEALLOCATE_FLAG;
    cdInfo.m_localMemory = pLocal;

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

        hkpVehicleLinearCastWheelCollide* wheelCollide =
            static_cast<hkpVehicleLinearCastWheelCollide*>(vehicle->m_wheelCollide);
        for (uint8_t j = 0; j < numWheels; ++j) {
            const hkpRootCdPoint* nearestHit = wheelCollide->determineNearestHit(j, command);
            if (nearestHit) {
                wheelCollide->getCollisionOutputFromCastResult(vehicle, j, *nearestHit, cdInfo.m_data[j]);
            }
            else {
                wheelCollide->getCollisionOutputWithoutHit(vehicle, j, cdInfo.m_data[j]);
            }
            wheelCollide->wheelCollideCallback(vehicle, j, cdInfo.m_data[j]);
            command += wheelCollide->getNumCommands(j);
        }
        m_registeredVehicles.m_data[i]->stepVehicleUsingWheelCollideOutput(stepInfo, cdInfo.m_data);
    }
    HK_TIMER_END();

    cdInfo.~hkLocalArray<hkpVehicleWheelCollide::CollisionDetectionWheelOutput>();
}

// The sum of the linear-cast commands of every registered vehicle's wheel collide (all of them are
// hkpVehicleLinearCastWheelCollide).
int32_t hkpVehicleLinearCastBatchingManager::getTotalNumCommands() const {
    int32_t totalNumCommands = 0;
    for (int32_t i = 0; i < m_registeredVehicles.m_size; ++i) {
        const hkpVehicleLinearCastWheelCollide* wheelCollide =
            static_cast<const hkpVehicleLinearCastWheelCollide*>(m_registeredVehicles.m_data[i]->m_wheelCollide);
        totalNumCommands += wheelCollide->getTotalNumCommands();
    }
    return totalNumCommands;
}

// The buffer's pieces in getBufferSize's order: the collidables per wheel, the commands and the outputs per command,
// then the job headers, each 16-byte aligned.
void hkpVehicleLinearCastBatchingManager::getLinearCastBatchFromBuffer(
    void* buffer,
    int32_t numJobs,
    LinearCastBatch& batchOut
) {
    const size_t numCommands = static_cast<size_t>(getTotalNumCommands());
    const size_t numWheels = static_cast<size_t>(m_totalNumWheels);
    uint8_t* pBase = static_cast<uint8_t*>(buffer);
    batchOut.m_collidableStorage = reinterpret_cast<hkpCollidable*>(pBase);
    uint8_t* v7 = pBase + ((sizeof(hkpCollidable) * numWheels + 15) & ~size_t(15));
    batchOut.m_commandStorage = reinterpret_cast<hkpPairLinearCastCommand*>(v7);
    uint8_t* v8 = v7 + ((sizeof(hkpPairLinearCastCommand) * numCommands + 15) & ~size_t(15));
    uint8_t* result = v8 + ((sizeof(hkpRootCdPoint) * numCommands + 15) & ~size_t(15));
    batchOut.m_outputStorage = reinterpret_cast<hkpRootCdPoint*>(v8);
    batchOut.m_jobHeaders = reinterpret_cast<hkpCollisionQueryJobHeader*>(result);
}

const hkClass& hkpVehicleLinearCastBatchingManager::staticClass() {
    return hkpVehicleLinearCastBatchingManagerClass;
}

void hkpPairLinearCastJob::setRunsOnSpuOrPpu() {}
