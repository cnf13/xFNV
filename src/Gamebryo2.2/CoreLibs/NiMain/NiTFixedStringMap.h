#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// Hash map keyed by NiFixedString: hashes the shared string's address. Its own base, not an NiTMapBase; only the
// item handling is virtual.
template <class Allocator, class TVAL> class NiTFixedStringMapBase : public NiMemObject {
public:
	struct AntiBloatAllocator : public Allocator {
		uint32_t m_uiCount;
	};

	// uiHashSize cleared buckets, no items (Gamebryo's default 37 buckets).
	NiTFixedStringMapBase(uint32_t uiHashSize = 37) : m_uiHashSize(uiHashSize) {
		m_kAllocator.m_uiCount = 0;
		m_ppkHashTable =
			static_cast<NiTMapItem<NiFixedString, TVAL>**>(_NiMalloc(sizeof(NiTMapItem<NiFixedString, TVAL>*) *
				m_uiHashSize));
		std::memset(m_ppkHashTable, 0, sizeof(NiTMapItem<NiFixedString, TVAL>*) * m_uiHashSize);
	}

	virtual ~NiTFixedStringMapBase();

	virtual NiTMapItem<NiFixedString, TVAL>* NewItem() = 0;
	virtual void DeleteItem(NiTMapItem<NiFixedString, TVAL>* pkItem) = 0;

	uint32_t GetCount() const;
	bool IsEmpty() const;
	void SetAt(const NiFixedString& kKey, TVAL val);
	bool RemoveAt(const NiFixedString& kKey);
	void RemoveAll();
	bool GetAt(const NiFixedString& kKey, TVAL& val) const;
	NiTMapItem<NiFixedString, TVAL>* GetFirstPos() const;
	void GetNext(NiTMapItem<NiFixedString, TVAL>*& pos, NiFixedString& kKey, TVAL& val) const;
	void SetValue(NiTMapItem<NiFixedString, TVAL>* pkItem, const NiFixedString& kKey, TVAL val);

	// The deleting destructors free the map through MemoryManager::Deallocate.
	static void operator delete(void* apMemory) { MemoryManager_Singleton.Deallocate(apMemory); }

	uint32_t m_uiHashSize;
	NiTMapItem<NiFixedString, TVAL>** m_ppkHashTable;
	AntiBloatAllocator m_kAllocator;
};

// Empties the table (the derived map has already) and frees the buckets.
template <class Allocator, class TVAL> NiTFixedStringMapBase<Allocator, TVAL>::~NiTFixedStringMapBase() {
	RemoveAll();
	_NiFree(m_ppkHashTable);
}

// The value stored under kKey, if any. The bucket is the handle's address modulo the table size.
template <class Allocator, class TVAL>
bool NiTFixedStringMapBase<Allocator, TVAL>::GetAt(const NiFixedString& kKey, TVAL& val) const {
	uint32_t uiIndex = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(kKey.GetHandle()) % m_uiHashSize);
	for (NiTMapItem<NiFixedString, TVAL>* pkItem = m_ppkHashTable[uiIndex]; pkItem; pkItem = pkItem->m_pkNext) {
		if (kKey.GetHandle() == pkItem->m_key.GetHandle()) {
			val = pkItem->m_val;
			return true;
		}
	}
	return false;
}

// pos's key and value, then pos moves on to the next item: its bucket's next, else the first of a later bucket.
template <class Allocator, class TVAL>
void NiTFixedStringMapBase<Allocator, TVAL>::GetNext(NiTMapItem<NiFixedString, TVAL>*& pos, NiFixedString& kKey,
	TVAL& val) const {
	kKey = pos->m_key;
	val = pos->m_val;
	if (pos->m_pkNext) {
		pos = pos->m_pkNext;
		return;
	}
	uint32_t ui = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pos->m_key.GetHandle()) % m_uiHashSize) + 1;
	for (; ui < m_uiHashSize; ++ui) {
		if (m_ppkHashTable[ui]) {
			pos = m_ppkHashTable[ui];
			return;
		}
	}
	pos = nullptr;
}

// The first item of the first non-empty bucket, or null.
template <class Allocator, class TVAL>
NiTMapItem<NiFixedString, TVAL>* NiTFixedStringMapBase<Allocator, TVAL>::GetFirstPos() const {
	for (uint32_t ui = 0; ui < m_uiHashSize; ++ui) {
		if (m_ppkHashTable[ui]) {
			return m_ppkHashTable[ui];
		}
	}
	return nullptr;
}

// The item's key (NiFixedString's assignment: the reference taken and the old one dropped) and value.
template <class Allocator, class TVAL>
void NiTFixedStringMapBase<Allocator, TVAL>::SetValue(NiTMapItem<NiFixedString, TVAL>* pkItem,
	const NiFixedString& kKey, TVAL val) {
	pkItem->m_key = kKey;
	pkItem->m_val = val;
}

// An existing key gets the new value in place; a new key gets a new item at the head of its bucket.
template <class Allocator, class TVAL>
void NiTFixedStringMapBase<Allocator, TVAL>::SetAt(const NiFixedString& kKey, TVAL val) {
	uint32_t uiIndex = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(kKey.GetHandle()) % m_uiHashSize);
	NiTMapItem<NiFixedString, TVAL>* pkItem = m_ppkHashTable[uiIndex];
	while (pkItem) {
		if (kKey.GetHandle() == pkItem->m_key.GetHandle()) {
			SetValue(pkItem, kKey, val);
			return;
		}
		pkItem = pkItem->m_pkNext;
	}

	pkItem = NewItem();
	SetValue(pkItem, kKey, val);
	pkItem->m_pkNext = m_ppkHashTable[uiIndex];
	m_ppkHashTable[uiIndex] = pkItem;
	++m_kAllocator.m_uiCount;
}

// Walks the bucket through the link that points at the current item (the bucket slot, then the previous item's
// m_pkNext), unlinks the match, deletes it through DeleteItem and drops the count.
template <class Allocator, class TVAL>
bool NiTFixedStringMapBase<Allocator, TVAL>::RemoveAt(const NiFixedString& kKey) {
	uint32_t uiIndex = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(kKey.GetHandle()) % m_uiHashSize);
	NiTMapItem<NiFixedString, TVAL>** ppkLink = &m_ppkHashTable[uiIndex];
	NiTMapItem<NiFixedString, TVAL>* pkCurr = *ppkLink;
	while (pkCurr) {
		if (kKey.GetHandle() == pkCurr->m_key.GetHandle()) {
			*ppkLink = pkCurr->m_pkNext;
			DeleteItem(pkCurr);
			--m_kAllocator.m_uiCount;
			return true;
		}
		ppkLink = &pkCurr->m_pkNext;
		pkCurr = *ppkLink;
	}
	return false;
}

// Deletes every item through DeleteItem, bucket by bucket, then zeroes the count.
template <class Allocator, class TVAL> void NiTFixedStringMapBase<Allocator, TVAL>::RemoveAll() {
	for (uint32_t ui = 0; ui < m_uiHashSize; ++ui) {
		while (m_ppkHashTable[ui]) {
			NiTMapItem<NiFixedString, TVAL>* pkSave = m_ppkHashTable[ui];
			m_ppkHashTable[ui] = pkSave->m_pkNext;
			DeleteItem(pkSave);
		}
	}
	m_kAllocator.m_uiCount = 0;
}

template <class TVAL>
class NiTFixedStringMap : public NiTFixedStringMapBase<DFALL<NiTMapItem<NiFixedString, TVAL>>, TVAL> {
public:
	NiTFixedStringMap(uint32_t uiHashSize = 37)
		: NiTFixedStringMapBase<DFALL<NiTMapItem<NiFixedString, TVAL>>, TVAL>(uiHashSize) {}

	~NiTFixedStringMap() override;

	NiTMapItem<NiFixedString, TVAL>* NewItem() override;
	void DeleteItem(NiTMapItem<NiFixedString, TVAL>* pkItem) override;
};

inline constexpr fnvd::Vtable NiTFixedStringMapBase_DFALL_NiTMapItem_NiFixedString_NiAVObject_NiAVObject_vftable{};
inline constexpr fnvd::Vtable NiTFixedStringMapBase_DFALL_NiTMapItem_NiFixedString_char_char_vftable{};
inline constexpr fnvd::Vtable
	NiTFixedStringMapBase_DFALL_NiTMapItem_NiFixedString_NiGlobalConstantEntry_NiGlobalConstantEntry_vftable{};
inline constexpr fnvd::Vtable
	NiTFixedStringMapBase_DFALL_NiTMapItem_NiFixedString_NiPointer_NiGPUProgramCache_NiGPUProgramDesc_NiPointer_NiGPUProgramCache_NiGPUProgramDesc_vftable{};
inline constexpr fnvd::Vtable NiTFixedStringMap_NiAVObject_vftable{};
inline constexpr fnvd::Vtable NiTFixedStringMap_char_vftable{};
inline constexpr fnvd::Vtable NiTFixedStringMap_NiGlobalConstantEntry_vftable{};
inline constexpr fnvd::Vtable NiTFixedStringMap_NiPointer_NiGPUProgramCache_NiGPUProgramDesc_vftable{};

// Empties the map while its own DeleteItem is in place.
template <class TVAL> NiTFixedStringMap<TVAL>::~NiTFixedStringMap() {
	this->RemoveAll();
}

// An item from the engine heap (DFALL's Allocate), its key constructed as the null string.
template <class TVAL> NiTMapItem<NiFixedString, TVAL>* NiTFixedStringMap<TVAL>::NewItem() {
	return new (MemoryManager_Singleton.Allocate(sizeof(NiTMapItem<NiFixedString, TVAL>)))
		NiTMapItem<NiFixedString, TVAL>;
}

// Clears the key (its reference dropped) and the value, destroys the item and gives it back to the heap.
template <class TVAL> void NiTFixedStringMap<TVAL>::DeleteItem(NiTMapItem<NiFixedString, TVAL>* pkItem) {
	using Item = NiTMapItem<NiFixedString, TVAL>;
	pkItem->m_key = static_cast<const char*>(nullptr);
	pkItem->m_val = TVAL();
	pkItem->~Item();
	MemoryManager_Singleton.Deallocate(pkItem);
}
