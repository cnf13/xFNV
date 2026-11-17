template <class T>
inline NiPointer<T>::NiPointer(T* apObject) : m_pObject(apObject) {
    if (m_pObject) {
        NiIncRefCount(m_pObject);
    }
}

template <class T>
inline NiPointer<T>::NiPointer(const NiPointer& ptr) : NiPointer(ptr.m_pObject) {}

template <class T>
inline NiPointer<T>::~NiPointer() {
    if (m_pObject) {
        NiDecRefCount(m_pObject);
    }
}

template <class T>
inline NiPointer<T>& NiPointer<T>::operator=(T* apObject) {
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

template <class T>
inline T* NiPointer<T>::operator->() const {
    return m_pObject;
}

template <class T>
inline bool NiPointer<T>::operator==(T* pObject) const {
    return m_pObject == pObject;
}

template <class T>
inline bool NiPointer<T>::operator==(const NiPointer& ptr) const {
    return m_pObject == ptr.m_pObject;
}

template <class T>
inline bool NiPointer<T>::operator!=(T* pObject) const {
    return m_pObject != pObject;
}

template <class T>
inline bool NiPointer<T>::operator!=(const NiPointer& ptr) const {
    return m_pObject != ptr.m_pObject;
}
// Releases the object without clearing the pointer.
template <class T>
inline void NiPointer<T>::Free() {
    if (m_pObject) {
        NiDecRefCount(m_pObject);
    }
}

// Assignment from another pointer; returns this.
template <class T>
inline NiPointer<T>* NiPointer<T>::SetPointer_(const NiPointer* apPointer) {
    *this = *apPointer;
    return this;
}
