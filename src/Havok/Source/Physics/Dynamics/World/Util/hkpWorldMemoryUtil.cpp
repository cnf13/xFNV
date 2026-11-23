#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldMemoryUtil.h"

#include "Havok/Source/Common/Base/Memory/Allocator/FreeList/hkFreeListAllocator.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Physics/Dynamics/World/Memory/hkWorldMemoryAvailableWatchDog.h"
#include "Havok/Source/Physics/Dynamics/World/Simulation/hkpSimulation.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

// While the solver allocator cannot hold the largest island's integration block: splits the largest island while it
// is sparse (halving the desired island size when a split changes nothing), then marks an unconnected island sparse
// or has the watchdog remove constraints of a connected one. The desired island size is restored at the end.
void hkpWorldMemoryUtil::checkMemoryForIntegration(hkpWorld* world) {
    world->lock();
    uint32_t minDesiredIslandSize = world->m_minDesiredIslandSize;
    hkpWorld::MemUsageInfo memInfo;
    world->getMemUsageForIntegration(memInfo);
    if (!hkMemoryRouter::getInstance().m_solver->canAllocSingleBlock(memInfo.m_maxRuntimeBlockSize)) {
        do {
            hkpSimulationIsland* island = memInfo.m_largestSimulationIsland;
            while (island->m_sparseEnabled) {
                int32_t maxRuntimeBlockSize = memInfo.m_maxRuntimeBlockSize;
                hkpSimulationIsland* splitIsland = island;
                do {
                    hkpWorldOperationUtil::splitSimulationIsland(world, splitIsland);
                    world->getMemUsageForIntegration(memInfo);
                    island = memInfo.m_largestSimulationIsland;
                    if (island != splitIsland || memInfo.m_maxRuntimeBlockSize != maxRuntimeBlockSize) {
                        break;
                    }
                    world->m_minDesiredIslandSize >>= 1;
                }
                while (island->m_sparseEnabled);
            }
            if (!hkMemoryRouter::getInstance().m_solver->canAllocSingleBlock(memInfo.m_maxRuntimeBlockSize)) {
                island = memInfo.m_largestSimulationIsland;
                if (!island->isFullyConnected()) {
                    island->m_sparseEnabled = 1;
                }
                else {
                    world->getMemoryWatchDog()->reduceConstraintsInIsland(island, 0);
                    world->getMemUsageForIntegration(memInfo);
                }
            }
        }
        while (!hkMemoryRouter::getInstance().m_solver->canAllocSingleBlock(memInfo.m_maxRuntimeBlockSize));
    }
    world->m_minDesiredIslandSize = minDesiredIslandSize;
    world->unlock();
}

// Asks the watchdog how much heap memory it wants free and, when the heap cannot supply it, has the watchdog free
// world's memory. The watchdog is fetched with a fresh getMemoryWatchDog call each time. Both
// monitor-stream blocks are HK_MONITOR_ENTER/HK_MONITOR_LEAVE: the entry's name is stored at m_end, the low 32 bits of
// rdtsc after it, and m_end advanced by 12. The stream's buffer is untyped bytes, so those two stores are casts of raw
// memory.
void hkpWorldMemoryUtil::watchHeapMemory(hkpWorld* world) {
    if (world->getMemoryWatchDog()) {
        hkWorldMemoryAvailableWatchDog* pWatchDog = world->getMemoryWatchDog();
        int32_t iRequested = pWatchDog->getAmountOfFreeHeapMemoryRequested();
        if (!hkMemoryRouter::getInstance().heap().canAllocTotal(iRequested)) {
            HK_TIMER_BEGIN("WatchDog:FreeMem", nullptr);
            world->getMemoryWatchDog()->freeHeapMemoryTillRequestedAmountIsAvailable(world);
            HK_TIMER_END();
        }
        if (hkOutOfMemoryState) {
            hkSetOutOfMemoryState(hkMemoryAllocator::MEMORY_STATE_OK);
        }
    }
}

void hkpWorldMemoryUtil::tryToRecoverFromMemoryErrors(hkpWorld* world) {
    if (world->getMemoryWatchDog()) {
        int32_t v1 = 10;
        while (world->m_simulation->m_previousStepResult) {
            int32_t v2 = v1--;
            if (v2 <= 0) {
                break;
            }
            hkWorldMemoryAvailableWatchDog* MemoryWatchDog = world->getMemoryWatchDog();
            MemoryWatchDog->freeHeapMemoryTillRequestedAmountIsAvailable(world);
            hkSetOutOfMemoryState(hkMemoryAllocator::MEMORY_STATE_OK);
            hkpWorldMemoryUtil::repeatCollideAndToiHandling(world);
        }
    }
}

void hkpWorldMemoryUtil::repeatCollideAndToiHandling(hkpWorld* world) {
    uint32_t m_previousStepResult = world->m_simulation->m_previousStepResult;
    if (!m_previousStepResult || m_previousStepResult == 2) {
        world->m_simulation->collide();
    }
    hkpSimulation* m_simulation = world->m_simulation;
    uint32_t v3 = m_simulation->m_previousStepResult;
    if (!v3 || v3 == 3) {
        m_simulation->advanceTime();
    }
}
