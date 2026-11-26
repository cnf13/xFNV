#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"

#include <stdint.h>

class NiPoint2;
class NiPoint3;
class TESRegion;
class TESRegionData;
class TESRegionPoint;
class TESWorldSpace;

enum REGION_DATA_ID : int32_t;

// A list of regions. The engine's class is polymorphic, so its vtable pointer comes before the list.
class TESRegionList : public BSSimpleList<TESRegion*> {
public:
    // The region data and the list of one region data kind. Nine of them are kept, indexed by REGION_DATA_ID.
    struct CachedData {
        TESRegionData* pData;
        TESRegionList* pList;
    };

    using kCachedData_t = TESRegionList::CachedData[9];

    TESRegionList(bool abOwnsRegionMemory);
    virtual ~TESRegionList();

    // The region data of kind aeFind for apPoint in apWorldSpace.
    TESRegionData* GetDerivedData(REGION_DATA_ID aeFind, TESRegionPoint* apPoint, TESWorldSpace* apWorldSpace);
    // The region data of kind aeFind at arPoint in apWorldSpace, through a TESRegionPoint copy of the point.
    TESRegionData* GetDerivedData(REGION_DATA_ID aeFind, const NiPoint2& arPoint, TESWorldSpace* apWorldSpace);
    // NiPoint3 overload of the NiPoint2 one; it reads only x and y. GetMapNameForLocation and GetGrassForLocation call
    // it with a NiPoint3.
    TESRegionData* GetDerivedData(REGION_DATA_ID aeFind, const NiPoint3& arPoint, TESWorldSpace* apWorldSpace);
    // Empties the list; a list owning its regions deletes each one.
    void Clear();
    // The region of the list whose form ID is auiFormID, or null.
    TESRegion* HasRegion(uint32_t auiFormID);
    // Adds apRegion to the list.
    bool Add(TESRegion* apRegion);
    // Whether apList holds the same regions in the same order; false when either list is null.
    bool Compare(TESRegionList* apList);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    bool bOwnsRegionMemory;

    static inline int32_t iOwnLists;
    static inline kCachedData_t kCachedData{};
};
