#include "BSMain/BSTextureData.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSTextureData::ms_RTTI("BSTextureData", &NiExtraData::ms_RTTI);

NiAVObject* BSTextureData::GetValue(uint32_t aiIndex) const {
    // The exe reads the 4-byte element at pBuffer + 4 * aiIndex: the element, at the pointer's own width.
    return pObjects.pBuffer[static_cast<int32_t>(aiIndex)];
}

BSTextureData::BSTextureData() : NiExtraData(BSTextureData::pTag) {}

BSTextureData::~BSTextureData() {}

NiObject* BSTextureData::CreateClone(NiCloningProcess& kCloning) {
    BSTextureData* v3;
    v3 = new BSTextureData;
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
    for (NiAVObject*& rpObject : pObjects) {
        rpObject = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
    }
}

bool BSTextureData::RegisterStreamables(NiStream& kStream) {
    if (!NiExtraData::RegisterStreamables(kStream)) {
        return false;
    }
    for (NiAVObject* pObject : pObjects) {
        pObject->RegisterStreamables(kStream);
    }
    return true;
}

void BSTextureData::SaveBinary(NiStream& kStream) {
    NiExtraData::SaveBinary(kStream);
    uint32_t uiNum = pObjects.iSize;
    NiStreamSaveBinary(kStream, uiNum);
    for (NiAVObject* pObject : pObjects) {
        kStream.SaveLinkID(pObject);
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
    for (NiAVObject* pObject : pObjects) {
        pStrings->Add(NiGetViewerString("ObjectName", static_cast<const char*>(pObject->m_kName)));
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
    NiFixedString* pTag = static_cast<NiFixedString*>(MemoryManager::Instance().Allocate(sizeof(NiFixedString)));
    if (pTag) {
        pTag->SetHandle(NiGlobalStringTable::AddString("TXD"));
    }
    BSTextureData::pTag = pTag;
}

NiObject* BSTextureData::CreateObject() {
    return new BSTextureData;
}

void BSTextureData::KillSDM() {
    NiFixedString* Block = BSTextureData::pTag;
    if (BSTextureData::pTag) {
        NiGlobalStringTable::ReleaseHandle(BSTextureData::pTag->GetHandle());
        MemoryManager::Instance().Deallocate(Block);
    }
    BSTextureData::pTag = nullptr;
}
