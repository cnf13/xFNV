#pragma once

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

// An hkStringMap that owns copies of its keys: insert duplicates a new key on the allocator, remove frees it. The map
// itself is hkStringMap's.
template <typename V, typename Allocator = hkContainerHeapAllocator>
class hkStorageStringMap : public hkStringMap<V, Allocator> {
public:
    // Frees every key's copy, then empties the map; the destructor calls it.
    void clear();
};

// Frees every key's copy on the thread's heap, then empties the map.
template <typename V, typename Allocator>
void hkStorageStringMap<V, Allocator>::clear() {
    using Iterator = hkCachedHashMap<hkStringMapOperations, Allocator>::Iterator;
    for (Iterator it = this->m_map.getIterator(); this->m_map.isValid(it).m_bool; it = this->m_map.getNext(it)) {
        // The map holds the key's address as an untyped hkUlong.
        char* k = reinterpret_cast<char*>(this->m_map.getKey(it));
        hkString::strFree_00CB1190(k, *hkMemoryRouter::getInstance().m_heap);
    }
    this->m_map.clear();
}
