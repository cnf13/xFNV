#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpNullBroadPhaseListener.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"

#include <cstddef>

class hkpCollidableCollidableFilter;

// Dispatches the broad phase's new and removed pairs to the listener registered for the pair's two handle types.
class hkpTypedBroadPhaseDispatcher {
public:
    hkpTypedBroadPhaseDispatcher();

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

    void addPairs(
        hkpTypedBroadPhaseHandlePair* newPairs,
        int32_t numNewPairs,
        const hkpCollidableCollidableFilter* filter
    ) const;
    void removePairs(hkpTypedBroadPhaseHandlePair* deletedPairs, int32_t numDeletedPairs) const;

    hkpBroadPhaseListener* getBroadPhaseListener(int32_t typeA, int32_t typeB) {
        return m_broadPhaseListeners[typeA][typeB];
    }

    void setBroadPhaseListener(hkpBroadPhaseListener* listener, int32_t a, int32_t b) {
        m_broadPhaseListeners[a][b] = listener;
    }

    hkpBroadPhaseListener* getNullBroadPhaseListener() {
        return &m_nullBroadPhaseListener;
    }

    // The implicit destructor destroys m_nullBroadPhaseListener.
    // Havok SDK.
    static void
    removeDuplicates(hkArray<hkpBroadPhaseHandlePair>& newPairs, hkArray<hkpBroadPhaseHandlePair>& delPairs);

    hkpBroadPhaseListener* m_broadPhaseListeners[8][8];
    hkpNullBroadPhaseListener m_nullBroadPhaseListener;
};
