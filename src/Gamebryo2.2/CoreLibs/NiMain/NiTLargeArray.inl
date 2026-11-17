template <class T, class Allocator>
uint32_t NiTLargeArray<T, Allocator>::GetSize() const {
    return m_uiSize;
}

template <class T, class Allocator>
uint32_t NiTLargeArray<T, Allocator>::GetEffectiveSize() const {
    return m_uiESize;
}

template <class T, class Allocator>
uint32_t NiTLargeArray<T, Allocator>::GetAllocatedSize() const {
    return m_uiMaxSize;
}

template <class T, class Allocator>
uint32_t NiTLargeArray<T, Allocator>::GetGrowBy() const {
    return m_uiGrowBy;
}

template <class T, class Allocator>
void NiTLargeArray<T, Allocator>::SetSize(uint32_t uiMaxSize) {
    if (uiMaxSize == m_uiMaxSize) {
        return;
    }

    for (uint32_t i = uiMaxSize; i < m_uiSize; ++i) {
        if (!(m_pBase[i] == T())) {
            m_pBase[i] = T();
            --m_uiESize;
        }
    }

    if (uiMaxSize < m_uiSize) {
        m_uiSize = uiMaxSize;
    }

    T* pSaveBase = m_pBase;
    m_uiMaxSize = uiMaxSize;
    if (uiMaxSize) {
        m_pBase = Allocator::Allocate(m_uiMaxSize);
        for (uint32_t i = 0; i < m_uiSize; ++i) {
            m_pBase[i] = pSaveBase[i];
        }
        for (uint32_t i = m_uiSize; i < m_uiMaxSize; ++i) {
            m_pBase[i] = T();
        }
    }
    else {
        m_pBase = nullptr;
    }
    Allocator::Deallocate(pSaveBase);
}

template <class T, class Allocator>
void NiTLargeArray<T, Allocator>::SetGrowBy(uint32_t uiGrowBy) {
    m_uiGrowBy = uiGrowBy;
}

template <class T, class Allocator>
T* NiTLargeArray<T, Allocator>::GetBase() const {
    return m_pBase;
}

template <class T, class Allocator>
T& NiTLargeArray<T, Allocator>::GetAt(uint32_t uiIndex) const {
    return m_pBase[uiIndex];
}

template <class T, class Allocator>
void NiTLargeArray<T, Allocator>::SetAt(uint32_t uiIndex, const T& element) {
    if (uiIndex >= m_uiSize) {
        m_uiSize = uiIndex + 1;
        if (!(element == T())) {
            ++m_uiESize;
        }
    }
    else if (m_pBase[uiIndex] == T()) {
        if (!(element == T())) {
            ++m_uiESize;
        }
    }
    else if (element == T()) {
        --m_uiESize;
    }
    m_pBase[uiIndex] = element;
}

template <class T, class Allocator>
uint32_t NiTLargeArray<T, Allocator>::SetAtGrow(uint32_t uiIndex, const T& element) {
    if (uiIndex >= m_uiMaxSize) {
        SetSize(uiIndex + (m_uiGrowBy ? m_uiGrowBy : 1));
    }

    SetAt(uiIndex, element);
    return uiIndex;
}

template <class T, class Allocator>
uint32_t NiTLargeArray<T, Allocator>::Add(const T& element) {
    return SetAtGrow(m_uiSize, element);
}

template <class T, class Allocator>
uint32_t NiTLargeArray<T, Allocator>::AddFirstEmpty(const T& element) {
    if (element == T()) {
        return 0xFFFFFFFF;
    }

    for (uint32_t i = 0; i < m_uiSize; ++i) {
        if (m_pBase[i] == T()) {
            m_pBase[i] = element;
            ++m_uiESize;
            return i;
        }
    }
    return Add(element);
}

template <class T, class Allocator>
T NiTLargeArray<T, Allocator>::RemoveAt(uint32_t uiIndex) {
    if (uiIndex >= m_uiSize) {
        return T();
    }

    T element = m_pBase[uiIndex];
    m_pBase[uiIndex] = T();
    if (!(element == T())) {
        --m_uiESize;
    }
    if (uiIndex == m_uiSize - 1u) {
        --m_uiSize;
    }
    return element;
}

template <class T, class Allocator>
T NiTLargeArray<T, Allocator>::RemoveAtAndFill(uint32_t uiIndex) {
    T element = m_pBase[uiIndex];
    uint32_t uiLast = m_uiSize - 1u;
    m_pBase[uiIndex] = m_pBase[uiLast];
    m_pBase[uiLast] = T();
    m_uiSize = uiLast;
    if (!(element == T())) {
        --m_uiESize;
    }
    return element;
}

template <class T, class Allocator>
T NiTLargeArray<T, Allocator>::RemoveEnd() {
    if (!m_uiSize) {
        return T();
    }

    --m_uiSize;
    T element = m_pBase[m_uiSize];
    m_pBase[m_uiSize] = T();
    if (!(element == T())) {
        --m_uiESize;
    }
    return element;
}

template <class T, class Allocator>
void NiTLargeArray<T, Allocator>::RemoveAll() {
    for (uint32_t i = 0; i < m_uiSize; ++i) {
        m_pBase[i] = T();
    }
    m_uiSize = 0;
    m_uiESize = 0;
}

template <class T, class Allocator>
uint32_t NiTLargeArray<T, Allocator>::Remove(const T& element) {
    if (element == T()) {
        return 0xFFFFFFFF;
    }

    for (uint32_t i = 0; i < m_uiSize; ++i) {
        if (m_pBase[i] == element) {
            m_pBase[i] = T();
            --m_uiESize;
            return i;
        }
    }
    return 0xFFFFFFFF;
}

template <class T, class Allocator>
void NiTLargeArray<T, Allocator>::Compact() {
    if (m_uiESize == m_uiSize) {
        return;
    }

    uint32_t uiPacked = 0;
    for (uint32_t i = 0; i < m_uiSize; ++i) {
        if (!(m_pBase[i] == T())) {
            if (i != uiPacked) {
                m_pBase[uiPacked] = m_pBase[i];
                m_pBase[i] = T();
            }
            ++uiPacked;
        }
    }
    m_uiSize = uiPacked;
    SetSize(uiPacked);
}

template <class T, class Allocator>
void NiTLargeArray<T, Allocator>::UpdateSize() {
    while (m_uiSize && m_pBase[m_uiSize - 1u] == T()) {
        --m_uiSize;
    }
}
