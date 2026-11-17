#pragma once

#include <cstddef>
#include <stdint.h>

class BGSImpactData;
class ScrapHeap;
class TESFaction;
class TESObjectREFR;

// The engine's heap entry points (BSCore/MemoryManager.h, which includes this header).
void* ourMalloc(uint32_t aSize);
void ourFree(void* apPtr);

// One entry of a BSMap bucket's singly linked chain: 0xC bytes (the derived map's NewItem allocates 0xC).
template <class TKEY, class TVAL> class BSMapItem {
public:
	BSMapItem* m_pkNext;
	TKEY m_key;
	TVAL m_val;
};

// The engine's hash map whose items the derived class allocates (BSMap from the heap, BSScrapMap from a scrap heap).
// Its virtual functions are NiTMapBase's: the hash, key test and SetValue bodies are shared with NiTMap, ClearValue
// does nothing, and NewItem and DeleteItem are pure. Only some of the members are declared.
template <class TKEY, class TVAL> class BSMapBase {
public:
	BSMapBase(uint32_t uiHashSize);
	virtual ~BSMapBase();

	virtual uint32_t KeyToHashIndex(TKEY key) const;
	virtual bool IsKeysEqual(TKEY key1, TKEY key2) const;
	virtual void SetValue(BSMapItem<TKEY, TVAL>* pkItem, TKEY key, TVAL val);
	virtual void ClearValue(BSMapItem<TKEY, TVAL>*) {}
	virtual BSMapItem<TKEY, TVAL>* NewItem() = 0;
	virtual void DeleteItem(BSMapItem<TKEY, TVAL>* pkItem) = 0;

	// SetAt sets the value of key, adding an item when the key is new; GetAt gets the value of key, false when it is
	// not in the map.
	void SetAt(TKEY key, TVAL val);
	bool GetAt(TKEY key, TVAL& val) const;
	// Iteration is defined in the specializations below. GetFirstPos returns the first item, null for an empty map;
	// GetNext advances pos to the item after it, with pos's key and value.
	BSMapItem<TKEY, TVAL>* GetFirstPos() const;
	void GetNext(BSMapItem<TKEY, TVAL>*& pos, TKEY& key, TVAL& val) const;
	// Removes key's item; false when key is not in the map. See the specializations below.
	bool RemoveAt(TKEY key);

	void RemoveAll_1() { RemoveAll(); }

	// RemoveAll of the scrap-heap map (same layout as NiTMapBase, whose RemoveAll it shares), which its destructor
	// calls.
	void RemoveAll_0() { RemoveAll(); }

	// NiTMapBase::RemoveAll over this layout: unlinks each item from its bucket, clears and deletes it, then empties
	// the count. The native side of RemoveAll_0 and RemoveAll_1.
	void RemoveAll();

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	uint32_t m_uiHashSize;
	BSMapItem<TKEY, TVAL>** m_ppkHashTable;
	uint32_t m_uiCount;
};
// The heap-allocated map: NewItem takes an item from the heap, DeleteItem clears its value and frees it. Overrides the
// destructor and the two item handlers; no fields of its own.
template <class TKEY, class TVAL> class BSMap : public BSMapBase<TKEY, TVAL> {
public:
	BSMap(uint32_t auiHashSize);
	virtual ~BSMap();

	virtual BSMapItem<TKEY, TVAL>* NewItem();
	virtual void DeleteItem(BSMapItem<TKEY, TVAL>* pkItem);
};

// The hash table of uiHashSize null buckets; none for a size of 0. The thread scrap heap map's instance takes its
// table from MemoryManager::Allocate where the others use ourMalloc: one native body for all.
template <class TKEY, class TVAL>
BSMapBase<TKEY, TVAL>::BSMapBase(uint32_t uiHashSize) : m_uiHashSize(uiHashSize), m_uiCount(0) {
	if (uiHashSize) {
		m_ppkHashTable = static_cast<BSMapItem<TKEY, TVAL>**>(ourMalloc(sizeof(BSMapItem<TKEY, TVAL>*) * uiHashSize));
		for (uint32_t i = 0; i < uiHashSize; ++i) {
			m_ppkHashTable[i] = nullptr;
		}
	}
}

template <class TKEY, class TVAL> BSMapBase<TKEY, TVAL>::~BSMapBase() {
	if (m_ppkHashTable) {
		ourFree(m_ppkHashTable);
	}
}

template <class TKEY, class TVAL> void BSMapBase<TKEY, TVAL>::RemoveAll() {
	for (uint32_t i = 0; i < m_uiHashSize; ++i) {
		while (m_ppkHashTable[i]) {
			BSMapItem<TKEY, TVAL>* pkItem = m_ppkHashTable[i];
			m_ppkHashTable[i] = pkItem->m_pkNext;
			ClearValue(pkItem);
			DeleteItem(pkItem);
		}
	}
	m_uiCount = 0;
}

// A key's bucket: the key's low 32 bits (a pointer key's address) modulo the table size, with an unsigned division.
template <class TKEY, class TVAL> uint32_t BSMapBase<TKEY, TVAL>::KeyToHashIndex(TKEY key) const {
	return static_cast<uint32_t>((size_t)key) % m_uiHashSize;
}

template <class TKEY, class TVAL> bool BSMapBase<TKEY, TVAL>::IsKeysEqual(TKEY key1, TKEY key2) const {
	return key1 == key2;
}

template <class TKEY, class TVAL>
void BSMapBase<TKEY, TVAL>::SetValue(BSMapItem<TKEY, TVAL>* pkItem, TKEY key, TVAL val) {
	pkItem->m_key = key;
	pkItem->m_val = val;
}

// An existing key's item has its value cleared and set again; a new key gets a new item at the head of its bucket. The
// searched key is passed first to IsKeysEqual.
template <class TKEY, class TVAL> void BSMapBase<TKEY, TVAL>::SetAt(TKEY key, TVAL val) {
	uint32_t uiIndex = KeyToHashIndex(key);
	for (BSMapItem<TKEY, TVAL>* pkItem = m_ppkHashTable[uiIndex]; pkItem; pkItem = pkItem->m_pkNext) {
		if (IsKeysEqual(key, pkItem->m_key)) {
			ClearValue(pkItem);
			SetValue(pkItem, key, val);
			return;
		}
	}
	BSMapItem<TKEY, TVAL>* pkItem = NewItem();
	SetValue(pkItem, key, val);
	pkItem->m_pkNext = m_ppkHashTable[uiIndex];
	m_ppkHashTable[uiIndex] = pkItem;
	++m_uiCount;
}

template <class TKEY, class TVAL> bool BSMapBase<TKEY, TVAL>::GetAt(TKEY key, TVAL& val) const {
	for (BSMapItem<TKEY, TVAL>* pkItem = m_ppkHashTable[KeyToHashIndex(key)]; pkItem; pkItem = pkItem->m_pkNext) {
		if (IsKeysEqual(key, pkItem->m_key)) {
			val = pkItem->m_val;
			return true;
		}
	}
	return false;
}

// The first item of the first non-empty bucket.
template <class TKEY, class TVAL> BSMapItem<TKEY, TVAL>* BSMapBase<TKEY, TVAL>::GetFirstPos() const {
	for (uint32_t i = 0; i < m_uiHashSize; ++i) {
		if (m_ppkHashTable[i]) {
			return m_ppkHashTable[i];
		}
	}
	return nullptr;
}

// pos's key and value out; pos moves to the next item in its chain, else to the head of the next non-empty bucket,
// else to null.
template <class TKEY, class TVAL>
void BSMapBase<TKEY, TVAL>::GetNext(BSMapItem<TKEY, TVAL>*& pos, TKEY& key, TVAL& val) const {
	BSMapItem<TKEY, TVAL>* pkItem = pos;
	key = pkItem->m_key;
	val = pkItem->m_val;
	if (pkItem->m_pkNext) {
		pos = pkItem->m_pkNext;
		return;
	}
	for (uint32_t i = KeyToHashIndex(pkItem->m_key) + 1; i < m_uiHashSize; ++i) {
		pkItem = m_ppkHashTable[i];
		if (pkItem) {
			pos = pkItem;
			return;
		}
	}
	pos = nullptr;
}

// Unlinks key's item, clears its value and deletes it.
template <class TKEY, class TVAL> bool BSMapBase<TKEY, TVAL>::RemoveAt(TKEY key) {
	uint32_t uiIndex = KeyToHashIndex(key);
	BSMapItem<TKEY, TVAL>* pkItem = m_ppkHashTable[uiIndex];
	if (!pkItem) {
		return false;
	}
	if (IsKeysEqual(key, pkItem->m_key)) {
		m_ppkHashTable[uiIndex] = pkItem->m_pkNext;
		ClearValue(pkItem);
		DeleteItem(pkItem);
		--m_uiCount;
		return true;
	}
	BSMapItem<TKEY, TVAL>* pkPrev = pkItem;
	BSMapItem<TKEY, TVAL>* pkCurr = pkPrev->m_pkNext;
	while (pkCurr && !IsKeysEqual(key, pkCurr->m_key)) {
		pkPrev = pkCurr;
		pkCurr = pkCurr->m_pkNext;
	}
	if (!pkCurr) {
		return false;
	}
	pkPrev->m_pkNext = pkCurr->m_pkNext;
	ClearValue(pkCurr);
	DeleteItem(pkCurr);
	--m_uiCount;
	return true;
}

template <class TKEY, class TVAL> BSMap<TKEY, TVAL>::BSMap(uint32_t auiHashSize) : BSMapBase<TKEY, TVAL>(auiHashSize) {}

// The items go back through the map's DeleteItem.
template <class TKEY, class TVAL> BSMap<TKEY, TVAL>::~BSMap() {
	this->RemoveAll_1();
}

// A 0xC-byte item from the heap (the thread scrap heap map's instance calls MemoryManager::Allocate, the same heap).
template <class TKEY, class TVAL> BSMapItem<TKEY, TVAL>* BSMap<TKEY, TVAL>::NewItem() {
	return static_cast<BSMapItem<TKEY, TVAL>*>(ourMalloc(sizeof(BSMapItem<TKEY, TVAL>)));
}

// The value cleared, then the item back to the heap.
template <class TKEY, class TVAL> void BSMap<TKEY, TVAL>::DeleteItem(BSMapItem<TKEY, TVAL>* pkItem) {
	pkItem->m_val = 0;
	ourFree(pkItem);
}
