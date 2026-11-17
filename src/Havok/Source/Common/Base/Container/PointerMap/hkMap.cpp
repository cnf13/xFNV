#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"

#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"

using hkMapBase_lunsigned_sint_cunsigned_sint_chkMapOperations_lunsigned_sint =
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>;

using hkMapBase_lunsigned_sint_cunsigned_sint_chkMapOperations_lunsigned_sint =
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>;

using hkMapBase_lunsigned_sint_cunsigned_sint_chkMapOperations_lunsigned_sint =
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>;

using hkMapBase_unsigned_int_unsigned_int_hkMapOperations_unsigned_int =
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>;

using hkMapBase_lunsigned_sint_cunsigned_sint_chkMapOperations_lunsigned_sint =
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>;

using hkMapBase_lunsigned_sint_cunsigned_sint_chkMapOperations_lunsigned_sint =
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>;
// The 64-bit-key map, used by the broad phase dispatcher's pair table. Its table can also sit in a buffer the caller
// hands in, which is why init and requiredBytes are here.
using hkPairMapBase = hkMapBase<uint64_t, uint64_t, hkMapOperations<uint64_t>>;

#if FNVD_64BIT
// The SDK's findOrInsertKey: the table grows first when it is half full; the probe walks from the key's hash to its
// slot (kept with its value) or the first empty one, where the key goes in with val.
int32_t hkPairMapBase::findOrInsertKey(hkMemoryAllocator& alloc, uint64_t key, uint64_t val) {
	if (2 * (m_numElems & NUM_ELEMS_MASK) > m_hashMod) {
		resizeTable(alloc, 2 * m_hashMod + 2);
	}
	uint32_t i = (static_cast<uint32_t>(key) >> 4) * 0x9E3779B1u & static_cast<uint32_t>(m_hashMod);
	while (m_elem[i].key != 0xFFFFFFFFFFFFFFFFull) {
		if (m_elem[i].key == key) {
			return static_cast<int32_t>(i);
		}
		i = (i + 1) & static_cast<uint32_t>(m_hashMod);
	}
	m_elem[i].key = key;
	m_elem[i].val = val;
	++m_numElems;
	return static_cast<int32_t>(i);
}

// The SDK's get: the value of key into *out (HK_SUCCESS), else HK_FAILURE.
int32_t hkPairMapBase::getWithDefaultStatus(uint64_t key, uint64_t* out) const {
	int32_t it = findKey(key);
	if (it > m_hashMod) {
		return 1;
	}
	*out = m_elem[it].val;
	return 0;
}

int32_t hkPairMapBase::getMemSize() const {
	return static_cast<int32_t>(sizeof(Pair)) * (m_hashMod + 1);
}
#endif
using hkPointerMapBase = hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>;

using hkMapBase_lunsigned_sint_cunsigned_sint_chkMapOperations_lunsigned_sint =
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>;

using hkMapBase_unsigned_int_unsigned_int_hkMapOperations_unsigned_int =
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>;

using hkMapBase_lunsigned_sint_cunsigned_sint_chkMapOperations_lunsigned_sint =
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>;

using hkMapBase_lunsigned_sint_cunsigned_sint_chkMapOperations_lunsigned_sint =
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>;

using hkMapBase_lunsigned_sint_cunsigned_sint_chkMapOperations_lunsigned_sint =
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>;

using hkMapBase_lunsigned_sint_cunsigned_sint_chkMapOperations_lunsigned_sint =
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>;

uint32_t hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::insert(hkMemoryAllocator& alloc, uint32_t key,
	uint32_t val) {
	int32_t m_hashMod = this->m_hashMod;
	if (2 * m_numElems > m_hashMod) {
		resizeTable(alloc, 2 * m_hashMod + 2);
	}
	int32_t v6 = this->m_hashMod;
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::Pair* m_elem = this->m_elem;
	uint32_t v8 = v6 & -0x61C8864F * (key >> 4);
	uint32_t result = 1;
	if (this->m_elem[v8].key != -1) {
		while (m_elem[v8].key != key) {
			v8 = v6 & v8 + 1;
			if (m_elem[v8].key == -1) {
				goto LABEL_8;
			}
		}
		result = 0;
	}
LABEL_8:;
	m_numElems += result;
	m_elem[v8].key = key;
	this->m_elem[v8].val = val;
	return result;
}

uint32_t hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::getWithDefault(uint32_t key, uint32_t def) const {
	int32_t m_hashMod = this->m_hashMod;
	if (m_hashMod <= 0) {
		return def;
	}
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::Pair* m_elem = this->m_elem;
	uint32_t v5 = m_hashMod & -0x61C8864F * (key >> 4);
	uint32_t v6 = this->m_elem[v5].key;
	if (v6 == -1) {
		return def;
	}
	while (v6 != key) {
		v5 = m_hashMod & v5 + 1;
		v6 = m_elem[v5].key;
		if (v6 == -1) {
			return def;
		}
	}
	return m_elem[v5].val;
}

int32_t hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::findKey(uint32_t key) const {
	int32_t m_hashMod = this->m_hashMod;
	if (m_hashMod <= 0) {
		return m_hashMod + 1;
	}
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::Pair* m_elem = this->m_elem;
	int32_t result = m_hashMod & -0x61C8864F * (key >> 4);
	uint32_t v5 = this->m_elem[result].key;
	if (v5 == -1) {
		return m_hashMod + 1;
	}
	while (v5 != key) {
		result = m_hashMod & result + 1;
		v5 = m_elem[result].key;
		if (v5 == -1) {
			return m_hashMod + 1;
		}
	}
	return result;
}

int32_t hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::remove(uint32_t key) {
	int32_t v3 = findKey(key);
	if (v3 > m_hashMod) {
		return 1;
	}
	remove(v3);
	return 0;
}

void hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::remove(int32_t aiIndex) {
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::Pair* m_elem = this->m_elem;
	--m_numElems;
	m_elem[aiIndex].key = -1;
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::Pair* v3 = this->m_elem;
	int32_t m_hashMod = this->m_hashMod;
	int32_t v5 = m_hashMod & m_hashMod + aiIndex;
	if (this->m_elem[v5].key != -1) {
		hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::Pair* v6 = this->m_elem;
		do {
			v5 = m_hashMod & m_hashMod + v5;
		} while (v3[v5].key != -1);
	}
	uint32_t v7 = aiIndex;
	uint32_t v8 = m_hashMod & aiIndex + 1;
	uint32_t a1 = m_hashMod & v5 + 1;
	if (v3[v8].key != -1) {
		do {
			uint32_t v9 = m_hashMod & -0x61C8864F * (this->m_elem[v8].key >> 4);
			if ((v8 < a1 || v9 <= v7) && (v8 >= v7 || (v9 <= v7 && v9 > v8)) && (v9 <= v7 || v9 >= a1)) {
				this->m_elem[v7] = this->m_elem[v8];
				this->m_elem[v8].key = -1;
				v7 = v8;
			}
			m_hashMod = this->m_hashMod;
			v8 = m_hashMod & v8 + 1;
		} while (this->m_elem[v8].key != -1);
	}
}

int32_t hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::findOrInsertKey(hkMemoryAllocator& alloc,
	uint32_t key, uint32_t val) {
	int32_t m_hashMod = this->m_hashMod;
	if (2 * m_numElems > m_hashMod) {
		resizeTable(alloc, 2 * m_hashMod + 2);
	}
	int32_t v6 = this->m_hashMod;
	hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::Pair* m_elem = this->m_elem;
	int32_t result = v6 & -0x61C8864F * (key >> 4);
	if (this->m_elem[result].key != key) {
		while (m_elem[result].key != -1) {
			result = v6 & result + 1;
			hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::Pair* v9 = this->m_elem;
			if (m_elem[result].key == key) {
				return result;
			}
		}
		m_elem[result].key = key;
		this->m_elem[result].val = val;
		++m_numElems;
	}
	return result;
}

void hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::clear() {
	int32_t v1 = m_hashMod + 1;
	for (int32_t i = 0; i < v1; ++i) {
		m_elem[i].key = -1;
	}
	m_numElems &= hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::DONT_DEALLOCATE_FLAG;
}

int32_t hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::reserve(hkMemoryAllocator& alloc,
	int32_t numElements) {
	int32_t i;
	int32_t result;
	for (i = 8; i < 2 * numElements; i *= 2) {
		;
	}
	resizeTable(alloc, i);
	return result;
}

// The new slots are emptied (key -1) before the old pairs go back in through insert; the old storage is freed with its
// old size.
void hkPointerMapBase::resizeTable(hkMemoryAllocator& alloc, int32_t newcap) {
	if (newcap <= 8)
		newcap = 8;
	int32_t dontDeallocate = m_numElems & DONT_DEALLOCATE_FLAG;
	int32_t oldcap = m_hashMod + 1;
	Pair* oldelem = m_elem;
	m_elem = static_cast<Pair*>(alloc.blockAlloc(sizeof(Pair) * newcap));
	for (int32_t i = 0; i < newcap; i++)
		m_elem[i].key = 0xFFFFFFFF;
	m_hashMod = newcap - 1;
	m_numElems = 0;
	for (int32_t i = 0; i < oldcap; i++) {
		if (oldelem[i].key != 0xFFFFFFFF)
			insert(alloc, oldelem[i].key, oldelem[i].val);
	}
	if (dontDeallocate == 0)
		alloc.blockFree(oldelem, sizeof(Pair) * oldcap);
}

void hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::clearAndDeallocate(hkMemoryAllocator& alloc) {
	clear();
	if (m_numElems >= 0) {
		alloc.blockFree(m_elem, sizeof(Pair) * (m_hashMod + 1));
	}
}

void hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::destructOperations() {}

// The table is laid over the caller's buffer, which the map does not own. The slot count is the byte count divided by
// sizeof(Pair), an unsigned division.
void hkPointerMapBase::init(void* apBuffer, int32_t aiBytes) {
	int32_t numSlots = static_cast<int32_t>(static_cast<uint32_t>(aiBytes) / sizeof(Pair));
	m_hashMod = numSlots - 1;
	m_elem = static_cast<Pair*>(apBuffer);
	m_numElems = DONT_DEALLOCATE_FLAG;
	for (int32_t i = 0; i < numSlots; i++) {
		m_elem[i].key = 0xFFFFFFFF;
	}
}

int32_t hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::getMemSize() const {
	return 8 * m_hashMod + 8;
}

int32_t hkMapBase<uint32_t, uint32_t, hkMapOperations<uint32_t>>::getWithDefaultStatus(uint32_t a1,
	uint32_t* a2) const {
	int32_t Key = findKey(a1);
	if (Key > m_hashMod) {
		return 1;
	}
	*a2 = m_elem[Key].val;
	return 0;
}

int32_t hkPairMapBase::requiredBytes(int32_t aiNumElems) {
	int32_t i;
	for (i = 8; i < 2 * aiNumElems; i *= 2) {
		;
	}
	return 16 * i;
}

// The table is laid over the caller's buffer, which the map does not own. The slot count is the byte count divided by
// sizeof(Pair), an unsigned division.
void hkPairMapBase::init(void* apBuffer, int32_t aiBytes) {
	int32_t numSlots = static_cast<int32_t>(static_cast<uint32_t>(aiBytes) / sizeof(Pair));
	m_elem = static_cast<Pair*>(apBuffer);
	m_numElems = DONT_DEALLOCATE_FLAG;
	m_hashMod = numSlots - 1;
	for (int32_t i = 0; i < numSlots; i++) {
		m_elem[i].key = 0xFFFFFFFFFFFFFFFFull;
	}
}

int32_t hkPairMapBase::findKey(uint64_t auiKey) const {
	int32_t m_hashMod = this->m_hashMod;
	if (m_hashMod > 0) {
		int32_t result;
		hkMapBase<uint64_t, uint64_t, hkMapOperations<uint64_t>>::Pair* m_elem = this->m_elem;
		for (uint32_t i = -0x61C8864F * (static_cast<uint32_t>(auiKey) >> 4);; i = result + 1) {
			result = m_hashMod & i;
			// Empty slot: both 32-bit halves of the key are -1, tested by ANDing the two halves and comparing the
			// result with 0xFFFFFFFF.
			if ((static_cast<uint32_t>(m_elem[result].key >> 32) & static_cast<uint32_t>(m_elem[result].key)) ==
				0xFFFFFFFFu) {
				break;
			}
			if (m_elem[result].key == auiKey) {
				return result;
			}
		}
	}
	return m_hashMod + 1;
}

// Probes from the hash of the key's low 32 bits to the key's slot (its value) or the first empty slot, both key halves
// -1 (def).
uint64_t hkPairMapBase::getWithDefault(uint64_t key, uint64_t def) const {
	const int32_t iHashMod = m_hashMod;
	if (iHashMod > 0) {
		const Pair* pElem = m_elem;
		for (uint32_t i = (static_cast<uint32_t>(key) >> 4) * 0x9E3779B1u;; ++i) {
			i &= static_cast<uint32_t>(iHashMod);
			const uint64_t uiSlotKey = pElem[i].key;
			if ((static_cast<uint32_t>(uiSlotKey) & static_cast<uint32_t>(uiSlotKey >> 32)) == 0xFFFFFFFFu) {
				break;
			}
			if (uiSlotKey == key) {
				return pElem[i].val;
			}
		}
	}
	return def;
}

// The table grows first when it is half full (the count masked of DONT_DEALLOCATE_FLAG); the probe walks from the key's
// hash to its slot or the first empty one, rereading m_hashMod at each step as the game does. Returns 1 when the key is
// new, 0 when its value was replaced.
uint32_t hkPairMapBase::insert(hkMemoryAllocator& alloc, uint64_t auiKey, uint64_t auiVal) {
	int32_t iHashMod = m_hashMod;
	if (2 * (m_numElems & NUM_ELEMS_MASK) > iHashMod) {
		resizeTable(alloc, 2 * iHashMod + 2);
	}
	Pair* pElem = m_elem;
	uint32_t result = 1;
	// hkMapOperations<hkUint64>::hash: the key's low 32 bits shifted right by 4 times the golden ratio.
	uint32_t i = (static_cast<uint32_t>(auiKey) >> 4) * 0x9E3779B1u & static_cast<uint32_t>(m_hashMod);
	// isValid: a slot is empty only when the key is all ones.
	while (pElem[i].key != 0xFFFFFFFFFFFFFFFFull) {
		if (pElem[i].key == auiKey) {
			result = 0;
			break;
		}
		i = (i + 1) & static_cast<uint32_t>(m_hashMod);
	}
	m_numElems += result;
	pElem[i].key = auiKey;
	m_elem[i].val = auiVal;
	return result;
}

int32_t hkPairMapBase::remove(uint64_t auiKey) {
	int32_t iSlot = findKey(auiKey);
	if (iSlot > m_hashMod) {
		return 1;
	}
	remove(iSlot);
	return 0;
}

// The slot is emptied, then the pairs after it in its unbroken run move back into the hole when their hash does not lie
// in (hole, slot] counted around the table from the run's first slot lo.
void hkPairMapBase::remove(int32_t aiIndex) {
	Pair* pElem = m_elem;
	uint32_t uiEmpty = static_cast<uint32_t>(aiIndex);
	m_numElems -= 1;
	// The SDK's hkMapOperations<hkUint64>::invalidate: both key dwords become -1.
	pElem[uiEmpty].key = 0xFFFFFFFFFFFFFFFFull;
	uint32_t uiHashMod = static_cast<uint32_t>(m_hashMod);
	pElem = m_elem;
	// The first slot of the run the removed pair sat in.
	uint32_t uiLo = (uiHashMod + uiEmpty) & uiHashMod;
	while (pElem[uiLo].key != 0xFFFFFFFFFFFFFFFFull) {
		uiLo = (uiLo + uiHashMod) & uiHashMod;
	}
	uiLo = (uiLo + 1) & uiHashMod;
	uint32_t i = uiEmpty;
	while (true) {
		pElem = m_elem;
		i = (i + 1) & uiHashMod;
		if (pElem[i].key == 0xFFFFFFFFFFFFFFFFull) {
			break;
		}
		uint32_t uiHash = (static_cast<uint32_t>(pElem[i].key) >> 4) * 0x9E3779B1u & uiHashMod;
		const bool bStays = (i >= uiLo && uiHash > uiEmpty) || (i < uiEmpty && (uiHash > uiEmpty || uiHash <= i)) ||
			(uiHash > uiEmpty && uiHash < uiLo);
		if (!bStays) {
			pElem[uiEmpty].key = pElem[i].key;
			m_elem[uiEmpty].val = m_elem[i].val;
			m_elem[i].key = 0xFFFFFFFFFFFFFFFFull;
			uiEmpty = i;
		}
		uiHashMod = static_cast<uint32_t>(m_hashMod);
	}
}

// The SDK's clear: every slot's key becomes -1 (both dwords) and the element count is cleared, the DONT_DEALLOCATE
// flag staying in m_numElems' high bit.
void hkPairMapBase::clear() {
	int32_t capacity = m_hashMod + 1;
	for (int32_t i = 0; i < capacity; i++) {
		m_elem[i].key = 0xFFFFFFFFFFFFFFFFull;
	}
	m_numElems &= DONT_DEALLOCATE_FLAG;
}

void hkPairMapBase::clearAndDeallocate(hkMemoryAllocator& alloc) {
	clear();
	if (m_numElems >= 0) {
		alloc.blockFree(m_elem, static_cast<int32_t>(sizeof(Pair)) * (m_hashMod + 1));
	}
}

// The table grows to twice the requested count, the next power of two from 8 slots, and is rebuilt.
void hkPairMapBase::reserve(hkMemoryAllocator& alloc, int32_t numElements) {
	int32_t i;
	for (i = 8; i < 2 * numElements; i *= 2) {
		;
	}
	resizeTable(alloc, i);
}

// Havok SDK: the new slots are emptied (key -1) before the old pairs go back in through insert; the old storage is
// freed with its old size unless the map does not own it.
void hkPairMapBase::resizeTable(hkMemoryAllocator& alloc, int32_t newcap) {
	if (newcap <= 8) {
		newcap = 8;
	}
	int32_t dontDeallocate = m_numElems & DONT_DEALLOCATE_FLAG;
	int32_t oldcap = m_hashMod + 1;
	Pair* oldelem = m_elem;
	m_elem = static_cast<Pair*>(alloc.blockAlloc(static_cast<int32_t>(sizeof(Pair)) * newcap));
	// The SDK's hkMapOperations<hkUint64>::invalidate: both key dwords become -1.
	for (int32_t i = 0; i < newcap; i++) {
		m_elem[i].key = 0xFFFFFFFFFFFFFFFFull;
	}
	m_numElems = 0;
	m_hashMod = newcap - 1;
	// isValid: a slot is empty only when the key is all ones.
	for (int32_t i = 0; i < oldcap; i++) {
		if (oldelem[i].key != 0xFFFFFFFFFFFFFFFFull) {
			insert(alloc, oldelem[i].key, oldelem[i].val);
		}
	}
	if (dontDeallocate == 0) {
		alloc.blockFree(oldelem, static_cast<int32_t>(sizeof(Pair)) * oldcap);
	}
}

hkPairMapBase::~hkMapBase() {}
