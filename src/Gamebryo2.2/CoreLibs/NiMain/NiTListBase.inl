#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"

template <class T>
void NiTListBase<T>::AddNodeTail(NiTListItem<T>* pkNode) {
    pkNode->m_pkNext = nullptr;
    pkNode->m_pkPrev = m_pkTail;
    if (m_pkTail) {
        m_pkTail->m_pkNext = pkNode;
        ++m_uiCount;
    }
    else {
        ++m_uiCount;
        m_pkHead = pkNode;
    }
    m_pkTail = pkNode;
}

template <class T>
NiTListItem<T>* NiTListBase<T>::GetNextPos(NiTListItem<T>* kPos) const {
    if (kPos) {
        return kPos->m_pkNext;
    }
    return nullptr;
}

template <class T>
uint32_t NiTListBase<T>::GetSize() const {
    return m_uiCount;
}

template <class T>
bool NiTListBase<T>::IsEmpty() const {
    return m_uiCount == 0;
}

template <class T>
NiTListItem<T>* NiTListBase<T>::GetHeadPos() const {
    return m_pkHead;
}

template <class T>
NiTListItem<T>* NiTListBase<T>::GetTailPos() const {
    return m_pkTail;
}

template <class T>
NiTListItem<T>* NiTListBase<T>::GetPrevPos(NiTListItem<T>* kPos) const {
    if (kPos) {
        return kPos->m_pkPrev;
    }
    return nullptr;
}

template <class T>
void NiTListBase<T>::AddNodeHead(NiTListItem<T>* pkNode) {
    pkNode->m_pkPrev = nullptr;
    pkNode->m_pkNext = m_pkHead;
    if (m_pkHead) {
        m_pkHead->m_pkPrev = pkNode;
    }
    else {
        m_pkTail = pkNode;
    }
    m_pkHead = pkNode;
    ++m_uiCount;
}

template <class T>
NiTListItem<T>* NiTListBase<T>::AddNodeAfter(NiTListItem<T>* kPos, NiTListItem<T>* pkNode) {
    pkNode->m_pkPrev = kPos;
    pkNode->m_pkNext = kPos->m_pkNext;
    if (kPos->m_pkNext) {
        kPos->m_pkNext->m_pkPrev = pkNode;
    }
    else {
        m_pkTail = pkNode;
    }
    kPos->m_pkNext = pkNode;
    ++m_uiCount;
    return pkNode;
}

template <class T>
NiTListItem<T>* NiTListBase<T>::AddNodeBefore(NiTListItem<T>* kPos, NiTListItem<T>* pkNode) {
    pkNode->m_pkNext = kPos;
    pkNode->m_pkPrev = kPos->m_pkPrev;
    if (kPos->m_pkPrev) {
        kPos->m_pkPrev->m_pkNext = pkNode;
    }
    else {
        m_pkHead = pkNode;
    }
    kPos->m_pkPrev = pkNode;
    ++m_uiCount;
    return pkNode;
}

// The head and tail are fixed before the node is unlinked; the count is not touched.
template <class T>
void NiTListBase<T>::MoveToBefore(NiTListItem<T>* kObjectToMove, NiTListItem<T>* kDestination) {
    if (kObjectToMove == kDestination) {
        return;
    }

    if (m_pkHead == kObjectToMove) {
        m_pkHead = kObjectToMove->m_pkNext;
    }
    if (m_pkHead == kDestination) {
        m_pkHead = kObjectToMove;
    }
    if (m_pkTail == kObjectToMove) {
        m_pkTail = kObjectToMove->m_pkPrev;
    }
    if (kObjectToMove->m_pkNext) {
        kObjectToMove->m_pkNext->m_pkPrev = kObjectToMove->m_pkPrev;
    }
    if (kObjectToMove->m_pkPrev) {
        kObjectToMove->m_pkPrev->m_pkNext = kObjectToMove->m_pkNext;
    }
    NiTListItem<T>* pkPrev = kDestination->m_pkPrev;
    kObjectToMove->m_pkPrev = pkPrev;
    kObjectToMove->m_pkNext = kDestination;
    if (pkPrev) {
        pkPrev->m_pkNext = kObjectToMove;
    }
    kDestination->m_pkPrev = kObjectToMove;
}

// As MoveToBefore.
template <class T>
void NiTListBase<T>::MoveToAfter(NiTListItem<T>* kObjectToMove, NiTListItem<T>* kDestination) {
    if (kObjectToMove == kDestination) {
        return;
    }

    if (m_pkHead == kObjectToMove) {
        m_pkHead = kObjectToMove->m_pkNext;
    }
    if (m_pkTail == kObjectToMove) {
        m_pkTail = kObjectToMove->m_pkPrev;
    }
    if (m_pkTail == kDestination) {
        m_pkTail = kObjectToMove;
    }
    if (kObjectToMove->m_pkNext) {
        kObjectToMove->m_pkNext->m_pkPrev = kObjectToMove->m_pkPrev;
    }
    if (kObjectToMove->m_pkPrev) {
        kObjectToMove->m_pkPrev->m_pkNext = kObjectToMove->m_pkNext;
    }
    NiTListItem<T>* pkNext = kDestination->m_pkNext;
    kObjectToMove->m_pkNext = pkNext;
    kObjectToMove->m_pkPrev = kDestination;
    if (pkNext) {
        pkNext->m_pkPrev = kObjectToMove;
    }
    kDestination->m_pkNext = kObjectToMove;
}

// Clears each node's element (a smart pointer releases its object) and returns the node to the shared node pool, then
// zeroes the count, head and tail.
template <class T>
void NiTListBase<T>::RemoveAll() {
    NiTListItem<T>* pkCurr = m_pkHead;
    while (pkCurr) {
        NiTListItem<T>* pkDel = pkCurr;
        pkCurr = pkCurr->m_pkNext;
        pkDel->m_element = T();
        NiTPointerAllocator<uint32_t> kAllocator;
        kAllocator.Deallocate(pkDel);
    }
    m_uiCount = 0;
    m_pkHead = nullptr;
    m_pkTail = nullptr;
}
