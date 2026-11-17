#include "BSMain/BSMultiBound.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"

NiRTTI BSMultiBound::ms_RTTI("BSMultiBound", &NiObject::ms_RTTI);

BSMultiBound::BSMultiBound() : NiObject(), uiBoundFrameCount(0), spShape(nullptr) {}

// The shape is released by its member's destructor.
BSMultiBound::~BSMultiBound() {}

NiObject* BSMultiBound::CreateClone(NiCloningProcess& arCloneData) {
    void* pMemory = NiMemObject::operator new(sizeof(BSMultiBound));
    BSMultiBound* pkClone = pMemory ? ::new (pMemory) BSMultiBound : nullptr;
    CopyMembers(pkClone, arCloneData);
    return pkClone;
}

// The shape's link ID is read here and resolved in LinkObject.
void BSMultiBound::LoadBinary(NiStream& arStream) {
    NiObject::LoadBinary(arStream);
    arStream.ReadLinkID();
}

void BSMultiBound::LinkObject(NiStream& arStream) {
    NiObject::LinkObject(arStream);
    spShape = static_cast<BSMultiBoundShape*>(arStream.GetObjectFromLinkID());
}

// The shape is registered without a null check.
bool BSMultiBound::RegisterStreamables(NiStream& arStream) {
    if (!NiObject::RegisterStreamables(arStream)) {
        return false;
    }

    spShape->RegisterStreamables(arStream);
    return true;
}

void BSMultiBound::SaveBinary(NiStream& arStream) {
    NiObject::SaveBinary(arStream);
    arStream.SaveLinkID(spShape);
}

// NiObject's verdict is computed and dropped: only the shape pointer and the frame count decide.
bool BSMultiBound::IsEqual(NiObject* apObject) {
    NiObject::IsEqual(apObject);
    BSMultiBound* pkOther = static_cast<BSMultiBound*>(apObject);
    return spShape.m_pObject == pkOther->spShape.m_pObject && uiBoundFrameCount == pkOther->uiBoundFrameCount;
}

void BSMultiBound::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiObject::GetViewerStrings(apStrings);
}

bool BSMultiBound::QPointWithin(NiPoint3& aPoint) {
    return spShape && spShape->QPointWithin(aPoint);
}

void BSMultiBound::SetShape(BSMultiBoundShape* apShape) {
    spShape = apShape;
}

BSMultiBoundShape* BSMultiBound::QShape() const {
    return spShape;
}

// The shape is cloned without a null check.
void BSMultiBound::CopyMembers(BSMultiBound* pDest, NiCloningProcess& arCloneData) {
    NiObject::CopyMembers(pDest, arCloneData);
    pDest->spShape = static_cast<BSMultiBoundShape*>(spShape->CreateClone(arCloneData));
}

NiObject* BSMultiBound::CreateObject() {
    void* pMemory = NiMemObject::operator new(sizeof(BSMultiBound));
    return pMemory ? ::new (pMemory) BSMultiBound : nullptr;
}
