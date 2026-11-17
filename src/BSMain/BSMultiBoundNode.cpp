#include "BSMain/BSMultiBoundNode.h"

#include "BSMain/BSMultiBound.h"
#include "BSMain/BSNiNode.h"
#include "BSMain/BSOcclusionPlane.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSMultiBoundNode::ms_RTTI("BSMultiBoundNode", &BSNiNode::ms_RTTI);

BSMultiBoundNode::BSMultiBoundNode() : BSNiNode(), spMultiBound(nullptr) {
    m_uFlags |= 2048u;
}

BSMultiBoundNode::BSMultiBoundNode(NiNode* a1) : BSNiNode(a1, true), spMultiBound(nullptr) {}

// The multibound is released by its member's destructor.
BSMultiBoundNode::~BSMultiBoundNode() {}

NiObject* BSMultiBoundNode::CreateClone(NiCloningProcess& arCloneData) {
    BSMultiBoundNode* pobj;
    BSMultiBoundNode* apMemory = static_cast<BSMultiBoundNode*>(NiMemObject::operator new(sizeof(BSMultiBoundNode)));
    if (apMemory) {
        pobj = ::new (apMemory) BSMultiBoundNode;
    }
    else {
        pobj = 0;
    }
    CopyMembers(pobj, arCloneData);
    return pobj;
}

void BSMultiBoundNode::LoadBinary(NiStream& arStream) {
    NiNode::LoadBinary(arStream);
    arStream.ReadLinkID();
}

// The linked object is taken as a multibound without a type check.
void BSMultiBoundNode::LinkObject(NiStream& arStream) {
    NiNode::LinkObject(arStream);
    spMultiBound = static_cast<BSMultiBound*>(arStream.GetObjectFromLinkID());
}

// The multibound is registered without a null check, as the game does.
bool BSMultiBoundNode::RegisterStreamables(NiStream& arStream) {
    if (!NiNode::RegisterStreamables(arStream)) {
        return false;
    }

    spMultiBound.m_pObject->RegisterStreamables(arStream);
    return true;
}

void BSMultiBoundNode::SaveBinary(NiStream& arStream) {
    NiNode::SaveBinary(arStream);
    arStream.SaveLinkID(spMultiBound.m_pObject);
}

// NiNode's comparison runs but its result is dropped: only the multibound pointers decide.
bool BSMultiBoundNode::IsEqual(NiObject* apObject) {
    NiNode::IsEqual(apObject);
    return spMultiBound.m_pObject == static_cast<BSMultiBoundNode*>(apObject)->spMultiBound.m_pObject;
}

void BSMultiBoundNode::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiNode::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    if (CullingMode == BSCullingProcess::BSCP_CULL_NORMAL) {
        apStrings->Add(NiGetViewerString("cull mode", "NORMAL"));
    }
    else {
        const char* pcMode = CullingMode == BSCullingProcess::BSCP_CULL_ALLPASS ? "ALLPASS" : "ALLFAIL";
        apStrings->Add(NiGetViewerString("cull mode", pcMode));
    }
    if (spMultiBound.m_pObject && spMultiBound.m_pObject->spShape.m_pObject) {
        spMultiBound.m_pObject->spShape.m_pObject->GetViewerStrings(apStrings);
    }
}

void BSMultiBoundNode::UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    NiNode::UpdateDownwardPass(UpdateData, uFlags);
    UpdateWorldBound();
}

// The bound comes from the multibound's shape when there is one, else from the children as NiNode does, but
// without NiNode's flag checks; an orphaned bound (flag 0x80000) is reset to radius 0 first.
void BSMultiBoundNode::UpdateWorldBound() {
    bool bCreateFromChildren = true;
    if (spMultiBound.m_pObject) {
        BSMultiBoundShape* pShape = spMultiBound.m_pObject->spShape.m_pObject;
        if (pShape) {
            CreateBoundIfNone();
            pShape->GetNiBound(*m_kWorldBound);
            bCreateFromChildren = false;
        }
    }
    if (!bCreateFromChildren) {
        return;
    }

    if (m_kWorldBound && (m_uFlags & HAS_ORPHANED_BOUND)) {
        m_kWorldBound->m_fRadius = 0.0f;
    }

    const uint32_t uiCount = m_kChildren.m_usSize;
    uint32_t uiIndex = 0;
    for (; uiIndex < uiCount; ++uiIndex) {
        NiAVObject* pChild = m_kChildren.m_pBase[uiIndex].m_pObject;
        if (pChild && pChild->m_kWorldBound && pChild->m_kWorldBound->m_fRadius != 0.0) {
            CreateBoundIfNone();
            const NiBound* pBound = pChild->m_kWorldBound ? pChild->m_kWorldBound : &NullBoundS;
            *m_kWorldBound = *pBound;
            ++uiIndex;
            break;
        }
    }
    for (; uiIndex < uiCount; ++uiIndex) {
        NiAVObject* pChild = m_kChildren.m_pBase[uiIndex].m_pObject;
        if (pChild && pChild->m_kWorldBound && pChild->m_kWorldBound->m_fRadius != 0.0) {
            m_kWorldBound->Merge(pChild->m_kWorldBound ? pChild->m_kWorldBound : &NullBoundS);
        }
    }
}

// Children are culled only when the multibound is visible (tested once per frame through the shape's cached cull
// result), under this node's culling mode.
void BSMultiBoundNode::OnVisible(NiCullingProcess& kCuller) {
    if (!m_kChildren.m_usESize) {
        return;
    }

    BSCullingProcess& rCuller = static_cast<BSCullingProcess&>(kCuller);
    BSCullingProcess::BSCPCullingType eCullMode = rCuller.kCullMode;
    bool bIsVisible = true;
    if (eCullMode != BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS &&
        eCullMode != BSCullingProcess::BSCP_CULL_ALLPASS && spMultiBound.m_pObject) {
        const NiFrustumPlanes& rPlanes = kCuller.m_kPlanes;
        int32_t eCullResult = spMultiBound.m_pObject->spShape.m_pObject->kCullResult;
        if (rCuller.kCullMode == BSCullingProcess::BSCP_CULL_FORCEMULTIBOUNDSNOUPDATE) {
            eCullResult = 2 - spMultiBound.m_pObject->spShape.m_pObject->WithinFrustum(rPlanes);
        }
        if (eCullResult == BSMultiBoundShape::BS_CULL_UNTESTED) {
            bIsVisible = spMultiBound.m_pObject->spShape.m_pObject->WithinFrustum(rPlanes);
            spMultiBound.m_pObject->spShape.m_pObject->kCullResult =
                static_cast<BSMultiBoundShape::BSMBCullResult>(2 - bIsVisible);
        }
        else {
            bIsVisible = eCullResult == BSMultiBoundShape::BS_CULL_VISIBLE;
        }
    }
    if (!bIsVisible) {
        return;
    }

    if (eCullMode != BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS) {
        rCuller.kCullMode = CullingMode;
    }
    for (uint32_t i = 0; i < m_kChildren.m_usSize; ++i) {
        NiAVObject* pChild = m_kChildren.m_pBase[i].m_pObject;
        if (pChild) {
            pChild->Cull(kCuller);
        }
    }
    rCuller.kCullMode = eCullMode;
}

void BSMultiBoundNode::UpdateUpwardPass() {
    NiNode::UpdateUpwardPass();
    UpdateWorldBound();
}

// No null check: a node without a multibound is never asked.
bool BSMultiBoundNode::QPointWithin(NiPoint3& aPoint) {
    return spMultiBound.m_pObject->QPointWithin(aPoint);
}

BSMultiBoundShape::BSMBIntersectResult BSMultiBoundNode::CheckBound(const NiBound& aTargetBound) {
    return spMultiBound.m_pObject->spShape.m_pObject->CheckBound(aTargetBound);
}

BSMultiBoundShape::BSMBIntersectResult BSMultiBoundNode::CheckBound(const BSMultiBound& aTargetBound) {
    return spMultiBound.m_pObject->spShape.m_pObject->CheckBound(aTargetBound);
}

void BSMultiBoundNode::SetMultiBound(BSMultiBound* apMultiBound) {
    if (spMultiBound.m_pObject == apMultiBound) {
        return;
    }

    if (spMultiBound.m_pObject) {
        NiDecRefCount(spMultiBound.m_pObject);
    }
    spMultiBound.m_pObject = apMultiBound;
    if (apMultiBound) {
        NiIncRefCount(apMultiBound);
    }
}

BSMultiBound* BSMultiBoundNode::QMultiBound() {
    return spMultiBound;
}

void BSMultiBoundNode::SetCullingMode(BSCullingProcess::BSCPCullingType aCullingMode) {
    CullingMode = aCullingMode;
}

void BSMultiBoundNode::CopyMembers(BSMultiBoundNode* pDest, NiCloningProcess& arCloneData) {
    BSNiNode::CopyMembers(pDest, arCloneData);
    pDest->spMultiBound.operator=(static_cast<BSMultiBound*>(spMultiBound.m_pObject->CreateClone(arCloneData)));
    pDest->CullingMode = CullingMode;
}

// The cull mode of a BSCullingProcess is read twice, but no caller passes one:
// ShadowSceneNode::UpdateMultiboundVisibility passes a plain NiCullingProcess, HighActorCuller::Operate its
// BSFadeNodeCuller. A culler that is not a BSCullingProcess culls as normal.
bool BSMultiBoundNode::VisibleAgainstActiveOccluders(
    NiPoint3 akViewPoint,
    NiCullingProcess& akCuller,
    BSOcclusionPlane* apOccluder
) {
    if (akCuller.GetRTTI() == &BSCullingProcess::ms_RTTI) {
        BSCullingProcess* pbsculler = static_cast<BSCullingProcess*>(&akCuller);
        if (pbsculler->kCullMode == BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS ||
            pbsculler->kCullMode == BSCullingProcess::BSCP_CULL_ALLPASS) {
            return 1;
        }
    }
    if (!apOccluder) {
        return 1;
    }
    NiFrustumPlanes kPlanes;
    apOccluder->GetFrustumPlanes(akViewPoint, kPlanes);
    if (!spMultiBound.m_pObject->spShape.m_pObject->CompletelyWithinFrustum(kPlanes)) {
        return 1;
    }
    spMultiBound.m_pObject->spShape.m_pObject->kCullResult = BSMultiBoundShape::BS_CULL_CULLED;
    return 0;
}

NiObject* BSMultiBoundNode::CreateObject() {
    void* apMemory = NiMemObject::operator new(sizeof(BSMultiBoundNode));
    if (apMemory) {
        return ::new (apMemory) BSMultiBoundNode;
    }
    else {
        return nullptr;
    }
}

BSMultiBoundNode* BSMultiBoundNode::FindMultiboundNode(NiAVObject* apObject) {
    bool pMultiBoundNode;
    while (apObject && apObject->GetRTTI() != &BSMultiBoundNode::ms_RTTI) {
        apObject = apObject->m_pkParent;
    }
    if (apObject) {
        pMultiBoundNode = apObject->GetRTTI() == &BSMultiBoundNode::ms_RTTI;
    }
    else {
        pMultiBoundNode = 0;
    }
    if (pMultiBoundNode) {
        return static_cast<BSMultiBoundNode*>(apObject);
    }
    else {
        return 0;
    }
}
