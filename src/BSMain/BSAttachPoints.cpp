#include "BSMain/BSAttachPoints.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI BSAttachPoints::ms_RTTI("BSAttachPoints", &NiExtraData::ms_RTTI);

BSAttachPointPair::BSAttachPointPair(const NiFixedString& arParent, const NiFixedString& arChild) {
    SetParent(arParent);
    SetChild(arChild);
}

// The members' destructors release the child name, then the parent name.
BSAttachPointPair::~BSAttachPointPair() {}

void BSAttachPointPair::SetParent(const NiFixedString& arParent) {
    (&Parent)->Assign(arParent);
}

void BSAttachPointPair::SetChild(const NiFixedString& arChild) {
    (&Child)->Assign(arChild);
}

BSAttachPoints::BSAttachPoints() : NiExtraData(BSAttachPoints::pTag), pAttachPointsA(0, 1) {}

BSAttachPoints::~BSAttachPoints() {
    uint32_t uiSize = pAttachPointsA.m_usESize;
    for (uint32_t i = 0; i < uiSize; ++i) {
        BSAttachPointPair* pPair = pAttachPointsA.m_pBase[i];
        if (pPair) {
            pPair->~BSAttachPointPair();
            MemoryManager_Singleton.Deallocate(pPair);
        }
    }
    pAttachPointsA.RemoveAll();
}

const NiRTTI* BSAttachPoints::GetRTTI() const {
    return &BSAttachPoints::ms_RTTI;
}

NiObject* BSAttachPoints::CreateClone(NiCloningProcess& arCloneData) {
    return this;
}

void BSAttachPoints::LoadBinary(NiStream& arStream) {
    uint32_t uiCount = 0;
    uint32_t i;
    NiExtraData::LoadBinary(arStream);
    NiStreamLoadBinary<uint32_t>(arStream, uiCount);
    pAttachPointsA.SetSize(uiCount);
    for (i = 0; i < uiCount; ++i) {
        NiFixedString kParent;
        NiFixedString kChild;
        arStream.LoadFixedString(kParent);
        arStream.LoadFixedString(kChild);
        AddPair(kParent, kChild);
    }
}

void BSAttachPoints::LinkObject(NiStream& arStream) {
    NiExtraData::LinkObject(arStream);
}

bool BSAttachPoints::RegisterStreamables(NiStream& arStream) {
    bool v7 = NiExtraData::RegisterStreamables(arStream);
    if (v7) {
        uint32_t m_usESize = pAttachPointsA.m_usESize;
        for (uint32_t i = 0; i < m_usESize; ++i) {
            BSAttachPointPair* v4 = pAttachPointsA.m_pBase[i];
            arStream.RegisterFixedString(v4->Parent);
            arStream.RegisterFixedString(v4->Child);
        }
    }
    return v7;
}

void BSAttachPoints::SaveBinary(NiStream& arStream) {
    NiExtraData::SaveBinary(arStream);
    uint32_t arValue = pAttachPointsA.m_usESize;
    NiStreamSaveBinary<uint32_t>(arStream, arValue);
    for (uint32_t i = 0; i < arValue; ++i) {
        BSAttachPointPair* kString = pAttachPointsA.m_pBase[i];
        arStream.SaveFixedString(kString->Parent);
        arStream.SaveFixedString(kString->Child);
    }
}

bool BSAttachPoints::IsEqual(NiObject* apObject) {
    return (bool)((uint8_t)NiExtraData::IsEqual(apObject));
}

void BSAttachPoints::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiExtraData::GetViewerStrings(apStrings);
    char* element = NiGetViewerString(BSAttachPoints::ms_RTTI.GetName());
    apStrings->Add(element);
    uint32_t m_usESize = pAttachPointsA.m_usESize;
    for (uint32_t I = 0; I < m_usESize; ++I) {
        char apDest[268];
        BSAttachPointPair* v7 = pAttachPointsA.m_pBase[I];
        const char* m_kHandle = v7->Parent.GetHandle();
        const char* v5 = v7->Child.GetHandle();
        BSsprintf(apDest, 260, "%d: %s - %s", I, m_kHandle, v5);
        char* v3 = apDest;
        apStrings->Add(v3);
    }
}

void BSAttachPoints::CopyMembers(BSAttachPoints* pDest, NiCloningProcess& arCloneData) {
    uint32_t i;
    uint32_t m_usESize;
    NiExtraData::CopyMembers(pDest, arCloneData);
    m_usESize = (uint32_t)pAttachPointsA.m_usESize;
    for (i = 0; i < m_usESize; ++i) {
        pDest->AddPair(pAttachPointsA.m_pBase[i]->Parent, pAttachPointsA.m_pBase[i]->Child);
    }
}

void BSAttachPoints::AddPair(const NiFixedString& arParent, const NiFixedString& arChild) {
    BSAttachPointPair* ppair;
    BSAttachPointPair* appkElement;
    BSAttachPointPair* apMemory =
        static_cast<BSAttachPointPair*>(MemoryManager_Singleton.Allocate(sizeof(BSAttachPointPair)));
    if (apMemory) {
        ppair = ::new (apMemory) BSAttachPointPair(arParent, arChild);
    }
    else {
        ppair = 0;
    }
    appkElement = ppair;
    pAttachPointsA.Add(appkElement);
}

void BSAttachPoints::InitSDM() {
    NiFixedString* pkTag = static_cast<NiFixedString*>(MemoryManager_Singleton.Allocate(sizeof(NiFixedString)));
    if (pkTag) {
        // SetHandle stores the handle AddString returned, with no second reference.
        pkTag->SetHandle(NiGlobalStringTable::AddString("ATP"));
    }
    pTag = pkTag;
}

NiObject* BSAttachPoints::CreateObject() {
    BSAttachPoints* v1;
    void* v2 = NiMemObject::operator new(sizeof(BSAttachPoints));
    if (v2) {
        v1 = ::new (v2) BSAttachPoints;
    }
    else {
        v1 = nullptr;
    }
    v1->SetName(EmptyFS);
    return v1;
}

void BSAttachPoints::KillSDM() {
    NiFixedString* Block = BSAttachPoints::pTag;
    if (BSAttachPoints::pTag) {
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(BSAttachPoints::pTag->GetHandle());
        MemoryManager_Singleton.Deallocate(Block);
    }
    BSAttachPoints::pTag = 0;
}
