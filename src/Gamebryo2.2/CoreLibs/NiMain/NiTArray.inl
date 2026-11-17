template <class T, class Allocator> T& NiTArray<T, Allocator>::operator[](uint32_t uiIndex) {
	return m_pBase[uiIndex];
}

template <class T, class Allocator> const T& NiTArray<T, Allocator>::GetAt(uint32_t uiIndex) const {
	return m_pBase[uiIndex];
}

template <class T, class Allocator> uint32_t NiTArray<T, Allocator>::GetSize() const {
	return m_usSize;
}

template <class T, class Allocator> uint32_t NiTArray<T, Allocator>::GetEffectiveSize() const {
	return m_usESize;
}

template <class T, class Allocator> void NiTArray<T, Allocator>::SetGrowBy(uint16_t uiGrowBy) {
	m_usGrowBy = uiGrowBy;
}

template <class T, class Allocator> uint32_t NiTArray<T, Allocator>::Add(const T& element) {
	return SetAtGrow(m_usSize, element);
}

template <class T, class Allocator> uint32_t NiTArray<T, Allocator>::SetAtGrow(uint32_t uiIndex, const T& element) {
	if (uiIndex >= m_usMaxSize)
		SetSize(uiIndex + m_usGrowBy);

	SetAt(uiIndex, element);
	return uiIndex;
}

template <class T, class Allocator> uint32_t NiTArray<T, Allocator>::GetAllocatedSize() const {
	return m_usMaxSize;
}

template <class T, class Allocator> uint32_t NiTArray<T, Allocator>::GetGrowBy() const {
	return m_usGrowBy;
}

template <class T, class Allocator> T* NiTArray<T, Allocator>::GetBase() const {
	return m_pBase;
}

template <class T, class Allocator> void NiTArray<T, Allocator>::SetSize(uint32_t uiMaxSize) {
	if (uiMaxSize == m_usMaxSize)
		return;

	for (uint32_t i = uiMaxSize; i < m_usSize; ++i) {
		if (!(m_pBase[i] == T(0))) {
			m_pBase[i] = T(0);
			--m_usESize;
		}
	}

	if (uiMaxSize < m_usSize)
		m_usSize = static_cast<uint16_t>(uiMaxSize);

	T* pSaveBase = m_pBase;
	m_usMaxSize = static_cast<uint16_t>(uiMaxSize);
	if (uiMaxSize) {
		m_pBase = Allocator::Allocate(m_usMaxSize);
		for (uint32_t i = 0; i < m_usSize; ++i)
			m_pBase[i] = pSaveBase[i];
		for (uint32_t i = m_usSize; i < m_usMaxSize; ++i)
			m_pBase[i] = T(0);
	} else {
		m_pBase = nullptr;
	}
	Allocator::Deallocate(pSaveBase);
}

template <class T, class Allocator> void NiTArray<T, Allocator>::SetAt(uint32_t uiIndex, const T& element) {
	// The exe keeps the zero element in a function-local static (PDB: tcmp), guarded for non-trivial T.
	static T tcmp(0);

	if (uiIndex >= m_usSize) {
		m_usSize = static_cast<uint16_t>(uiIndex + 1);
		if (!(element == tcmp))
			++m_usESize;
	} else if (element == tcmp) {
		if (!(m_pBase[uiIndex] == tcmp))
			--m_usESize;
	} else if (m_pBase[uiIndex] == tcmp) {
		++m_usESize;
	}
	m_pBase[uiIndex] = element;
}

template <class T, class Allocator> uint32_t NiTArray<T, Allocator>::AddFirstEmpty(const T& element) {
	if (element == T(0))
		return 0xFFFFFFFF;

	for (uint32_t i = 0; i < m_usSize; ++i) {
		if (m_pBase[i] == T(0)) {
			m_pBase[i] = element;
			++m_usESize;
			return i;
		}
	}
	return SetAtGrow(m_usSize, element);
}

template <class T, class Allocator> void NiTArray<T, Allocator>::InsertAt(uint32_t uiIndex, const T& element) {
	uint32_t uiSize = m_usSize > uiIndex ? m_usSize : uiIndex;
	if (uiSize >= m_usMaxSize)
		SetSize(uiSize + (m_usGrowBy ? m_usGrowBy : 1));

	for (uint32_t i = uiSize; i > uiIndex; --i)
		m_pBase[i] = m_pBase[i - 1];

	m_pBase[uiIndex] = T(0);
	m_usSize = static_cast<uint16_t>(uiSize + 1);
	SetAt(uiIndex, element);
}

template <class T, class Allocator> T NiTArray<T, Allocator>::RemoveAt(uint32_t uiIndex) {
	if (uiIndex >= m_usSize)
		return T(0);

	T element = m_pBase[uiIndex];
	m_pBase[uiIndex] = T(0);
	if (!(element == T(0)))
		--m_usESize;
	if (uiIndex == m_usSize - 1u)
		--m_usSize;
	return element;
}

template <class T, class Allocator> T NiTArray<T, Allocator>::RemoveAtAndFill(uint32_t uiIndex) {
	T element = m_pBase[uiIndex];
	uint32_t uiLast = m_usSize - 1u;
	m_pBase[uiIndex] = m_pBase[uiLast];
	m_pBase[uiLast] = T(0);
	m_usSize = static_cast<uint16_t>(uiLast);
	if (!(element == T(0)))
		--m_usESize;
	return element;
}

template <class T, class Allocator> T NiTArray<T, Allocator>::RemoveEnd() {
	if (!m_usSize)
		return T(0);

	--m_usSize;
	T element = m_pBase[m_usSize];
	m_pBase[m_usSize] = T(0);
	if (!(element == T(0)))
		--m_usESize;
	return element;
}

template <class T, class Allocator> void NiTArray<T, Allocator>::RemoveAll() {
	for (uint32_t i = 0; i < m_usSize; ++i)
		m_pBase[i] = T(0);
	m_usSize = 0;
	m_usESize = 0;
}

template <class T, class Allocator> void NiTArray<T, Allocator>::Compact() {
	if (m_usESize == m_usSize)
		return;

	uint32_t uiPacked = 0;
	for (uint32_t i = 0; i < m_usSize; ++i) {
		if (!(m_pBase[i] == T(0))) {
			if (i != uiPacked) {
				m_pBase[uiPacked] = m_pBase[i];
				m_pBase[i] = T(0);
			}
			++uiPacked;
		}
	}
	m_usSize = static_cast<uint16_t>(uiPacked);
	SetSize(uiPacked);
}

template <class T, class Allocator> void NiTArray<T, Allocator>::UpdateSize() {
	while (m_usSize && m_pBase[m_usSize - 1u] == T(0))
		--m_usSize;
}
