#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleArray.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

class BSMultiBoundRoom;
class BSOcclusionPlane;
class BSPortal;

// The occlusion and room/portal state of one cell's scene graph.
class BSPortalGraph : public NiRefObject {
public:
    BSPortalGraph();
    ~BSPortalGraph() override;

    void SetRoomRoot(BSMultiBoundRoom* apRoom);
    void SetPortalNodeRoot(NiNode* apNode);
    NiNode* GetRootNode() const;
    NiTPointerList<NiPointer<BSMultiBoundRoom>>* GetMBRoomAccumList();
    void ResetRoomAccumList();
    const NiTPointerList<NiPointer<BSMultiBoundRoom>>& QRooms() const;
    BSMultiBoundRoom* QRoomRoot();
    NiTPointerList<BSOcclusionPlane*>& QOccluderList();
    NiTPointerList<BSPortal*>& QPortalList();
    uint32_t GetAlwaysRenderArrayCount() const;
    NiAVObject* GetAlwaysRenderAt(uint32_t i) const;
    void ProcessQueue();
    void AddRoomToAccumList(BSMultiBoundRoom* apRoom);
    // Queues pkChild to join the always-rendered children.
    void AttachAlwaysRenderChild(NiAVObject* pkChild);
    // Queues pkChild to leave the always-rendered children.
    void DetachAlwaysRenderChild(NiAVObject* pkChild);
    // Adds apOccluder to the graph's occluder list.
    void AddOccluder(BSOcclusionPlane* apOccluder);
    void AddUnboundNode(NiNode* apNode);
    // Adds apRoom and apPortal to the graph's room and portal lists.
    void AddRoom(BSMultiBoundRoom* apRoom);
    void AddPortal(BSPortal* apPortal);
    void AttachQAlwaysRenderChild(NiAVObject* pkChild);
    void DetachQAlwaysRenderChild(NiAVObject* pkChild);
    void RemoveAllOccluders();
    void RemoveOccluder(BSOcclusionPlane* apOccluder);
    void RemoveRoom(BSMultiBoundRoom* apRoom);
    void RemovePortal(BSPortal* apPortal);
    void RemoveAllAlwaysRenderChildren();
    void RemoveUnboundNode(NiNode* apNode);

    // A pointer list: walked with NiTPointerListBase::GetNext and emptied with NiTPointerList<T*>::RemoveAll, as
    // BSMultiBoundRoom's list.
    NiTPointerList<BSOcclusionPlane*> lOccluderList;
    NiTPointerList<BSPortal*> lPortalList;
    // The list holds NiPointer elements: AddRoom adds to it, RemoveRoom removes from it, the destructor empties it, and
    // QRooms returns it as this type.
    NiTPointerList<NiPointer<BSMultiBoundRoom>> lMultiBoundRoomList;
    NiPointer<NiObject> spMultiBoundRoomRoot;
    NiTPointerList<NiPointer<BSMultiBoundRoom>> lMultiBoundRoomAccumList;
    NiTObjectArray<NiPointer<NiAVObject>> m_kAlwaysRenderChildren;
    NiPointer<NiNode> spPortalNodeRoot;
    NiTPointerList<NiPointer<NiAVObject>> kAttachAlwaysRenderQueue;
    NiTPointerList<NiPointer<NiAVObject>> kDetachAlwaysRenderQueue;

    BSSimpleArray<NiPointer<NiNode>, 1024> kUnboundNodes;

    static BSCriticalSection kAlwaysQueueCrit;
};

inline void BSPortalGraph::AddRoomToAccumList(BSMultiBoundRoom* apRoom) {
    if (apRoom) {
        apRoom->IncRefCount();
    }

    auto* pItem = lMultiBoundRoomAccumList.m_pkHead;
    bool bAdd = true;
    while (pItem) {
        auto* pElement = &pItem->m_element;
        pItem = pItem->m_pkNext;
        if (apRoom == pElement->m_pObject) {
            bAdd = false;
            break;
        }
    }
    NiRefObject* pRoom = apRoom;
    if (pRoom) {
        pRoom->DecRefCount();
    }

    if (bAdd) {
        NiPointer<BSMultiBoundRoom> spRoom;
        spRoom.m_pObject = apRoom;
        if (apRoom) {
            apRoom->IncRefCount();
        }
        lMultiBoundRoomAccumList.AddTail(spRoom);
        spRoom.m_pObject = nullptr;
        if (pRoom) {
            pRoom->DecRefCount();
        }
    }
}

inline const NiTPointerList<NiPointer<BSMultiBoundRoom>>& BSPortalGraph::QRooms() const {
    return lMultiBoundRoomList;
}

inline BSMultiBoundRoom* BSPortalGraph::QRoomRoot() {
    return static_cast<BSMultiBoundRoom*>(spMultiBoundRoomRoot.m_pObject);
}

inline NiTPointerList<BSOcclusionPlane*>& BSPortalGraph::QOccluderList() {
    return lOccluderList;
}
