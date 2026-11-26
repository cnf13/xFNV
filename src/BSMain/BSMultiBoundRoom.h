#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSMain/BSMultiBoundNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"

class BSCompoundFrustum;
class BSOcclusionPlane;
class BSPortal;

// A plain pointer list's destructor.
void RemoveAllOcclusionPlanes(NiTPointerListBase<BSOcclusionPlane*>* apThis, [[maybe_unused]] void* apEdx);

// a room of the portal graph: a BSMultiBoundNode the engine creates at runtime and never streams, so its stream
// virtuals are stubs
class BSMultiBoundRoom : public BSMultiBoundNode {
public:
    BSMultiBoundRoom();

    ~BSMultiBoundRoom() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess&) override {
        return nullptr;
    }
    void LoadBinary(NiStream&) override {}
    void LinkObject(NiStream&) override {}
    bool RegisterStreamables(NiStream&) override {
        return true;
    }
    void SaveBinary(NiStream&) override {}
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;
    void UpdateWorldBound() override;
    void OnVisible(NiCullingProcess& kCuller) override;
    BSMultiBoundRoom* GetMultiBoundRoom() override {
        return this;
    }
    bool QPointWithin(NiPoint3& aPoint) override;
    BSMultiBoundShape::BSMBIntersectResult CheckBound(const NiBound& aTargetBound) override;
    BSMultiBoundShape::BSMBIntersectResult CheckBound(const BSMultiBound& aTargetBound) override;

    // Deletes the room's compound frustum and clears the pointer.
    void RemoveFrustum();
    void SetCompoundFrustum(BSCompoundFrustum* apFrustum);
    NiTPointerList<BSPortal*>& QPortalList();
    // Returns the joined multibound list, kJoinedMultiBoundList.
    NiTPointerList<NiPointer<BSMultiBound>>& QJoinedMultiBoundList();
    // Joins apBound to the room's culling.
    void AddJoinedMultibound(BSMultiBound* apBound);
    const NiTPointerList<BSOcclusionPlane*>& GetOccluders() const;

    // Sets the cull result of the room's shape and of each joined bound's shape (inline in the engine).
    void SetCullResult(BSMultiBoundShape::BSMBCullResult kcullres) {
        spMultiBound->spShape->kCullResult = kcullres;
        for (NiTListItem<NiPointer<BSMultiBound>>* pItem = kJoinedMultiBoundList.m_pkHead; pItem;) {
            BSMultiBound* pBound = pItem->m_element.m_pObject;
            pItem = pItem->m_pkNext;
            if (pBound) {
                pBound->spShape->kCullResult = kcullres;
            }
        }
    }

    BSMultiBoundShape::BSMBCullResult GetCullResult();
    void AddOccluder(BSOcclusionPlane* apOccluder);
    void RemoveOccluder(BSOcclusionPlane* apOccluder);

    NiTPointerList<BSPortal*> kPortalList;
    NiTPointerList<BSOcclusionPlane*> lOccluderList;
    BSCompoundFrustum* pCompoundFrustum;
    NiTPointerList<NiPointer<BSMultiBound>> kJoinedMultiBoundList;
    BSSimpleArray<ShadowSceneLight*, 1024> kLights;

    static NiRTTI ms_RTTI;
};

NiSmartPointer(BSMultiBoundRoom);

inline NiTPointerList<NiPointer<BSMultiBound>>& BSMultiBoundRoom::QJoinedMultiBoundList() {
    return kJoinedMultiBoundList;
}

inline const NiTPointerList<BSOcclusionPlane*>& BSMultiBoundRoom::GetOccluders() const {
    return lOccluderList;
}
