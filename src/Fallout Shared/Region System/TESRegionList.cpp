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
            for (CachedData& kData : kCachedData) {
                TESRegionData* pDataDelete = kData.pData;
                if (pDataDelete) {
                    kData.pData = nullptr;
                    delete pDataDelete;
                }
                TESRegionList* pListDelete = kData.pList;
                if (pListDelete) {
                    kData.pList = nullptr;
                    delete pListDelete;
                }
            }
        }
        ++iOwnLists;
    }
}

TESRegionList::~TESRegionList() {
    Clear();
    if (bOwnsRegionMemory && !--iOwnLists) {
        for (CachedData& kData : kCachedData) {
            TESRegionData* pDataDelete = kData.pData;
            if (pDataDelete) {
                kData.pData = nullptr;
                delete pDataDelete;
            }
            TESRegionList* pListDelete = kData.pList;
            if (pListDelete) {
                kData.pList = nullptr;
                delete pListDelete;
            }
        }
    }
}

TESRegionData*
TESRegionList::GetDerivedData(REGION_DATA_ID aeFind, TESRegionPoint* apPoint, TESWorldSpace* apWorldSpace) {
    char cFoundOverride = 0;
    uint16_t sHighestOverride = 0;
    TESRegionData* pHighestOverride = nullptr;
    TESRegionData* pDerived = nullptr;
    BSSimpleList<TESRegion*>* pCurrent = nullptr;
    if (this) {
        pCurrent = static_cast<BSSimpleList<TESRegion*>*>(this);
    }
    TESRegionList* pRegionsWithData = new TESRegionList(false);
    while (pCurrent && pCurrent->m_item) {
        TESRegion* pRegion = pCurrent->m_item;
        if (!pRegion->GetDelete() && (!apWorldSpace || apWorldSpace == pRegion->pWorldSpace)) {
            TESRegionData* pFound = pRegion->pDataList->Find(aeFind);
            if (pFound && !pFound->bIgnore) {
                if (apPoint) {
                    for (TESRegionPointList*& rpItem : pRegion->pPointLists->ItemsUntilNull()) {
                        if (rpItem->IsInsideRegion(apPoint)) {
                            pRegionsWithData->Add(pRegion);
                            break;
                        }
                    }
                }
                else {
                    pRegionsWithData->Add(pRegion);
                }
            }
        }
        pCurrent = pCurrent->m_pkNext;
    }
    if (TESRegionList::kCachedData[aeFind].pData &&
        TESRegionList::kCachedData[aeFind].pList &&
        TESRegionList::kCachedData[aeFind].pList->Compare(pRegionsWithData)) {
        pRegionsWithData->RemoveAll();
        if (pRegionsWithData) {
            delete pRegionsWithData;
        }
        return TESRegionList::kCachedData[aeFind].pData;
    }

    TESRegionData* pDataDelete = TESRegionList::kCachedData[aeFind].pData;
    if (pDataDelete) {
        TESRegionList::kCachedData[aeFind].pData = nullptr;
        delete pDataDelete;
    }
    TESRegionList* pListDelete = TESRegionList::kCachedData[aeFind].pList;
    if (pListDelete) {
        TESRegionList::kCachedData[aeFind].pList = nullptr;
        delete pListDelete;
    }
    pCurrent = static_cast<BSSimpleList<TESRegion*>*>(pRegionsWithData);
    for (TESRegion*& rpItem : pCurrent->ItemsUntilNull()) {
        TESRegionData* pFound = rpItem->pDataList->Find(aeFind);
        if (pFound) {
            if (pFound->bOverride) {
                cFoundOverride = 1;
                if (pDerived) {
                    delete pDerived;
                    pDerived = nullptr;
                }
                if (pFound->cPriority > static_cast<int32_t>(sHighestOverride)) {
                    pHighestOverride = pFound;
                    sHighestOverride = pFound->cPriority;
                }
            }
            else if (!cFoundOverride) {
                if (pDerived) {
                    pDerived->BlendInto(pFound, pRegionsWithData->ItemsInList());
                }
                else {
                    pDerived = pFound->Copy();
                }
            }
        }
    }
    if (cFoundOverride) {
        pDerived = pHighestOverride->Copy();
    }
    if (pDerived) {
        if (TESRegionList::kCachedData[aeFind].pData) {
            delete TESRegionList::kCachedData[aeFind].pData;
        }
        TESRegionList::kCachedData[aeFind].pData = pDerived;
        if (TESRegionList::kCachedData[aeFind].pList) {
            TESRegionList::kCachedData[aeFind].pList->RemoveAll();
            if (TESRegionList::kCachedData[aeFind].pList) {
                delete TESRegionList::kCachedData[aeFind].pList;
            }
        }
        TESRegionList::kCachedData[aeFind].pList = pRegionsWithData;
        pRegionsWithData = nullptr;
    }
    if (pRegionsWithData) {
        pRegionsWithData->RemoveAll();
        delete pRegionsWithData;
    }
    return pDerived;
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
    TESRegion* pRegion = GetItem();
    while (pRegion) {
        RemoveHead();
        if (bOwnsRegionMemory) {
            delete pRegion;
        }
        pRegion = GetItem();
    }
}

TESRegion* TESRegionList::HasRegion(uint32_t auiFormID) {
    BSSimpleList<TESRegion*>* pIter = nullptr;
    if (this) {
        pIter = static_cast<BSSimpleList<TESRegion*>*>(this);
    }
    for (; pIter; pIter = pIter->m_pkNext) {
        if (pIter->m_item) {
            if (pIter->m_item->GetFormID() == auiFormID) {
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
    BSSimpleList<TESRegion*>* pIter = nullptr;
    if (this) {
        pIter = static_cast<BSSimpleList<TESRegion*>*>(this);
    }
    BSSimpleList<TESRegion*>* pOther = static_cast<BSSimpleList<TESRegion*>*>(apList);
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
