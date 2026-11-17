#pragma once

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

#include <cstdint>

// The hash and comparison of hkStringMap's keys: an empty policy object. The hybrid build's map members are the
// exe's per-instance copies, which inline both; the native build's generic bodies call them.
struct hkStringMapOperations {
	// A key is a string's address held as an untyped hkUlong (the map stores every key and value as one), so both
	// functions read it back as the string.
	// The SDK's inline equality of two keys (the exe inlines it in findKey and insert): the strings compare equal.
	bool equal(hkUlong k1, hkUlong k2) const {
		return hkString::strCmp(reinterpret_cast<const char*>(k1), reinterpret_cast<const char*>(k2)) == 0;
	}

	// The SDK's inline hash of a key (inlined as equal): 31 * h + c over the string, the top bit cleared so that no
	// hash is the empty slot's.
	hkUlong hash(hkUlong key) const {
		const char* p = reinterpret_cast<const char*>(key);
		hkUlong h = 0;
		for (int32_t i = 0; p[i] != 0; ++i) {
			h = 31 * h + p[i];
		}
		return h & (hkUlong(-1) >> 1);
	}
};

// An open-addressing hash map that stores each key's hash next to it.
template <typename Operations, typename Allocator> class hkCachedHashMap {
public:
	// A slot index, opaque to the map's users. As in the SDK, the elaborated name declares the global class Dummy
	// (the libraries' mangled names carry PAVDummy@@).
	typedef class Dummy* Iterator;

	// The empty map's constructor (SDK: hkCachedHashMap(Operations ops = Operations())): allocates 16 empty slots.
	// The Operations object is empty and passed by value; the dword holding it is the caller's stack word with its
	// low byte cleared, so src passes that word as is.
	// The default argument is the SDK's: an hkStringMap member or local is the empty map.
	hkCachedHashMap([[maybe_unused]] uint32_t auiAllocator = 0)
		: m_elem(static_cast<hkUlong*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(48 *
			  sizeof(hkUlong))))),
		  m_numElems(0), m_hashMod(15) {
		// The 16 slots' hashes empty (all bits set).
		for (int32_t i = 0; i < 16; ++i) {
			m_elem[i] = static_cast<hkUlong>(-1);
		}
	}

	// The SDK's destructor: the slot table freed through clearAndDeallocate (0xD877D0), which the exe's owner
	// destructors call for their maps; the owners' native destructors leave the map to free its own storage.
	~hkCachedHashMap() { clearAndDeallocate(); }

	// The slot holding key, or one past the last slot when the map has no such key. Out of line per instantiation
	// in the exe, like the members below: each instantiation src uses gets an explicit specialization.
	Iterator findKey(hkUlong key) const;
	// Whether it is a slot findKey found. The exe returns the hkBool through a hidden pointer, which isValid takes as
	// its first argument.
	hkBool* isValid(hkBool* arResult, Iterator it) const;
	// The value in slot it.
	hkUlong getValue(Iterator it) const;
	// The value of key into *out: HK_SUCCESS when the map has key, else HK_FAILURE and *out untouched.
	hkResult get(hkUlong key, hkUlong* out) const;
	// The first used slot, and the used slot after it.
	Iterator getIterator() const;
	Iterator getNext(Iterator it) const;
	// Frees the slot table; the map is unusable afterwards.
	void clearAndDeallocate();
	// Marks every slot empty and zeroes the count, keeping the slot table.
	void clear();
	// Adds key with val, or replaces its value.
	void insert(hkUlong key, hkUlong val);
	// Reallocates the slot table with newcap slots and re-inserts every used old slot; insert calls
	// it when the map is half full.
	void resizeTable(int32_t newcap);
	// The value of key; when the map has no such key, adds it with def and returns def.
	hkUlong getOrInsert(hkUlong key, hkUlong ifNotFound);
	// The value of key, or def when the map has no such key.
	hkUlong getWithDefault(hkUlong key, hkUlong def) const;
	// Removes key: HK_SUCCESS when the map had it, else HK_FAILURE.
	hkResult remove(hkUlong key);
	// Empties slot it and rehashes the run of used slots after it (thiscall, ret 4). The exe's eax is read by no
	// caller.
	void remove(Iterator it);
	// The key in slot it.
	hkUlong getKey(Iterator it) const;
	// Replaces the value in slot it with val.
	void setValue(Iterator it, hkUlong val);

	// The slot table, read as words (findKey, 0xD87580, reads the hash at the slot's index).
	hkUlong* m_elem;
	int32_t m_numElems;
	int32_t m_hashMod;
	Operations m_ops;
};
// The SDK's bodies (hkCachedHashMap.cxx). The slot table holds m_hashMod + 1 hashes, then as many keys, then as many
// values; an Iterator is a slot's index stored in the pointer. The table comes from the thread's heap allocator.
template <typename Operations, typename Allocator>
typename hkCachedHashMap<Operations, Allocator>::Iterator hkCachedHashMap<Operations, Allocator>::getIterator() const {
	int32_t i;
	for (i = 0; i <= m_hashMod; ++i) {
		if (m_elem[i] != hkUlong(-1)) {
			break;
		}
	}
	return reinterpret_cast<Iterator>(hkUlong(i));
}

template <typename Operations, typename Allocator>
hkUlong hkCachedHashMap<Operations, Allocator>::getKey(Iterator it) const {
	int32_t i = static_cast<int32_t>(reinterpret_cast<hkUlong>(it));
	return m_elem[i + m_hashMod + 1];
}

template <typename Operations, typename Allocator>
hkUlong hkCachedHashMap<Operations, Allocator>::getValue(Iterator it) const {
	int32_t i = static_cast<int32_t>(reinterpret_cast<hkUlong>(it));
	return m_elem[i + 2 * m_hashMod + 2];
}

template <typename Operations, typename Allocator>
void hkCachedHashMap<Operations, Allocator>::setValue(Iterator it, hkUlong val) {
	int32_t i = static_cast<int32_t>(reinterpret_cast<hkUlong>(it));
	m_elem[i + 2 * m_hashMod + 2] = val;
}

template <typename Operations, typename Allocator>
typename hkCachedHashMap<Operations, Allocator>::Iterator
hkCachedHashMap<Operations, Allocator>::getNext(Iterator it) const {
	int32_t i;
	for (i = static_cast<int32_t>(reinterpret_cast<hkUlong>(it)) + 1; i <= m_hashMod; ++i) {
		if (m_elem[i] != hkUlong(-1)) {
			break;
		}
	}
	return reinterpret_cast<Iterator>(hkUlong(i));
}

// The SDK returns the hkBool; the exe's copy stores it through the hidden result pointer and returns that.
template <typename Operations, typename Allocator>
hkBool* hkCachedHashMap<Operations, Allocator>::isValid(hkBool* arResult, Iterator it) const {
	arResult->m_bool = static_cast<int32_t>(reinterpret_cast<hkUlong>(it)) <= m_hashMod;
	return arResult;
}

template <typename Operations, typename Allocator>
void hkCachedHashMap<Operations, Allocator>::insert(hkUlong key, hkUlong val) {
	hkUlong hash = m_ops.hash(key);
	// Grows the table when it is half full.
	if (m_numElems + m_numElems > m_hashMod) {
		resizeTable(m_hashMod + m_hashMod + 2);
	}
	hkUlong i = hash & m_hashMod;
	while (true) {
		if (m_elem[i] == hkUlong(-1)) {
			m_numElems += 1;
			break;
		}
		if (m_elem[i] != hash || !m_ops.equal(key, m_elem[i + m_hashMod + 1])) {
			i = (i + 1) & m_hashMod;
		} else {
			break;
		}
	}
	m_elem[i] = hash;
	m_elem[i + m_hashMod + 1] = key;
	m_elem[i + 2 * m_hashMod + 2] = val;
}

template <typename Operations, typename Allocator>
typename hkCachedHashMap<Operations, Allocator>::Iterator
hkCachedHashMap<Operations, Allocator>::findKey(hkUlong key) const {
	hkUlong hash = m_ops.hash(key);
	for (hkUlong i = hash & m_hashMod; m_elem[i] != hkUlong(-1); i = (i + 1) & m_hashMod) {
		if (m_elem[i] == hash && m_ops.equal(key, m_elem[i + m_hashMod + 1])) {
			return reinterpret_cast<Iterator>(i);
		}
	}
	return reinterpret_cast<Iterator>(hkUlong(m_hashMod + 1));
}

template <typename Operations, typename Allocator>
hkResult hkCachedHashMap<Operations, Allocator>::get(hkUlong key, hkUlong* out) const {
	Iterator it = findKey(key);
	hkBool valid;
	if (isValid(&valid, it)->m_bool) {
		*out = getValue(it);
		return HK_SUCCESS;
	}
	return HK_FAILURE;
}

template <typename Operations, typename Allocator>
hkUlong hkCachedHashMap<Operations, Allocator>::getOrInsert(hkUlong key, hkUlong ifNotFound) {
	Iterator it = findKey(key);
	hkBool valid;
	if (isValid(&valid, it)->m_bool) {
		return getValue(it);
	}
	insert(key, ifNotFound);
	return ifNotFound;
}

// get leaves def untouched when the map has no such key.
template <typename Operations, typename Allocator>
hkUlong hkCachedHashMap<Operations, Allocator>::getWithDefault(hkUlong key, hkUlong def) const {
	get(key, &def);
	return def;
}

// Empties slot it, then moves each later slot of the run whose home slot lies at or before the hole into the hole,
// so that every key stays reachable from its home slot.
template <typename Operations, typename Allocator> void hkCachedHashMap<Operations, Allocator>::remove(Iterator it) {
	hkUlong i = reinterpret_cast<hkUlong>(it);
	--m_numElems;
	m_elem[i] = hkUlong(-1);
	// The first slot of the run holding i.
	hkUlong lo = (i + m_hashMod) & m_hashMod;
	while (m_elem[lo] != hkUlong(-1)) {
		lo = (lo + m_hashMod) & m_hashMod;
	}
	lo = (lo + 1) & m_hashMod;
	hkUlong empty = i;
	for (i = (i + 1) & m_hashMod; m_elem[i] != hkUlong(-1); i = (i + 1) & m_hashMod) {
		hkUlong hmod = m_elem[i] & m_hashMod;
		if (i >= lo && hmod > empty) {
			continue;
		} else if (i < empty && (hmod > empty || hmod <= i)) {
			continue;
		} else if (hmod > empty && hmod < lo) {
			continue;
		}
		m_elem[empty] = m_elem[i];
		m_elem[empty + m_hashMod + 1] = m_elem[i + m_hashMod + 1];
		m_elem[empty + 2 * m_hashMod + 2] = m_elem[i + 2 * m_hashMod + 2];
		m_elem[i] = hkUlong(-1);
		empty = i;
	}
}

template <typename Operations, typename Allocator>
hkResult hkCachedHashMap<Operations, Allocator>::remove(hkUlong key) {
	Iterator it = findKey(key);
	hkBool valid;
	if (isValid(&valid, it)->m_bool) {
		remove(it);
		return HK_SUCCESS;
	}
	return HK_FAILURE;
}

// The table gets newcap slots, its hashes empty, and every used old slot is inserted again.
template <typename Operations, typename Allocator>
void hkCachedHashMap<Operations, Allocator>::resizeTable(int32_t newcap) {
	int32_t oldcap = m_hashMod + 1;
	hkUlong* oldelem = m_elem;
	hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
	m_elem = static_cast<hkUlong*>(heap->blockAlloc(static_cast<int32_t>(3 * sizeof(hkUlong)) * newcap));
	hkString::memSet(m_elem, 0xFF, static_cast<int32_t>(sizeof(hkUlong)) * newcap);
	m_numElems = 0;
	m_hashMod = newcap - 1;
	for (int32_t i = 0; i < oldcap; ++i) {
		if (oldelem[i] != hkUlong(-1)) {
			insert(oldelem[i + oldcap], oldelem[i + oldcap + oldcap]);
		}
	}
	heap->blockFree(oldelem, static_cast<int32_t>(3 * sizeof(hkUlong)) * oldcap);
}

// Marks every slot empty and zeroes the count, keeping the table.
template <typename Operations, typename Allocator> void hkCachedHashMap<Operations, Allocator>::clear() {
	for (int32_t i = 0; i < m_hashMod + 1; ++i) {
		m_elem[i] = hkUlong(-1);
	}
	m_numElems = 0;
}

// Frees the slot table.
template <typename Operations, typename Allocator> void hkCachedHashMap<Operations, Allocator>::clearAndDeallocate() {
	hkMemoryRouter::getInstance().m_heap->blockFree(m_elem,
		static_cast<int32_t>(3 * sizeof(hkUlong)) * (m_hashMod + 1));
}
