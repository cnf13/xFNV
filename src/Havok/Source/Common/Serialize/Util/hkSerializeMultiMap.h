#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Reconstruction.hpp"

#include <cstdint>
#include <stdint.h>

// A map from a key to several values: each key's values form a chain through m_valueChain, and m_indexMap holds
// the index of each key's first value.
template <typename K, typename V> class hkSerializeMultiMap {
public:
	// One value and the index of the next value of the same key, or -1.
	struct Value {
		// The SDK's: plain data when V is (hkTypeTraits.h).
		HK_DECLARE_POD_TYPE_IF_POD(V);

		V value;
		int32_t next;
	};

	hkSerializeMultiMap() : m_freeChainStart(-1) {}
	// The chain and the index map free their storage themselves.
	~hkSerializeMultiMap() {}

	// Adds v to k's values; returns the index map's insert result (the SDK's hkBool32: whether k was new). Out of
	// line per instance in the exe: each instance src uses gets an explicit specialization.
	int32_t insert(K k, const V& value);
	// The index of k's first value in m_valueChain, or -1 when k has none. Out of line in the exe, one body for
	// every instance: each instance src uses gets an explicit specialization.
	int32_t find(K k) const;
	// Drops k's values and its index. Out of line in the exe, one body for every instance.
	void remove(K k);
	// Empties the chain and the index map. Out of line in the exe, one body for every instance.
	void clear();
	// Drops the value at removeIndex from k's chain, moving the next value of the chain into its place when there is
	// one; the index the removed value's place holds afterwards, or -1 when it ended k's chain. Out of line per
	// instance in the exe: each instance src uses gets an explicit specialization.
	int32_t removeByIndex(K key, int32_t removeIndex);
	// Drops the first value of k equal to v. Out of line per instance in the exe: each instance src uses gets an
	// explicit specialization.
	void removeByValue(K key, const V& v);

protected:
	// The index of an unused entry of m_valueChain: the free chain's first, else a new entry at the end. Out of
	// line in the exe, one body for every instance whose Value has 8 bytes.
	int32_t getFreeIndex();

public:
	hkArray<Value> m_valueChain;
	hkPointerMap<K, int32_t> m_indexMap;
	// The first unused entry of m_valueChain, or -1.
	int32_t m_freeChainStart;
};
// The SDK's bodies (hkSerializeMultiMap.inl). The exe's copies differ only by K and V: find 0xDC8E30, remove
// 0xDC9030, clear 0xDC8FC0, removeByIndex 0xDCB4B0, removeByValue 0xDC8FE0, insert 0xDC9EC0, getFreeIndex 0xDBE710.
template <typename K, typename V> int32_t hkSerializeMultiMap<K, V>::getFreeIndex() {
	int32_t i;
	if (m_freeChainStart != -1) {
		i = m_freeChainStart;
		m_freeChainStart = m_valueChain.m_data[m_freeChainStart].next;
	} else {
		i = m_valueChain.m_size;
		m_valueChain.expandOne();
	}
	return i;
}

template <typename K, typename V> int32_t hkSerializeMultiMap<K, V>::insert(K k, const V& value) {
	int32_t chainIndex = m_indexMap.getWithDefault(k, -1);
	int32_t thisIndex = getFreeIndex();
	Value& ref = m_valueChain.m_data[thisIndex];
	ref.value = value;
	ref.next = chainIndex;
	return m_indexMap.insert(k, thisIndex);
}

template <typename K, typename V> int32_t hkSerializeMultiMap<K, V>::find(K k) const {
	return m_indexMap.getWithDefault(k, -1);
}

// The SDK's removeKey: k's chain is put at the head of the free chain.
template <typename K, typename V> void hkSerializeMultiMap<K, V>::remove(K k) {
	int32_t it = m_indexMap.findKey(k);
	int32_t start = m_indexMap.getValue(it);
	m_indexMap.m_map.remove(it);
	if (start != -1) {
		int32_t end = start;
		while (m_valueChain.m_data[end].next != -1) {
			end = m_valueChain.m_data[end].next;
		}
		m_valueChain.m_data[end].next = m_freeChainStart;
		m_freeChainStart = start;
	}
}

template <typename K, typename V> void hkSerializeMultiMap<K, V>::clear() {
	m_valueChain.clear();
	m_indexMap.m_map.clear();
	m_freeChainStart = -1;
}

template <typename K, typename V> int32_t hkSerializeMultiMap<K, V>::removeByIndex(K key, int32_t removeIndex) {
	int32_t nextIndex = m_valueChain.m_data[removeIndex].next;
	int32_t freeIndex = nextIndex;
	if (nextIndex != -1) {
		// The next value moves into the removed one's place; its entry is freed.
		m_valueChain.m_data[removeIndex] = m_valueChain.m_data[nextIndex];
		nextIndex = removeIndex;
	} else {
		int32_t it = m_indexMap.findKey(key);
		int32_t headIndex = m_indexMap.getValue(it);
		if (headIndex == removeIndex) {
			m_indexMap.m_map.m_elem[it].val = static_cast<typename hkPointerMap<K, int32_t>::Storage>(-1);
			freeIndex = removeIndex;
		} else if (m_valueChain.m_data[headIndex].next == removeIndex) {
			m_valueChain.m_data[headIndex].next = -1;
			freeIndex = removeIndex;
		} else {
			// The head's value moves into the removed one's place and the head's entry is freed.
			m_indexMap.m_map.m_elem[it].val =
				static_cast<typename hkPointerMap<K, int32_t>::Storage>(m_valueChain.m_data[headIndex].next);
			m_valueChain.m_data[removeIndex].value = m_valueChain.m_data[headIndex].value;
			freeIndex = headIndex;
		}
	}
	m_valueChain.m_data[freeIndex].next = m_freeChainStart;
	m_freeChainStart = freeIndex;
	return nextIndex;
}

template <typename K, typename V> void hkSerializeMultiMap<K, V>::removeByValue(K key, const V& v) {
	for (int32_t i = find(key); i != -1; i = m_valueChain.m_data[i].next) {
		if (m_valueChain.m_data[i].value == v) {
			removeByIndex(key, i);
			return;
		}
	}
}
