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
        TESRegionData* v4 = pThis->m_item;
        if (v4 && v4->GetID() == aFind) {
            return v4;
        }
        pThis = static_cast<TESRegionDataList*>(pThis->m_pkNext);
    }
    return 0;
}

bool TESRegionDataList::Add(TESRegionData* apData) {
    if (!apData) {
        return 0;
    }
    REGION_DATA_ID v2 = apData->GetID();
    if (Find(v2)) {
        return 0;
    }
    AddHead(apData);
    return 1;
}

// Calls each entry's Validate. The iterator is deliberately never advanced, as the game does: an empty list or a first
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
        TESRegionDataList* pcopy_2;
        TESRegionDataList* apMemory = static_cast<TESRegionDataList*>(ourMalloc(sizeof(TESRegionDataList)));
        if (apMemory) {
            pcopy_2 = ::new (apMemory) TESRegionDataList(bOwnsDataMemory);
        }
        else {
            pcopy_2 = 0;
        }
        pcopy = pcopy_2;
    }
    while (pcurrent) {
        if (pcopy->bOwnsDataMemory) {
            TESRegionData* m_item = pcurrent->m_item;
            TESRegionData* pdataCopy = m_item->Copy();
            if (!pcopy->Add(pdataCopy)) {
                if (pdataCopy) {
                    delete pdataCopy;
                }
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
        TESRegionData* m_item = this->m_item;
        RemoveHead();
        if (bOwnsDataMemory) {
            if (m_item) {
                delete m_item;
            }
        }
    }
}
