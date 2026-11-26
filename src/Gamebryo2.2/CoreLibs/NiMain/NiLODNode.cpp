#include "Gamebryo2.2/CoreLibs/NiMain/NiLODNode.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLODData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRangeLODData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSwitchNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiLODNode::ms_RTTI("NiLODNode", &NiSwitchNode::ms_RTTI);

// No LOD data, the first child, LOD switching on.
NiLODNode::NiLODNode() : m_spLODData(nullptr), m_bLODActive(true) {
    m_iIndex = 0;
}

// The LOD data pointer and the base's child revision array are destroyed by the compiler.
NiLODNode::~NiLODNode() {}

const NiRTTI* NiLODNode::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiLODNode::CreateClone(NiCloningProcess& kCloning) {
    NiLODNode* pkObject = new NiLODNode;
    CopyMembers(pkObject, kCloning);
    return pkObject;
}

// Up to 10.1.0.11 the ranges were stored inline in the node; they now live in a linked NiLODData.
void NiLODNode::LoadBinary(NiStream& kStream) {
    NiSwitchNode::LoadBinary(kStream);
    if (kStream.m_uiNifFileVersion > 0xA00010B) {
        kStream.ReadLinkID();
        return;
    }
    NiRangeLODData* pkLODData = new NiRangeLODData;
    NiPoint3 kCenter;
    kCenter.LoadBinary(kStream);
    pkLODData->m_kCenter = kCenter;
    uint32_t uiNumRanges;
    NiStreamLoadBinary(kStream, uiNumRanges);
    if (uiNumRanges) {
        pkLODData->SetNumRanges(uiNumRanges);
        for (uint32_t i = 0; i < uiNumRanges; ++i) {
            float fNear;
            float fFar;
            NiStreamLoadBinary(kStream, fNear);
            NiStreamLoadBinary(kStream, fFar);
            if (i >= pkLODData->m_uiNumRanges) {
                pkLODData->SetNumRanges(i + 1);
            }
            pkLODData->m_pkRanges[i].m_fNear = fNear;
            pkLODData->m_pkRanges[i].m_fFar = fFar;
        }
    }
    m_spLODData = pkLODData;
}

void NiLODNode::LinkObject(NiStream& kStream) {
    NiSwitchNode::LinkObject(kStream);
    if (kStream.m_uiNifFileVersion >= 0xA00010C) {
        m_spLODData = static_cast<NiLODData*>(kStream.GetObjectFromLinkID());
    }
}

bool NiLODNode::RegisterStreamables(NiStream& kStream) {
    if (!NiSwitchNode::RegisterStreamables(kStream)) {
        return false;
    }
    if (m_spLODData) {
        m_spLODData->RegisterStreamables(kStream);
    }
    return true;
}

void NiLODNode::SaveBinary(NiStream& kStream) {
    NiSwitchNode::SaveBinary(kStream);
    kStream.SaveLinkID(m_spLODData);
}

// LOD data of different classes compare equal: returns true on an RTTI mismatch, deliberately.
bool NiLODNode::IsEqual(NiObject* pkObject) {
    if (!NiSwitchNode::IsEqual(pkObject)) {
        return false;
    }
    NiLODData* pkOther = static_cast<NiLODNode*>(pkObject)->m_spLODData;
    if (!m_spLODData) {
        return !pkOther;
    }
    if (!pkOther) {
        return false;
    }
    return m_spLODData->GetRTTI() != pkOther->GetRTTI() || m_spLODData->IsEqual(pkOther);
}

void NiLODNode::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiSwitchNode::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    if (m_spLODData) {
        m_spLODData->GetViewerStrings(pkStrings);
    }
    else {
        // A literal, not an allocated copy like the other entries.
        pkStrings->Add(const_cast<char*>("NULL LOD Data"));
    }
}

void NiLODNode::UpdateWorldData(NiUpdateData& UpdateData) {
    NiAVObject::UpdateWorldData(UpdateData);
    if (m_spLODData) {
        m_spLODData->UpdateWorldData(this);
    }
}

// Picks the level from the camera (or the global override), then falls back to the nearest lower level that has a
// child.
void NiLODNode::OnVisible(NiCullingProcess& kCuller) {
    if (m_bLODActive && m_spLODData) {
        m_iIndex = m_spLODData->GetLODLevel(kCuller.m_pkCamera, this);
        if (ms_iGlobalLOD >= 0) {
            m_iIndex = m_spLODData->GetLODIndex(ms_iGlobalLOD);
        }
        if (m_iIndex >= 0) {
            do {
                if (m_iIndex < m_kChildren.m_usSize && m_kChildren.m_pBase[m_iIndex].m_pObject) {
                    break;
                }
                --m_iIndex;
            }
            while (m_iIndex >= 0);
        }
    }
    NiSwitchNode::OnVisible(kCuller);
}

// Updates every child whose revision is behind the node's, with the saved time.
void NiLODNode::UpdateAllChildren() {
    NiUpdateData kUpdate;
    kUpdate.fTime = m_fSavedTime;
    kUpdate.bUpdateControllers = (NiSwitchNode::m_uFlags & NiSwitchNode::UPDATECONTROLLERS_MASK) != 0;
    kUpdate.bParallelUpdate = false;
    kUpdate.bFoundParticles = false;
    kUpdate.bFoundMorphController = false;
    kUpdate.bSceneGraphChange = false;
    for (int32_t iLoop = 0; iLoop < m_kChildren.m_usSize; ++iLoop) {
        NiAVObject* pkChild = m_kChildren.m_pBase[iLoop].m_pObject;
        if (pkChild && m_kChildRevID.m_pBase[iLoop] != m_uiRevID) {
            m_kChildRevID.SetAt(iLoop, m_uiRevID);
            pkChild->UpdateDownwardPass(kUpdate, 0);
        }
    }
}

void NiLODNode::SetLODData(NiLODData* pkLODData) {
    NiLODData* pkOld = m_spLODData.m_pObject;
    if (pkOld == pkLODData) {
        return;
    }
    if (pkOld) {
        pkOld->DecRefCount();
    }
    m_spLODData.m_pObject = pkLODData;
    if (pkLODData) {
        pkLODData->IncRefCount();
    }
}

// The clone gets a duplicate of the LOD data, or loses its own.
void NiLODNode::CopyMembers(NiLODNode* pkDest, NiCloningProcess& kCloning) {
    NiSwitchNode::CopyMembers(pkDest, kCloning);
    NiLODData* pkLODData = m_spLODData.m_pObject;
    if (pkLODData) {
        pkDest->SetLODData(pkLODData->Duplicate());
        return;
    }
    pkDest->m_spLODData = nullptr;
}

NiObject* NiLODNode::CreateObject() {
    return new NiLODNode;
}
