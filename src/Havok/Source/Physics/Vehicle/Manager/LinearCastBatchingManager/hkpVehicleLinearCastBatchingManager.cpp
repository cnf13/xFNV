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
        ((sizeof(hkpCollisionQueryJobHeader) * static_cast<size_t>(numJobs) + 15) & ~static_cast<size_t>(15)) +
        ((sizeof(hkpPairLinearCastCommand) * numCommands + 15) & ~static_cast<size_t>(15)) +
        ((sizeof(hkpRootCdPoint) * numCommands + 15) & ~static_cast<size_t>(15)) +
        ((sizeof(hkpCollidable) * static_cast<size_t>(m_totalNumWheels) + 15) & ~static_cast<size_t>(15))
    );
}

// Builds every registered vehicle's linear-cast commands into the buffer, then splits them into at most numJobs pair
// linear-cast jobs (no more jobs than commands, the remainder spread one each over the first jobs) and queues them
// at low priority. Returns the number of jobs queued. filterSize is not read.
// KEEP: partcopy. Every field of the 64-byte hkpPairLinearCastJob is written, but hkJob's padding03, hkJob's
// alignment padding and the job's four tail padding bytes are not; hkJobQueue::addJob copies all m_size bytes into
// the queue, so they reach it unwritten.
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
            const hkpVehicleLinearCastWheelCollide* wheelCollide = static_cast<const hkpVehicleLinearCastWheelCollide*>(
                vehicle->m_wheelCollide
            );
            const int32_t numCommands = wheelCollide->buildLinearCastCommands(
                vehicle,
                filter,
                collidableStorage,
                commandStorage,
                outputStorage
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
        hkpPairLinearCastJob
            job(world->m_collisionInput, jobHeader, commandStorage, numCommands, nullptr, 0.0f, semaphore);
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

    hkLocalArray<hkpVehicleWheelCollide::CollisionDetectionWheelOutput> cdInfo(16);

    for (int32_t i = 0; i < numVehicles; ++i) {
        hkpVehicleInstance* vehicle = m_registeredVehicles.m_data[i];
        const int32_t numWheels = static_cast<uint8_t>(vehicle->m_data->m_numWheels);
        cdInfo.setSize(numWheels);

        hkpVehicleLinearCastWheelCollide* wheelCollide = static_cast<hkpVehicleLinearCastWheelCollide*>(
            vehicle->m_wheelCollide
        );
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
}

// The sum of the linear-cast commands of every registered vehicle's wheel collide (all of them are
// hkpVehicleLinearCastWheelCollide).
int32_t hkpVehicleLinearCastBatchingManager::getTotalNumCommands() const {
    int32_t totalNumCommands = 0;
    for (const hkpVehicleInstance* vehicle : m_registeredVehicles) {
        const hkpVehicleLinearCastWheelCollide* wheelCollide = static_cast<const hkpVehicleLinearCastWheelCollide*>(
            vehicle->m_wheelCollide
        );
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
    uint8_t* v7 = pBase + ((sizeof(hkpCollidable) * numWheels + 15) & ~static_cast<size_t>(15));
    batchOut.m_commandStorage = reinterpret_cast<hkpPairLinearCastCommand*>(v7);
    uint8_t* v8 = v7 + ((sizeof(hkpPairLinearCastCommand) * numCommands + 15) & ~static_cast<size_t>(15));
    uint8_t* result = v8 + ((sizeof(hkpRootCdPoint) * numCommands + 15) & ~static_cast<size_t>(15));
    batchOut.m_outputStorage = reinterpret_cast<hkpRootCdPoint*>(v8);
    batchOut.m_jobHeaders = reinterpret_cast<hkpCollisionQueryJobHeader*>(result);
}

const hkClass& hkpVehicleLinearCastBatchingManager::staticClass() {
    return hkpVehicleLinearCastBatchingManagerClass;
}

void hkpPairLinearCastJob::setRunsOnSpuOrPpu() {}
