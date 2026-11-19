#include "Gamebryo2.2/CoreLibs/NiMain/NiSortAdjustNode.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVisibleArray.h"

NiRTTI NiSortAdjustNode::ms_RTTI("NiSortAdjustNode", &NiNode::ms_RTTI);

NiSortAdjustNode::NiSortAdjustNode() : NiNode(0), m_eSortingMode(NiSortAdjustNode::SORTING_INHERIT) {}

NiSortAdjustNode::~NiSortAdjustNode() {}

const NiRTTI* NiSortAdjustNode::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiSortAdjustNode::CreateClone(NiCloningProcess& kCloning) {
    NiSortAdjustNode* pkObject_2 = new NiSortAdjustNode;
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_eSortingMode = m_eSortingMode;
    return pkObject_2;
}

// Unknown modes fall back to SORTING_INHERIT. Files before 20.0.0.4 also link an accumulator, which is dropped.
void NiSortAdjustNode::LoadBinary(NiStream& stream) {
    NiNode::LoadBinary(stream);
    int32_t iMode;
    NiStreamLoadBinary(stream, iMode);
    m_eSortingMode = SortingMode(iMode);
    if (!(iMode < SORTING_MAX)) {
        m_eSortingMode = SORTING_INHERIT;
    }
    if (stream.m_uiNifFileVersion < 0x14000004) {
        stream.ReadLinkID();
    }
}

// KEEP: the exe reads the mode into its `push ecx` slot (holding `this` until then), so a short read (malformed
// data) leaves that value there and the mode test stores 0, which src cannot express; porting.md's unwritten-slot
// exception covers GetChunkData only.
void NiSortAdjustNode::LinkObject(NiStream& stream) {
    NiNode::LinkObject(stream);
    if (stream.m_uiNifFileVersion < 0x14000004) {
        stream.GetObjectFromLinkID();
    }
}

bool NiSortAdjustNode::RegisterStreamables(NiStream& stream) {
    return NiNode::RegisterStreamables(stream);
}

void NiSortAdjustNode::SaveBinary(NiStream& stream) {
    NiNode::SaveBinary(stream);
    NiStreamSaveBinary(stream, m_eSortingMode);
}

bool NiSortAdjustNode::IsEqual(NiObject* pObject) {
    if (!NiNode::IsEqual(pObject)) {
        return false;
    }
    return m_eSortingMode == static_cast<NiSortAdjustNode*>(pObject)->m_eSortingMode;
}

void NiSortAdjustNode::ProcessClone(NiCloningProcess& kCloning) {
    NiNode::ProcessClone(kCloning);
}

// Marks the geometry the node's subtree added to the visible set: sorting off sets DISABLE_SORTING, any other mode
// clears it.
void NiSortAdjustNode::OnVisible(NiCullingProcess& kCuller) {
    uint32_t uiStart = kCuller.m_pkVisibleSet->m_uiCurrentSize;
    bool bSort = m_eSortingMode != SORTING_OFF;
    NiNode::OnVisible(kCuller);
    NiVisibleArray* pkVisible = kCuller.m_pkVisibleSet;
    uint32_t uiEnd = pkVisible->m_uiCurrentSize;
    for (uint32_t i = uiStart; i < uiEnd; ++i) {
        NiGeometry* pkGeom = pkVisible->m_ppkArray[i];
        if (bSort) {
            pkGeom->m_uFlags = NiAVObject::NiFlags(pkGeom->m_uFlags & ~NiAVObject::DISABLE_SORTING);
        }
        else {
            pkGeom->m_uFlags = NiAVObject::NiFlags(pkGeom->m_uFlags | NiAVObject::DISABLE_SORTING);
        }
    }
}

NiObject* NiSortAdjustNode::CreateObject() {
    return new NiSortAdjustNode;
}
