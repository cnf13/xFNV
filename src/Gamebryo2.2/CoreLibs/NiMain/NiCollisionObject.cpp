#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"

#include "BSHavok/bhkNiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiCollisionObject::ms_RTTI("NiCollisionObject", &NiObject::ms_RTTI);

NiCollisionObject::NiCollisionObject() : m_pkSceneObject(nullptr) {}

NiCollisionObject::NiCollisionObject(NiAVObject* pkSceneObject) : m_pkSceneObject(pkSceneObject) {
    if (pkSceneObject && pkSceneObject->m_spCollisionObject.m_pObject != this) {
        pkSceneObject->SetCollisionObject(this);
    }
}

NiCollisionObject::~NiCollisionObject() {}

void NiCollisionObject::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
    kStream.ReadLinkID();
}

// Links the scene object the collision object belongs to.
void NiCollisionObject::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
    m_pkSceneObject = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
}

bool NiCollisionObject::RegisterStreamables(NiStream& kStream) {
    if (!NiObject::RegisterStreamables(kStream)) {
        return false;
    }
    if (m_pkSceneObject) {
        m_pkSceneObject->RegisterStreamables(kStream);
    }
    return true;
}

// Saves the scene object's link ID.
void NiCollisionObject::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
    kStream.SaveLinkID(m_pkSceneObject);
}

// Compares only whether the scene object and apObject are both set or both null.
bool NiCollisionObject::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }
    if (m_pkSceneObject) {
        return pkObject != nullptr;
    }
    return pkObject == nullptr;
}

// The class title, then the members through the virtual AddViewerStrings.
void NiCollisionObject::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.m_pcName));
    AddViewerStrings(pkStrings);
}

void NiCollisionObject::AddViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    pkStrings->Add(NiGetViewerString(ms_RTTI.m_pcName));
    pkStrings->Add(NiGetViewerString("m_pkSceneObject", static_cast<const void*>(m_pkSceneObject)));
}

void NiCollisionObject::SetSceneGraphObject(NiAVObject* pkSceneObject) {
    m_pkSceneObject = pkSceneObject;
    if (pkSceneObject &&
        (static_cast<const void*>(pkSceneObject->m_spCollisionObject.m_pObject) != static_cast<const void*>(this))) {
        pkSceneObject->SetCollisionObject(this);
    }
}
