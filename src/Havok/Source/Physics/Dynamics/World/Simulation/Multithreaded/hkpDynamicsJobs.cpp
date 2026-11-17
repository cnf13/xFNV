#include "Havok/Source/Physics/Dynamics/World/Simulation/Multithreaded/hkpDynamicsJobs.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Thread/JobQueue/hkJobQueue.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

// A task header with newly split islands: this job takes them on (its island range grows over them), and each joins
// the world's active islands with its storage index; the header's list is freed. Otherwise this job hands one island
// to out while more than one is left.
hkJobQueue::JobPopFuncResult
hkpBroadPhaseJob::popJobTask(hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>& islands, hkpBroadPhaseJob& out) {
    int32_t m_size;
    hkpBuildJacobianTaskHeader* m_taskHeader = this->m_taskHeader;
    if (m_taskHeader && (m_size = m_taskHeader->m_newSplitIslands.m_size) != 0) {
        m_taskHeader->m_referenceCount += m_size;
        m_taskHeader->m_numIslandsAfterSplit += m_size;
        m_islandIndex = islands.m_size;
        this->m_numIslands = m_size;
        hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>* p_m_activeSimulationIslands =
            &out.m_island->getWorld()->m_activeSimulationIslands;
        for (int32_t i = 0; i < m_size; ++i) {
            hkpSimulationIsland* pIsland = m_taskHeader->m_newSplitIslands.m_data[i];
            pIsland->m_storageIndex = static_cast<uint16_t>(p_m_activeSimulationIslands->m_size);
            hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
            if (p_m_activeSimulationIslands->m_size ==
                (p_m_activeSimulationIslands->m_capacityAndFlags & hkArrayBase<hkpSimulationIsland*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_activeSimulationIslands, sizeof(hkpSimulationIsland*));
            }
            p_m_activeSimulationIslands->m_data[p_m_activeSimulationIslands->m_size++] = pIsland;
        }
        // EXE-INLINED 0x62A0B0
        hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
        int32_t m_capacityAndFlags = m_taskHeader->m_newSplitIslands.m_capacityAndFlags;
        m_taskHeader->m_newSplitIslands.m_size = 0;
        if (m_capacityAndFlags >= 0) {
            m_heap->bufFree(
                m_taskHeader->m_newSplitIslands.m_data,
                static_cast<int32_t>(
                    sizeof(hkpSimulationIsland*) *
                    (m_capacityAndFlags & hkArrayBase<hkpSimulationIsland*>::CAPACITY_MASK)
                )
            );
        }
        m_taskHeader->m_newSplitIslands.m_data = nullptr;
        m_taskHeader->m_newSplitIslands.m_capacityAndFlags = hkArrayBase<hkpSimulationIsland*>::DONT_DEALLOCATE_FLAG;
        return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
    }
    else {
        uint16_t m_numIslands = this->m_numIslands;
        if (m_numIslands <= 1u) {
            return hkJobQueue::POP_QUEUE_ENTRY;
        }
        else {
            ++m_islandIndex;
            this->m_numIslands = m_numIslands - 1;
            out.m_numIslands = 1;
            return hkJobQueue::DO_NOT_POP_QUEUE_ENTRY;
        }
    }
}
