#include "BSMain/BSPortal.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSCompoundFrustum.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/BSMultiBoundShape.h"
#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSPortalSharedNode.h"
#include "BSShader/ShadowSceneLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

NiRTTI BSPortal::ms_RTTI("BSPortal", &BSOcclusionPlane::ms_RTTI);
NiRTTI BSPortalSharedNode::ms_RTTI("BSPortalSharedNode", &NiNode::ms_RTTI);

// The default constructor deliberately leaves the rooms unset.
BSPortal::BSPortal() : spSharedGeometry(nullptr) {}

BSPortal::BSPortal(NiPoint3* apPosition, NiPoint2* apBounds, NiMatrix3* apMatrix)
  : BSOcclusionPlane(apPosition, apBounds, apMatrix), pMultiBoundRoom{nullptr, nullptr}, spSharedGeometry(nullptr) {}

const NiRTTI* BSPortal::GetRTTI() const {
    return &BSPortal::ms_RTTI;
}

NiObject* BSPortal::CreateClone(NiCloningProcess& arCloneData) {
    BSPortal* pobj;
    pobj = new BSPortal;
    CopyMembers(pobj, arCloneData);
    return pobj;
}

// spSharedGeometry releases itself.
BSPortal::~BSPortal() {}

void BSPortal::SetMultiBoundRoomLeft(BSMultiBoundRoom* apRoom) {
    pMultiBoundRoom[0] = apRoom;
}

void BSPortal::SetMultiBoundRoomRight(BSMultiBoundRoom* apRoom) {
    pMultiBoundRoom[1] = apRoom;
}

void BSPortal::SetSharedGeometryNode(BSPortalSharedNode* apNode) {
    spSharedGeometry = apNode;
}

uint32_t BSPortal::AnyRoomsVisible() {
    int32_t v5 = 0;
    BSMultiBoundRoom* v3 = pMultiBoundRoom[0];
    if (v3) {
        v5 = v3->GetCullResult() == BSMultiBoundShape::BS_CULL_VISIBLE;
    }
    BSMultiBoundRoom* v4 = pMultiBoundRoom[1];
    if (v4) {
        return v5 | (v4->GetCullResult() == BSMultiBoundShape::BS_CULL_VISIBLE);
    }
    return v5;
}

void BSPortal::CopyMembers(BSPortal* pDest, NiCloningProcess& arCloneData) {
    pDest->pMultiBoundRoom[0] = pMultiBoundRoom[0];
    pDest->pMultiBoundRoom[1] = pMultiBoundRoom[1];
}

// The node gets its own compound frustum from the engine heap and is always drawn.
BSPortalSharedNode::BSPortalSharedNode(BSPortal* apPortal) : NiNode(0), pCompoundFrustum(nullptr), pPortal(nullptr) {
    pCompoundFrustum = new BSCompoundFrustum();
    m_uFlags |= ALWAYS_DRAW;
    pPortal = apPortal;
}

NiObject* BSPortalSharedNode::CreateClone(NiCloningProcess& arCloneData) {
    BSPortalSharedNode* v3;
    v3 = new BSPortalSharedNode(pPortal);
    CopyMembers(v3, arCloneData);
    return v3;
}

// Culls the children with the culler's compound frustum swapped for the node's own, then restores the culler's.
void BSPortalSharedNode::OnVisible(NiCullingProcess& kCuller) {
    if (m_kChildren.m_usESize) {
        BSCullingProcess* pCuller = static_cast<BSCullingProcess*>(&kCuller);
        BSCompoundFrustum* pOrgFrustum = pCuller->pCompoundFrustum;
        pCuller->pCompoundFrustum = pCompoundFrustum;
        if (pCompoundFrustum) {
            pCompoundFrustum->PrethreadOpList();
        }
        uint32_t uiChildCount = m_kChildren.m_usSize;
        for (uint32_t i = 0; i < uiChildCount; ++i) {
            NiAVObject* pkChild = m_kChildren.m_pBase[i].m_pObject;
            if (pkChild) {
                pkChild->Cull(kCuller);
            }
        }
        pCuller->pCompoundFrustum = pOrgFrustum;
    }
}

// The compound frustum is destroyed and freed through the engine heap, the lights are dropped and the portal cleared.
BSPortalSharedNode::~BSPortalSharedNode() {
    delete pCompoundFrustum;
    kLights.Clear(true);
    pPortal = nullptr;
}

void BSPortalSharedNode::CopyMembers(BSPortalSharedNode* pDest, NiCloningProcess& arCloneData) {}
