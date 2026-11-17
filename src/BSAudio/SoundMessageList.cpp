#include "BSAudio/SoundMessageList.h"

#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSGameSound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"

void SoundMessageList::AddTail_0(BSSoundMessage** apMsg) {
    // The node comes from the node pool, called with the list's count field as its `this`: that field is treated as the
    // allocator, which has no storage of its own.
    NiTListItem<BSSoundMessage*>* pkNode = static_cast<NiTListItem<BSSoundMessage*>*>(
        reinterpret_cast<NiTPointerAllocator<uint32_t>&>(uiCount).Allocate()
    );
    pkNode->m_element = *apMsg;
    AddNodeTail(pkNode);
}

void SoundMessageList::AddNodeTail(NiTListItem<BSSoundMessage*>* apNode) {
    apNode->m_pkNext = 0;
    apNode->m_pkPrev = pTail;
    if (pTail) {
        pTail->m_pkNext = apNode;
    }
    else {
        pHead = apNode;
    }
    pTail = apNode;
    ++uiCount;
}

BSSoundMessage* SoundMessageList::RemoveHead() {
    NiTListItem<BSSoundMessage*>* pkNode = pHead;
    pHead = pHead->m_pkNext;
    if (pHead) {
        pHead->m_pkPrev = 0;
    }
    else {
        pTail = 0;
    }
    BSSoundMessage* pMsg = pkNode->m_element;
    pkNode->m_element = 0;
    reinterpret_cast<NiTPointerAllocator<uint32_t>&>(uiCount).Deallocate(pkNode);
    --uiCount;
    return pMsg;
}

void SoundMessageList::RemoveAll() {
    NiTListItem<BSSoundMessage*>* pkNode = pHead;
    while (pkNode) {
        NiTListItem<BSSoundMessage*>* pkNext = pkNode->m_pkNext;
        pkNode->m_element = 0;
        reinterpret_cast<NiTPointerAllocator<uint32_t>&>(uiCount).Deallocate(pkNode);
        pkNode = pkNext;
    }
    uiCount = 0;
    pHead = 0;
    pTail = 0;
}
