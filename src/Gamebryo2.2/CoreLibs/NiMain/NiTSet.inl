template <class T, class Allocator>
uint32_t NiTSet<T, Allocator>::GetSize() const {
    return m_uiUsed;
}

template <class T, class Allocator>
T* NiTSet<T, Allocator>::GetBase() const {
    return m_pBase;
}

template <class T, class Allocator>
T& NiTSet<T, Allocator>::GetAt(uint32_t uiIndex) const {
    return m_pBase[uiIndex];
}

template <class T, class Allocator>
void NiTSet<T, Allocator>::Add(const T& element) {
    if (m_uiUsed == m_uiAlloced) {
        Realloc();
    }

    m_pBase[m_uiUsed++] = element;
}

template <class T, class Allocator>
void NiTSet<T, Allocator>::AddUnique(const T& element) {
    if (Find(element) == -1) {
        Add(element);
    }
}

template <class T, class Allocator>
void NiTSet<T, Allocator>::RemoveAt(uint32_t uiIndex) {
    if (uiIndex < m_uiUsed) {
        m_pBase[uiIndex] = m_pBase[--m_uiUsed];
    }
}

template <class T, class Allocator>
void NiTSet<T, Allocator>::OrderedRemoveAt(uint32_t uiIndex) {
    if (uiIndex >= m_uiUsed) {
        return;
    }

    for (uint32_t i = uiIndex; i + 1 < m_uiUsed; ++i) {
        m_pBase[i] = m_pBase[i + 1];
    }
    --m_uiUsed;
}

template <class T, class Allocator>
void NiTSet<T, Allocator>::Remove(const T& element) {
    int32_t iIndex = Find(element);
    if (iIndex != -1) {
        RemoveAt(static_cast<uint32_t>(iIndex));
    }
}

template <class T, class Allocator>
void NiTSet<T, Allocator>::OrderedRemove(const T& element) {
    int32_t iIndex = Find(element);
    if (iIndex != -1) {
        OrderedRemoveAt(static_cast<uint32_t>(iIndex));
    }
}

template <class T, class Allocator>
void NiTSet<T, Allocator>::ReplaceAt(uint32_t uiIndex, const T& element) {
    if (uiIndex < m_uiUsed) {
        m_pBase[uiIndex] = element;
    }
}

template <class T, class Allocator>
void NiTSet<T, Allocator>::RemoveAll() {
    m_uiUsed = 0;
}

template <class T, class Allocator>
int32_t NiTSet<T, Allocator>::Find(const T& element) const {
    for (uint32_t i = 0; i < m_uiUsed; ++i) {
        if (m_pBase[i] == element) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

template <class T, class Allocator>
void NiTSet<T, Allocator>::Realloc() {
    Realloc(m_uiAlloced ? m_uiAlloced * 2 : 1);
}

template <class T, class Allocator>
void NiTSet<T, Allocator>::Realloc(uint32_t uiNewSize) {
    if (uiNewSize == m_uiAlloced) {
        return;
    }

    T* pNewBase = nullptr;
    if (uiNewSize) {
        pNewBase = Allocator::Allocate(uiNewSize);
        for (uint32_t i = 0; i < m_uiUsed; ++i) {
            pNewBase[i] = m_pBase[i];
        }
    }

    Allocator::Deallocate(m_pBase);
    m_pBase = pNewBase;
    m_uiAlloced = uiNewSize;
}
