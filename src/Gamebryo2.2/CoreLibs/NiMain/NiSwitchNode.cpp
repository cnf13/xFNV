#include "Gamebryo2.2/CoreLibs/NiMain/NiSwitchNode.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiSwitchNode::ms_RTTI("NiSwitchNode", &NiNode::ms_RTTI);

NiSwitchNode::NiSwitchNode()
  : NiNode(0), m_uFlags(0), m_iIndex(-1), m_fSavedTime(0.0f), m_uiRevID(1), m_kChildRevID(1, 1) {
    m_uFlags |= UPDATE_ONLY_ACTIVE_CHILD_MASK | UPDATECONTROLLERS_MASK;
}

NiSwitchNode::~NiSwitchNode() {}

const NiRTTI* NiSwitchNode::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiSwitchNode::CreateClone(NiCloningProcess& kCloning) {
    NiSwitchNode* pkClone = new NiSwitchNode;
    NiNode::CopyMembers(pkClone, kCloning);
    pkClone->m_uFlags = m_uFlags;
    pkClone->m_iIndex = m_iIndex;
    return pkClone;
}

// Before 10.0.1.2 the flags sat in the NiAVObject flags; a loaded node always updates its controllers.
void NiSwitchNode::LoadBinary(NiStream& kStream) {
    NiNode::LoadBinary(kStream);
    if (kStream.m_uiNifFileVersion >= 0xA000102) {
        NiStreamLoadBinary(kStream, m_uFlags);
    }
    else {
        m_uFlags = static_cast<uint16_t>((kStream.m_usNiAVObjectFlags >> 6) & 0x1FF);
    }
    NiStreamLoadBinary(kStream, m_iIndex);
    m_uFlags |= UPDATECONTROLLERS_MASK;
}

// NiBillboardNode, NiBSPNode and BSOrderedNode use the same function.
void NiSwitchNode::LinkObject(NiStream& kStream) {
    NiNode::LinkObject(kStream);
}

// NiBillboardNode, NiBSPNode and NiSortAdjustNode use the same function.
bool NiSwitchNode::RegisterStreamables(NiStream& kStream) {
    return NiNode::RegisterStreamables(kStream);
}

void NiSwitchNode::SaveBinary(NiStream& kStream) {
    NiNode::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uFlags);
    NiStreamSaveBinary(kStream, m_iIndex);
}

// Only the index and the update-only-active bit are compared.
bool NiSwitchNode::IsEqual(NiObject* pkObject) {
    if (!NiNode::IsEqual(pkObject)) {
        return false;
    }
    NiSwitchNode* pkOther = static_cast<NiSwitchNode*>(pkObject);
    return m_iIndex == pkOther->m_iIndex && ((m_uFlags ^ pkOther->m_uFlags) & UPDATE_ONLY_ACTIVE_CHILD_MASK) == 0;
}

void NiSwitchNode::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiNode::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_iIndex", m_iIndex));
    pkStrings->Add(
        NiGetViewerString("m_bUpdateOnlyActive", static_cast<bool>(m_uFlags & UPDATE_ONLY_ACTIVE_CHILD_MASK))
    );
}

// With update-only-active set, only the active child's controllers run.
void NiSwitchNode::UpdateControllers(NiUpdateData& UpdateData) {
    if (m_uFlags & UPDATE_ONLY_ACTIVE_CHILD_MASK) {
        if (m_iIndex >= 0) {
            UpdateObjectControllers(UpdateData, true);
            NiAVObject* pkChild = m_kChildren.m_pBase[m_iIndex].m_pObject;
            if (pkChild) {
                pkChild->UpdateControllers(UpdateData);
            }
        }
    }
    else {
        NiNode::UpdateControllers(UpdateData);
    }
}

// With update-only-active set, only the active child is updated and its bound becomes the node's; the child's
// revision is stamped so OnVisible knows it is current.
void NiSwitchNode::UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    if (UpdateData.bUpdateControllers) {
        m_uFlags |= UPDATECONTROLLERS_MASK;
    }
    else {
        m_uFlags &= ~UPDATECONTROLLERS_MASK;
    }
    m_fSavedTime = UpdateData.fTime;
    if (!(m_uFlags & UPDATE_ONLY_ACTIVE_CHILD_MASK)) {
        NiNode::UpdateDownwardPass(UpdateData, uFlags);
        return;
    }
    ++m_uiRevID;
    if (UpdateData.bUpdateControllers) {
        UpdateObjectControllers(UpdateData, true);
    }
    UpdateWorldData(UpdateData);
    if (m_iIndex >= 0) {
        NiAVObject* pkChild = m_kChildren.m_pBase[m_iIndex].m_pObject;
        if (pkChild) {
            pkChild->UpdateDownwardPass(UpdateData, uFlags);
            m_kChildRevID.SetAt(m_iIndex, m_uiRevID);
            *m_kWorldBound = pkChild->GetWorldBound();
        }
    }
}

// The controller flag lent to UpdateObjectControllers and the child's selective-update bits are NiAVObject's flags
// (+0x30), not the switch node's own m_uFlags.
void NiSwitchNode::UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    m_fSavedTime = UpdateData.fTime;
    if (!(m_uFlags & UPDATE_ONLY_ACTIVE_CHILD_MASK)) {
        NiNode::UpdateSelectedDownwardPass(UpdateData, uFlags);
        return;
    }
    ++m_uiRevID;
    bool bUpdateControllers = UpdateData.bUpdateControllers;
    UpdateData.bUpdateControllers = (NiAVObject::m_uFlags & SELECTIVE_UPDATE_CONTROLLER) != 0;
    UpdateObjectControllers(UpdateData, true);
    UpdateData.bUpdateControllers = bUpdateControllers;
    UpdateWorldData(UpdateData);
    if (m_iIndex >= 0) {
        NiAVObject* pkChild = m_kChildren.m_pBase[m_iIndex].m_pObject;
        if (pkChild) {
            if (pkChild->m_uFlags & SELECTIVE_UPDATE) {
                if (pkChild->m_uFlags & SELECTIVE_UPDATE_RIGID) {
                    pkChild->UpdateRigidDownwardPass(UpdateData, uFlags);
                }
                else {
                    pkChild->UpdateSelectedDownwardPass(UpdateData, uFlags);
                }
            }
            m_kChildRevID.SetAt(m_iIndex, m_uiRevID);
            *m_kWorldBound = pkChild->GetWorldBound();
        }
    }
}

// As UpdateSelectedDownwardPass: the controller flag is NiAVObject's.
void NiSwitchNode::UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    m_fSavedTime = UpdateData.fTime;
    if (!(m_uFlags & UPDATE_ONLY_ACTIVE_CHILD_MASK)) {
        NiNode::UpdateRigidDownwardPass(UpdateData, uFlags);
        return;
    }
    ++m_uiRevID;
    bool bUpdateControllers = UpdateData.bUpdateControllers;
    UpdateData.bUpdateControllers = (NiAVObject::m_uFlags & SELECTIVE_UPDATE_CONTROLLER) != 0;
    UpdateObjectControllers(UpdateData, true);
    UpdateData.bUpdateControllers = bUpdateControllers;
    UpdateWorldData(UpdateData);
    if (m_iIndex >= 0) {
        NiAVObject* pkChild = m_kChildren.m_pBase[m_iIndex].m_pObject;
        if (pkChild) {
            if (pkChild->m_uFlags & SELECTIVE_UPDATE) {
                pkChild->UpdateRigidDownwardPass(UpdateData, uFlags);
            }
            m_kChildRevID.SetAt(m_iIndex, m_uiRevID);
            *m_kWorldBound = pkChild->GetWorldBound();
        }
    }
}

// With update-only-active set, the bound is the active child's (radius 0 without one).
void NiSwitchNode::UpdateWorldBound() {
    if (!(m_uFlags & UPDATE_ONLY_ACTIVE_CHILD_MASK)) {
        NiNode::UpdateWorldBound();
        return;
    }
    NiAVObject* pkChild = m_iIndex >= 0 ? m_kChildren.m_pBase[m_iIndex].m_pObject : nullptr;
    if (pkChild) {
        *m_kWorldBound = pkChild->GetWorldBound();
    }
    else {
        m_kWorldBound->m_fRadius = 0.0f;
    }
}

// Culls only the active child, first bringing it up to date when it missed updates while inactive.
void NiSwitchNode::OnVisible(NiCullingProcess& kCuller) {
    if (m_iIndex < 0) {
        return;
    }
    NiAVObject* pkChild = m_kChildren.m_pBase[m_iIndex].m_pObject;
    if (!pkChild) {
        return;
    }
    if (m_kChildRevID.m_pBase[m_iIndex] != m_uiRevID) {
        m_kChildRevID.SetAt(m_iIndex, m_uiRevID);
        NiUpdateData kUpdateData;
        kUpdateData.fTime = m_fSavedTime;
        kUpdateData.bUpdateControllers = (m_uFlags & UPDATECONTROLLERS_MASK) != 0;
        kUpdateData.bParallelUpdate = false;
        kUpdateData.bFoundParticles = false;
        kUpdateData.bFoundMorphController = false;
        kUpdateData.bSceneGraphChange = false;
        pkChild->UpdateDownwardPass(kUpdateData, 0);
    }
    pkChild->Cull(kCuller);
}

// A new child starts with revision 0, so it is brought up to date when first shown.
void NiSwitchNode::AttachChild(NiAVObject* pkChild, int32_t bFirstAvail) {
    NiNode::AttachChild(pkChild, bFirstAvail);
    m_uiRevID = 1;
    m_kChildRevID.Add(0);
}

// The active index is dropped when its child is gone.
void NiSwitchNode::DetachChild(NiAVObject* pkChild, NiPointer<NiAVObject>& aspAVObject) {
    m_uiRevID = 1;
    for (uint32_t i = 0; i < m_kChildren.m_usSize; ++i) {
        if (m_kChildren.m_pBase[i].m_pObject == pkChild) {
            m_kChildRevID.RemoveAt(i);
            break;
        }
    }
    NiPointer<NiAVObject> spAVObject;
    NiNode::DetachChild(pkChild, spAVObject);
    if (m_iIndex > -1 && (m_iIndex >= m_kChildren.m_usSize || !m_kChildren.m_pBase[m_iIndex].m_pObject)) {
        m_iIndex = -1;
    }
    aspAVObject = spAVObject;
}

void NiSwitchNode::DetachChildAt(uint32_t i, NiPointer<NiAVObject>& aspAVObject) {
    m_uiRevID = 1;
    if (i < m_kChildren.m_usSize) {
        m_kChildRevID.RemoveAt(i);
    }
    NiPointer<NiAVObject> spAVObject;
    NiNode::DetachChildAt(i, spAVObject);
    if (m_iIndex > -1 && (m_iIndex >= m_kChildren.m_usSize || !m_kChildren.m_pBase[m_iIndex].m_pObject)) {
        m_iIndex = -1;
    }
    aspAVObject = spAVObject;
}

void NiSwitchNode::SetAt(uint32_t i, NiAVObject* pkChild, NiPointer<NiAVObject>& aspAVObject) {
    m_uiRevID = 1;
    m_kChildRevID.SetAtGrow(i, 0);
    NiNode::SetAt(i, pkChild, aspAVObject);
}

NiAVObject* NiSwitchNode::GetActiveChild() const {
    int32_t m_iIndex;
    m_iIndex = this->m_iIndex;
    if ((m_iIndex < 0) || (m_iIndex >= m_kChildren.m_usSize)) {
        return nullptr;
    }

    return m_kChildren.m_pBase[m_iIndex].m_pObject;
}

void NiSwitchNode::CopyMembers(NiSwitchNode* pkDest, NiCloningProcess& kCloning) {
    NiNode::CopyMembers(pkDest, kCloning);
    pkDest->m_uFlags = m_uFlags;
    pkDest->m_iIndex = m_iIndex;
}

NiObject* NiSwitchNode::CreateObject() {
    return new NiSwitchNode;
}
