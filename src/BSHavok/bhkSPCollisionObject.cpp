#include "BSHavok/bhkSPCollisionObject.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkPCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShapePhantom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI bhkSPCollisionObject::ms_RTTI("bhkSPCollisionObject", &bhkPCollisionObject::ms_RTTI);

bhkSPCollisionObject::bhkSPCollisionObject() {}

bhkSPCollisionObject::bhkSPCollisionObject(NiNode* apObject) : bhkPCollisionObject(apObject) {}

// The clone's allocation is charged to the Havok heap context, restored after the copy.
NiObject* bhkSPCollisionObject::CreateClone(NiCloningProcess& arCloneData) {
    int32_t iOldMemContext = GetThreadMemContext();
    GetThreadMemContext() = MEM_CONTEXT_HAVOK;
    bhkSPCollisionObject* pClone = new bhkSPCollisionObject;
    // Copies even into a null clone, as the game does.
    CopyMembers(pClone, arCloneData);
    GetThreadMemContext() = iOldMemContext;
    return pClone;
}

void bhkSPCollisionObject::LoadBinary(NiStream& arStream) {
    bhkPCollisionObject::LoadBinary(arStream);
}

void bhkSPCollisionObject::LinkObject(NiStream& arStream) {
    bhkPCollisionObject::LinkObject(arStream);
}

bool bhkSPCollisionObject::RegisterStreamables(NiStream& arStream) {
    return bhkPCollisionObject::RegisterStreamables(arStream);
}

void bhkSPCollisionObject::SaveBinary(NiStream& arStream) {
    bhkPCollisionObject::SaveBinary(arStream);
}

bool bhkSPCollisionObject::IsEqual(NiObject* apObject) {
    return bhkPCollisionObject::IsEqual(apObject);
}

void bhkSPCollisionObject::AddViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    TLSData* v2 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v2->eMemContext;
    v2->eMemContext = MEM_CONTEXT_HAVOK;
    bhkPCollisionObject::AddViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkSPCollisionObject::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v8 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v8);
    v2->eMemContext = eMemContext;
}

// The phantom's Havok transform, converted to game units, becomes the scene object's (through the vtable).
void bhkSPCollisionObject::hktoNiTransform() {
    // spWorldObject is a NiPointer<bhkWorldObject>; this collision object's world object is a phantom.
    bhkShapePhantom* psp = static_cast<bhkShapePhantom*>(static_cast<bhkWorldObject*>(spWorldObject));
    if ((psp) && (psp->phkObject)) {
        NiTransform kTransform;
        psp->GetTransform(kTransform);
        SetTransform(kTransform);
    }
}

void bhkSPCollisionObject::NitohkTransform() {
    NiTransform nit;
    // spWorldObject is a NiPointer<bhkWorldObject>; this collision object's world object is a phantom.
    bhkShapePhantom* psp = static_cast<bhkShapePhantom*>(static_cast<bhkWorldObject*>(spWorldObject));
    NiAVObject* m_pkSceneObject = this->m_pkSceneObject;
    if (m_pkSceneObject) {
        this->m_pkSceneObject->UpdateWorldLoc();
        if ((psp) && (psp->phkObject)) {
            nit = m_pkSceneObject->m_kWorld;
            bhkNiCollisionObject::kKeyframeLock.Lock(nullptr);
            psp->SetTransform(nit);
            if (!--bhkNiCollisionObject::kKeyframeLock.uiLockCount) {
                bhkNiCollisionObject::kKeyframeLock.uiOwningThread = 0;
            }
        }
    }
}

void bhkSPCollisionObject::CopyMembers(bhkSPCollisionObject* apDest, NiCloningProcess& arCloneData) {
    bhkPCollisionObject::CopyMembers(apDest, arCloneData);
}

NiObject* bhkSPCollisionObject::CreateObject() {
    return new bhkSPCollisionObject;
}
