#include "BSMain/BSXFlags.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiIntegerExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSXFlags::ms_RTTI("BSXFlags", &NiIntegerExtraData::ms_RTTI);

BSXFlags::BSXFlags() {
    Init();
}

BSXFlags::BSXFlags(const int32_t a1) : NiIntegerExtraData(a1) {
    Init();
}

BSXFlags::~BSXFlags() {}

const NiRTTI* BSXFlags::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* BSXFlags::CreateClone(NiCloningProcess& arCloneData) {
    return this;
}

void BSXFlags::LoadBinary(NiStream& arStream) {
    NiIntegerExtraData::LoadBinary(arStream);
}

void BSXFlags::LinkObject(NiStream& arStream) {
    NiIntegerExtraData::LinkObject(arStream);
}

bool BSXFlags::RegisterStreamables(NiStream& arStream) {
    // Returns the negated base result, as the game does.
    return !NiIntegerExtraData::RegisterStreamables(arStream);
}

void BSXFlags::SaveBinary(NiStream& arStream) {
    NiIntegerExtraData::SaveBinary(arStream);
}

bool BSXFlags::IsEqual(NiObject* apObject) {
    return NiIntegerExtraData::IsEqual(apObject);
}

void BSXFlags::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiIntegerExtraData::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(BSXFlags::ms_RTTI.GetName()));
    apStrings->Add(NiGetViewerString("bAnimated", (m_iValue & ANIMATED) != 0));
    apStrings->Add(NiGetViewerString("bHavok", (m_iValue & HAVOK) != 0));
    apStrings->Add(NiGetViewerString("bRagDoll", (m_iValue & RAGDOLL) != 0));
    apStrings->Add(NiGetViewerString("bComplex", (m_iValue & COMPLEX) != 0));
    apStrings->Add(NiGetViewerString("bAddon", (m_iValue & ADDON) != 0));
    apStrings->Add(NiGetViewerString("bEditorMarker", (m_iValue & EDITORMARKER) != 0));
    apStrings->Add(NiGetViewerString("bDynamic", (m_iValue & DYNAMIC) != 0));
    apStrings->Add(NiGetViewerString("bArticulated", (m_iValue & ARTICULATED) != 0));
    apStrings->Add(NiGetViewerString("bIKTarget", (m_iValue & IKTARGET) != 0));
}

uint32_t BSXFlags::GetFlags(uint32_t aiFlags) const {
    return aiFlags & static_cast<uint32_t>(m_iValue);
}

bool BSXFlags::QAnimated() const {
    return GetFlags(ANIMATED) != 0;
}

bool BSXFlags::QEditorMarker() const {
    return GetFlags(EDITORMARKER) != 0;
}

bool BSXFlags::QAddon() const {
    return GetFlags(BSXFlags::ADDON) != 0;
}

void BSXFlags::SetFlags(uint32_t aiFlags, bool abTF) {
    if (abTF) {
        m_iValue |= aiFlags;
    }
    else {
        ClearFlags(aiFlags);
    }
}

void BSXFlags::ClearFlags(uint32_t aiFlags) {
    m_iValue &= ~static_cast<int32_t>(aiFlags);
}

void BSXFlags::Init() {
    SetName(*pTag);
}

bool BSXFlags::QIKTarget() {
    return (GetFlags(BSXFlags::IKTARGET) != 0);
}

void BSXFlags::CopyMembers(BSXFlags* pDest, NiCloningProcess& arCloneData) {
    NiIntegerExtraData::CopyMembers(pDest, arCloneData);
}

const NiFixedString& BSXFlags::Tag() {
    return *pTag;
}

// Creates the "BSX" tag string stored in pTag.
void BSXFlags::InitSDM() {
    NiFixedString* pString = static_cast<NiFixedString*>(MemoryManager_Singleton.Allocate(sizeof(NiFixedString)));
    if (pString) {
        pString->SetHandle(NiGlobalStringTable::AddString("BSX"));
    }
    BSXFlags::pTag = pString;
}

// The object's BSXFlags extra data, null when it has none.
BSXFlags* BSXFlags::Find(NiObjectNET* apObject) {
    if (apObject) {
        NiExtraData* pExtraData = apObject->GetExtraData(*pTag);
        if (pExtraData && pExtraData->GetRTTI() == &ms_RTTI) {
            return static_cast<BSXFlags*>(pExtraData);
        }
    }
    return nullptr;
}

int32_t BSXFlags::FindFlag(NiObjectNET* apObject, FLAGS aiFlags) {
    BSXFlags* pFlags = Find(apObject);
    if (!pFlags) {
        return 0;
    }
    return pFlags->GetFlags(aiFlags);
}

NiObject* BSXFlags::CreateObject() {
    BSXFlags* v1;
    void* apMemory = NiMemObject::operator new(sizeof(BSXFlags));
    if (apMemory) {
        v1 = ::new (apMemory) BSXFlags;
    }
    else {
        v1 = nullptr;
    }
    v1->SetName(EmptyFS);
    return v1;
}

void BSXFlags::KillSDM() {
    NiFixedString* Block = BSXFlags::pTag;
    if (BSXFlags::pTag) {
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(BSXFlags::pTag->GetHandle());
        MemoryManager_Singleton.Deallocate(Block);
    }
    BSXFlags::pTag = 0;
}
