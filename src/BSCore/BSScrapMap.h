#pragma once

#include "BSCore/BSMap.h"
#include "BSCore/ScrapHeap.h"

#include <stdint.h>

// A BSMapBase whose items come from its own scrap heap; used for short-lived maps on the stack. Only some of its
// members are declared.
template <class TKEY, class TVAL> class BSScrapMap : public BSMapBase<TKEY, TVAL> {
public:
	BSScrapMap(uint32_t uiHashSize, bool abClearOnDestruct);
	virtual ~BSScrapMap();

	virtual BSMapItem<TKEY, TVAL>* NewItem();
	virtual void DeleteItem(BSMapItem<TKEY, TVAL>* pkItem);

	// Clears every item's value in every bucket (the map's ClearValue) and zeroes the count; the destructor calls it
	// when bClearOnDestruct is set.
	void ClearAll();

	bool bClearOnDestruct;
	ScrapHeap MapHeap;
};

// An empty base map, then uiHashSize null buckets from the map's own scrap heap.
template <class TKEY, class TVAL>
BSScrapMap<TKEY, TVAL>::BSScrapMap(uint32_t uiHashSize, bool abClearOnDestruct)
	: BSMapBase<TKEY, TVAL>(0), bClearOnDestruct(abClearOnDestruct) {
	this->m_uiHashSize = uiHashSize;
	this->m_ppkHashTable =
		static_cast<BSMapItem<TKEY, TVAL>**>(MapHeap.Allocate(sizeof(BSMapItem<TKEY, TVAL>*) * uiHashSize,
			ScrapHeap::ALIGNMENT));
	for (uint32_t i = 0; i < uiHashSize; ++i) {
		this->m_ppkHashTable[i] = nullptr;
	}
}

// The table lives in MapHeap, freed with it: the base must not free it.
template <class TKEY, class TVAL> BSScrapMap<TKEY, TVAL>::~BSScrapMap() {
	if (bClearOnDestruct) {
		ClearAll();
	}
	this->m_ppkHashTable = nullptr;
	this->m_uiCount = 0;
	this->m_uiHashSize = 0;
}

// A 0xC-byte item from the map's own scrap heap.
template <class TKEY, class TVAL> BSMapItem<TKEY, TVAL>* BSScrapMap<TKEY, TVAL>::NewItem() {
	return static_cast<BSMapItem<TKEY, TVAL>*>(MapHeap.Allocate(sizeof(BSMapItem<TKEY, TVAL>), ScrapHeap::ALIGNMENT));
}

// The value cleared, then the item back to the map's scrap heap.
template <class TKEY, class TVAL> void BSScrapMap<TKEY, TVAL>::DeleteItem(BSMapItem<TKEY, TVAL>* pkItem) {
	pkItem->m_val = 0;
	MapHeap.Deallocate(pkItem);
}

template <class TKEY, class TVAL> void BSScrapMap<TKEY, TVAL>::ClearAll() {
	for (uint32_t i = 0; i < this->m_uiHashSize; ++i) {
		for (BSMapItem<TKEY, TVAL>* pkItem = this->m_ppkHashTable[i]; pkItem; pkItem = pkItem->m_pkNext) {
			this->ClearValue(pkItem);
		}
	}
	this->m_uiCount = 0;
}
