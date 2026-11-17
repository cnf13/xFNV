template <class T> inline NiPointer<T>::NiPointer(T* apObject) : m_pObject(apObject) {
	if (m_pObject) {
		NiIncRefCount(m_pObject);
	}
}

template <class T> inline NiPointer<T>::NiPointer(const NiPointer& ptr) : NiPointer(ptr.m_pObject) {}

template <class T> inline NiPointer<T>::~NiPointer() {
	if (m_pObject) {
		NiDecRefCount(m_pObject);
	}
}

template <class T> inline NiPointer<T>& NiPointer<T>::operator=(T* apObject) {
	if (m_pObject != apObject) {
		if (m_pObject) {
			NiDecRefCount(m_pObject);
		}
		m_pObject = apObject;
		if (m_pObject) {
			NiIncRefCount(m_pObject);
		}
	}
	return *this;
}

template <class T> inline T* NiPointer<T>::operator->() const {
	return m_pObject;
}

template <class T> inline bool NiPointer<T>::operator==(T* pObject) const {
	return m_pObject == pObject;
}

template <class T> inline bool NiPointer<T>::operator==(const NiPointer& ptr) const {
	return m_pObject == ptr.m_pObject;
}

template <class T> inline bool NiPointer<T>::operator!=(T* pObject) const {
	return m_pObject != pObject;
}

template <class T> inline bool NiPointer<T>::operator!=(const NiPointer& ptr) const {
	return m_pObject != ptr.m_pObject;
}
// Releases the object without clearing the pointer (BSCloneReserver's 0x92C890).
template <class T> inline void NiPointer<T>::Free() {
	if (m_pObject) {
		NiDecRefCount(m_pObject);
	}
}

// An instance's out-of-line assignment from another pointer (operator=, 0xC46270 and the other copies); returns
// this.
template <class T> inline NiPointer<T>* NiPointer<T>::SetPointer_(const NiPointer* apPointer) {
	*this = *apPointer;
	return this;
}
