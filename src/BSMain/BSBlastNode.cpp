#include "BSMain/BSBlastNode.h"

#include "BSMain/BSRangeNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSBlastNode::ms_RTTI("BSBlastNode", &BSRangeNode::ms_RTTI);

BSBlastNode::BSBlastNode() {}

const NiRTTI* BSBlastNode::GetRTTI() const {
    return &ms_RTTI;
}

// The default constructor, then the members copied.
NiObject* BSBlastNode::CreateClone(NiCloningProcess& arCloneData) {
    BSBlastNode* pClone = static_cast<BSBlastNode*>(NiMemObject::operator new(sizeof(BSBlastNode)));
    if (pClone) {
        ::new (pClone) BSBlastNode;
    }
    else {
        pClone = nullptr;
    }
    CopyMembers(pClone, arCloneData);
    return pClone;
}

void BSBlastNode::LoadBinary(NiStream& arStream) {
    BSRangeNode::LoadBinary(arStream);
}

void BSBlastNode::LinkObject(NiStream& arStream) {
    BSRangeNode::LinkObject(arStream);
}

bool BSBlastNode::RegisterStreamables(NiStream& arStream) {
    return BSRangeNode::RegisterStreamables(arStream);
}

void BSBlastNode::SaveBinary(NiStream& arStream) {
    BSRangeNode::SaveBinary(arStream);
}

bool BSBlastNode::IsEqual(NiObject* apObject) {
    return BSRangeNode::IsEqual(apObject);
}

void BSBlastNode::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSRangeNode::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

void BSBlastNode::CopyMembers(BSBlastNode* pDest, NiCloningProcess& arCloneData) {
    BSRangeNode::CopyMembers(pDest, arCloneData);
}

NiObject* BSBlastNode::CreateObject() {
    BSBlastNode* pObject = static_cast<BSBlastNode*>(NiMemObject::operator new(sizeof(BSBlastNode)));
    if (!pObject) {
        return 0;
    }
    return ::new (pObject) BSBlastNode;
}
