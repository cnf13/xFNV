template <class Allocator, class TKEY, class TVAL>
uint32_t NiTMapBase<Allocator, TKEY, TVAL>::KeyToHashIndex(TKEY key) const {
    return static_cast<uint32_t>((size_t)key % m_uiHashSize);
}

template <class Allocator, class TKEY, class TVAL>
bool NiTMapBase<Allocator, TKEY, TVAL>::IsKeysEqual(TKEY key1, TKEY key2) const {
    return key1 == key2;
}

template <class Allocator, class TKEY, class TVAL>
void NiTMapBase<Allocator, TKEY, TVAL>::SetValue(NiTMapItem<TKEY, TVAL>* pkItem, TKEY key, TVAL val) {
    pkItem->m_key = key;
    pkItem->m_val = val;
}

template <class Allocator, class TKEY, class TVAL>
uint32_t NiTMapBase<Allocator, TKEY, TVAL>::GetCount() const {
    return m_kAllocator.m_uiCount;
}

template <class Allocator, class TKEY, class TVAL>
bool NiTMapBase<Allocator, TKEY, TVAL>::IsEmpty() const {
    return m_kAllocator.m_uiCount == 0;
}

template <class Allocator, class TKEY, class TVAL>
void NiTMapBase<Allocator, TKEY, TVAL>::SetAt(TKEY key, TVAL val) {
    uint32_t uiIndex = KeyToHashIndex(key);
    for (NiTMapItem<TKEY, TVAL>* pkItem = m_ppkHashTable[uiIndex]; pkItem; pkItem = pkItem->m_pkNext) {
        // The searched key is passed first.
        if (IsKeysEqual(key, pkItem->m_key)) {
            pkItem->m_val = val;
            return;
        }
    }

    NiTMapItem<TKEY, TVAL>* pkItem = NewItem();
    SetValue(pkItem, key, val);
    pkItem->m_pkNext = m_ppkHashTable[uiIndex];
    m_ppkHashTable[uiIndex] = pkItem;
    ++m_kAllocator.m_uiCount;
}

template <class Allocator, class TKEY, class TVAL>
bool NiTMapBase<Allocator, TKEY, TVAL>::RemoveAt(TKEY key) {
    NiTMapItem<TKEY, TVAL>** ppkLink = &m_ppkHashTable[KeyToHashIndex(key)];
    for (NiTMapItem<TKEY, TVAL>* pkItem = *ppkLink; pkItem; ppkLink = &pkItem->m_pkNext, pkItem = *ppkLink) {
        if (IsKeysEqual(key, pkItem->m_key)) {
            *ppkLink = pkItem->m_pkNext;
            ClearValue(pkItem);
            DeleteItem(pkItem);
            --m_kAllocator.m_uiCount;
            return true;
        }
    }
    return false;
}

template <class Allocator, class TKEY, class TVAL>
void NiTMapBase<Allocator, TKEY, TVAL>::RemoveAll() {
    for (uint32_t i = 0; i < m_uiHashSize; ++i) {
        NiTMapItem<TKEY, TVAL>* pkItem = m_ppkHashTable[i];
        while (pkItem) {
            NiTMapItem<TKEY, TVAL>* pkNext = pkItem->m_pkNext;
            ClearValue(pkItem);
            DeleteItem(pkItem);
            pkItem = pkNext;
        }
        m_ppkHashTable[i] = nullptr;
    }
    m_kAllocator.m_uiCount = 0;
}

template <class Allocator, class TKEY, class TVAL>
bool NiTMapBase<Allocator, TKEY, TVAL>::GetAt(TKEY key, TVAL& val) const {
    for (NiTMapItem<TKEY, TVAL>* pkItem = m_ppkHashTable[KeyToHashIndex(key)]; pkItem; pkItem = pkItem->m_pkNext) {
        if (IsKeysEqual(key, pkItem->m_key)) {
            val = pkItem->m_val;
            return true;
        }
    }
    return false;
}

template <class Allocator, class TKEY, class TVAL>
NiTMapItem<TKEY, TVAL>* NiTMapBase<Allocator, TKEY, TVAL>::GetFirstPos() const {
    for (uint32_t i = 0; i < m_uiHashSize; ++i) {
        if (m_ppkHashTable[i]) {
            return m_ppkHashTable[i];
        }
    }
    return nullptr;
}

template <class Allocator, class TKEY, class TVAL>
void NiTMapBase<Allocator, TKEY, TVAL>::GetNext(NiTMapItem<TKEY, TVAL>*& pos, TKEY& key, TVAL& val) const {
    NiTMapItem<TKEY, TVAL>* pkItem = pos;
    key = pkItem->m_key;
    val = pkItem->m_val;

    if (pkItem->m_pkNext) {
        pos = pkItem->m_pkNext;
        return;
    }

    pos = nullptr;
    for (uint32_t i = KeyToHashIndex(pkItem->m_key) + 1; i < m_uiHashSize; ++i) {
        if (m_ppkHashTable[i]) {
            pos = m_ppkHashTable[i];
            return;
        }
    }
}
