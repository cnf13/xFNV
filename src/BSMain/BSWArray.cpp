#include "BSMain/BSWArray.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiIntegersExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI BSWArray::ms_RTTI("BSWArray", &NiIntegersExtraData::ms_RTTI);

// The base starts as an empty NiIntegersExtraData (no array, size 0).
BSWArray::BSWArray() : NiIntegersExtraData(0, nullptr) {
    Init();
}

BSWArray::BSWArray(uint32_t aiSize, int32_t* apArray) : NiIntegersExtraData(aiSize, apArray) {
    Init();
}

// The array is freed by NiIntegersExtraData's destructor.
BSWArray::~BSWArray() {}

NiObject* BSWArray::CreateClone(NiCloningProcess& arCloneData) {
    return this;
}

void BSWArray::LoadBinary(NiStream& arStream) {
    NiIntegersExtraData::LoadBinary(arStream);
}

void BSWArray::LinkObject(NiStream& arStream) {
    NiIntegersExtraData::LinkObject(arStream);
}

bool BSWArray::RegisterStreamables(NiStream& arStream) {
    // Returns the negated base result, as the game does.
    return !NiIntegersExtraData::RegisterStreamables(arStream);
}

void BSWArray::SaveBinary(NiStream& arStream) {
    NiIntegersExtraData::SaveBinary(arStream);
}

bool BSWArray::IsEqual(NiObject* apObject) {
    return NiIntegersExtraData::IsEqual(apObject);
}

void BSWArray::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiIntegersExtraData::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(BSWArray::ms_RTTI.GetName()));
    apStrings->Add(NiGetViewerString("m_uiSize", m_uiSize));
}

void BSWArray::Init() {
    SetName(*BSWArray::pTag);
}

void BSWArray::CopyMembers(BSWArray* pDest, NiCloningProcess& arCloneData) {
    NiIntegersExtraData::CopyMembers(pDest, arCloneData);
}

// Creates the "BSW" tag string stored in pTag.
void BSWArray::InitSDM() {
    NiFixedString* pString = static_cast<NiFixedString*>(MemoryManager_Singleton.Allocate(sizeof(NiFixedString)));
    if (pString) {
        pString->SetHandle(NiGlobalStringTable::AddString("BSW"));
    }
    BSWArray::pTag = pString;
}

NiObject* BSWArray::CreateObject() {
    BSWArray* v1;
    void* apMemory = NiMemObject::operator new(sizeof(BSWArray));
    if (apMemory) {
        v1 = ::new (apMemory) BSWArray;
    }
    else {
        v1 = nullptr;
    }
    v1->SetName(EmptyFS);
    return v1;
}

BSWArray* BSWArray::Find(NiObjectNET* apObject) {
    BSWArray* v5 = 0;
    if (apObject) {
        NiExtraData* ExtraData = apObject->GetExtraData(*BSWArray::pTag);
        if (ExtraData) {
            char v2;
            for (const NiRTTI* i = ExtraData->GetRTTI(); i; i = i->GetBaseRTTI()) {
                if (i == &BSWArray::ms_RTTI) {
                    v2 = 1;
                    return v2 != 0 ? static_cast<BSWArray*>(ExtraData) : 0;
                }
            }
            v2 = 0;
            return v2 != 0 ? static_cast<BSWArray*>(ExtraData) : 0;
        }
        else {
            return 0;
        }
    }
    return v5;
}

void BSWArray::KillSDM() {
    NiFixedString* Block = BSWArray::pTag;
    if (BSWArray::pTag) {
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(BSWArray::pTag->GetHandle());
        MemoryManager_Singleton.Deallocate(Block);
    }
    BSWArray::pTag = 0;
}
