#include "BSMain/BSBound.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSBound::ms_RTTI("BSBound", &NiExtraData::ms_RTTI);

BSBound::BSBound() : NiExtraData(*pTag), Center(NiPoint3_ZERO), Extents(NiPoint3_ZERO) {}

BSBound::~BSBound() {}

NiObject* BSBound::CreateClone(NiCloningProcess& arCloneData) {
    return this;
}

void BSBound::LoadBinary(NiStream& arStream) {
    NiExtraData::LoadBinary(arStream);
    Center.LoadBinary(arStream);
    Extents.LoadBinary(arStream);
}

void BSBound::LinkObject(NiStream& arStream) {
    NiExtraData::LinkObject(arStream);
}

bool BSBound::RegisterStreamables(NiStream& arStream) {
    // Returns the negated base result, as the game does.
    return !NiExtraData::RegisterStreamables(arStream);
}

void BSBound::SaveBinary(NiStream& arStream) {
    NiExtraData::SaveBinary(arStream);
    Center.SaveBinary(arStream);
    Extents.SaveBinary(arStream);
}

bool BSBound::IsEqual(NiObject* apObject) {
    bool bEqual = NiExtraData::IsEqual(apObject);
    if (bEqual) {
        BSBound* pOther = static_cast<BSBound*>(apObject);
        bEqual = Center == pOther->Center;
        bEqual &= Extents == pOther->Extents;
    }
    return bEqual;
}

void BSBound::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiExtraData::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(BSBound::ms_RTTI.GetName()));
    apStrings->Add(Center.GetViewerString("Center"));
    apStrings->Add(Extents.GetViewerString("Extents"));
}

void BSBound::CopyMembers(BSBound* pDest, NiCloningProcess& arCloneData) {
    NiExtraData::CopyMembers(pDest, arCloneData);
    pDest->Center = Center;
    pDest->Extents = Extents;
}

BSBound* BSBound::Find(NiObjectNET* apObject) {
    if (!apObject) {
        return nullptr;
    }
    // Loads the tag pointer once and passes the string it points at by reference.
    NiExtraData* pkExtraData = apObject->GetExtraData(*pTag);
    if (!pkExtraData) {
        return nullptr;
    }
    // Walks the object's class chain up to &ms_RTTI; found only if it reaches it.
    const NiRTTI* pkRTTI = pkExtraData->GetRTTI();
    while (pkRTTI && pkRTTI != &ms_RTTI) {
        pkRTTI = pkRTTI->m_pkBaseRTTI;
    }
    return pkRTTI ? static_cast<BSBound*>(pkExtraData) : nullptr;
}

void BSBound::InitSDM() {
    NiFixedString* pkTag = static_cast<NiFixedString*>(MemoryManager_Singleton.Allocate(sizeof(NiFixedString)));
    if (pkTag) {
        // SetHandle stores the handle AddString returned, with no second reference.
        pkTag->SetHandle(NiGlobalStringTable::AddString("BBX"));
    }
    pTag = pkTag;
}

void BSBound::KillSDM() {
    NiFixedString* Block = BSBound::pTag;
    if (BSBound::pTag) {
        NiGlobalStringTable::ReleaseHandle(BSBound::pTag->GetHandle());
        MemoryManager_Singleton.Deallocate(Block);
    }
    BSBound::pTag = nullptr;
}

// The name is cleared on the failed allocation's path too, as the game does.
NiObject* BSBound::CreateObject() {
    BSBound* pb_2;
    pb_2 = new BSBound;
    pb_2->SetName(EmptyFS);
    return pb_2;
}
