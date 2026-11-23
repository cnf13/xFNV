#include "Fallout Shared/Havok/TriggerEntry.h"

#include "BSCore/BSList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"

// The events are the list's raw allocations: freed here, the list's own nodes by its destructor.
TriggerEntry::RefList::~RefList() {
    iArraySize = 0;
    if (pRefArray) {
        ourFree(pRefArray);
    }
    pRefArray = nullptr;
    while (!EventList.IsEmpty()) {
        ourFree(EventList.RemoveHead());
    }
}

bool TriggerEntry::RefList::IsInList(TESObjectREFR* apRef) {
    uint32_t uiFormID = apRef->TESForm::GetFormID();
    for (int32_t i = 0; i < iArraySize; ++i) {
        if (uiFormID == pRefArray[i].RefID) {
            return true;
        }
    }
    return false;
}

void TriggerEntry::RefList::Reset() {
    iArraySize = 0;
    while (!EventList.IsEmpty()) {
        ourFree(EventList.RemoveHead());
    }
}

void TriggerEntry::RefList::ForceLeaveTrigger(TESObjectREFR* apTriggerRef) {
    if (iArraySize) {
        // The source's new[]: the count converted to the unsigned size, -1 when the byte size overflows.
        uint32_t* pIDs = static_cast<uint32_t*>(ourMalloc(
            static_cast<uint32_t>(iArraySize) <= SIZE_MAX / sizeof(uint32_t)
                ? sizeof(uint32_t) * static_cast<uint32_t>(iArraySize)
                : SIZE_MAX
        ));
        uint32_t uiSize = iArraySize;
        for (int32_t i = 0; i < iArraySize; ++i) {
            pIDs[i] = pRefArray[i].RefID;
        }
        ExtraDataList* pExtraList = &apTriggerRef->m_Extra;
        for (uint32_t ui = 0; ui < uiSize; ++ui) {
            TESObjectREFR* pRef = static_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(pIDs[ui]));
            if (pRef && pRef->IsReference()) {
                Script::SetActionFlag(pRef, pExtraList, SCRIPT_ACTION_ONTRIGGERLEAVE);
                apTriggerRef->SetActionRef(pRef);
                apTriggerRef->RunScript();
            }
        }
        ourFree(pIDs);
    }
}

bool TriggerEntry::IsInTrigger(TESObjectREFR* apRef) {
    return CurrentRefs.IsInList(apRef);
}

TriggerEntry* TriggerEntry::GetTriggerEntry(const hkpCollidable* apCol) {
    return static_cast<TriggerEntry*>(GetEntry(apCol));
}
