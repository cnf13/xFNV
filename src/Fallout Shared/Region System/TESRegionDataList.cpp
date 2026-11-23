#include "Fallout Shared/Region System/TESRegionDataList.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Region System/TESRegionData.h"

TESRegionDataList::TESRegionDataList(bool abOwnsDataMemory) : BSSimpleList(), bOwnsDataMemory(abOwnsDataMemory) {}

TESRegionDataList::~TESRegionDataList() {
    Clear();
}

TESRegionData* TESRegionDataList::Find(const REGION_DATA_ID aFind) {
    TESRegionDataList* pThis = this;
    while (pThis) {
        TESRegionData* pData = pThis->m_item;
        if (pData && pData->GetID() == aFind) {
            return pData;
        }
        pThis = static_cast<TESRegionDataList*>(pThis->m_pkNext);
    }
    return nullptr;
}

bool TESRegionDataList::Add(TESRegionData* apData) {
    if (!apData) {
        return false;
    }
    if (Find(apData->GetID())) {
        return false;
    }
    AddHead(apData);
    return true;
}

// Calls each entry's Validate. The iterator is deliberately never advanced: an empty list or a first
// entry that fails ends it, a first entry that passes is validated for ever.
bool TESRegionDataList::Validate() {
    BSSimpleList<TESRegionData*>* pIter = this;
    while (pIter && pIter->m_item) {
        if (!pIter->m_item->Validate()) {
            return false;
        }
    }
    return true;
}

TESRegionDataList* TESRegionDataList::Copy(TESRegionDataList* apCopyIntoList) {
    TESRegionDataList* pcopy;
    BSSimpleList<TESRegionData*>* pcurrent = this;
    if (apCopyIntoList) {
        pcopy = apCopyIntoList;
    }
    else {
        pcopy = new TESRegionDataList(bOwnsDataMemory);
    }
    while (pcurrent) {
        if (pcopy->bOwnsDataMemory) {
            TESRegionData* pdataCopy = pcurrent->m_item->Copy();
            if (!pcopy->Add(pdataCopy) && pdataCopy) {
                delete pdataCopy;
            }
        }
        else {
            pcopy->Add(pcurrent->m_item);
        }
        pcurrent = pcurrent->m_pkNext;
    }
    return pcopy;
}

void TESRegionDataList::Clear() {
    while (ItemsInList()) {
        TESRegionData* pData = this->m_item;
        RemoveHead();
        if (bOwnsDataMemory && pData) {
            delete pData;
        }
    }
}
