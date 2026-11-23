#include "BSMain/BSMultiBoundRoom.h"

#include "BSCore/BSSimpleArray.h"
#include "BSMain/BSCompoundFrustum.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSMultiBoundShape.h"
#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSPortal.h"
#include "BSMain/Setting.h"
#include "BSShader/ShadowSceneLight.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/TESObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSMultiBoundRoom::ms_RTTI("BSMultiBoundRoom", &NiObject::ms_RTTI);

// A plain pointer list's destructor: RemoveAll. It is used for BSPortalGraph::lOccluderList, which is why the parameter
// has that list's type.
void RemoveAllOcclusionPlanes(NiTPointerListBase<BSOcclusionPlane*>* apThis, [[maybe_unused]] void* apEdx) {
    apThis->RemoveAll();
}

BSMultiBoundRoom::BSMultiBoundRoom() : BSMultiBoundNode(), pCompoundFrustum(nullptr) {
    m_uFlags |= ALWAYS_DRAW;
}

// Empties the portal, occluder, joined-bound and light containers and drops the compound frustum; the members'
// destructors do the rest.
BSMultiBoundRoom::~BSMultiBoundRoom() {
    kPortalList.RemoveAll();
    lOccluderList.RemoveAll();
    kJoinedMultiBoundList.RemoveAll();
    kLights.Clear(true);
    if (pCompoundFrustum) {
        RemoveFrustum();
    }
}

// Compares as a plain NiObject, skipping the node classes' comparisons.
bool BSMultiBoundRoom::IsEqual(NiObject* pkObject) {
    return NiObject::IsEqual(pkObject);
}

void BSMultiBoundRoom::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    BSMultiBoundNode::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// As BSMultiBoundNode::UpdateWorldBound, but the shape's bound also takes in every joined multibound's.
void BSMultiBoundRoom::UpdateWorldBound() {
    bool bCreateFromChildren = true;
    if (spMultiBound.m_pObject) {
        BSMultiBoundShape* pShape = spMultiBound->spShape.m_pObject;
        if (pShape) {
            CreateBoundIfNone();
            pShape->GetNiBound(*m_kWorldBound);
            if (kJoinedMultiBoundList.m_uiCount) {
                NiTListItem<NiPointer<BSMultiBound>>* pkPos = kJoinedMultiBoundList.m_pkHead;
                while (pkPos) {
                    NiPointer<BSMultiBound>* pspJoined = &pkPos->m_element;
                    pkPos = pkPos->m_pkNext;
                    pShape = pspJoined->m_pObject->spShape.m_pObject;
                    NiBound kBound;
                    pShape->GetNiBound(kBound);
                    m_kWorldBound->Merge(&kBound);
                }
            }
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

// A room with children or portals culls its children when its shape was found visible (or the culler ignores
// multibounds), under the room's culling mode and, unless that mode skips multibounds, the room's compound
// frustum; the culler's mode and frustum are restored afterwards.
void BSMultiBoundRoom::OnVisible(NiCullingProcess& kCuller) {
    if (!m_kChildren.m_usESize && !kPortalList.m_uiCount) {
        return;
    }

    BSCullingProcess& rCuller = static_cast<BSCullingProcess&>(kCuller);
    int32_t eCullResult = spMultiBound->spShape->kCullResult;
    if (rCuller.kCullMode == BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS ||
        rCuller.kCullMode == BSCullingProcess::BSCP_CULL_ALLPASS) {
        eCullResult = BSMultiBoundShape::BS_CULL_VISIBLE;
    }
    if (eCullResult != BSMultiBoundShape::BS_CULL_VISIBLE &&
        rCuller.kCullMode != BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS &&
        rCuller.kCullMode != BSCullingProcess::BSCP_CULL_ALLPASS) {
        return;
    }

    BSCompoundFrustum* pOldFrustum = rCuller.pCompoundFrustum;
    BSCompoundFrustum* pFrustum = pCompoundFrustum;
    BSCullingProcess::BSCPCullingType eCullMode = rCuller.kCullMode;
    if (rCuller.kCullMode != BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS) {
        rCuller.kCullMode = CullingMode;
    }
    if (rCuller.kCullMode != BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS &&
        rCuller.kCullMode != BSCullingProcess::BSCP_CULL_ALLPASS) {
        if (pFrustum) {
            pFrustum->PrethreadOpList();
        }
        rCuller.pCompoundFrustum = pFrustum;
        if (pFrustum) {
            pFrustum->PrethreadOpList();
        }
    }
    for (uint32_t i = 0; i < m_kChildren.m_usSize; ++i) {
        NiAVObject* pChild = m_kChildren.m_pBase[i].m_pObject;
        if (pChild) {
            pChild->Cull(kCuller);
        }
    }
    rCuller.kCullMode = eCullMode;
    if (rCuller.kCullMode != BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS &&
        rCuller.kCullMode != BSCullingProcess::BSCP_CULL_ALLPASS) {
        rCuller.pCompoundFrustum = pOldFrustum;
    }
}

// Inside the room's own multibound or any joined one.
bool BSMultiBoundRoom::QPointWithin(NiPoint3& aPoint) {
    bool bWithin = spMultiBound->QPointWithin(aPoint);
    if (bWithin || !kJoinedMultiBoundList.m_uiCount) {
        return bWithin;
    }

    NiTListItem<NiPointer<BSMultiBound>>* pkPos = kJoinedMultiBoundList.m_pkHead;
    while (!bWithin && pkPos) {
        NiPointer<BSMultiBound>* pspJoined = &pkPos->m_element;
        pkPos = pkPos->m_pkNext;
        bWithin = pspJoined->m_pObject->QPointWithin(aPoint);
    }
    return bWithin;
}

// The room's shape decides unless it reports only a partial hit; then the joined multibounds' last non-empty
// result wins, stopping at a partial one.
BSMultiBoundShape::BSMBIntersectResult BSMultiBoundRoom::CheckBound(const NiBound& aTargetBound) {
    BSMultiBoundShape::BSMBIntersectResult eResult = spMultiBound->spShape->CheckBound(aTargetBound);
    if (eResult != BSMultiBoundShape::BS_INTERSECT_PARTIAL && kJoinedMultiBoundList.m_uiCount) {
        NiTListItem<NiPointer<BSMultiBound>>* pkPos = kJoinedMultiBoundList.m_pkHead;
        while (eResult != BSMultiBoundShape::BS_INTERSECT_PARTIAL && pkPos) {
            NiPointer<BSMultiBound>* pspJoined = &pkPos->m_element;
            pkPos = pkPos->m_pkNext;
            BSMultiBoundShape::BSMBIntersectResult eJoined = pspJoined->m_pObject->spShape->CheckBound(aTargetBound);
            if (eJoined != BSMultiBoundShape::BS_INTERSECT_NONE) {
                eResult = eJoined;
            }
        }
    }
    return eResult;
}

BSMultiBoundShape::BSMBIntersectResult BSMultiBoundRoom::CheckBound(const BSMultiBound& aTargetBound) {
    BSMultiBoundShape::BSMBIntersectResult eResult = spMultiBound->spShape->CheckBound(aTargetBound);
    if (eResult != BSMultiBoundShape::BS_INTERSECT_PARTIAL && kJoinedMultiBoundList.m_uiCount) {
        NiTListItem<NiPointer<BSMultiBound>>* pkPos = kJoinedMultiBoundList.m_pkHead;
        while (eResult != BSMultiBoundShape::BS_INTERSECT_PARTIAL && pkPos) {
            NiPointer<BSMultiBound>* pspJoined = &pkPos->m_element;
            pkPos = pkPos->m_pkNext;
            BSMultiBoundShape::BSMBIntersectResult eJoined = pspJoined->m_pObject->spShape->CheckBound(aTargetBound);
            if (eJoined != BSMultiBoundShape::BS_INTERSECT_NONE) {
                eResult = eJoined;
            }
        }
    }
    return eResult;
}

void BSMultiBoundRoom::RemoveFrustum() {
    if (pCompoundFrustum) {
        delete pCompoundFrustum;
    }
    pCompoundFrustum = nullptr;
}

void BSMultiBoundRoom::SetCompoundFrustum(BSCompoundFrustum* apFrustum) {
    if (pCompoundFrustum && (pCompoundFrustum != apFrustum)) {
        RemoveFrustum();
    }

    pCompoundFrustum = apFrustum;
}

NiTPointerList<BSPortal*>& BSMultiBoundRoom::QPortalList() {
    return kPortalList;
}

void BSMultiBoundRoom::AddJoinedMultibound(BSMultiBound* apBound) {
    NiPointer<BSMultiBound> spBound(apBound);
    kJoinedMultiBoundList.AddTail(spBound);
}

BSMultiBoundShape::BSMBCullResult BSMultiBoundRoom::GetCullResult() {
    BSMultiBound* m_pObject;
    if (!spMultiBound.m_pObject) {
        return BSMultiBoundShape::BS_CULL_UNTESTED;
    }
    m_pObject = spMultiBound.m_pObject;
    if (m_pObject->spShape.m_pObject) {
        return m_pObject->spShape->kCullResult;
    }

    return BSMultiBoundShape::BS_CULL_UNTESTED;
}

void BSMultiBoundRoom::AddOccluder(BSOcclusionPlane* apOccluder) {
    if (apOccluder) {
        NiTListItem<BSOcclusionPlane*>* pkNode = lOccluderList.NewItem();
        pkNode->m_element = apOccluder;
        lOccluderList.AddNodeTail(pkNode);
    }
}

void BSMultiBoundRoom::RemoveOccluder(BSOcclusionPlane* apOccluder) {
    if (apOccluder) {
        lOccluderList.Remove(apOccluder);
    }
}
