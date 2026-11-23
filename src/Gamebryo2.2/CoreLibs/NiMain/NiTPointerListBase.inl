template <class T, class TheAllocator>
T& NiTPointerListBase<T, TheAllocator>::GetHead() const {
    return this->m_pkHead->m_element;
}

template <class T, class TheAllocator>
T& NiTPointerListBase<T, TheAllocator>::GetTail() const {
    return this->m_pkTail->m_element;
}

template <class T, class TheAllocator>
T& NiTPointerListBase<T, TheAllocator>::Get(NiTListItem<T>* kPos) const {
    return kPos->m_element;
}

template <class T, class TheAllocator>
T& NiTPointerListBase<T, TheAllocator>::GetNext(NiTListItem<T>*& kPos) const {
    T& element = kPos->m_element;
    kPos = kPos->m_pkNext;
    return element;
}

template <class T, class TheAllocator>
T& NiTPointerListBase<T, TheAllocator>::GetPrev(NiTListItem<T>*& kPos) const {
    T& element = kPos->m_element;
    kPos = kPos->m_pkPrev;
    return element;
}

template <class T, class TheAllocator>
NiTListItem<T>* NiTPointerListBase<T, TheAllocator>::FindPos(const T& element, NiTListItem<T>* kStart) const {
    NiTListItem<T>* pkItem = kStart ? kStart : this->m_pkHead;
    for (; pkItem; pkItem = pkItem->m_pkNext) {
        if (pkItem->m_element == element) {
            return pkItem;
        }
    }
    return nullptr;
}

template <class T, class TheAllocator>
T NiTPointerListBase<T, TheAllocator>::RemoveHead() {
    NiTListItem<T>* pkNode = this->m_pkHead;
    this->m_pkHead = pkNode->m_pkNext;
    if (this->m_pkHead) {
        this->m_pkHead->m_pkPrev = nullptr;
    }
    else {
        this->m_pkTail = nullptr;
    }
    T element = pkNode->m_element;
    DeleteItem(pkNode);
    this->m_uiCount--;
    return element;
}

// The allocator takes the item count field as its `this`.
template <class T, class TheAllocator>
NiTListItem<T>* NiTPointerListBase<T, TheAllocator>::NewItem() {
    return static_cast<NiTListItem<T>*>(reinterpret_cast<TheAllocator&>(this->m_uiCount).Allocate());
}

// Frees every node through DeleteItem (which clears its element), then zeroes the count, head and tail, in that order.
template <class T, class TheAllocator>
void NiTPointerListBase<T, TheAllocator>::RemoveAll() {
    NiTListItem<T>* pkCurr = this->m_pkHead;
    while (pkCurr) {
        NiTListItem<T>* pkDel = pkCurr;
        pkCurr = pkCurr->m_pkNext;
        DeleteItem(pkDel);
    }
    this->m_uiCount = 0;
    this->m_pkHead = nullptr;
    this->m_pkTail = nullptr;
}

// Clears the node's element (releasing what it holds), then returns the node to the allocator. The allocator keeps no
// state of its own (the pool's is static, the heap's is the engine's), so a local one frees the node as the one the
// list folds into its count would.
template <class T, class TheAllocator>
void NiTPointerListBase<T, TheAllocator>::DeleteItem(NiTListItem<T>* pkItem) {
    pkItem->m_element = T();
    TheAllocator kAllocator;
    kAllocator.Deallocate(pkItem);
}

template <class T, class TheAllocator>
void NiTPointerListBase<T, TheAllocator>::AddHead(const T& element) {
    NiTListItem<T>* pkNode = NewItem();
    pkNode->m_element = element;
    this->AddNodeHead(pkNode);
}

template <class T, class TheAllocator>
void NiTPointerListBase<T, TheAllocator>::AddTail(const T& element) {
    NiTListItem<T>* pkNode = NewItem();
    pkNode->m_element = element;
    this->AddNodeTail(pkNode);
}

template <class T, class TheAllocator>
NiTListItem<T>* NiTPointerListBase<T, TheAllocator>::InsertBefore(NiTListItem<T>* kPos, const T& element) {
    NiTListItem<T>* pkNode = NewItem();
    pkNode->m_element = element;
    return this->AddNodeBefore(kPos, pkNode);
}

template <class T, class TheAllocator>
NiTListItem<T>* NiTPointerListBase<T, TheAllocator>::InsertAfter(NiTListItem<T>* kPos, const T& element) {
    NiTListItem<T>* pkNode = NewItem();
    pkNode->m_element = element;
    return this->AddNodeAfter(kPos, pkNode);
}

template <class T, class TheAllocator>
T NiTPointerListBase<T, TheAllocator>::RemoveTail() {
    NiTListItem<T>* pkNode = this->m_pkTail;
    this->m_pkTail = pkNode->m_pkPrev;
    if (this->m_pkTail) {
        this->m_pkTail->m_pkNext = nullptr;
    }
    else {
        this->m_pkHead = nullptr;
    }
    T element = pkNode->m_element;
    DeleteItem(pkNode);
    this->m_uiCount--;
    return element;
}

// The head and the tail go through RemoveHead and RemoveTail; an inner node is unlinked here.
template <class T, class TheAllocator>
T NiTPointerListBase<T, TheAllocator>::RemovePos(NiTListItem<T>*& kPos) {
    NiTListItem<T>* pkNode = kPos;
    if (pkNode == this->m_pkHead) {
        kPos = pkNode->m_pkNext;
        return RemoveHead();
    }
    if (pkNode == this->m_pkTail) {
        kPos = nullptr;
        return RemoveTail();
    }
    NiTListItem<T>* pkPrev = pkNode->m_pkPrev;
    NiTListItem<T>* pkNext = pkNode->m_pkNext;
    kPos = pkNext;
    if (pkPrev) {
        pkPrev->m_pkNext = pkNext;
    }
    if (pkNext) {
        pkNext->m_pkPrev = pkPrev;
    }
    T element = pkNode->m_element;
    DeleteItem(pkNode);
    this->m_uiCount--;
    return element;
}

// The removed element, or element itself when the list does not hold it.
template <class T, class TheAllocator>
T NiTPointerListBase<T, TheAllocator>::Remove(const T& element) {
    NiTListItem<T>* kPos = FindPos(element);
    return kPos ? RemovePos(kPos) : element;
}
