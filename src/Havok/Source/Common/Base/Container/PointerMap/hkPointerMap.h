#pragma once

#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

#include <stdint.h>
#include <type_traits>

// A map from a pointer-sized key to a pointer-sized value: an hkMap of pointer-sized integers, or of 64-bit integers
// for a 64-bit integer key.
template <typename K, typename V, typename Allocator = hkContainerHeapAllocator>
class hkPointerMap {
public:
    // The SDK's hkPointerMap(int numElements = 0): the map's own constructor, grown to hold numElements.
    explicit hkPointerMap(int32_t numElements = 0) : m_map(numElements) {}

    // The map's integer: 64-bit for an 8-byte key, else pointer-sized.
    using Storage = std::conditional_t<sizeof(K) == 8, uint64_t, uintptr_t>;

    // The key and value go through the map as its storage integers, and insert allocates from the calling thread's heap
    // and returns the map's hkBool32 (whether the key was new).
    int32_t insert(K key, V val) {
        hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
        return static_cast<int32_t>(m_map.insert(kHeap, ToMapInt(key), ToMapInt(val)));
    }

    V getWithDefault(K key, V def) const {
        return FromMapInt<V>(m_map.getWithDefault(ToMapInt(key), ToMapInt(def)));
    }

    // The SDK's inline get(K key, V* out): the map's lookup into a storage integer, copied to *out when the
    // key is found.
    hkResult get(K key, V* out) const {
        Storage tmp;
        if (m_map.getWithDefaultStatus(ToMapInt(key), &tmp) == HK_SUCCESS) {
            *out = FromMapInt<V>(tmp);
            return HK_SUCCESS;
        }
        return HK_FAILURE;
    }

    // An iterator is valid (the key was found) while it is at most m_hashMod; the compare is signed.
    int32_t findKey(K key) const {
        return m_map.findKey(ToMapInt(key));
    }
    bool isValid(int32_t it) const {
        return it <= m_map.m_hashMod;
    }
    // Whether key is in the map (the SDK's hkBool32).
    int32_t hasKey(K key) const {
        return isValid(findKey(key));
    }
    // The SDK's inline remove(K key): the map's remove of the key as its storage integer; the hkResult as int32_t.
    int32_t remove(K key) {
        return m_map.remove(ToMapInt(key));
    }
    // The SDK's clear(): empties the map, keeping its table.
    void clear() {
        m_map.clear();
    }
    // The SDK's inline getKey(Iterator): the key at slot it.
    K getKey(int32_t it) const {
        return FromMapInt<K>(m_map.m_elem[it].key);
    }

    // The first used slot (an iterator like findKey's: past m_hashMod for an empty map). An empty slot holds the key
    // -1.
    int32_t getIterator() const {
        int32_t i;
        for (i = 0; i <= m_map.m_hashMod; ++i) {
            if (m_map.m_elem[i].key != static_cast<Storage>(-1)) {
                break;
            }
        }
        return i;
    }

    // The SDK's inline getValue(Iterator): the value at slot it.
    V getValue(int32_t it) const {
        return FromMapInt<V>(m_map.m_elem[it].val);
    }

    // The SDK's inline getNext(Iterator): the next used slot after it (past m_hashMod at the end).
    int32_t getNext(int32_t it) const {
        int32_t i;
        for (i = it + 1; i <= m_map.m_hashMod; ++i) {
            if (m_map.m_elem[i].key != static_cast<Storage>(-1)) {
                break;
            }
        }
        return i;
    }

    // A pointer or integer key or value as the map's integer, and back.
    template <typename T>
    static Storage ToMapInt(T x) {
        if constexpr (__is_pointer(T)) {
            return static_cast<Storage>(reinterpret_cast<uintptr_t>(x));
        }
        else {
            return static_cast<Storage>(x);
        }
    }

    template <typename T>
    static T FromMapInt(Storage x) {
        if constexpr (__is_pointer(T)) {
            return reinterpret_cast<T>(static_cast<uintptr_t>(x));
        }
        else {
            return static_cast<T>(x);
        }
    }

    hkMap<Storage, Storage, hkMapOperations<Storage>, Allocator> m_map;
};
