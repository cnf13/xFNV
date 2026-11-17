template <class T> T& NiTPointerListBase<T>::GetHead() const {
	return this->m_pkHead->m_element;
}

template <class T> T& NiTPointerListBase<T>::GetTail() const {
	return this->m_pkTail->m_element;
}

template <class T> T& NiTPointerListBase<T>::Get(NiTListItem<T>* kPos) const {
	return kPos->m_element;
}

template <class T> T& NiTPointerListBase<T>::GetNext(NiTListItem<T>*& kPos) const {
	T& element = kPos->m_element;
	kPos = kPos->m_pkNext;
	return element;
}

template <class T> T& NiTPointerListBase<T>::GetPrev(NiTListItem<T>*& kPos) const {
	T& element = kPos->m_element;
	kPos = kPos->m_pkPrev;
	return element;
}

template <class T> NiTListItem<T>* NiTPointerListBase<T>::FindPos(const T& element, NiTListItem<T>* kStart) const {
	NiTListItem<T>* pkItem = kStart ? kStart : this->m_pkHead;
	for (; pkItem; pkItem = pkItem->m_pkNext) {
		if (pkItem->m_element == element)
			return pkItem;
	}
	return nullptr;
}

template <class T> T NiTPointerListBase<T>::RemoveHead() {
	NiTListItem<T>* pkNode = this->m_pkHead;
	this->m_pkHead = pkNode->m_pkNext;
	if (this->m_pkHead)
		this->m_pkHead->m_pkPrev = nullptr;
	else
		this->m_pkTail = nullptr;
	T element = pkNode->m_element;
	DeleteItem(pkNode);
	this->m_uiCount--;
	return element;
}

// The node pool takes the item count field as its `this`.
template <class T> NiTListItem<T>* NiTPointerListBase<T>::NewItem() {
	return static_cast<NiTListItem<T>*>(reinterpret_cast<NiTPointerAllocator<uint32_t>&>(this->m_uiCount).Allocate());
}

// Frees every node through DeleteItem (which clears its element), then zeroes the count, head and tail, in that order.
template <class T> void NiTPointerListBase<T>::RemoveAll() {
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

// Clears the node's element (releasing what it holds), then returns the node to the pool. The allocator keeps no state
// of its own (the pool is its statics), so a local one frees to the same pool as the one the list folds into its count.
template <class T> void NiTPointerListBase<T>::DeleteItem(NiTListItem<T>* pkItem) {
	pkItem->m_element = T();
	NiTPointerAllocator<uint32_t> kAllocator;
	kAllocator.Deallocate(pkItem);
}

// The string list allocates its nodes from the heap, constructed (DFALL<NiTListItem<BSStringT<char>>>::Allocate), and
// frees them through the node's deleting destructor, which destroys the string. Defined in NiTPointerListBase.cpp.
template <> NiTListItem<BSStringT<char>>* NiTPointerListBase<BSStringT<char>>::NewItem();
template <> void NiTPointerListBase<BSStringT<char>>::DeleteItem(NiTListItem<BSStringT<char>>* pkItem);

template <class T> void NiTPointerListBase<T>::AddHead(const T& element) {
	NiTListItem<T>* pkNode = NewItem();
	pkNode->m_element = element;
	this->AddNodeHead(pkNode);
}

template <class T> void NiTPointerListBase<T>::AddTail(const T& element) {
	NiTListItem<T>* pkNode = NewItem();
	pkNode->m_element = element;
	this->AddNodeTail(pkNode);
}

template <class T> NiTListItem<T>* NiTPointerListBase<T>::InsertBefore(NiTListItem<T>* kPos, const T& element) {
	NiTListItem<T>* pkNode = NewItem();
	pkNode->m_element = element;
	return this->AddNodeBefore(kPos, pkNode);
}

template <class T> NiTListItem<T>* NiTPointerListBase<T>::InsertAfter(NiTListItem<T>* kPos, const T& element) {
	NiTListItem<T>* pkNode = NewItem();
	pkNode->m_element = element;
	return this->AddNodeAfter(kPos, pkNode);
}

template <class T> T NiTPointerListBase<T>::RemoveTail() {
	NiTListItem<T>* pkNode = this->m_pkTail;
	this->m_pkTail = pkNode->m_pkPrev;
	if (this->m_pkTail)
		this->m_pkTail->m_pkNext = nullptr;
	else
		this->m_pkHead = nullptr;
	T element = pkNode->m_element;
	DeleteItem(pkNode);
	this->m_uiCount--;
	return element;
}

// The head and the tail go through RemoveHead and RemoveTail; an inner node is unlinked here.
template <class T> T NiTPointerListBase<T>::RemovePos(NiTListItem<T>*& kPos) {
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
	if (pkPrev)
		pkPrev->m_pkNext = pkNext;
	if (pkNext)
		pkNext->m_pkPrev = pkPrev;
	T element = pkNode->m_element;
	DeleteItem(pkNode);
	this->m_uiCount--;
	return element;
}

// The removed element, or element itself when the list does not hold it.
template <class T> T NiTPointerListBase<T>::Remove(const T& element) {
	NiTListItem<T>* kPos = FindPos(element);
	return kPos ? RemovePos(kPos) : element;
}
