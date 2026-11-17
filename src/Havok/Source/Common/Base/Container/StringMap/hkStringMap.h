#pragma once

#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"

#include <stdint.h>

// A map from a C string to a pointer-sized value, held in an hkCachedHashMap.
template <typename V, typename Allocator = hkContainerHeapAllocator>
class hkStringMap {
public:
    // The value of key, or def.
    V getWithDefault(const char* key, V def) const {
        return (V)m_map.getWithDefault(hkUlong(key), hkUlong(def));
    }

    // Adds key with val, or replaces its value.
    void insert(const char* key, V val) {
        m_map.insert(hkUlong(key), hkUlong(val));
    }

    // Removes key: HK_SUCCESS when the map had it.
    hkResult remove(const char* key) {
        return m_map.remove(hkUlong(key));
    }

    // The value at it, as the SDK's inline getValue: the map stores every V as an untyped pointer-sized integer
    // (hkUlong), converted back to V here.
    V getValue(typename hkCachedHashMap<hkStringMapOperations, Allocator>::Iterator i) const {
        return reinterpret_cast<V>(m_map.getValue(i));
    }

    hkCachedHashMap<hkStringMapOperations, Allocator> m_map;
};
