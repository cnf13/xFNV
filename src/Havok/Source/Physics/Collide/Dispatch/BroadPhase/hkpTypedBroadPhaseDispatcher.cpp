#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseDispatcher.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseHandle.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseListener.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpNullBroadPhaseListener.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"

// Drops the pairs found in both lists: a new pair whose two handles are the deleted pair's (in either order) leaves
// both lists, and a new pair seen twice in newPairs keeps only its first entry. With fewer than 32 pairs on either side
// it scans newPairs for each deleted pair (O(n*m)) and compacts delPairs in place; above that it keys a 64-bit map by
// the pair's handles sorted (the smaller handle is the key's low word), the value being the index of the new pair
// shifted by 8 with a count of how often it was seen in the low byte. The map's table comes from the router's temp
// allocator. Both arrays' sizes are re-read as it goes, and removals shift the tail of newPairs down.
void hkpTypedBroadPhaseDispatcher::removeDuplicates(
    hkArray<hkpBroadPhaseHandlePair>& newPairs,
    hkArray<hkpBroadPhaseHandlePair>& delPairs
) {
    int32_t iSmaller = newPairs.getSize() < delPairs.getSize() ? newPairs.getSize() : delPairs.getSize();
    if (iSmaller < 32) {
        int32_t iDst = 0;
        for (int32_t iDel = 0; iDel < delPairs.getSize(); ++iDel) {
            hkpBroadPhaseHandlePair& delPair = delPairs.m_data[iDel];
            int32_t iNew;
            for (iNew = 0; iNew < newPairs.getSize(); ++iNew) {
                hkpBroadPhaseHandlePair& newPair = newPairs.m_data[iNew];
                if ((delPair.m_a == newPair.m_a) && (delPair.m_b == newPair.m_b)) {
                    break;
                }

                if (delPair.m_a == newPair.m_b && delPair.m_b == newPair.m_a) {
                    break;
                }
            }
            if (iNew == newPairs.getSize()) {
                if (iDst != iDel) {
                    delPairs.m_data[iDst] = delPair;
                }
                ++iDst;
            }
            else {
                // EXE-INLINED none: hkArray::removeAt
                int32_t iSize = newPairs.m_size - 1;
                newPairs.m_size = static_cast<uint32_t>(iSize);
                for (int32_t i = iNew; i < iSize; ++i) {
                    newPairs.m_data[i] = newPairs.m_data[i + 1];
                }
            }
        }
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        int32_t iCapacity = delPairs.getCapacity();
        if (iCapacity < iDst) {
            int32_t iReq = 2 * iCapacity;
            if (iDst >= iReq) {
                iReq = iDst;
            }
            hkArrayUtil::_reserve(&heap, &delPairs, iReq, sizeof(hkpBroadPhaseHandlePair));
        }
        delPairs.setSizeUnchecked(iDst);
        return;
    }
    int32_t iMapBytes = hkMapBase<uint64_t, uint64_t, hkMapOperations<uint64_t>>::requiredBytes(newPairs.getSize());
    void* pBuffer = nullptr;
    int32_t iAllocatedBytes = INT32_MIN;
    if (iMapBytes) {
        int32_t iSize = iMapBytes;
        pBuffer = hkMemoryRouter::getInstance().temp().bufAlloc(iSize);
        iAllocatedBytes = iSize;
        if (!iAllocatedBytes) {
            iAllocatedBytes = INT32_MIN;
        }
    }
    hkMapBase<uint64_t, uint64_t, hkMapOperations<uint64_t>> map;
    map.init(pBuffer, iMapBytes);
    // On 32 bits a pair's key holds the smaller handle's address in the low 32 bits and the larger's in the high 32
    // bits, one key per pair. Two 64-bit addresses do not fit in 64 bits: the 64-bit program mixes them into the key,
    // so two pairs can share one. Every lookup therefore compares the handles of the new pair the entry indexes and, on
    // another pair's entry, probes on with the next key (never on 32 bits, where the key is the pair).
    for (int32_t i = 0; i < newPairs.getSize(); ++i) {
        hkpBroadPhaseHandle* pA = newPairs.m_data[i].m_a;
        hkpBroadPhaseHandle* pB = newPairs.m_data[i].m_b;
        uintptr_t uiLo = reinterpret_cast<uintptr_t>(pA);
        uintptr_t uiHi = reinterpret_cast<uintptr_t>(pB);
        if (uiLo > uiHi) {
            uiLo = reinterpret_cast<uintptr_t>(pB);
            uiHi = reinterpret_cast<uintptr_t>(pA);
        }
#if FNVD_64BIT
        uint64_t uiKey = (uiLo * 0x9E3779B97F4A7C15ull) ^ (uiHi * 0xC2B2AE3D27D4EB4Full);
        // ~0 is the map's empty key.
        if (uiKey == ~0ull) {
            uiKey = 0;
        }
#else
        uint64_t uiKey = (static_cast<uint64_t>(uiHi) << 32) | uiLo;
#endif
        int32_t iSlot = map.findKey(uiKey);
        while (iSlot <= map.m_hashMod) {
            const hkpBroadPhaseHandlePair& kEntry = newPairs.m_data[static_cast<uint32_t>(map.m_elem[iSlot].val) >> 8];
            if ((kEntry.m_a == pA && kEntry.m_b == pB) || (kEntry.m_a == pB && kEntry.m_b == pA)) {
                break;
            }
            uiKey = uiKey + 1 == ~0ull ? 0 : uiKey + 1;
            iSlot = map.findKey(uiKey);
        }
        if (iSlot > map.m_hashMod) {
            map.insert(
                hkMemoryRouter::getInstance().heap(), uiKey, static_cast<uint64_t>(static_cast<int32_t>((i << 8) | 1))
            );
        }
        else {
            ++map.m_elem[iSlot].val;
            newPairs.m_data[i].m_a = nullptr;
        }
    }
    int32_t iDst = 0;
    for (int32_t iDel = 0; iDel < delPairs.getSize(); ++iDel) {
        hkpBroadPhaseHandle* pA = delPairs.m_data[iDel].m_a;
        hkpBroadPhaseHandle* pB = delPairs.m_data[iDel].m_b;
        uintptr_t uiLo = reinterpret_cast<uintptr_t>(pA);
        uintptr_t uiHi = reinterpret_cast<uintptr_t>(pB);
        if (uiLo > uiHi) {
            uiLo = reinterpret_cast<uintptr_t>(pB);
            uiHi = reinterpret_cast<uintptr_t>(pA);
        }
#if FNVD_64BIT
        uint64_t uiKey = (uiLo * 0x9E3779B97F4A7C15ull) ^ (uiHi * 0xC2B2AE3D27D4EB4Full);
        // ~0 is the map's empty key.
        if (uiKey == ~0ull) {
            uiKey = 0;
        }
#else
        uint64_t uiKey = (static_cast<uint64_t>(uiHi) << 32) | uiLo;
#endif
        int32_t iSlot = map.findKey(uiKey);
        while (iSlot <= map.m_hashMod) {
            const hkpBroadPhaseHandlePair& kEntry = newPairs.m_data[static_cast<uint32_t>(map.m_elem[iSlot].val) >> 8];
            if ((kEntry.m_a == pA && kEntry.m_b == pB) || (kEntry.m_a == pB && kEntry.m_b == pA)) {
                break;
            }
            uiKey = uiKey + 1 == ~0ull ? 0 : uiKey + 1;
            iSlot = map.findKey(uiKey);
        }
        if (iSlot > map.m_hashMod) {
            if (iDst != iDel) {
                delPairs.m_data[iDst] = delPairs.m_data[iDel];
            }
            ++iDst;
        }
        else {
            uint64_t uiValue = map.m_elem[iSlot].val;
            // The count is the low byte: the last sighting drops the new pair as well.
            if ((static_cast<uint32_t>(uiValue) & 0xFF) <= 1) {
#if FNVD_64BIT
                // The 64-bit program keeps the entry: removing it would cut the probe sequence of a pair that shares
                // its key and sits further on. With its new pair's handle cleared below it matches no pair again.
#else
                map.remove(iSlot);
#endif
                newPairs.m_data[static_cast<uint32_t>(uiValue) >> 8].m_a = nullptr;
            }
            else {
                map.m_elem[iSlot].val = uiValue - 1;
            }
        }
    }
    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
    int32_t iCapacity = delPairs.getCapacity();
    if (iCapacity < iDst) {
        int32_t iReq = 2 * iCapacity;
        if (iDst >= iReq) {
            iReq = iDst;
        }
        hkArrayUtil::_reserve(&heap, &delPairs, iReq, sizeof(hkpBroadPhaseHandlePair));
    }
    delPairs.setSizeUnchecked(iDst);
    int32_t iKept = 0;
    for (int32_t i = 0; i < newPairs.getSize(); ++i) {
        if (newPairs.m_data[i].m_a) {
            if (iKept != i) {
                newPairs.m_data[iKept] = newPairs.m_data[i];
            }
            ++iKept;
        }
    }
    int32_t iNewCapacity = newPairs.getCapacity();
    if (iNewCapacity < iKept) {
        int32_t iReq = 2 * iNewCapacity;
        if (iKept >= iReq) {
            iReq = iKept;
        }
        hkArrayUtil::_reserve(&heap, &newPairs, iReq, sizeof(hkpBroadPhaseHandlePair));
    }
    newPairs.setSizeUnchecked(iKept);
    map.clearAndDeallocate(hkMemoryRouter::getInstance().heap());
    if (iAllocatedBytes >= 0) {
        hkMemoryRouter::getInstance().temp().bufFree(pBuffer, iAllocatedBytes & 0x3FFFFFFF);
    }
}

// With 32-bit pointers, the key holds m_a's address in the low 32 bits and m_b's in the high 32 bits.
int64_t keyFromPair(const hkpBroadPhaseHandlePair& pair) {
    return static_cast<int64_t>(
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(pair.m_a)) +
        (static_cast<uint64_t>(reinterpret_cast<uintptr_t>(pair.m_b)) << 32)
    );
}

hkpTypedBroadPhaseDispatcher::hkpTypedBroadPhaseDispatcher() {
    for (int32_t a = 0; a < 8; a++) {
        for (int32_t b = 0; b < 8; b++) {
            m_broadPhaseListeners[a][b] = &m_nullBroadPhaseListener;
        }
    }
}

// Hands each of the numNewPairs pairs filter lets collide to the listener registered for its two handle types.
void hkpTypedBroadPhaseDispatcher::addPairs(
    hkpTypedBroadPhaseHandlePair* newPairs,
    int32_t numNewPairs,
    const hkpCollidableCollidableFilter* filter
) const {
    for (int32_t i = numNewPairs - 1; i >= 0; --i) {
        hkpTypedBroadPhaseHandle* handleA = newPairs->getElementA();
        // The owner sits m_ownerOffset bytes from the handle, an untyped byte offset.
        hkpTypedBroadPhaseHandle* handleB = newPairs->getElementB();
        // EXE-INLINED 0x9B2C90
        const hkpCollidable* collA = static_cast<const hkpCollidable*>(
            static_cast<void*>(reinterpret_cast<char*>(handleA) + handleA->m_ownerOffset)
        );
        // EXE-INLINED 0x9B2C90
        const hkpCollidable* collB = static_cast<const hkpCollidable*>(
            static_cast<void*>(reinterpret_cast<char*>(handleB) + handleB->m_ownerOffset)
        );
        if (filter->isCollisionEnabled(*collA, *collB).m_bool) {
            // Reloads both handles after the filter's call.
            int32_t typeA = newPairs->getElementA()->getType();
            int32_t typeB = newPairs->getElementB()->getType();
            m_broadPhaseListeners[typeA][typeB]->addCollisionPair(*newPairs);
        }
        ++newPairs;
    }
}

void hkpTypedBroadPhaseDispatcher::removePairs(
    hkpTypedBroadPhaseHandlePair* deletedPairs,
    int32_t numDeletedPairs
) const {
    for (int32_t i = numDeletedPairs - 1; i >= 0; --i) {
        hkpBroadPhaseListener* v6 = m_broadPhaseListeners[static_cast<int8_t>(deletedPairs->m_a[1].m_id)]
                                                         [static_cast<int8_t>(deletedPairs->m_b[1].m_id)];
        v6->removeCollisionPair(*deletedPairs++);
    }
}

void hkReferencedObject::addReference_CF7490() {
    addReference();
}

void hkReferencedObject::removeReference_CF74A0() {
    removeReference();
}
