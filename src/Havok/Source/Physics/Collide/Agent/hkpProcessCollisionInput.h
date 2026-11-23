#pragma once

#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"

#include <cstddef>

class hkpCollisionAgentConfig;
class hkpCollisionQualityInfo;

// The collision input of a processing step: the step's times, the quality table, the spare agent sector and the agent
// configuration. hkPadSpu<T> members are plain T on PC. Not polymorphic.
class hkpProcessCollisionInput : public hkpCollisionInput {
public:
    // The class allocator: blocks of the Havok heap.
    static void* operator new(size_t aSize) noexcept {
        return hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(aSize));
    }
    static void operator delete(void* apMemory, size_t aSize) {
        hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
    }
    static void* operator new(size_t, void* apMemory) noexcept {
        return apMemory;
    }
    static void operator delete(void*, void*) noexcept {}

    hkpProcessCollisionInput() : m_spareAgentSector(nullptr) {
        m_weldClosestPoints.m_storage = 0;
    }
    // The base copied, no spare sector, then the assignment.
    hkpProcessCollisionInput(const hkpProcessCollisionInput& other) : hkpCollisionInput(other) {
        m_spareAgentSector = nullptr;
        *this = other;
    }
    hkpProcessCollisionInput& operator=(const hkpProcessCollisionInput& other);
    ~hkpProcessCollisionInput() {
        delete m_spareAgentSector;
    }

    hkStepInfo m_stepInfo;
    // Mutable because hkpSimulation::processAgentCollideDiscrete sets it through its const input.
    mutable hkpCollisionQualityInfo* m_collisionQualityInfo;
    // Mutable because hkAgent1nMachine_Process takes and returns the spare sector through its const input.
    mutable hkpAgent1nSector* m_spareAgentSector;
    void* m_dynamicsInfo;
    hkBool m_enableDeprecatedWelding;
    hkBool m_allowToSkipConfirmedCallbacks;
    hkpCollisionAgentConfig* m_config;
};
