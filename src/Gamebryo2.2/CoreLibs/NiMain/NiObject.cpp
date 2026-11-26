#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMainSDM.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cstring>

static NiMainSDM NiMainSDMObject;
NiRTTI NiObject::ms_RTTI("NiObject", nullptr);

NiObject::NiObject() {}

NiObject::~NiObject() {}

void NiObject::LoadBinary(NiStream& stream) {
    // Files from 5.0.0.6 up to (not including) 10.1.0.114 store each object's group ID.
    if (stream.GetFileVersion() >= NiStream::GetVersion(5, 0, 0, 6) &&
        stream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 114)) {
        uint32_t uiGroupID;
        NiStreamLoadBinary(stream, uiGroupID);
        SetGroup(stream.GetGroupFromID(uiGroupID));
    }
}

void NiObject::LinkObject(NiStream& stream) {}

bool NiObject::RegisterStreamables(NiStream& stream) {
    return stream.RegisterSaveObject(this);
}

// The base comparison: both objects are of the same class (by RTTI name).
bool NiObject::IsEqual(NiObject* pObject) {
    if (!pObject) {
        return false;
    }

    const char* pcOtherName = pObject->GetRTTI()->GetName();
    return strcmp(GetRTTI()->GetName(), pcOtherName) == 0;
}

void NiObject::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    pStrings->Add(NiGetViewerString(NiObject::ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("this", static_cast<const void*>(this)));
    pStrings->Add(NiGetViewerString("m_uiRefCount", m_uiRefCount));
}

// Marks this object processed.
void NiObject::ProcessClone(NiCloningProcess& kCloning) {
    kCloning.m_pkProcessMap->SetAt(this, true);
}

const NiRTTI* NiObject::GetStreamableRTTI() const {
    return GetRTTI();
}

void NiObject::CopyMembers(NiObject* pDest, NiCloningProcess& kCloning) {
    kCloning.m_pkCloneMap->SetAt(this, pDest);
}

// An object already cloned in this process is shared; anything else gets a new clone.
NiObject* NiObject::CreateSharedClone(NiCloningProcess& kCloning) {
    NiObject* pkClone;
    if (kCloning.m_pkCloneMap->GetAt(this, pkClone)) {
        return pkClone;
    }
    return CreateClone(kCloning);
}

NiObject* NiObject::Clone() {
    NiCloningProcess kCloning(257u);
    NiObject* v2 = CreateClone(kCloning);
    ProcessClone(kCloning);
    return v2;
}

void NiObject::CreateDeepCopy(NiPointer<NiObject>& aspObject) {
    NiStream stream;
    stream.InsertObject(this);
    char* pBuffer = nullptr;
    int32_t iBufferSize = 0;
    stream.NiStream::Save(pBuffer, iBufferSize);
    stream.NiStream::Load(pBuffer, iBufferSize);
    NiObject* m_pObject = stream.m_kTopObjects.m_pBase->m_pObject;
    if (stream.m_kTopObjects.m_pBase->m_pObject) {
        NiIncRefCount(m_pObject);
    }
    _NiFree(pBuffer);
    NiObject* v4 = aspObject.m_pObject;
    if (aspObject.m_pObject != m_pObject) {
        if (v4) {
            NiDecRefCount(v4);
        }
        aspObject.m_pObject = m_pObject;
        if (m_pObject) {
            NiIncRefCount(m_pObject);
        }
    }
    if (m_pObject) {
        NiDecRefCount(m_pObject);
    }
}

NiObject* NiObject::Clone(NiCloningProcess& arCloning) {
    NiObject* v3 = CreateClone(arCloning);
    ProcessClone(arCloning);
    return v3;
}

bool NiObject::IsKindOfMember(const NiRTTI* pkRTTI) const {
    for (const NiRTTI* i = GetRTTI(); i; i = i->m_pkBaseRTTI) {
        if (i == pkRTTI) {
            return true;
        }
    }
    return false;
}

bool NiObject::IsExactKindOf(const NiRTTI* pkRTTI, const NiObject* pkObject) {
    return pkObject && pkObject->IsExactKindOf(pkRTTI);
}

bool NiObject::IsKindOf_0(NiRTTI* pkRTTI, NiObject* pkObject) {
    return pkObject && pkObject->IsKindOfMember(pkRTTI);
}

NiObject* NiObject::DynamicCastMember(const NiRTTI* pkRTTI) const {
    return IsKindOfMember(pkRTTI) ? const_cast<NiObject*>(this) : nullptr;
}

// Static NiObject::DynamicCast: null for a null object, else the member DynamicCast.
NiObject* NiObject::DynamicCastStatic(const NiRTTI* pkRTTI, const NiObject* pkObject) {
    if (!pkObject) {
        return nullptr;
    }
    return pkObject->DynamicCastMember(pkRTTI);
}
