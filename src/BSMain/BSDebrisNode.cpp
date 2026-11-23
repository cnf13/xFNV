#include "BSMain/BSDebrisNode.h"

#include "BSMain/BSRangeNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSDebrisNode::ms_RTTI("BSDebrisNode", &BSRangeNode::ms_RTTI);

BSDebrisNode::BSDebrisNode() {}

const NiRTTI* BSDebrisNode::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* BSDebrisNode::CreateClone(NiCloningProcess& arCloneData) {
    BSDebrisNode* pClone = new BSDebrisNode;
    CopyMembers(pClone, arCloneData);
    return pClone;
}

void BSDebrisNode::LoadBinary(NiStream& arStream) {
    BSRangeNode::LoadBinary(arStream);
}

void BSDebrisNode::LinkObject(NiStream& arStream) {
    BSRangeNode::LinkObject(arStream);
}

bool BSDebrisNode::RegisterStreamables(NiStream& arStream) {
    return BSRangeNode::RegisterStreamables(arStream);
}

void BSDebrisNode::SaveBinary(NiStream& arStream) {
    BSRangeNode::SaveBinary(arStream);
}

bool BSDebrisNode::IsEqual(NiObject* apObject) {
    return BSRangeNode::IsEqual(apObject);
}

void BSDebrisNode::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSRangeNode::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

void BSDebrisNode::CopyMembers(BSDebrisNode* pDest, NiCloningProcess& arCloneData) {
    BSRangeNode::CopyMembers(pDest, arCloneData);
}

NiObject* BSDebrisNode::CreateObject() {
    return new BSDebrisNode;
}
