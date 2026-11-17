template <class T> T BSPointerListBase<T>::RemoveHead() {
	BSListItem<T>* pkHead = this->m_pkHead;
	this->m_pkHead = pkHead->m_pkNext;
	if (this->m_pkHead != nullptr) {
		this->m_pkHead->m_pkPrev = nullptr;
	} else {
		this->m_pkTail = nullptr;
	}
	T element = pkHead->m_element;
	// Frees the detached node through the list's DeleteItem.
	this->DeleteItem(pkHead);
	--this->m_uiCount;
	return element;
}

template <class T> BSListItem<T>* BSPointerListBase<T>::NewItem() {
	return static_cast<BSListItem<T>*>(ourMalloc(sizeof(BSListItem<T>)));
}

// Clears the node's element before freeing it.
template <class T> void BSPointerListBase<T>::DeleteItem(BSListItem<T>* pkItem) {
	pkItem->m_element = nullptr;
	ourFree(pkItem);
}

// Frees every node through DeleteItem and empties the list.
template <class T> void BSListBase<T>::RemoveAll() {
	BSListItem<T>* pkNode = m_pkHead;
	while (pkNode) {
		BSListItem<T>* pkDelete = pkNode;
		pkNode = pkNode->m_pkNext;
		DeleteItem(pkDelete);
	}
	m_uiCount = 0;
	m_pkHead = nullptr;
	m_pkTail = nullptr;
}

// The node before apPos; null for a null apPos.
template <class T> void* BSListBase<T>::GetPrevPos(void* kPos) {
	if (kPos) {
		return static_cast<BSListItem<T>*>(kPos)->m_pkPrev;
	}
	return nullptr;
}

// Links apItem in as the first node.
template <class T> void BSListBase<T>::AddNodeHead(BSListItem<T>* pkNode) {
	pkNode->m_pkPrev = nullptr;
	pkNode->m_pkNext = m_pkHead;
	if (m_pkHead) {
		m_pkHead->m_pkPrev = pkNode;
	} else {
		m_pkTail = pkNode;
	}
	m_pkHead = pkNode;
	++m_uiCount;
}

// Links apItem in as the last node.
template <class T> void BSListBase<T>::AddNodeTail(BSListItem<T>* pkNode) {
	pkNode->m_pkNext = nullptr;
	pkNode->m_pkPrev = m_pkTail;
	if (m_pkTail) {
		m_pkTail->m_pkNext = pkNode;
	} else {
		m_pkHead = pkNode;
	}
	m_pkTail = pkNode;
	++m_uiCount;
}

// Whether the list has no nodes.
template <class T> bool BSListBase<T>::IsEmpty() const {
	return m_uiCount == 0;
}

// Adds arElement at the head in a NewItem node.
template <class T> void BSPointerListBase<T>::AddHead(const T& element) {
	BSListItem<T>* pkItem = NewItem();
	pkItem->m_element = element;
	this->AddNodeHead(pkItem);
}

// Removes the node at arpPos and returns its element; arpPos moves to the next node (null after the tail). The head
// and the tail go through RemoveHead and RemoveTail.
template <class T> T BSPointerListBase<T>::RemovePos(void*& kPos) {
	BSListItem<T>* pkNode = static_cast<BSListItem<T>*>(kPos);
	if (pkNode == this->m_pkHead) {
		kPos = pkNode->m_pkNext;
		return RemoveHead();
	}
	if (pkNode == this->m_pkTail) {
		kPos = nullptr;
		return RemoveTail();
	}
	BSListItem<T>* pkPrev = pkNode->m_pkPrev;
	BSListItem<T>* pkNext = pkNode->m_pkNext;
	kPos = pkNext;
	if (pkPrev) {
		pkPrev->m_pkNext = pkNext;
	}
	if (pkNext) {
		pkNext->m_pkPrev = pkPrev;
	}
	T element = pkNode->m_element;
	this->DeleteItem(pkNode);
	--this->m_uiCount;
	return element;
}

// The element at arpPos; arpPos moves to the previous node.
template <class T> T& BSPointerListBase<T>::GetPrev(void*& kPos) {
	BSListItem<T>* pkNode = static_cast<BSListItem<T>*>(kPos);
	kPos = pkNode->m_pkPrev;
	return pkNode->m_element;
}

// Adds arElement at the tail in a NewItem node.
template <class T> void BSPointerListBase<T>::AddTail(const T& element) {
	BSListItem<T>* pkItem = NewItem();
	pkItem->m_element = element;
	this->AddNodeTail(pkItem);
}

// Removes the last node from the list and returns its element.
template <class T> T BSPointerListBase<T>::RemoveTail() {
	BSListItem<T>* pkTail = this->m_pkTail;
	this->m_pkTail = pkTail->m_pkPrev;
	if (this->m_pkTail) {
		this->m_pkTail->m_pkNext = nullptr;
	} else {
		this->m_pkHead = nullptr;
	}
	T element = pkTail->m_element;
	this->DeleteItem(pkTail);
	--this->m_uiCount;
	return element;
}
