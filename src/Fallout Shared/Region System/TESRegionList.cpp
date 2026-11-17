#include "Fallout Shared/Region System/TESRegionList.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Region System/TESRegion.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/Region System/TESRegionDataList.h"
#include "Fallout Shared/Region System/TESRegionPoint.h"
#include "Fallout Shared/Region System/TESRegionPointList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// The first list owning its regions, and the last one going, empty the cache of derived data.
TESRegionList::TESRegionList(bool abOwnsRegionMemory) : BSSimpleList(), bOwnsRegionMemory(abOwnsRegionMemory) {
    if (bOwnsRegionMemory) {
        if (!iOwnLists) {
            for (uint32_t i = 0; i < 9; ++i) {
                TESRegionData* pdataDelete = kCachedData[i].pData;
                if (pdataDelete) {
                    kCachedData[i].pData = nullptr;
                    delete pdataDelete;
                }
                TESRegionList* plistDelete = kCachedData[i].pList;
                if (plistDelete) {
                    kCachedData[i].pList = nullptr;
                    delete plistDelete;
                }
            }
        }
        ++iOwnLists;
    }
}

TESRegionList::~TESRegionList() {
    Clear();
    if (bOwnsRegionMemory && !--iOwnLists) {
        for (uint32_t i = 0; i < 9; ++i) {
            TESRegionData* pdataDelete = kCachedData[i].pData;
            if (pdataDelete) {
                kCachedData[i].pData = nullptr;
                delete pdataDelete;
            }
            TESRegionList* plistDelete = kCachedData[i].pList;
            if (plistDelete) {
                kCachedData[i].pList = nullptr;
                delete plistDelete;
            }
        }
    }
}

TESRegionData*
TESRegionList::GetDerivedData(REGION_DATA_ID aeFind, TESRegionPoint* apPoint, TESWorldSpace* apWorldSpace) {
    TESRegionList* pRegionsWithData;
    BSSimpleList<TESRegion*>* pcurrent;
    char v12 = 0;
    uint16_t shighestOverride = 0;
    TESRegionData* phighestOverride = nullptr;
    TESRegionData* pderived = nullptr;
    if (this) {
        pcurrent = static_cast<BSSimpleList<TESRegion*>*>(this);
    }
    else {
        pcurrent = nullptr;
    }
    BSSimpleList<TESRegion*>* pcurrent_2 = pcurrent;
    TESRegionList* apMemory = static_cast<TESRegionList*>(ourMalloc(sizeof(TESRegionList)));
    if (apMemory) {
        pRegionsWithData = ::new (apMemory) TESRegionList(false);
    }
    else {
        pRegionsWithData = nullptr;
    }
    TESRegionList* pRegionsWithData_2 = pRegionsWithData;
    while (pcurrent_2 && pcurrent_2->m_item) {
        TESRegion* pregion = pcurrent_2->m_item;
        if (!pregion->GetDelete() && (!apWorldSpace || apWorldSpace == pregion->pWorldSpace)) {
            TESRegionData* pfound = pregion->pDataList->Find(aeFind);
            if ((pfound) && (!pfound->bIgnore)) {
                if (apPoint) {
                    for (TESRegionPointList*& rpItem : pregion->pPointLists->ItemsUntilNull()) {
                        if (rpItem->IsInsideRegion(apPoint)) {
                            goto LABEL_24;
                        }
                    }
                }
                else {
                LABEL_24:;
                    pRegionsWithData->Add(pregion);
                }
            }
        }
        pcurrent_2 = pcurrent_2->m_pkNext;
    }
    if (TESRegionList::kCachedData[aeFind].pData &&
        (TESRegionList::kCachedData[aeFind].pList &&
         TESRegionList::kCachedData[aeFind].pList->Compare(pRegionsWithData))) {
        pRegionsWithData->RemoveAll();
        if (pRegionsWithData) {
            delete pRegionsWithData;
        }
        return TESRegionList::kCachedData[aeFind].pData;
    }

    BSSimpleList<TESRegion*>* pcurrent_3;
    TESRegionData* pdataDelete = TESRegionList::kCachedData[aeFind].pData;
    if (pdataDelete) {
        TESRegionList::kCachedData[aeFind].pData = nullptr;
        delete pdataDelete;
    }
    TESRegionList* plistDelete = TESRegionList::kCachedData[aeFind].pList;
    if (plistDelete) {
        TESRegionList::kCachedData[aeFind].pList = nullptr;
        delete plistDelete;
    }
    if (pRegionsWithData) {
        pcurrent_3 = static_cast<BSSimpleList<TESRegion*>*>(pRegionsWithData);
    }
    else {
        pcurrent_3 = nullptr;
    }
    for (TESRegion*& rpItem : pcurrent_3->ItemsUntilNull()) {
        TESRegionData* pfound_2 = rpItem->pDataList->Find(aeFind);
        if (pfound_2) {
            if (pfound_2->bOverride) {
                v12 = 1;
                if (pderived) {
                    delete pderived;
                    pderived = nullptr;
                }
                if (pfound_2->cPriority > static_cast<int32_t>(shighestOverride)) {
                    phighestOverride = pfound_2;
                    shighestOverride = pfound_2->cPriority;
                }
            }
            else {
                if (!v12) {
                    if (pderived) {
                        pderived->BlendInto(pfound_2, pRegionsWithData->ItemsInList());
                    }
                    else {
                        pderived = pfound_2->Copy();
                    }
                }
            }
        }
    }
    if (v12) {
        pderived = phighestOverride->Copy();
    }
    if (pderived) {
        if (TESRegionList::kCachedData[aeFind].pData) {
            delete TESRegionList::kCachedData[aeFind].pData;
        }
        TESRegionList::kCachedData[aeFind].pData = pderived;
        if (TESRegionList::kCachedData[aeFind].pList) {
            TESRegionList::kCachedData[aeFind].pList->RemoveAll();
            if (TESRegionList::kCachedData[aeFind].pList) {
                delete TESRegionList::kCachedData[aeFind].pList;
            }
        }
        TESRegionList::kCachedData[aeFind].pList = pRegionsWithData;
        pRegionsWithData_2 = nullptr;
    }
    if (pRegionsWithData_2) {
        pRegionsWithData_2->RemoveAll();
        delete pRegionsWithData_2;
    }
    return pderived;
}

TESRegionData*
TESRegionList::GetDerivedData(REGION_DATA_ID aeFind, const NiPoint2& arPoint, TESWorldSpace* apWorldSpace) {
    alignas(TESRegionPoint) uint8_t aPointStorage[sizeof(TESRegionPoint)];
    TESRegionPoint& kPoint = *::new (aPointStorage) TESRegionPoint;
    kPoint.SetX(arPoint.x);
    kPoint.SetY(arPoint.y);
    return GetDerivedData(aeFind, &kPoint, apWorldSpace);
}

TESRegionData*
TESRegionList::GetDerivedData(REGION_DATA_ID aeFind, const NiPoint3& arPoint, TESWorldSpace* apWorldSpace) {
    TESRegionPoint kPoint;
    kPoint.SetX(arPoint.x);
    kPoint.SetY(arPoint.y);
    return GetDerivedData(aeFind, &kPoint, apWorldSpace);
}

void TESRegionList::Clear() {
    TESRegion* pRegion;
    pRegion = GetItem();
    while (pRegion) {
        RemoveHead();
        if (bOwnsRegionMemory) {
            delete pRegion;
        }
        pRegion = GetItem();
    }
}

TESRegion* TESRegionList::HasRegion(uint32_t auiFormID) {
    BSSimpleList<TESRegion*>* pList;
    if (this) {
        pList = static_cast<BSSimpleList<TESRegion*>*>(this);
    }
    else {
        pList = nullptr;
    }
    for (BSSimpleList<TESRegion*>* pIter = pList; pIter; pIter = pIter->m_pkNext) {
        if (pIter->m_item) {
            BSSimpleList<TESRegion*>* v2 = pIter;
            if (v2->m_item->GetFormID() == auiFormID) {
                return pIter->m_item;
            }
        }
    }
    return nullptr;
}

bool TESRegionList::Add(TESRegion* apRegion) {
    if (!apRegion) {
        return false;
    }
    if (!IsInList(apRegion)) {
        AddHead(apRegion);
    }
    return true;
}

bool TESRegionList::Compare(TESRegionList* apList) {
    BSSimpleList<TESRegion*>* pIter;
    BSSimpleList<TESRegion*>* pOther;
    if (this) {
        pIter = static_cast<BSSimpleList<TESRegion*>*>(this);
    }
    else {
        pIter = nullptr;
    }
    if (apList) {
        pOther = static_cast<BSSimpleList<TESRegion*>*>(apList);
    }
    else {
        pOther = nullptr;
    }
    if (!pIter || !pOther) {
        return false;
    }
    while (pIter) {
        if (pIter->m_item != pOther->m_item) {
            return false;
        }
        pIter = pIter->m_pkNext;
        pOther = pOther->m_pkNext;
        if (!pIter && !pOther) {
            return true;
        }
        if ((!pIter && pOther) || (pIter && !pOther)) {
            return false;
        }
    }
    return true;
}
