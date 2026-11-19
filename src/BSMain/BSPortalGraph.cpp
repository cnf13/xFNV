#include "BSMain/BSPortalGraph.h"

#include "BSCore/BSSimpleArray.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSPortal.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "SDK/MSVC/stdlib.h"

BSCriticalSection BSPortalGraph::kAlwaysQueueCrit;

// Points the smart pointer at apNew: releases the old target, then references the new one.
template <class T>
static void AssignPointer(NiPointer<T>& arpPointer, T* apNew) {
    if (arpPointer.m_pObject == apNew) {
        return;
    }

    if (arpPointer.m_pObject) {
        NiDecRefCount(arpPointer.m_pObject);
    }
    arpPointer.m_pObject = apNew;
    if (apNew) {
        NiIncRefCount(apNew);
    }
}

// Empty lists and arrays, no room root and no portal node root.
BSPortalGraph::BSPortalGraph()
  : spMultiBoundRoomRoot(nullptr), m_kAlwaysRenderChildren(0, 1), spPortalNodeRoot(nullptr) {}

// Empties the lists and the always-rendered children and drops both roots; the members' destructors do the rest.
BSPortalGraph::~BSPortalGraph() {
    lPortalList.RemoveAll();
    lMultiBoundRoomList.RemoveAll();
    lOccluderList.RemoveAll();
    m_kAlwaysRenderChildren.RemoveAll();
    spMultiBoundRoomRoot = nullptr;
    spPortalNodeRoot = nullptr;
}

void BSPortalGraph::SetRoomRoot(BSMultiBoundRoom* apRoom) {
    AssignPointer<NiObject>(spMultiBoundRoomRoot, apRoom);
}

void BSPortalGraph::SetPortalNodeRoot(NiNode* apNode) {
    AssignPointer(spPortalNodeRoot, apNode);
}

NiNode* BSPortalGraph::GetRootNode() const {
    return spPortalNodeRoot.m_pObject;
}

NiTPointerList<NiPointer<BSMultiBoundRoom>>* BSPortalGraph::GetMBRoomAccumList() {
    return &lMultiBoundRoomAccumList;
}

void BSPortalGraph::ResetRoomAccumList() {
    lMultiBoundRoomAccumList.RemoveAll();
}

NiTPointerList<BSPortal*>& BSPortalGraph::QPortalList() {
    return lPortalList;
}

uint32_t BSPortalGraph::GetAlwaysRenderArrayCount() const {
    return m_kAlwaysRenderChildren.m_usESize;
}

NiAVObject* BSPortalGraph::GetAlwaysRenderAt(uint32_t i) const {
    return m_kAlwaysRenderChildren.m_pBase[i].m_pObject;
}

void BSPortalGraph::ProcessQueue() {
    EnterCriticalSection(&kAlwaysQueueCrit.CriticalSection);
    auto* pItem = kAttachAlwaysRenderQueue.m_pkHead;
    while (pItem) {
        auto* pElement = &pItem->m_element;
        pItem = pItem->m_pkNext;
        NiAVObject* pChild = pElement->m_pObject;
        if (pChild) {
            AttachQAlwaysRenderChild(pChild);
        }
    }
    kAttachAlwaysRenderQueue.RemoveAll();
    pItem = kDetachAlwaysRenderQueue.m_pkHead;
    while (pItem) {
        auto* pElement = &pItem->m_element;
        pItem = pItem->m_pkNext;
        NiAVObject* pChild = pElement->m_pObject;
        if (pChild) {
            DetachQAlwaysRenderChild(pChild);
        }
    }
    kDetachAlwaysRenderQueue.RemoveAll();
    LeaveCriticalSection(&kAlwaysQueueCrit.CriticalSection);
}

void BSPortalGraph::AttachAlwaysRenderChild(NiAVObject* pkChild) {
    if (!pkChild) {
        return;
    }
    EnterCriticalSection(&BSPortalGraph::kAlwaysQueueCrit.CriticalSection);
    // Tests the position after it has stepped past the match, so a child that is the queue's last entry is queued
    // again, as the game does.
    NiTListItem<NiPointer<NiAVObject>>* pkPos = kAttachAlwaysRenderQueue.m_pkHead;
    while (pkPos) {
        NiAVObject* pkEntry = pkPos->m_element.m_pObject;
        pkPos = pkPos->m_pkNext;
        if (pkEntry == pkChild) {
            break;
        }
    }
    if (!pkPos) {
        kAttachAlwaysRenderQueue.AddTail(pkChild);
        pkPos = kDetachAlwaysRenderQueue.m_pkHead;
        while (pkPos) {
            NiTListItem<NiPointer<NiAVObject>>* kPos = pkPos;
            NiAVObject* pkEntry = pkPos->m_element.m_pObject;
            pkPos = pkPos->m_pkNext;
            if (pkEntry == pkChild) {
                // The removed element comes back by value and is released right away.
                kDetachAlwaysRenderQueue.RemovePos(kPos);
            }
        }
    }
    LeaveCriticalSection(&BSPortalGraph::kAlwaysQueueCrit.CriticalSection);
}

void BSPortalGraph::DetachAlwaysRenderChild(NiAVObject* pkChild) {
    EnterCriticalSection(&BSPortalGraph::kAlwaysQueueCrit.CriticalSection);
    kDetachAlwaysRenderQueue.AddTail(pkChild);
    NiTListItem<NiPointer<NiAVObject>>* pkPos = kAttachAlwaysRenderQueue.m_pkHead;
    while (pkPos) {
        NiTListItem<NiPointer<NiAVObject>>* kPos = pkPos;
        NiAVObject* pkEntry = pkPos->m_element.m_pObject;
        pkPos = pkPos->m_pkNext;
        if (pkEntry == pkChild) {
            // The removed element comes back by value and is released right away.
            kAttachAlwaysRenderQueue.RemovePos(kPos);
            break;
        }
    }
    LeaveCriticalSection(&BSPortalGraph::kAlwaysQueueCrit.CriticalSection);
}

void BSPortalGraph::AddOccluder(BSOcclusionPlane* apOccluder) {
    if (apOccluder) {
        // The node comes from the node pool, called with the list's count field as its `this`: that field is treated as
        // the allocator, which has no storage of its own.
        NiTListItem<BSOcclusionPlane*>* pkNode = static_cast<NiTListItem<BSOcclusionPlane*>*>(
            reinterpret_cast<NiTPointerAllocator<uint32_t>&>(lOccluderList.m_uiCount).Allocate()
        );
        pkNode->m_element = apOccluder;
        lOccluderList.AddNodeTail(pkNode);
    }
}

void BSPortalGraph::AddUnboundNode(NiNode* apNode) {
    if (!apNode) {
        return;
    }

    NiPointer<NiNode> spNode;
    spNode.m_pObject = apNode;
    apNode->IncRefCount();
    kUnboundNodes.Add(spNode);
    NiRefObject* pNode = spNode.m_pObject;
    spNode.m_pObject = nullptr;
    if (pNode) {
        pNode->DecRefCount();
    }
}

void BSPortalGraph::AddRoom(BSMultiBoundRoom* apRoom) {
    if (apRoom) {
        NiTListItem<NiPointer<BSMultiBoundRoom>>* v4;
        NiIncRefCount(apRoom);
        NiTListItem<NiPointer<BSMultiBoundRoom>>* m_pkHead = lMultiBoundRoomList.m_pkHead;
        while (m_pkHead) {
            NiTListItem<NiPointer<BSMultiBoundRoom>>* v7 = m_pkHead;
            BSMultiBoundRoom** p_m_element = &m_pkHead->m_element.m_pObject;
            m_pkHead = m_pkHead->m_pkNext;
            if (apRoom == *p_m_element) {
                v4 = v7;
                goto LABEL_8;
            }
        }
        v4 = nullptr;
    LABEL_8:;
        bool v10 = v4 == nullptr;
        NiDecRefCount(apRoom);
        if (v10) {
            NiPointer<BSMultiBoundRoom> spRoom = apRoom;
            lMultiBoundRoomList.AddTail(spRoom);
        }
    }
}

void BSPortalGraph::AddPortal(BSPortal* apPortal) {
    if (apPortal) {
        // The node comes from the node pool, called with the list's count field as its `this`: that field is treated as
        // the allocator, which has no storage of its own.
        NiTListItem<BSPortal*>* pkNode = static_cast<NiTListItem<BSPortal*>*>(
            reinterpret_cast<NiTPointerAllocator<uint32_t>&>(lPortalList.m_uiCount).Allocate()
        );
        pkNode->m_element = apPortal;
        lPortalList.AddNodeTail(pkNode);
    }
}

void BSPortalGraph::AttachQAlwaysRenderChild(NiAVObject* pkChild) {
    if (!pkChild) {
        return;
    }
    bool bFound = false;
    for (const NiPointer<NiAVObject>& spEntry : m_kAlwaysRenderChildren) {
        if (spEntry.m_pObject && spEntry.m_pObject == pkChild) {
            bFound = true;
        }
    }
    if (!bFound) {
        NiPointer<NiAVObject> spChild = pkChild;
        NiAVObjectArray_AddFirstEmpty(&m_kAlwaysRenderChildren, &spChild);
    }
}

void BSPortalGraph::DetachQAlwaysRenderChild(NiAVObject* pkChild) {
    for (uint32_t a3 = 0; a3 < m_kAlwaysRenderChildren.m_usSize; ++a3) {
        NiAVObject* m_pObject = m_kAlwaysRenderChildren.m_pBase[a3].m_pObject;
        if (m_pObject) {
            NiIncRefCount(m_pObject);
        }
        if (m_pObject && m_pObject == pkChild) {
            {
                NiPointer<NiAVObject> a2 = (m_kAlwaysRenderChildren.RemoveAtAndFill(a3));
            }
            m_pObject->m_uFlags &= ~1u;
            if (m_pObject) {
                NiDecRefCount(m_pObject);
            }
            return;
        }
        if (m_pObject) {
            NiDecRefCount(m_pObject);
        }
    }
}

void BSPortalGraph::RemoveAllOccluders() {
    lOccluderList.RemoveAll();
}

void BSPortalGraph::RemoveOccluder(BSOcclusionPlane* apOccluder) {
    if (apOccluder) {
        static_cast<NiTPointerListBase<BSOcclusionPlane*>*>(&lOccluderList)->Remove(apOccluder);
    }
}

// The element Remove returns is released at once; then the local reference is released.
void BSPortalGraph::RemoveRoom(BSMultiBoundRoom* apRoom) {
    if (apRoom) {
        NiPointer<BSMultiBoundRoom> spRoom = apRoom;
        lMultiBoundRoomList.Remove(spRoom);
    }
}

void BSPortalGraph::RemovePortal(BSPortal* apPortal) {
    if (apPortal) {
        lPortalList.Remove(apPortal);
    }
}

void BSPortalGraph::RemoveAllAlwaysRenderChildren() {
    for (NiAVObject* pChild : m_kAlwaysRenderChildren) {
        if (pChild) {
            NiIncRefCount(pChild);
            pChild->m_uFlags &= ~1u;
            NiDecRefCount(pChild);
        }
    }
    m_kAlwaysRenderChildren.RemoveAll();
}

void BSPortalGraph::RemoveUnboundNode(NiNode* apNode) {
    uint32_t ifindindex_2;
    for (uint32_t ifindindex = 0; ifindindex < kUnboundNodes.iSize; ++ifindindex) {
        if (kUnboundNodes.pBuffer && kUnboundNodes.pBuffer[ifindindex].m_pObject == apNode) {
            ifindindex_2 = ifindindex;
            goto LABEL_8;
        }
    }
    ifindindex_2 = -1;
LABEL_8:;
    if (ifindindex_2 != -1) {
        kUnboundNodes.RemoveFast(ifindindex_2, 1u);
    }
}

// The vector deleting destructor of NiPointer<NiAVObject>: releases and frees the array apBase, auiFlags 3.
void* AlwaysRenderArray_DeleteBase(NiPointer<NiAVObject>* apBase, uint32_t auiFlags) {
    if ((auiFlags & 2) != 0) {
        // The element count new[] stores before the array (its cookie), read untyped.
        int32_t iCount = static_cast<int32_t>(reinterpret_cast<size_t*>(apBase)[-1]);
        for (int32_t i = iCount; i-- > 0;) {
            apBase[i].~NiPointer<NiAVObject>();
        }
        if ((auiFlags & 1) != 0) {
            ourFree(reinterpret_cast<size_t*>(apBase) - 1);
        }
        return reinterpret_cast<size_t*>(apBase) - 1;
    }

    apBase->~NiPointer<NiAVObject>();
    if ((auiFlags & 1) != 0) {
        ourFree(apBase);
    }
    return (void*)apBase;
}
