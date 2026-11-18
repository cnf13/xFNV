#pragma once

#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Reconstruction.hpp"

// The key operations a map hashes and compares its keys with (hash, invalidate, isValid and equal; an invalid key is
// K(-1)). No data.
template <typename K>
struct hkMapOperations {};

// The open-addressing hash map of K to V: m_hashMod + 1 slots of pairs at m_elem, empty slots holding an invalid key;
// m_numElems counts the pairs, with DONT_DEALLOCATE_FLAG in its high bit when the map does not own m_elem. Most
// instances are the explicit specializations below and in the headers of their keys. The members here are the generic
// ones, for the instances only the x64 program has (hkpPairCollisionFilter's map of 16-byte keys); Operations hashes,
// invalidates, validates and compares the keys.
template <typename K, typename V = K, typename Operations = hkMapOperations<K>>
class hkMapBase {
public:
    // Values from the SDK.
    enum : int32_t {
        NUM_ELEMS_MASK = 0x7fffffff,
        DONT_DEALLOCATE_FLAG = INT32_MIN,
    };

    struct Pair {
        K key;
        V val;
    };

    // The empty map.
    hkMapBase() : m_elem(nullptr), m_numElems(0), m_hashMod(-1) {}

    // Adds key with val, or replaces its value, allocating from alloc; whether the key was new.
    uint32_t insert(hkMemoryAllocator& alloc, K key, V val);
    // The slot index of key, or m_hashMod + 1 when it is absent (an Iterator in the SDK).
    int32_t findKey(K key) const;
    // The value of key, or def.
    V getWithDefault(K key, V def) const;
    // Drops the pair at slot aiIndex and moves the later pairs of its probe chain up.
    void remove(int32_t aiIndex);
    // Drops key's pair when it is present; the hkResult as int32_t, 1 (failure) when it is absent.
    int32_t remove(K key);
    // Empties the table, the DONT_DEALLOCATE flag staying in m_numElems' high bit.
    void clear();
    // Empties the table and frees its slots through alloc unless the map does not own them.
    void clearAndDeallocate(hkMemoryAllocator& alloc);
    // Grows the table to hold numElements: twice that many slots, a power of two from 8.
    void reserve(hkMemoryAllocator& alloc, int32_t numElements);
    // Rebuilds the table with newcap slots (at least 8) from alloc, inserting the old pairs again.
    void resizeTable(hkMemoryAllocator& alloc, int32_t newcap);

    Pair* m_elem;
    int32_t m_numElems;
    int32_t m_hashMod;
};

template <typename K, typename V, typename Operations>
uint32_t hkMapBase<K, V, Operations>::insert(hkMemoryAllocator& alloc, K key, V val) {
    const int32_t numElems = m_numElems & NUM_ELEMS_MASK;
    if (numElems + numElems > m_hashMod) {
        resizeTable(alloc, m_hashMod + m_hashMod + 2);
    }
    uint32_t i;
    uint32_t isNewKey = 1;
    for (i = Operations::hash(key, m_hashMod); Operations::isValid(m_elem[i].key); i = (i + 1) & m_hashMod) {
        if (Operations::equal(m_elem[i].key, key)) {
            isNewKey = 0;
            break;
        }
    }
    m_numElems += isNewKey;
    m_elem[i].key = key;
    m_elem[i].val = val;
    return isNewKey;
}

template <typename K, typename V, typename Operations>
int32_t hkMapBase<K, V, Operations>::findKey(K key) const {
    if (m_hashMod > 0) {
        for (uint32_t i = Operations::hash(key, m_hashMod); Operations::isValid(m_elem[i].key);
             i = (i + 1) & m_hashMod) {
            if (Operations::equal(m_elem[i].key, key)) {
                return static_cast<int32_t>(i);
            }
        }
    }
    return m_hashMod + 1;
}

template <typename K, typename V, typename Operations>
V hkMapBase<K, V, Operations>::getWithDefault(K key, V def) const {
    if (m_hashMod > 0) {
        for (uint32_t i = Operations::hash(key, m_hashMod); Operations::isValid(m_elem[i].key);
             i = (i + 1) & m_hashMod) {
            if (Operations::equal(m_elem[i].key, key)) {
                return m_elem[i].val;
            }
        }
    }
    return def;
}

template <typename K, typename V, typename Operations>
void hkMapBase<K, V, Operations>::remove(int32_t aiIndex) {
    uint32_t i = static_cast<uint32_t>(aiIndex);
    const uint32_t hashMod = static_cast<uint32_t>(m_hashMod);
    --m_numElems;
    Operations::invalidate(m_elem[i].key);
    // The start of the probe run the freed slot was in.
    uint32_t lo = (i + hashMod) & hashMod;
    while (Operations::isValid(m_elem[lo].key)) {
        lo = (lo + hashMod) & hashMod;
    }
    lo = (lo + 1) & hashMod;
    // Each later pair of the run whose hash does not lie between the hole and itself moves into the hole.
    uint32_t empty = i;
    for (i = (i + 1) & hashMod; Operations::isValid(m_elem[i].key); i = (i + 1) & hashMod) {
        const uint32_t hash = Operations::hash(m_elem[i].key, hashMod);
        if (i >= lo && hash > empty) {
            continue;
        }
        if (i < empty && (hash > empty || hash <= i)) {
            continue;
        }
        if (hash > empty && hash < lo) {
            continue;
        }
        m_elem[empty].key = m_elem[i].key;
        m_elem[empty].val = m_elem[i].val;
        Operations::invalidate(m_elem[i].key);
        empty = i;
    }
}

template <typename K, typename V, typename Operations>
int32_t hkMapBase<K, V, Operations>::remove(K key) {
    const int32_t i = findKey(key);
    if (i <= m_hashMod) {
        remove(i);
        return 0;
    }
    return 1;
}

template <typename K, typename V, typename Operations>
void hkMapBase<K, V, Operations>::clear() {
    const int32_t capacity = m_hashMod + 1;
    for (int32_t i = 0; i < capacity; i++) {
        Operations::invalidate(m_elem[i].key);
    }
    m_numElems &= DONT_DEALLOCATE_FLAG;
}

template <typename K, typename V, typename Operations>
void hkMapBase<K, V, Operations>::clearAndDeallocate(hkMemoryAllocator& alloc) {
    clear();
    if ((m_numElems & DONT_DEALLOCATE_FLAG) == 0) {
        alloc.blockFree(m_elem, static_cast<int32_t>(sizeof(Pair)) * (m_hashMod + 1));
    }
}

template <typename K, typename V, typename Operations>
void hkMapBase<K, V, Operations>::reserve(hkMemoryAllocator& alloc, int32_t numElements) {
    const int32_t minCap = numElements * 2;
    int32_t cap = 8;
    while (cap < minCap) {
        cap *= 2;
    }
    resizeTable(alloc, cap);
}

template <typename K, typename V, typename Operations>
void hkMapBase<K, V, Operations>::resizeTable(hkMemoryAllocator& alloc, int32_t newcap) {
    if (newcap < 8) {
        newcap = 8;
    }
    const int32_t dontDeallocate = m_numElems & DONT_DEALLOCATE_FLAG;
    const int32_t oldcap = m_hashMod + 1;
    Pair* oldelem = m_elem;
    m_elem = static_cast<Pair*>(alloc.blockAlloc(static_cast<int32_t>(sizeof(Pair)) * newcap));
    for (int32_t i = 0; i < newcap; i++) {
        Operations::invalidate(m_elem[i].key);
    }
    m_numElems = 0;
    m_hashMod = newcap - 1;
    for (int32_t i = 0; i < oldcap; ++i) {
        if (Operations::isValid(oldelem[i].key)) {
            insert(alloc, oldelem[i].key, oldelem[i].val);
        }
    }
    if (dontDeallocate == 0) {
        alloc.blockFree(oldelem, static_cast<int32_t>(sizeof(Pair)) * oldcap);
    }
}

// Havok's open-addressing hash map from a pointer-sized key to a pointer-sized value (empty slots hold the key -1),
// with 8-byte pairs.
template <>
class hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>> {
public:
    enum : int32_t {
        NUM_ELEMS_MASK = 0x7fffffff,
        DONT_DEALLOCATE_FLAG = INT32_MIN,
    };

    struct Pair {
        uint32_t key;
        uint32_t val;
    };

    hkMapBase() : m_elem(nullptr), m_numElems(0), m_hashMod(-1) {}

    // Adds key with val, or replaces its value, allocating from alloc; returns whether the key
    // was new.
    uint32_t insert(hkMemoryAllocator& alloc, uint32_t key, uint32_t val);
    // The value of key, or def.
    uint32_t getWithDefault(uint32_t key, uint32_t def) const;
    // The slot index of key, or a value past m_hashMod when it is absent.
    int32_t findKey(uint32_t key) const;
    // The SDK's remove(K key): drops key's pair (remove at the slot findKey returns) when it is present; the hkResult
    // as int32_t, 1 (failure) when it is absent.
    int32_t remove(uint32_t key);
    // The SDK's remove(Iterator it): drops the pair at slot aiIndex and moves the later pairs
    // of its probe chain up.
    void remove(int32_t aiIndex);
    // The slot of key, adding it with val when it is absent (an existing key keeps its value); the slot index (an
    // Iterator in the SDK).
    int32_t findOrInsertKey(hkMemoryAllocator& alloc, uint32_t key, uint32_t val);
    // Empties the table: every slot's key becomes -1 and the element count is cleared, the DONT_DEALLOCATE flag
    // staying in m_numElems' high bit.
    void clear();
    // Grows the table to hold numElements without rehashing, allocating from alloc; returns an
    // hkResult spelled as int32_t (0 is success).
    int32_t reserve(hkMemoryAllocator& alloc, int32_t numElements);
    // Rebuilds the table with newcap slots (at least 8) from alloc, inserting the old pairs again; the old storage
    // goes back to alloc unless the map does not own it.
    void resizeTable(hkMemoryAllocator& alloc, int32_t newcap);
    // Empties the table and frees its slots through alloc.
    void clearAndDeallocate(hkMemoryAllocator& alloc);
    // Empty operations destructor, which every container destructor calls after clearing its map.
    void destructOperations();
    // The SDK's init: lays the map over apBuffer, aiBytes of it, the map not owning it, and
    // empties every slot.
    void init(void* apBuffer, int32_t aiBytes);
    // The bytes of the table: m_hashMod + 1 pairs.
    int32_t getMemSize() const;
    // get(key, out): writes the value of key into *a2 and returns HK_SUCCESS (0), else returns 1 with *a2 unwritten. It
    // is const because lookups are made through a const map.
    int32_t getWithDefaultStatus(uint32_t a1, uint32_t* a2) const;

    Pair* m_elem;
    int32_t m_numElems;
    int32_t m_hashMod;
};

// The same hash map with 64-bit keys and values (hkpPairCollisionFilter's pairs of collidables): 16-byte pairs, key
// first.
template <>
class hkMapBase<uint64_t, uint64_t, hkMapOperations<uint64_t>> {
public:
    enum : int32_t {
        NUM_ELEMS_MASK = 0x7fffffff,
        DONT_DEALLOCATE_FLAG = INT32_MIN,
    };

    hkMapBase() : m_elem(nullptr), m_numElems(0), m_hashMod(-1) {}

    struct Pair {
        uint64_t key;
        uint64_t val;
    };

    // The bytes a table for aiNumElems keys needs: the next power of two from 8 slots, 16
    // bytes each.
    static int32_t requiredBytes(int32_t aiNumElems);
    // Lays the map over apBuffer, aiBytes of it, emptying every slot.
    void init(void* apBuffer, int32_t aiBytes);
    // The slot index of key, or a value past m_hashMod when it is absent.
    int32_t findKey(uint64_t auiKey) const;
    // The SDK's getWithDefault: the value stored for key, or def when key is absent.
    uint64_t getWithDefault(uint64_t key, uint64_t def) const;
    // Adds key with val, or replaces its value, allocating from alloc.
    uint32_t insert(hkMemoryAllocator& alloc, uint64_t auiKey, uint64_t auiVal);
    // The SDK's remove(K key): drops key's pair (remove at the slot findKey returns) when it is present; the hkResult
    // as int32_t, 1 (failure) when it is absent.
    int32_t remove(uint64_t auiKey);
    // Drops the pair in slot aiIndex.
    void remove(int32_t aiIndex);
    // Empties the table: every slot's key becomes -1 and the element count is cleared, the DONT_DEALLOCATE flag
    // staying in m_numElems' high bit.
    void clear();
    // Frees the table through alloc unless the map does not own it.
    void clearAndDeallocate(hkMemoryAllocator& alloc);
    // Grows the table to hold numElements, allocating from alloc: calls resizeTable with twice numElements, rounded up
    // to a power of two from 8 slots.
    void reserve(hkMemoryAllocator& alloc, int32_t numElements);

protected:
    // Rebuilds the table with newcap slots (at least 8) from alloc, inserting the old pairs again; the old storage goes
    // back to alloc unless the map does not own it.
    void resizeTable(hkMemoryAllocator& alloc, int32_t newcap);

public:
#if XFNV_64BIT
    // The members the pointer map's users call on its storage, which is this map on x64 (the SDK's
    // hkPointerMapStorage<8>). The exe has them on the 32-bit map only; these are the SDK's bodies.
    // The slot of key, adding it with val when it is absent (an existing key keeps its value).
    int32_t findOrInsertKey(hkMemoryAllocator& alloc, uint64_t key, uint64_t val);
    // The SDK's get(key, out): the value of key into *out and HK_SUCCESS (0), else 1 with *out unwritten.
    int32_t getWithDefaultStatus(uint64_t key, uint64_t* out) const;
    // The bytes of the table: m_hashMod + 1 pairs.
    int32_t getMemSize() const;
    // The empty operations destructor the containers call after clearing their map.
    void destructOperations() {}
#endif
    ~hkMapBase();

    Pair* m_elem;
    int32_t m_numElems;
    int32_t m_hashMod;
};

// The map that owns its storage through Allocator. Only the two hkMapBase instances above have a layout.
template <
    typename K,
    typename V,
    typename Operations = hkMapOperations<K>,
    typename Allocator = hkContainerHeapAllocator>
class hkMap : public hkMapBase<K, V, Operations> {
public:
    // The SDK's hkMap(int nelem = 0): the empty map, grown to hold a1 elements.
    hkMap(int32_t a1 = 0) {
        reserve(a1);
    }
    // The SDK's ~hkMap: frees the table through Allocator.
    ~hkMap() {
        this->clearAndDeallocate(Allocator().get(this));
    }

    // Grows the table to hold numElements, allocating through Allocator; returns the hkResult (as int32_t). Each
    // instantiation gets an explicit specialization in hkMap.cpp.
    int32_t reserve(int32_t numElements);
};

// The pointer map's storage: pointer-sized keys and values, the 32-bit map above on 32-bit targets and the 64-bit map
// on x64.
using hkUlongMapBase = hkMapBase<uintptr_t, uintptr_t, hkMapOperations<uintptr_t>>;
using hkUlongMap = hkMap<uintptr_t, uintptr_t, hkMapOperations<uintptr_t>, hkContainerHeapAllocator>;

// hkMap::reserve calls the base's reserve with Allocator's allocator. The 64-bit base's reserve returns nothing, so
// that map reports success (0).
template <typename K, typename V, typename Operations, typename Allocator>
int32_t hkMap<K, V, Operations, Allocator>::reserve(int32_t numElements) {
    using Base = hkMapBase<K, V, Operations>;
    if constexpr (std::is_void_v<decltype(Base::reserve(Allocator().get(this), numElements))>) {
        Base::reserve(Allocator().get(this), numElements);
        return 0;
    }
    else {
        return Base::reserve(Allocator().get(this), numElements);
    }
}
