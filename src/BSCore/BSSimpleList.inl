class BSPortal;

template <class T>
bool BSSimpleList<T>::IsInList(const T& item) const {
    const BSSimpleList* node = this;
    while (node && node->m_item != item) {
        node = node->m_pkNext;
    }
    return node != nullptr;
}

// Counts the nodes that hold an item.
template <class T>
int32_t BSSimpleList<T>::ItemsInList() const {
    int32_t count = 0;
    for (const BSSimpleList* node = this; node; node = node->GetNext()) {
        if (node->GetItem()) {
            ++count;
        }
    }
    return count;
}

// The new node is allocated with ourMalloc and constructed with the head's item, linked in after the head, and only
// then does the head take the new item. On a failed allocation it skips the constructor but links the node anyway, as
// the game does.
template <class T>
void BSSimpleList<T>::AddHead(const T& item) {
    if (!item) {
        return;
    }

    if (m_item) {
        void* memory = ourMalloc(sizeof(BSSimpleList));
        BSSimpleList* node = memory ? new (memory) BSSimpleList(m_item) : nullptr;
        node->m_pkNext = m_pkNext;
        m_pkNext = node;
        m_item = item;
    }
    else {
        m_item = item;
    }
}

// The new node is allocated with ourMalloc and constructed with the item; on a failed allocation it skips the
// constructor and links the null result, as the game does.
template <class T>
void BSSimpleList<T>::AddTail(const T& item) {
    if (!item) {
        return;
    }

    BSSimpleList* last = this;
    while (last->m_pkNext) {
        last = last->m_pkNext;
    }

    if (last->m_item) {
        void* memory = ourMalloc(sizeof(BSSimpleList));
        BSSimpleList* node = memory ? new (memory) BSSimpleList(item) : nullptr;
        last->m_pkNext = node;
    }
    else {
        last->m_item = item;
    }
}

// A freed node is destroyed in two steps: RemoveAll runs on the unlinked node, which clears its item, then ourFree
// releases it. BSSimpleList has no destructor (every owner embeds one), so both steps are written out.
template <class T>
void BSSimpleList<T>::Remove(const T& item) {
    if (!item || IsEmpty()) {
        return;
    }

    BSSimpleList* node = this;
    BSSimpleList* prev = this;
    while (node && node->m_item != item) {
        prev = node;
        node = node->m_pkNext;
    }

    if (!node) {
        return;
    }

    if (node == this) {
        if (m_pkNext) {
            BSSimpleList* next = m_pkNext;
            m_pkNext = next->m_pkNext;
            m_item = next->m_item;
            next->m_pkNext = nullptr;
            next->RemoveAll();
            ourFree(next);
        }
        else {
            m_item = T();
        }
    }
    else {
        prev->m_pkNext = node->m_pkNext;
        node->m_pkNext = nullptr;
        node->RemoveAll();
        ourFree(node);
    }
}

// Each node is unlinked and freed through its deleting destructor (RemoveAll on the node, which clears its item, then
// ourFree); see Remove.
template <class T>
void BSSimpleList<T>::RemoveAll() {
    while (m_pkNext) {
        BSSimpleList* next = m_pkNext->m_pkNext;
        m_pkNext->m_pkNext = nullptr;
        BSSimpleList* node = m_pkNext;
        node->RemoveAll();
        ourFree(node);
        m_pkNext = next;
    }
    m_item = T();
}

// Sets the node's item; a null item is ignored.
template <class T>
void BSSimpleList<T>::SetItem(const T& item) {
    if (item) {
        m_item = item;
    }
}

// No next node and an empty item. The item test is the item type's own: `!m_item` works for pointers and integers
// alike.
template <class T>
bool BSSimpleList<T>::IsEmpty() const {
    return m_pkNext == nullptr && !m_item;
}

// Copies the items into a stack array, sorts that, then rebuilds the list in order: the head keeps the first item and
// each later item becomes a new node. A failed allocation links a null node, as the game does.
template <class T>
void BSSimpleList<T>::Sort(int32_t (*apCompareFn)(T, T)) {
    int32_t iCount = ItemsInList();
    // A 16-byte aligned stack array of one item per node, plus a spare.
    T* pArray = static_cast<T*>(__builtin_alloca_with_align(sizeof(T) * iCount + sizeof(T), 128));
    if (!pArray) {
        BubbleSort(apCompareFn);
        return;
    }
    BSSimpleList* pNode = this;
    for (int32_t i = 0; i < iCount; ++i) {
        pArray[i] = pNode->m_item;
        pNode = pNode->m_pkNext;
    }
    ShellSort(pArray, apCompareFn, iCount);
    RemoveAll();
    pNode = this;
    BSSimpleList* pPrevious = nullptr;
    for (int32_t i = 0; i < iCount; ++i) {
        if (i > 0) {
            void* memory = ourMalloc(sizeof(BSSimpleList));
            pNode = memory ? new (memory) BSSimpleList(pArray[i]) : nullptr;
        }
        else {
            pNode->m_item = pArray[i];
            pNode->m_pkNext = nullptr;
        }
        if (pPrevious) {
            pPrevious->m_pkNext = pNode;
        }
        pPrevious = pNode;
    }
}

// The next node's item and link move into the head; the unlinked node is freed. An only node just loses
// its item.
template <class T>
void BSSimpleList<T>::RemoveHead() {
    BSSimpleList* pNext = m_pkNext;
    if (pNext) {
        m_pkNext = pNext->m_pkNext;
        m_item = pNext->m_item;
        pNext->m_pkNext = nullptr;
        delete pNext;
    }
    else {
        m_item = T();
    }
}

// Shell sort with the 3h+1 gap sequence, the largest gap not above a ninth of the array.
template <class T>
void BSSimpleList<T>::ShellSort(T* apArray, int32_t (*apCompareFn)(T, T), int32_t aiCount) {
    int32_t iLast = aiCount - 1;
    int32_t iGap = 1;
    while (iGap <= iLast / 9) {
        iGap = 3 * iGap + 1;
    }
    while (iGap > 0) {
        for (int32_t i = iGap; i <= iLast; ++i) {
            int32_t j = i;
            T item = apArray[i];
            while (j >= iGap && apCompareFn(item, apArray[j - iGap]) < 0) {
                apArray[j] = apArray[j - iGap];
                j -= iGap;
            }
            apArray[j] = item;
        }
        iGap /= 3;
    }
}

// Swaps neighbouring items apCompare ranks out of order. The walk is deliberately never rewound to the head, as the
// game does, so only the first pass compares anything.
template <class T>
void BSSimpleList<T>::BubbleSort(int32_t (*apCompareFn)(T, T)) {
    BSSimpleList* pkCur = this;
    BSSimpleList* pkPrev = nullptr;
    int32_t iCount = ItemsInList();
    bool bSwapped = true;
    while (iCount && bSwapped) {
        bSwapped = false;
        while (pkCur) {
            if (pkPrev) {
                T curItem = pkCur->GetItem();
                T prevItem = pkPrev->GetItem();
                if (apCompareFn(prevItem, curItem) > 0) {
                    pkCur->SetItem(prevItem);
                    pkPrev->SetItem(curItem);
                    bSwapped = true;
                }
            }
            pkPrev = pkCur;
            pkCur = pkCur->GetNext();
        }
        --iCount;
    }
}

// Walks to the first item apCompare does not rank apNewItem above and links a new node before it (AddHead at the head),
// or after the last item; an empty node takes the item. A failed allocation links the null node, deliberately, as the
// game does.
template <class T>
void BSSimpleList<T>::Insert(T apNewItem, int32_t (*apCompareFn)(T, T)) {
    if (!apNewItem) {
        return;
    }
    BSSimpleList* pIter = this;
    BSSimpleList* pPrev = nullptr;
    bool bAdded = false;
    while (pIter && !bAdded) {
        if (pIter->m_item) {
            if (apCompareFn(apNewItem, pIter->m_item) > 0) {
                if (!pIter->m_pkNext) {
                    void* pMemory = ourMalloc(sizeof(BSSimpleList));
                    pIter->m_pkNext = pMemory ? new (pMemory) BSSimpleList(apNewItem) : nullptr;
                    bAdded = true;
                }
            }
            else {
                if (pPrev) {
                    void* pMemory = ourMalloc(sizeof(BSSimpleList));
                    BSSimpleList* pNewEntry = pMemory ? new (pMemory) BSSimpleList(apNewItem) : nullptr;
                    pPrev->m_pkNext = pNewEntry;
                    pNewEntry->m_pkNext = pIter;
                }
                else {
                    AddHead(apNewItem);
                }
                bAdded = true;
            }
        }
        else {
            pIter->m_item = apNewItem;
            bAdded = true;
        }
        pPrev = pIter;
        pIter = pIter->m_pkNext;
    }
}

// Calls apTraverseFn on the head's item, then, while it returns true, on the rest of the list.
template <class T>
void BSSimpleList<T>::Traverse(bool (*apTraverseFn)(T, uint32_t), uint32_t aiIn) {
    if (apTraverseFn(m_item, aiIn)) {
        if (GetNext()) {
            GetNext()->Traverse(apTraverseFn, aiIn);
        }
    }
}

// RemoveAll, then AddTail of each of apSource's items, unless apSource is empty.
template <class T>
void BSSimpleList<T>::CopyList(const BSSimpleList* apSource) {
    RemoveAll();
    const BSSimpleList* pkIter = apSource;
    if (!pkIter->IsEmpty()) {
        while (pkIter) {
            AddTailByValue(pkIter->m_item);
            pkIter = pkIter->GetNext();
        }
    }
}
