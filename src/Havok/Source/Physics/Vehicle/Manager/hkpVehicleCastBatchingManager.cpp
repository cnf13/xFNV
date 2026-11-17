#include "Havok/Source/Physics/Vehicle/Manager/hkpVehicleCastBatchingManager.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Common/Base/Thread/Pool/hkJobThreadPool.h"
#include "Havok/Source/Common/Base/Thread/Semaphore/hkSemaphore.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleData.h"
#include "Havok/Source/Physics/Vehicle/hkpVehicleInstance.h"

// The reflected members of hkpVehicleCastBatchingManager.
using hkpVehicleCastBatchingManagerClass_MembersArray = const hkClassMember[1];
static hkpVehicleCastBatchingManagerClass_MembersArray hkpVehicleCastBatchingManagerClass_Members{
    {"totalNumWheels",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT16},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x14,
     nullptr}
};

hkClass hkpVehicleCastBatchingManagerClass(
    "hkpVehicleCastBatchingManager",
    &hkpVehicleManagerClass,
    24,
    nullptr,
    0,
    nullptr,
    0,
    hkpVehicleCastBatchingManagerClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);

// The signed m_numWheels is deliberately zero-extended here, as the game does, while the brake reads the same field
// signed.
void hkpVehicleCastBatchingManager::addVehicle(hkpVehicleInstance* vehicle) {
    hkpVehicleManager::addVehicle(vehicle);
    m_totalNumWheels += static_cast<uint8_t>(vehicle->m_data->m_numWheels);
}

// m_numWheels is zero-extended here too.
void hkpVehicleCastBatchingManager::removeVehicle(hkpVehicleInstance* vehicle) {
    hkpVehicleManager::removeVehicle(vehicle);
    m_totalNumWheels -= static_cast<uint8_t>(vehicle->m_data->m_numWheels);
}

// The vehicles are prepared, their cast jobs built into a buffer taken from the thread's stack allocator and run on the
// thread pool and on this thread, then the vehicles are stepped from the cast results with the world's dynamics step
// info.
void hkpVehicleCastBatchingManager::stepVehiclesSynchronously(
    hkpWorld* world,
    hkJobThreadPool* threadPool,
    hkJobQueue* jobQueue,
    int32_t numJobs
) {
    HK_TIMER_BEGIN("DoVehiclesSynchronously", nullptr);
    updateBeforeCollisionDetection();
    const int32_t bufferSize = getBufferSize(numJobs);
    // EXE-INLINED none: hkAllocateStack<char>
    hkMemoryRouter* router = &hkMemoryRouter::getInstance();
    char* buffer = router->m_stack.m_next.m_storage;
    router->m_stack.m_next.m_storage = buffer + ((bufferSize + 127) & ~127);

    // EXE-INLINED none: hkSemaphore's class operator new
    void* pMemory = hkMemoryRouter::alignedAlloc(*hkMemoryRouter::getInstance().m_heap, sizeof(hkSemaphore), 128);
    hkSemaphore* semaphore = static_cast<hkSemaphore*>(pMemory);
    if (pMemory) {
        ::new (pMemory) hkSemaphore(0, 1000);
    }
    buildAndAddCastJobs(world, getStandardFilterSize(world), numJobs, jobQueue, semaphore, buffer);

    world->lockReadOnly();
    threadPool->processAllJobs(jobQueue, HK_JOB_TYPE_MAX);
    jobQueue->processAllJobs();
    threadPool->waitForCompletion();
    semaphore->acquire();
    world->unlockReadOnly();
    semaphore->~hkSemaphore();
    // EXE-INLINED none: hkSemaphore's class operator delete
    hkMemoryRouter::alignedFree(*hkMemoryRouter::getInstance().m_heap, semaphore);

    stepVehiclesUsingCastResults(world->m_dynamicsStepInfo.m_stepInfo, numJobs, buffer);
    // EXE-INLINED none: hkDeallocateStack<char>
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage = buffer;
    HK_TIMER_END();
}

void hkpVehicleCastBatchingManager::updateBeforeCollisionDetection() {
    int32_t m_size = m_registeredVehicles.m_size;
    for (int32_t i = 0; i < m_size; ++i) {
        m_registeredVehicles.m_data[i]->updateBeforeCollisionDetection();
    }
}

int32_t hkpVehicleCastBatchingManager::getStandardFilterSize(const hkpWorld* world) {
    return world->m_collisionFilter->m_type.m_storage != 2 ? 0 : 256;
}
