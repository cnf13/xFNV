#include "BSMain/BSDamageStage.h"

#include "BSMain/BSNiNode.h"
#include "BSMain/BSRangeNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSDamageStage::ms_RTTI("BSDamageStage", &BSRangeNode::ms_RTTI);

BSDamageStage::BSDamageStage() {}

const NiRTTI* BSDamageStage::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* BSDamageStage::CreateClone(NiCloningProcess& arCloneData) {
    BSDamageStage* pClone = static_cast<BSDamageStage*>(NiMemObject::operator new(sizeof(BSDamageStage)));
    if (pClone) {
        ::new (pClone) BSDamageStage;
    }
    else {
        pClone = nullptr;
    }
    CopyMembers(pClone, arCloneData);
    return pClone;
}

void BSDamageStage::LoadBinary(NiStream& arStream) {
    BSRangeNode::LoadBinary(arStream);
}

void BSDamageStage::LinkObject(NiStream& arStream) {
    BSRangeNode::LinkObject(arStream);
}

bool BSDamageStage::RegisterStreamables(NiStream& arStream) {
    return BSRangeNode::RegisterStreamables(arStream);
}

void BSDamageStage::SaveBinary(NiStream& arStream) {
    BSRangeNode::SaveBinary(arStream);
}

bool BSDamageStage::IsEqual(NiObject* apObject) {
    return BSRangeNode::IsEqual(apObject);
}

void BSDamageStage::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSRangeNode::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// An app-culled stage is not updated.
void BSDamageStage::UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    if (!(m_uFlags & APP_CULLED)) {
        NiNode::UpdateDownwardPass(UpdateData, uFlags);
    }
}

// An app-culled stage is not updated.
void BSDamageStage::UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    if (!(m_uFlags & APP_CULLED)) {
        NiNode::UpdateSelectedDownwardPass(UpdateData, uFlags);
    }
}

// An app-culled stage is not updated.
void BSDamageStage::UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) {
    if (!(m_uFlags & APP_CULLED)) {
        NiNode::UpdateRigidDownwardPass(UpdateData, uFlags);
    }
}

void BSDamageStage::CopyMembers(BSDamageStage* pDest, NiCloningProcess& arCloneData) {
    BSRangeNode::CopyMembers(pDest, arCloneData);
}

NiObject* BSDamageStage::CreateObject() {
    BSDamageStage* pObject = static_cast<BSDamageStage*>(NiMemObject::operator new(sizeof(BSDamageStage)));
    if (!pObject) {
        return nullptr;
    }
    return ::new (pObject) BSDamageStage;
}
