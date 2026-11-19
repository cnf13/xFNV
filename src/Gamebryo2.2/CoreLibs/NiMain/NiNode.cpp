#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSBlastNode.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshRender.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiNode::ms_RTTI("NiNode", &NiAVObject::ms_RTTI);

// Gamebryo's NiTArray::InsertAt: each element moves up one slot through a copy held in a NiPointer local.
void NiAVObjectArray_InsertAt(
    NiTObjectArray<NiPointer<NiAVObject>>* apArray,
    uint32_t auiIndex,
    const NiPointer<NiAVObject>* apElement
) {
    if (apElement->m_pObject) {
        uint32_t i = apArray->m_usSize;
        uint32_t uiSize = i;
        if (i > auiIndex) {
            do {
                NiPointer<NiAVObject> spElement = apArray->m_pBase[i - 1];
                if (i == uiSize && i >= apArray->m_usMaxSize) {
                    NiAVObjectArray_SetSize(apArray, i + apArray->m_usGrowBy);
                }
                NiAVObjectArray_SetAt(apArray, i, &spElement);
                --i;
            }
            while (i > auiIndex);
        }
        if (auiIndex >= apArray->m_usMaxSize) {
            NiAVObjectArray_SetSize(apArray, auiIndex + apArray->m_usGrowBy);
        }
        NiAVObjectArray_SetAt(apArray, auiIndex, apElement);
    }
}

void NiAVObjectArray_UpdateSize(NiTObjectArray<NiPointer<NiAVObject>>* apArray) {
    while (apArray->m_usSize) {
        bool v3 = apArray->m_pBase[apArray->m_usSize - 1] != NiPointer<NiAVObject>(static_cast<NiAVObject*>(nullptr));
        if (v3) {
            break;
        }
        --apArray->m_usSize;
    }
}

NiNode::NiNode(uint32_t auiNumChildren) : m_kChildren(auiNumChildren, 1) {}

// Every child is detached and released (the count is read each iteration); the child array frees its buffer.
NiNode::~NiNode() {
    for (uint32_t uiIndex = 0; uiIndex < m_kChildren.m_usSize; ++uiIndex) {
        NiPointer<NiAVObject> spChild;
        DetachChildAt(uiIndex, spChild);
    }
}

NiObject* NiNode::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiNode));
    NiNode* v4 = static_cast<NiNode*>(v3);
    if (v3) {
        ::new (v3) NiNode;
        CopyMembers(v4, kCloning);
        return v4;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

// Two link ID blocks follow the base object's data; the pick-off flag is cleared afterwards.
void NiNode::LoadBinary(NiStream& kStream) {
    NiAVObject::LoadBinary(kStream);
    kStream.ReadMultipleLinkIDs();
    kStream.ReadMultipleLinkIDs();
    // ANDs the whole flags word.
    uint32_t& rFlags = m_uFlags;
    rFlags = NiFlags(rFlags & ~IGNORES_PICKING);
}

void NiNode::LinkObject(NiStream& kStream) {
    NiAVObject::LinkObject(kStream);
    uint32_t uiSize = kStream.GetNumberOfLinkIDs();
    if (uiSize) {
        NiAVObjectArray_SetSize(&m_kChildren, uiSize);
        uint32_t i = 0;
        do {
            NiAVObject* pkChild = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
            SetAt(i++, pkChild);
        }
        while (i < uiSize);
    }
    uint32_t uiSize_3 = kStream.GetNumberOfLinkIDs();
    uint32_t uiSize_4 = uiSize_3;
    if ((&kStream)->GetFileVersion() >= 0x4010008) {
        if (uiSize_3) {
            do {
                --uiSize_4;
                kStream.GetObjectFromLinkID();
            }
            while (uiSize_4);
        }
    }
    else {
        if (uiSize_3) {
            do {
                --uiSize_4;
                kStream.GetObjectFromLinkID();
            }
            while (uiSize_4);
        }
    }
}

// Registers the node, then every child.
bool NiNode::RegisterStreamables(NiStream& kStream) {
    if (!NiAVObject::RegisterStreamables(kStream)) {
        return false;
    }
    for (uint32_t uiIndex = 0; uiIndex < m_kChildren.m_usSize; ++uiIndex) {
        NiAVObject* pChild = m_kChildren.m_pBase[uiIndex].m_pObject;
        if (pChild) {
            pChild->RegisterStreamables(kStream);
        }
    }
    return true;
}

// The child count goes out as one 4-byte component, then each child's link ID through the stream's virtual
// SaveLinkID, then a 4-byte zero. The loop bound is the count read before the first write.
void NiNode::SaveBinary(NiStream& kStream) {
    NiAVObject::SaveBinary(kStream);
    uint32_t uiCount = m_kChildren.m_usSize;
    uint32_t uiComponentSize = 4;
    kStream.m_pkOstr->BinaryWrite(&uiCount, 4, &uiComponentSize, 1);
    for (uint32_t uiIndex = 0; uiIndex < uiCount; ++uiIndex) {
        kStream.SaveLinkID(m_kChildren.m_pBase[uiIndex].m_pObject);
    }
    uint32_t uiZero = 0;
    kStream.m_pkOstr->BinaryWrite(&uiZero, 4, &uiComponentSize, 1);
}

bool NiNode::IsEqual(NiObject* pkObject) {
    bool result = NiAVObject::IsEqual(pkObject);
    if (result) {
        // The base test has matched the RTTI: apObject is a NiNode, as in Gamebryo's NiNode::IsEqual.
        NiNode* pkNode = static_cast<NiNode*>(pkObject);
        uint32_t m_usSize = m_kChildren.m_usSize;
        if (m_usSize != pkNode->m_kChildren.m_usSize) {
            return false;
        }
        uint32_t i = 0;
        if (!m_kChildren.m_usSize) {
            return true;
        }
        while (true) {
            NiAVObject* pkChild0 = m_kChildren.m_pBase[i].m_pObject;
            NiAVObject* pkChild1 = pkNode->m_kChildren.m_pBase[i].m_pObject;
            if (pkChild0) {
                if (!pkChild1 || !pkChild0->IsEqual(pkChild1)) {
                    return false;
                }
            }
            else {
                if (pkChild1) {
                    return false;
                }
            }
            if (++i >= m_usSize) {
                return true;
            }
        }
    }
    return result;
}

void NiNode::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiAVObject::GetViewerStrings(pkStrings);
    char* ViewerString = NiGetViewerString(NiNode::ms_RTTI.GetName());
    uint32_t m_usSize = pkStrings->m_usSize;
    uint32_t m_usMaxSize = pkStrings->m_usMaxSize;
    char* v11 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        pkStrings->SetSize(m_usSize + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(m_usSize, v11);
    BSNiBound* m_kWorldBound = this->m_kWorldBound;
    bool bVisual = m_kWorldBound && 0.0f != static_cast<NiBound&>(*m_kWorldBound).m_fRadius;
    char* ViewerString_0 = NiGetViewerString("m_bVisual", bVisual);
    uint32_t v10 = pkStrings->m_usSize;
    v11 = ViewerString_0;
    if (v10 >= pkStrings->m_usMaxSize) {
        pkStrings->SetSize(v10 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v10, v11);
}

// Looks this node up in the clone map and drops the result, as the game does.
void NiNode::ProcessClone(NiCloningProcess& kCloning) {
    NiAVObject::ProcessClone(kCloning);
    NiObject* pkClone;
    kCloning.m_pkCloneMap->GetAt(this, pkClone);
    for (uint32_t uiIndex = 0; uiIndex < m_kChildren.m_usSize; ++uiIndex) {
        NiAVObject* pChild = m_kChildren.m_pBase[uiIndex].m_pObject;
        if (pChild) {
            pChild->ProcessClone(kCloning);
        }
    }
}

void NiNode::SetBound(BSNiBound* apBound) {
    NiAVObject::SetBound(apBound);
}

// The child count is read once, before the loop.
void NiNode::UpdateControllers(NiUpdateData& UpdateData) {
    UpdateObjectControllers(UpdateData, true);
    const uint32_t uiCount = m_kChildren.m_usSize;
    for (uint32_t uiIndex = 0; uiIndex < uiCount; ++uiIndex) {
        NiAVObject* pChild = m_kChildren.m_pBase[uiIndex].m_pObject;
        if (pChild) {
            pChild->UpdateControllers(UpdateData);
        }
    }
}

void NiNode::ApplyTransform(const NiMatrix3& kMat, const NiPoint3& kTrn, bool bOnLeft) {
    NiMatrix3* v6;
    uint32_t v7;
    NiAVObject* m_pObject;
    uint32_t i;
    NiAVObject* v10;
    float v11;
    float v12;
    float v13;
    float v14;
    float v15;
    float v16;
    float v17;
    float v18;
    float v19;
    float v20;
    float v21;
    float v22;
    NiPoint3 v23;
    NiMatrix3 v24;
    NiMatrix3 result;
    float bOnLefta;
    if (bOnLeft) {
        v6 = &(result = kMat * m_kLocal.m_Rotate);
        (v24 = (&m_kLocal.m_Rotate)->TransposeTimes(*v6));
        v11 =
            (((&kMat)->m_pEntry[0][1] * m_kLocal.m_Translate.y) + (m_kLocal.m_Translate.x * (&kMat)->m_pEntry[0][0])) +
            ((&kMat)->m_pEntry[0][2] * m_kLocal.m_Translate.z);
        v13 =
            (((&kMat)->m_pEntry[1][0] * m_kLocal.m_Translate.x) + ((&kMat)->m_pEntry[1][1] * m_kLocal.m_Translate.y)) +
            ((&kMat)->m_pEntry[1][2] * m_kLocal.m_Translate.z);
        v15 =
            (((&kMat)->m_pEntry[2][0] * m_kLocal.m_Translate.x) + ((&kMat)->m_pEntry[2][1] * m_kLocal.m_Translate.y)) +
            ((&kMat)->m_pEntry[2][2] * m_kLocal.m_Translate.z);
        v17 = (&kTrn)->x + v11;
        v19 = (&kTrn)->y + v13;
        v21 = (&kTrn)->z + v15;
        v12 = v17 - m_kLocal.m_Translate.x;
        v14 = v19 - m_kLocal.m_Translate.y;
        v16 = v21 - m_kLocal.m_Translate.z;
        v18 = (m_kLocal.m_Rotate.m_pEntry[2][0] * v16) +
            ((m_kLocal.m_Rotate.m_pEntry[1][0] * v14) + (v12 * m_kLocal.m_Rotate.m_pEntry[0][0]));
        v20 = ((m_kLocal.m_Rotate.m_pEntry[1][1] * v14) + (m_kLocal.m_Rotate.m_pEntry[0][1] * v12)) +
            (m_kLocal.m_Rotate.m_pEntry[2][1] * v16);
        v7 = 0;
        v22 = (v16 * m_kLocal.m_Rotate.m_pEntry[2][2]) +
            ((v14 * m_kLocal.m_Rotate.m_pEntry[1][2]) + (v12 * m_kLocal.m_Rotate.m_pEntry[0][2]));
        bOnLefta = (float)(1.0 / m_kLocal.m_fScale);
        v23.x = (bOnLefta * v18);
        v23.y = (v20 * bOnLefta);
        for (v23.z = (bOnLefta * v22); v7 < ((uint32_t)((int32_t)m_kChildren.m_usSize)); ++v7) {
            m_pObject = m_kChildren.m_pBase[v7].m_pObject;
            if (m_pObject) {
                m_pObject->ApplyTransform(v24, v23, true);
            }
        }
    }
    else {
        for (i = 0; i < ((uint32_t)((int32_t)m_kChildren.m_usSize)); ++i) {
            v10 = m_kChildren.m_pBase[i].m_pObject;
            if (v10) {
                v10->ApplyTransform(kMat, kTrn, true);
            }
        }
    }
}

NiAVObject* NiNode::GetObjectByName(const NiFixedString& kName) {
    NiAVObject* pFound = NiAVObject::GetObjectByName(kName);
    if (pFound) {
        return pFound;
    }
    for (uint32_t uiIndex = 0; uiIndex < m_kChildren.m_usSize; ++uiIndex) {
        NiAVObject* pChild = m_kChildren.m_pBase[uiIndex].m_pObject;
        if (pChild) {
            pFound = pChild->GetObjectByName(kName);
            if (pFound) {
                return pFound;
            }
        }
    }
    return nullptr;
}

// The flag is the low byte of bSelectiveUpdateTransforms: when it is clear that byte is rewritten in place, and the
// whole value is passed on to each child.
void NiNode::SetSelectiveUpdateFlags(bool& bSelectiveUpdate, int32_t bSelectiveUpdateTransforms, bool& bRigid) {
    bool bTransformController = HasTransformController();
    if ((!static_cast<uint8_t>(bSelectiveUpdateTransforms)) &&
        (bTransformController || (m_uFlags & SELECTIVE_UPDATE_TRANSFORMS_OVERRIDE)))
    // The low byte is zero here: or-ing the bool in sets it, the upper bytes stay the caller's.
    {
        bSelectiveUpdateTransforms |= 1;
    }

    // The flag's raw low byte is stored and the stored bytes are tested against 0: a bool store would normalise the
    // byte to 0/1, and a bool load of any other byte is undefined in clang. So the flags are written and read as
    // untyped bytes, as the game does.
    uint8_t* pucSelectiveUpdate = static_cast<uint8_t*>(static_cast<void*>(&bSelectiveUpdate));
    *pucSelectiveUpdate = static_cast<uint8_t>(bSelectiveUpdateTransforms);
    bRigid = true;
    for (uint32_t i = 0; i < m_kChildren.m_usSize; ++i) {
        NiAVObject* pkChild = m_kChildren.m_pBase[i].m_pObject;
        if (pkChild) {
            bool bChildSelectiveUpdate = false;
            bool bChildRigid = true;
            pkChild->SetSelectiveUpdateFlags(bChildSelectiveUpdate, bSelectiveUpdateTransforms, bChildRigid);
            // A child node stores the raw low byte of the value passed on.
            if (*static_cast<uint8_t*>(static_cast<void*>(&bChildSelectiveUpdate))) {
                *pucSelectiveUpdate = 1;
            }
            if (!bChildRigid) {
                bRigid = false;
            }
        }
    }
    bool bPropertyController = HasPropertyController();
    if (bPropertyController || m_spControllers) {
        *pucSelectiveUpdate = 1;
        m_uFlags = NiFlags(m_uFlags | SELECTIVE_UPDATE);
    }
    else if (*pucSelectiveUpdate) {
        m_uFlags = NiFlags(m_uFlags | SELECTIVE_UPDATE);
    }
    else {
        m_uFlags = NiFlags(m_uFlags & ~SELECTIVE_UPDATE);
    }
    if (static_cast<uint8_t>(bSelectiveUpdateTransforms)) {
        m_uFlags = NiFlags(m_uFlags | SELECTIVE_UPDATE_TRANSFORMS);
    }
    else {
        m_uFlags = NiFlags(m_uFlags & ~SELECTIVE_UPDATE_TRANSFORMS);
    }
    if (bPropertyController) {
        m_uFlags = NiFlags(m_uFlags | SELECTIVE_UPDATE_CONTROLLER);
    }
    else {
        m_uFlags = NiFlags(m_uFlags & ~SELECTIVE_UPDATE_CONTROLLER);
    }
    if (bRigid) {
        m_uFlags = NiFlags(m_uFlags | SELECTIVE_UPDATE_RIGID);
    }
    else {
        m_uFlags = NiFlags(m_uFlags & ~SELECTIVE_UPDATE_RIGID);
    }
    if (bTransformController) {
        bRigid = false;
    }
}

void NiNode::UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    if (UpdateData.bUpdateControllers) {
        UpdateObjectControllers(UpdateData, true);
    }
    UpdateWorldData(UpdateData);
    if ((this->m_uFlags & 1) != 0) {
        uFlags |= 1u;
    }
    uint32_t m_uFlags = this->m_uFlags;
    uint32_t m_usSize = m_kChildren.m_usSize;
    uint32_t i = 0;
    uint32_t apData = m_usSize;
    if ((m_uFlags & NiAVObject::HAS_ORPHANED_BOUND) == 0) {
        goto LABEL_19;
    }
    if ((m_uFlags & NiAVObject::FIXED_BOUND) != 0) {
        CreateBoundIfNone();
    LABEL_19:;
        if (i < m_usSize) {
            do {
                NiAVObject* pkChild_2 = m_kChildren.m_pBase[i].m_pObject;
                if (pkChild_2) {
                    pkChild_2->UpdateDownwardPass(UpdateData, uFlags);
                    if ((this->m_uFlags & NiAVObject::FIXED_BOUND) == 0 &&
                        (this->m_uFlags & NiAVObject::HAS_ORPHANED_BOUND) != 0) {
                        NiBound* m_kWorldBound = pkChild_2->m_kWorldBound;
                        if ((m_kWorldBound) && (0.0f != m_kWorldBound->m_fRadius)) {
                            this->m_kWorldBound->Merge(m_kWorldBound);
                        }
                    }
                }
                ++i;
            }
            while (i < apData);
        }
        return;
    }
    BSNiBound* v8 = this->m_kWorldBound;
    if (v8) {
        static_cast<NiBound&>(*v8).m_fRadius = 0.0f;
    }
    if (m_usSize) {
        NiAVObject* pkChild;
        while (true) {
            pkChild = m_kChildren.m_pBase[i].m_pObject;
            if (pkChild) {
                pkChild->UpdateDownwardPass(UpdateData, uFlags);
                NiBound* v10 = pkChild->m_kWorldBound;
                if ((v10) && (0.0f != v10->m_fRadius)) {
                    break;
                }
            }
            if (++i >= apData) {
                return;
            }
        }
        CreateBoundIfNone();
        NiBound* v11 = pkChild->m_kWorldBound;
        if (!v11) {
            v11 = &NiAVObject::NullBoundS;
        }
        BSNiBound* v12 = this->m_kWorldBound;
        float childWorldBoundRadius = v11->m_fRadius;
        m_usSize = apData;
        v12->m_kCenter.x = v11->m_kCenter.x;
        v12->m_kCenter.y = v11->m_kCenter.y;
        float z = v11->m_kCenter.z;
        static_cast<NiBound&>(*v12).m_fRadius = childWorldBoundRadius;
        v12->m_kCenter.z = z;
        ++i;
        goto LABEL_19;
    }
}

void NiNode::UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    bool bUpdateControllers = UpdateData.bUpdateControllers;
    UpdateData.bUpdateControllers = (this->m_uFlags & NiAVObject::SELECTIVE_UPDATE_CONTROLLER) != 0;
    UpdateObjectControllers(UpdateData, true);
    bool v6 = !UpdateData.bParallelUpdate;
    UpdateData.bUpdateControllers = bUpdateControllers;
    if ((this->m_uFlags & NiAVObject::SELECTIVE_UPDATE_TRANSFORMS) != 0 && v6) {
        UpdateWorldData(UpdateData);
    }
    if ((this->m_uFlags & NiAVObject::APP_CULLED) != 0) {
        uFlags |= 1u;
    }
    uint32_t m_uFlags = this->m_uFlags;
    uint32_t m_usSize = m_kChildren.m_usSize;
    uint32_t v9 = 0;
    uint32_t v18 = m_usSize;
    if ((m_uFlags & NiAVObject::HAS_ORPHANED_BOUND) == 0) {
        goto LABEL_26;
    }
    if ((m_uFlags & NiAVObject::FIXED_BOUND) != 0) {
        CreateBoundIfNone();
    LABEL_26:;
        if (v9 < m_usSize) {
            do {
                NiAVObject* m_pObject = m_kChildren.m_pBase[v9].m_pObject;
                if (m_pObject) {
                    if ((m_pObject->m_uFlags & NiAVObject::SELECTIVE_UPDATE) != 0) {
                        if ((m_pObject->m_uFlags & NiAVObject::SELECTIVE_UPDATE_RIGID) != 0) {
                            m_pObject->UpdateRigidDownwardPass(UpdateData, uFlags);
                        }
                        else {
                            m_pObject->UpdateSelectedDownwardPass(UpdateData, uFlags);
                        }
                    }
                    if ((this->m_uFlags & NiAVObject::FIXED_BOUND) == 0 &&
                        (this->m_uFlags & NiAVObject::HAS_ORPHANED_BOUND) != 0) {
                        NiBound* m_kWorldBound = m_pObject->m_kWorldBound;
                        if ((m_kWorldBound) && (0.0 != m_kWorldBound->m_fRadius)) {
                            this->m_kWorldBound->Merge(m_kWorldBound);
                        }
                    }
                }
                ++v9;
            }
            while (v9 < v18);
        }
        return;
    }
    if (v6) {
        NiBound* v10 = this->m_kWorldBound;
        if (v10) {
            v10->m_fRadius = 0.0;
        }
    }
    if (m_usSize) {
        NiAVObject* v11;
        while (true) {
            v11 = m_kChildren.m_pBase[v9].m_pObject;
            if (v11) {
                if ((v11->m_uFlags & NiAVObject::SELECTIVE_UPDATE) != 0) {
                    if ((v11->m_uFlags & NiAVObject::SELECTIVE_UPDATE_RIGID) != 0) {
                        v11->UpdateRigidDownwardPass(UpdateData, uFlags);
                    }
                    else {
                        v11->UpdateSelectedDownwardPass(UpdateData, uFlags);
                    }
                }
                NiBound* v12 = v11->m_kWorldBound;
                if ((v12) && (0.0 != v12->m_fRadius && v6)) {
                    break;
                }
            }
            if (++v9 >= v18) {
                return;
            }
        }
        CreateBoundIfNone();
        NiBound* v13 = v11->m_kWorldBound;
        if (!v13) {
            v13 = &NiAVObject::NullBoundS;
        }
        NiBound* v14 = this->m_kWorldBound;
        float arDataa = v13->m_fRadius;
        m_usSize = v18;
        v14->m_kCenter.x = v13->m_kCenter.x;
        v14->m_kCenter.y = v13->m_kCenter.y;
        float z = v13->m_kCenter.z;
        v14->m_fRadius = arDataa;
        v14->m_kCenter.z = z;
        ++v9;
        goto LABEL_26;
    }
}

void NiNode::UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    uint8_t bUpdateControllers;
    uint32_t m_usSize;
    uint32_t i;
    NiAVObject* m_pObject;
    bUpdateControllers = (uint8_t)(&UpdateData)->bUpdateControllers;
    (&UpdateData)->bUpdateControllers = ((((int32_t)m_uFlags) & 8) != 0);
    NiAVObject::UpdateObjectControllers(UpdateData, true);
    (&UpdateData)->bUpdateControllers = (bool)bUpdateControllers;
    if ((((int32_t)m_uFlags) & 4) != 0) {
        UpdateWorldData(UpdateData);
    }
    if ((((int32_t)m_uFlags) & 1) != 0) {
        uFlags |= 1u;
    }
    m_usSize = (uint32_t)m_kChildren.m_usSize;
    for (i = 0; i < m_usSize; ++i) {
        m_pObject = m_kChildren.m_pBase[i].m_pObject;
        if ((m_pObject) && ((((int32_t)m_pObject->m_uFlags) & 2) != 0)) {
            m_pObject->UpdateRigidDownwardPass(UpdateData, uFlags);
        }
    }
}

// The base body is called directly, so the call is qualified; the children then get the pushed state through their
// virtual function. That state is freed unless PushLocalProperties handed the parent's own state back. The parameter,
// zeroed first, is used as PushLocalProperties' out value.
void NiNode::UpdatePropertiesDownward(NiPropertyState* pkParentState) {
    NiPropertyState* pParentState = pkParentState;
    NiAVObject::UpdatePropertiesDownward(pParentState);
    pkParentState = nullptr;
    PushLocalProperties(pParentState, true, pkParentState);
    for (uint32_t uiIndex = 0; uiIndex < m_kChildren.m_usSize; ++uiIndex) {
        NiAVObject* pChild = m_kChildren.m_pBase[uiIndex].m_pObject;
        if (pChild) {
            pChild->UpdatePropertiesDownward(pkParentState);
        }
    }
    NiPropertyState* pState = pkParentState;
    if ((pParentState != pState) && (pState)) {
        pState->~NiPropertyState();
        MemoryManager_Singleton.Deallocate(pState);
    }
}

void NiNode::UpdateWorldBound() {
    if ((m_uFlags & FIXED_BOUND) || !(m_uFlags & HAS_ORPHANED_BOUND)) {
        return;
    }

    const uint32_t uiCount = m_kChildren.m_usSize;
    uint32_t uiIndex = 0;
    NiAVObject* pFirst = nullptr;
    for (; uiIndex < uiCount; ++uiIndex) {
        pFirst = m_kChildren.m_pBase[uiIndex].m_pObject;
        if (pFirst && pFirst->m_kWorldBound && pFirst->m_kWorldBound->m_fRadius != 0.0f) {
            break;
        }
    }
    if (uiIndex >= uiCount) {
        return;
    }

    CreateBoundIfNone();
    const NiBound* pFirstBound = pFirst->m_kWorldBound ? pFirst->m_kWorldBound : &NullBoundS;
    *m_kWorldBound = *pFirstBound;

    for (++uiIndex; uiIndex < uiCount; ++uiIndex) {
        NiAVObject* pChild = m_kChildren.m_pBase[uiIndex].m_pObject;
        if (pChild && pChild->m_kWorldBound && pChild->m_kWorldBound->m_fRadius != 0.0f) {
            m_kWorldBound->Merge(pChild->m_kWorldBound);
        }
    }
}

void NiNode::UpdateTransformAndBounds(NiUpdateData& UpdateData) {
    if ((m_uFlags & NiAVObject::SELECTIVE_UPDATE_TRANSFORMS) != 0) {
        UpdateWorldData(UpdateData);
    }
    uint32_t m_usSize = m_kChildren.m_usSize;
    uint32_t v4 = 0;
    if ((m_uFlags & NiAVObject::HAS_ORPHANED_BOUND) == 0) {
        goto LABEL_17;
    }
    if ((m_uFlags & NiAVObject::FIXED_BOUND) != 0) {
        CreateBoundIfNone();
    LABEL_17:;
        while (v4 < m_usSize) {
            NiAVObject* m_pObject = m_kChildren.m_pBase[v4].m_pObject;
            if (m_pObject) {
                m_pObject->UpdateTransformAndBounds(UpdateData);
                if ((m_uFlags & NiAVObject::FIXED_BOUND) == 0 && (m_uFlags & NiAVObject::HAS_ORPHANED_BOUND) != 0) {
                    NiBound* m_kWorldBound = m_pObject->m_kWorldBound;
                    if ((m_kWorldBound) && (0.0 != m_kWorldBound->m_fRadius)) {
                        this->m_kWorldBound->Merge(m_kWorldBound);
                    }
                }
            }
            ++v4;
        }
        return;
    }
    NiBound* v5 = this->m_kWorldBound;
    if (v5) {
        v5->m_fRadius = 0.0;
    }
    if (m_usSize) {
        NiAVObject* v6;
        while (true) {
            v6 = m_kChildren.m_pBase[v4].m_pObject;
            if (v6) {
                v6->UpdateTransformAndBounds(UpdateData);
                NiBound* v7 = v6->m_kWorldBound;
                if ((v7) && (0.0 != v7->m_fRadius)) {
                    break;
                }
            }
            if (++v4 >= m_usSize) {
                return;
            }
        }
        CreateBoundIfNone();
        NiBound* v8 = v6->m_kWorldBound;
        if (!v8) {
            v8 = &NiAVObject::NullBoundS;
        }
        NiBound* v9 = this->m_kWorldBound;
        float m_fRadius = v8->m_fRadius;
        v9->m_kCenter.x = v8->m_kCenter.x;
        v9->m_kCenter.y = v8->m_kCenter.y;
        float z = v8->m_kCenter.z;
        v9->m_fRadius = m_fRadius;
        v9->m_kCenter.z = z;
        ++v4;
        goto LABEL_17;
    }
}

void NiNode::OnVisible(NiCullingProcess& kCuller) {
    if (m_kWorldBound && m_kWorldBound->m_fRadius != 0.0f) {
        for (uint32_t uiIndex = 0; uiIndex < m_kChildren.m_usSize; ++uiIndex) {
            NiAVObject* pChild = m_kChildren.m_pBase[uiIndex].m_pObject;
            if (pChild) {
                pChild->Cull(kCuller);
            }
        }
    }
}

void NiNode::PurgeRendererData(NiRenderer* apkRenderer) {
    NiAVObject::PurgeRendererData(apkRenderer);
    for (uint32_t uiIndex = 0; uiIndex < m_kChildren.m_usSize; ++uiIndex) {
        NiAVObject* pChild = m_kChildren.m_pBase[uiIndex].m_pObject;
        if (pChild) {
            pChild->PurgeRendererData(apkRenderer);
        }
    }
}

// The child takes a reference of its own before it is parented, and the local smart pointer that the array stores holds
// another; both are released at the end, the second one releasing the caller's reference.
void NiNode::AttachChild(NiAVObject* pkChild, int32_t bFirstAvail) {
    if (!pkChild) {
        return;
    }
    NiIncRefCount(pkChild);
    pkChild->AttachParent(this);
    {
        NiPointer<NiAVObject> spChild(pkChild);
        if (static_cast<uint8_t>(bFirstAvail)) {
            NiAVObjectArray_AddFirstEmpty(&m_kChildren, &spChild);
        }
        else {
            const uint32_t uiIndex = m_kChildren.m_usSize;
            if (uiIndex >= m_kChildren.m_usMaxSize) {
                NiAVObjectArray_SetSize(&m_kChildren, uiIndex + m_kChildren.m_usGrowBy);
            }
            NiAVObjectArray_SetAt(&m_kChildren, uiIndex, &spChild);
        }
    }
    NiDecRefCount(pkChild);
}

// The child takes a reference of its own before it is parented, and the local smart pointer that InsertAt stores
// holds another; both are released at the end, the second one releasing the caller's reference.
void NiNode::InsertChildAt(uint32_t i, NiAVObject* pkChild) {
    if (!pkChild) {
        return;
    }
    NiIncRefCount(pkChild);
    pkChild->AttachParent(this);
    {
        NiPointer<NiAVObject> spChild(pkChild);
        NiAVObjectArray_InsertAt(&m_kChildren, i, &spChild);
    }
    NiDecRefCount(pkChild);
}

void NiNode::DetachChild(NiAVObject* apChild) {
    // the detached child's reference is released when spDetached goes out of scope
    NiPointer<NiAVObject> spDetached;
    DetachChild(apChild, spDetached);
}

void NiNode::DetachChild(NiAVObject* apChild, NiPointer<NiAVObject>& arspAVObject) {
    NiAVObject* v5;
    uint32_t i = 0;
    if (!m_kChildren.m_usSize) {
    LABEL_7:;
        NiPointer<NiAVObject>* v6 = &arspAVObject;
        NiAVObject* m_pObject = arspAVObject.m_pObject;
        if (arspAVObject.m_pObject) {
            NiDecRefCount(m_pObject);
            v6->m_pObject = nullptr;
        }
        return;
    }
    while (true) {
        v5 = m_kChildren.m_pBase[i].m_pObject;
        if (v5) {
            break;
        }
    LABEL_6:;
        if (++i >= m_kChildren.m_usSize) {
            goto LABEL_7;
        }
    }
    NiIncRefCount(v5);
    if (v5 != apChild) {
        NiDecRefCount(v5);
        goto LABEL_6;
    }
    v5->m_pkParent = nullptr;
    { NiPointer<NiAVObject> v11 = (m_kChildren.RemoveAt(i)); }
    NiPointer<NiAVObject>* v9 = &arspAVObject;
    NiAVObject* v10 = arspAVObject.m_pObject;
    if (arspAVObject.m_pObject != v5) {
        if (v10) {
            NiDecRefCount(v10);
        }
        v9->m_pObject = v5;
        NiIncRefCount(v5);
    }
    NiDecRefCount(v5);
}

void NiNode::DetachChildAt(uint32_t auiIndex) {
    NiPointer<NiAVObject> spChild;
    this->DetachChildAt(auiIndex, spChild);
}

void NiNode::DetachChildAt(uint32_t auiIndex, NiPointer<NiAVObject>& arspAVObject) {
    if (auiIndex >= m_kChildren.m_usSize) {
        NiPointer<NiAVObject>* v9 = &arspAVObject;
        NiAVObject* m_pObject = arspAVObject.m_pObject;
        if (arspAVObject.m_pObject) {
            NiDecRefCount(m_pObject);
            v9->m_pObject = nullptr;
        }
    }
    else {
        NiAVObject* v5 = m_kChildren.m_pBase[auiIndex].m_pObject;
        if (v5) {
            NiIncRefCount(v5);
            v5->m_pkParent = nullptr;
            NiPointer<NiAVObject> v11 = (m_kChildren.RemoveAt(auiIndex));
        }
        NiPointer<NiAVObject>* v7 = &arspAVObject;
        NiAVObject* v8 = arspAVObject.m_pObject;
        if (arspAVObject.m_pObject == v5) {
            goto LABEL_12;
        }
        if (v8) {
            NiDecRefCount(v8);
        }
        v7->m_pObject = v5;
        if (v5) {
            NiIncRefCount(v5);
        LABEL_12:;
            if (v5) {
                NiDecRefCount(v5);
            }
        }
    }
}

void NiNode::SetAt(uint32_t auiIndex, NiAVObject* apChild) {
    // the replaced child's reference is released when spReplaced goes out of scope
    NiPointer<NiAVObject> spReplaced;
    SetAt(auiIndex, apChild, spReplaced);
}

void NiNode::SetAt(uint32_t auiIndex, NiAVObject* apChild, NiPointer<NiAVObject>& arspAVObject) {
    if (m_kChildren.m_usSize <= auiIndex) {
        if (apChild) {
            apChild->AttachParent(this);
        }
        {
            NiPointer<NiAVObject> spChild(apChild);
            if (auiIndex >= m_kChildren.m_usMaxSize) {
                NiAVObjectArray_SetSize(&m_kChildren, auiIndex + m_kChildren.m_usGrowBy);
            }
            NiAVObjectArray_SetAt(&m_kChildren, auiIndex, &spChild);
        }
        NiPointer<NiAVObject>* v6 = &arspAVObject;
        NiAVObject* m_pObject = arspAVObject.m_pObject;
        if (arspAVObject.m_pObject) {
            NiDecRefCount(m_pObject);
            v6->m_pObject = nullptr;
        }
        return;
    }
    NiAVObject* v8 = m_kChildren.m_pBase[auiIndex].m_pObject;
    if (v8) {
        NiIncRefCount(v8);
        v8->m_pkParent = nullptr;
    }
    if (apChild) {
        apChild->AttachParent(this);
    }
    {
        NiPointer<NiAVObject> spChild(apChild);
        NiAVObjectArray_SetAt(&m_kChildren, auiIndex, &spChild);
    }
    NiPointer<NiAVObject>* v10 = &arspAVObject;
    NiAVObject* v11 = arspAVObject.m_pObject;
    if (arspAVObject.m_pObject == v8) {
        goto LABEL_30;
    }
    if (v11) {
        NiDecRefCount(v11);
    }
    v10->m_pObject = v8;
    if (v8) {
        NiIncRefCount(v8);
    LABEL_30:;
        if (v8) {
            NiDecRefCount(v8);
        }
    }
}

void NiNode::UpdateUpwardPass() {
    NiNode* pParent = m_pkParent;
    UpdateWorldBound();
    if (pParent) {
        pParent->UpdateUpwardPass();
    }
}

uint32_t NiNode::GetArrayCount() const {
    return m_kChildren.m_usSize;
}

uint32_t NiNode::GetChildCount() const {
    return m_kChildren.m_usESize;
}

NiAVObject* NiNode::GetAt(uint32_t i) const {
    return m_kChildren.m_pBase[i].m_pObject;
}

NiAVObject* NiNode::GetAtChecked(uint32_t i) const {
    if (uint32_t(m_kChildren.m_usSize) <= i) {
        return nullptr;
    }
    return m_kChildren.m_pBase[i].m_pObject;
}

void NiNode::CompactChildArray() {
    NiAVObjectArray_Compact(&m_kChildren);
    NiAVObjectArray_UpdateSize(&m_kChildren);
}

// Copies the base members, then clones each child into the same slot of apDest.
void NiNode::CopyMembers(NiNode* pkDest, NiCloningProcess& kCloning) {
    NiAVObject::CopyMembers(pkDest, kCloning);
    for (uint32_t uiIndex = 0; uiIndex < m_kChildren.m_usSize; ++uiIndex) {
        NiAVObject* pChild = m_kChildren.m_pBase[uiIndex].m_pObject;
        if (pChild) {
            pkDest->SetAt(uiIndex, static_cast<NiAVObject*>(pChild->CreateClone(kCloning)));
        }
    }
}

// The state built from the parent is freed unless PushLocalProperties kept it.
void NiNode::UpdatePropertiesUpward(NiPropertyState*& aspPropertyState) {
    uint32_t& ruFlags = m_uFlags;
    if (HasPropertyController()) {
        ruFlags = NiFlags(ruFlags | HAS_PROPERTY_CONTROLLER);
    }
    else {
        ruFlags = NiFlags(ruFlags & ~HAS_PROPERTY_CONTROLLER);
    }
    NiPropertyState* pState = nullptr;
    NiNode* pParent = m_pkParent;
    if (pParent) {
        pParent->UpdatePropertiesUpward(pState);
    }
    else {
        void* pvMemory = MemoryManager_Singleton.Allocate(sizeof(NiPropertyState));
        pState = pvMemory ? ::new (pvMemory) NiPropertyState : nullptr;
    }
    PushLocalProperties(pState, false, aspPropertyState);
    if ((pState != aspPropertyState) && (pState)) {
        pState->~NiPropertyState();
        MemoryManager_Singleton.Deallocate(pState);
    }
}

NiTObjectArray<NiPointer<NiAVObject>>* NiNode::GetChildren() {
    return &m_kChildren;
}

float NiNode::GetLocalScale() const {
    return m_kLocal.m_fScale;
}

char NiNode::GetBlastNodePos(NiAVObject* arg0, NiPoint3* a2) {
    if (!arg0) {
        return 0;
    }
    char BlastNodePos = 0;
    if (NiObject::IsKindOf_0(&BSBlastNode::ms_RTTI, arg0)) {
        BlastNodePos = 1;
        *a2 = arg0->GetWorldTranslate();
    }
    NiNode* v7 = arg0->IsNode();
    if (v7) {
        for (uint32_t a1 = 0; !BlastNodePos && a1 < v7->GetArrayCount(); ++a1) {
            NiAVObject* v4 = v7->GetAt(a1);
            BlastNodePos = GetBlastNodePos(v4, a2);
        }
    }
    return BlastNodePos;
}

bool NiNode::IsLeftFacingUp(NiAVObject* apObject, char* apString) {
    if (!apObject) {
        return false;
    }
    NiAVObject* pkObject = nullptr;
    if (apString) {
        pkObject = apObject->GetObjectByName(apString);
    }
    if (!pkObject) {
        pkObject = apObject;
    }
    return pkObject->m_kWorld.m_Rotate.m_pEntry[2][2] > 0.0f;
}

NiObject* NiNode::CreateObject() {
    void* v0 = NiMemObject::operator new(sizeof(NiNode));
    NiNode* v1 = static_cast<NiNode*>(v0);
    if (!v0) {
        return nullptr;
    }
    ::new (v0) NiNode;
    return v1;
}

void NiAVObjectArray_SetAt(
    NiTObjectArray<NiPointer<NiAVObject>>* apArray,
    uint32_t auiIndex,
    const NiPointer<NiAVObject>* apElement
) {
    // `tcmp` is NiTArray::SetAt's guarded null element.
    static NiPointer<NiAVObject> tcmp = static_cast<NiAVObject*>(nullptr);
    if (auiIndex < apArray->m_usSize) {
        if (*apElement != tcmp) {
            if (apArray->m_pBase[auiIndex] == tcmp) {
                ++apArray->m_usESize;
            }
        }
        else {
            if (apArray->m_pBase[auiIndex] != tcmp) {
                --apArray->m_usESize;
            }
        }
    }
    else {
        apArray->m_usSize = auiIndex + 1;
        if (*apElement != tcmp) {
            ++apArray->m_usESize;
        }
    }
    apArray->m_pBase[auiIndex] = *apElement;
}

void NiAVObjectArray_Deallocate(NiPointer<NiAVObject>* apArray) {
    if (apArray) {
        AlwaysRenderArray_DeleteBase(apArray, 3u);
    }
}

// Returns the slot the element went to, or ~0 for a null element (NiTArray::AddFirstEmpty).
uint32_t
NiAVObjectArray_AddFirstEmpty(NiTObjectArray<NiPointer<NiAVObject>>* apArray, const NiPointer<NiAVObject>* apElement) {
    if (!apElement->m_pObject) {
        return 0xFFFFFFFFu;
    }
    uint16_t m_usSize = apArray->m_usSize;
    uint16_t v4 = 0;
    if (m_usSize) {
        NiPointer<NiAVObject>* m_pBase = apArray->m_pBase;
        while (m_pBase[v4].m_pObject) {
            if (++v4 >= apArray->m_usSize) {
                goto LABEL_6;
            }
        }
        int32_t v6 = v4;
        NiAVObject* m_pObject = m_pBase[v4].m_pObject;
        if (m_pObject != apElement->m_pObject) {
            if (m_pObject) {
                NiDecRefCount(m_pObject);
            }
            NiAVObject* v8 = apElement->m_pObject;
            m_pBase[v6].m_pObject = apElement->m_pObject;
            if (v8) {
                NiIncRefCount(v8);
            }
        }
        ++apArray->m_usESize;
        return v4;
    }
LABEL_6:
    if (m_usSize >= static_cast<uint32_t>(apArray->m_usMaxSize)) {
        NiAVObjectArray_SetSize(apArray, m_usSize + apArray->m_usGrowBy);
    }
    NiAVObjectArray_SetAt(apArray, m_usSize, apElement);
    return m_usSize;
}

// NiTArray<NiPointer<NiAVObject>>::Compact and ::UpdateSize. Each null test builds a null NiPointer<NiAVObject> and
// destroys it.
void NiAVObjectArray_Compact(NiTObjectArray<NiPointer<NiAVObject>>* apArray) {
    if (apArray->m_usESize != apArray->m_usSize) {
        if (apArray->m_usESize) {
            uint16_t v6 = 0;
            uint16_t v5 = 0;
            while (v6 < static_cast<int32_t>(apArray->m_usSize)) {
                bool v3 = apArray->m_pBase[v6] != NiPointer<NiAVObject>(static_cast<NiAVObject*>(nullptr));
                if (v3) {
                    if (apArray->m_pBase[v5] != apArray->m_pBase[v6]) {
                        apArray->m_pBase[v5] = apArray->m_pBase[v6];
                    }
                    ++v5;
                }
                ++v6;
            }
        }
        NiPointer<NiAVObject>* pSaveBase = apArray->m_pBase;
        apArray->m_usSize = apArray->m_usESize;
        apArray->m_usMaxSize = apArray->m_usSize;
        if (apArray->m_usMaxSize) {
            apArray->m_pBase = NiTNewInterface<NiPointer<NiAVObject>>::Allocate(apArray->m_usMaxSize);
            for (uint16_t i = 0; i < static_cast<int32_t>(apArray->m_usSize); ++i) {
                apArray->m_pBase[i] = pSaveBase[i];
            }
        }
        else {
            apArray->m_pBase = nullptr;
        }
        NiAVObjectArray_Deallocate(pSaveBase);
    }
}
