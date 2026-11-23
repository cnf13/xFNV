template <class T, class Allocator>
T& NiTArray<T, Allocator>::operator[](uint32_t uiIndex) {
    return m_pBase[uiIndex];
}

template <class T, class Allocator>
const T& NiTArray<T, Allocator>::GetAt(uint32_t uiIndex) const {
    return m_pBase[uiIndex];
}

template <class T, class Allocator>
uint32_t NiTArray<T, Allocator>::GetSize() const {
    return m_usSize;
}

template <class T, class Allocator>
uint32_t NiTArray<T, Allocator>::GetEffectiveSize() const {
    return m_usESize;
}

template <class T, class Allocator>
void NiTArray<T, Allocator>::SetGrowBy(uint16_t uiGrowBy) {
    m_usGrowBy = uiGrowBy;
}

template <class T, class Allocator>
uint32_t NiTArray<T, Allocator>::Add(const T& element) {
    return SetAtGrow(m_usSize, element);
}

template <class T, class Allocator>
uint32_t NiTArray<T, Allocator>::SetAtGrow(uint32_t uiIndex, const T& element) {
    if (uiIndex >= m_usMaxSize) {
        SetSize(uiIndex + m_usGrowBy);
    }

    SetAt(uiIndex, element);
    return uiIndex;
}

template <class T, class Allocator>
uint32_t NiTArray<T, Allocator>::GetAllocatedSize() const {
    return m_usMaxSize;
}

template <class T, class Allocator>
uint32_t NiTArray<T, Allocator>::GetGrowBy() const {
    return m_usGrowBy;
}

template <class T, class Allocator>
T* NiTArray<T, Allocator>::GetBase() const {
    return m_pBase;
}

template <class T, class Allocator>
void NiTArray<T, Allocator>::SetSize(uint32_t uiMaxSize) {
    if (uiMaxSize == m_usMaxSize) {
        return;
    }

    if (uiMaxSize < m_usSize) {
        for (uint16_t i = static_cast<uint16_t>(uiMaxSize); i < m_usSize; ++i) {
            if (!(m_pBase[i] == T(0))) {
                m_pBase[i] = T(0);
                --m_usESize;
            }
        }
        m_usSize = static_cast<uint16_t>(uiMaxSize);
    }

    T* pSaveBase = m_pBase;
    m_usMaxSize = static_cast<uint16_t>(uiMaxSize);
    if (uiMaxSize) {
        m_pBase = Allocator::Allocate(m_usMaxSize);
        for (uint16_t i = 0; i < m_usSize; ++i) {
            m_pBase[i] = pSaveBase[i];
        }
        for (uint16_t i = m_usSize; i < m_usMaxSize; ++i) {
            m_pBase[i] = T(0);
        }
    }
    else {
        m_pBase = nullptr;
    }
    Allocator::Deallocate(pSaveBase);
}

template <class T, class Allocator>
void NiTArray<T, Allocator>::SetAt(uint32_t uiIndex, const T& element) {
    static T tcmp(0);

    if (uiIndex >= m_usSize) {
        m_usSize = static_cast<uint16_t>(uiIndex + 1);
        if (!(element == tcmp)) {
            ++m_usESize;
        }
    }
    else if (element == tcmp) {
        if (!(m_pBase[uiIndex] == tcmp)) {
            --m_usESize;
        }
    }
    else if (m_pBase[uiIndex] == tcmp) {
        ++m_usESize;
    }
    m_pBase[uiIndex] = element;
}

template <class T, class Allocator>
uint32_t NiTArray<T, Allocator>::AddFirstEmpty(const T& element) {
    if (element == T(0)) {
        return 0xFFFFFFFF;
    }

    for (uint16_t i = 0; i < m_usSize; ++i) {
        if (m_pBase[i] == T(0)) {
            m_pBase[i] = element;
            ++m_usESize;
            return i;
        }
    }
    return SetAtGrow(m_usSize, element);
}

template <class T, class Allocator>
void NiTArray<T, Allocator>::InsertAt(uint32_t uiIndex, const T& element) {
    if (element == T(0)) {
        return;
    }

    uint32_t uiSize = m_usSize;
    for (uint32_t i = uiSize; i > uiIndex; --i) {
        // A copy: growing the buffer frees the slot it comes from.
        T moved = m_pBase[i - 1];
        if (i == uiSize && i >= m_usMaxSize) {
            SetSize(i + m_usGrowBy);
        }
        SetAt(i, moved);
    }

    if (uiIndex >= m_usMaxSize) {
        SetSize(uiIndex + m_usGrowBy);
    }
    SetAt(uiIndex, element);
}

template <class T, class Allocator>
T NiTArray<T, Allocator>::RemoveAt(uint32_t uiIndex) {
    if (uiIndex >= m_usSize) {
        return T(0);
    }

    T element = m_pBase[uiIndex];
    m_pBase[uiIndex] = T(0);
    if (!(element == T(0))) {
        --m_usESize;
    }
    if (uiIndex == m_usSize - 1u) {
        --m_usSize;
    }
    return element;
}

template <class T, class Allocator>
T NiTArray<T, Allocator>::RemoveAtAndFill(uint32_t uiIndex) {
    T element = m_pBase[uiIndex];
    uint32_t uiLast = m_usSize - 1u;
    m_pBase[uiIndex] = m_pBase[uiLast];
    m_pBase[uiLast] = T(0);
    m_usSize = static_cast<uint16_t>(uiLast);
    if (!(element == T(0))) {
        --m_usESize;
    }
    return element;
}

template <class T, class Allocator>
T NiTArray<T, Allocator>::RemoveEnd() {
    if (!m_usSize) {
        return T(0);
    }

    --m_usSize;
    T element = m_pBase[m_usSize];
    m_pBase[m_usSize] = T(0);
    if (!(element == T(0))) {
        --m_usESize;
    }
    return element;
}

template <class T, class Allocator>
void NiTArray<T, Allocator>::RemoveAll() {
    for (uint32_t i = 0; i < m_usSize; ++i) {
        m_pBase[i] = T(0);
    }
    m_usSize = 0;
    m_usESize = 0;
}

template <class T, class Allocator>
void NiTArray<T, Allocator>::Compact() {
    if (m_usESize == m_usSize) {
        return;
    }

    if (m_usESize) {
        for (uint16_t i = 0, j = 0; i < m_usSize; ++i) {
            if (!(m_pBase[i] == T(0))) {
                if (!(m_pBase[j] == m_pBase[i])) {
                    m_pBase[j] = m_pBase[i];
                }
                ++j;
            }
        }
    }

    T* pSaveBase = m_pBase;
    m_usSize = m_usESize;
    m_usMaxSize = m_usSize;
    if (m_usMaxSize) {
        m_pBase = Allocator::Allocate(m_usMaxSize);
        for (uint16_t i = 0; i < m_usSize; ++i) {
            m_pBase[i] = pSaveBase[i];
        }
    }
    else {
        m_pBase = nullptr;
    }
    Allocator::Deallocate(pSaveBase);
}

template <class T, class Allocator>
void NiTArray<T, Allocator>::UpdateSize() {
    while (m_usSize && m_pBase[m_usSize - 1u] == T(0)) {
        --m_usSize;
    }
}
