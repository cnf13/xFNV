#include "BSMain/BSTextureData.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSTextureData::ms_RTTI("BSTextureData", &NiExtraData::ms_RTTI);

NiAVObject* BSTextureData::GetValue(uint32_t aiIndex) const {
#if FNVD_64BIT
    // The exe reads the 4-byte element at pBuffer + 4 * aiIndex: the element, at the pointer's own width.
    return pObjects.pBuffer[static_cast<int32_t>(aiIndex)];
#else
    return reinterpret_cast<NiAVObject*>(static_cast<uintptr_t>(*reinterpret_cast<uint32_t*>(
        static_cast<uintptr_t>(reinterpret_cast<uintptr_t>(pObjects.pBuffer) + (4 * ((int32_t)aiIndex)))
    )));
#endif
}

BSTextureData::BSTextureData() : NiExtraData(BSTextureData::pTag) {}

BSTextureData::~BSTextureData() {}

NiObject* BSTextureData::CreateClone(NiCloningProcess& kCloning) {
    BSTextureData* v3;
    void* v5 = NiMemObject::operator new(sizeof(BSTextureData));
    if (v5) {
        v3 = ::new (v5) BSTextureData;
    }
    else {
        v3 = nullptr;
    }
    CopyMembers(v3, kCloning);
    return v3;
}

// The count is read with NiStreamLoadBinary<uint32_t> and is compared as signed.
void BSTextureData::LoadBinary(NiStream& kStream) {
    NiExtraData::LoadBinary(kStream);
    uint32_t uiNum;
    NiStreamLoadBinary(kStream, uiNum);
    for (int32_t i = 0; i < static_cast<int32_t>(uiNum); i++) {
        kStream.ReadLinkID();
    }
    pObjects.SetSize(uiNum, true);
}

void BSTextureData::LinkObject(NiStream& kStream) {
    NiExtraData::LinkObject(kStream);
    for (uint32_t i = 0; i < pObjects.iSize; i++) {
        pObjects[i] = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
    }
}

bool BSTextureData::RegisterStreamables(NiStream& kStream) {
    if (!NiExtraData::RegisterStreamables(kStream)) {
        return false;
    }
    for (uint32_t i = 0; i < pObjects.iSize; i++) {
        pObjects[i]->RegisterStreamables(kStream);
    }
    return true;
}

void BSTextureData::SaveBinary(NiStream& kStream) {
    NiExtraData::SaveBinary(kStream);
    uint32_t uiNum = pObjects.iSize;
    NiStreamSaveBinary(kStream, uiNum);
    for (uint32_t i = 0; i < pObjects.iSize; i++) {
        kStream.SaveLinkID(pObjects[i]);
    }
}

bool BSTextureData::IsEqual(NiObject* pObject) {
    if (!pObject) {
        return false;
    }
    BSTextureData* pOther = static_cast<BSTextureData*>(pObject);
    if (pObjects.iSize != pOther->pObjects.iSize) {
        return false;
    }
    for (uint32_t i = 0; i < pObjects.iSize; i++) {
        if (pObjects[i] != pOther->pObjects[i]) {
            return false;
        }
    }
    return true;
}

void BSTextureData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiExtraData::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(BSTextureData::ms_RTTI.GetName()));
    for (uint32_t i = 0; i < pObjects.iSize; i++) {
        pStrings->Add(NiGetViewerString("ObjectName", static_cast<const char*>(pObjects[i]->m_kName)));
    }
}

void BSTextureData::CopyMembers(BSTextureData* pDest, NiCloningProcess& kCloning) {
    NiExtraData::CopyMembers(pDest, kCloning);
    pObjects.CopyTo(&pDest->pObjects);
}

int32_t BSTextureData::AddValue(NiAVObject* aObject) {
    return pObjects.Add(aObject);
}

// Creates the "TXD" tag in the engine's heap.
void BSTextureData::InitSDM() {
    NiFixedString* pTag = static_cast<NiFixedString*>(MemoryManager_Singleton.Allocate(sizeof(NiFixedString)));
    if (pTag) {
        pTag->SetHandle(NiGlobalStringTable::AddString("TXD"));
    }
    BSTextureData::pTag = pTag;
}

NiObject* BSTextureData::CreateObject() {
    void* v2 = NiMemObject::operator new(sizeof(BSTextureData));
    if (v2) {
        return ::new (v2) BSTextureData;
    }
    else {
        return nullptr;
    }
}

void BSTextureData::KillSDM() {
    NiFixedString* Block = BSTextureData::pTag;
    if (BSTextureData::pTag) {
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(BSTextureData::pTag->GetHandle());
        MemoryManager_Singleton.Deallocate(Block);
    }
    BSTextureData::pTag = nullptr;
}
