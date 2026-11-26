#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

#include <stdint.h>

class NiAVObject;
class NiNode;

// A ray pick over a scene graph: the settings of the pick and the records of what it hit.
class NiPick : public NiMemObject {
public:
    enum PickType {
        FIND_ALL = 0,
        FIND_FIRST = 1,
    };

    enum SortType {
        SORT = 0,
        NO_SORT = 1,
    };

    enum IntersectType {
        BOUND_INTERSECT = 0,
        TRIANGLE_INTERSECT = 1,
    };

    enum CoordinateType {
        MODEL_COORDINATES = 0,
        WORLD_COORDINATES = 1,
    };

    // One object the ray hit, with the point, distance and (optionally) texture coordinate, normal and color there.
    class Record : public NiMemObject {
    public:
        // m_usTriangleIndex.
        uint16_t GetTriangleIndex() const;
        // m_fDistance.
        float GetDistance() const {
            return m_fDistance;
        }
        NiAVObject* GetAVObject();
        void Initialize(NiAVObject* pObject);

        NiPointer<NiAVObject> m_spObject;
        NiPointer<NiNode> m_spProxyParent;
        NiPoint3 m_intersect;
        float m_fDistance;
        uint16_t m_usTriangleIndex;
        uint16_t m_usVertexIndex[3];
        NiPoint2 m_texture;
        NiPoint3 m_normal;
        NiColorA m_color;
    };

    // The records, of which the first m_uiValidResults are this pick's; the rest are kept for reuse.
    class Results : public NiTPrimitiveArray<Record*> {
    public:
        Results(uint32_t uiMaxSize, uint32_t uiGrowBy);
        ~Results() override;

        // bDeallocateRecords is int32_t rather than bool because the caller's whole value is passed on; a bool would be
        // zero-extended.
        void ClearArray(int32_t bDeallocateRecords);
        void SortResults();
        void FindClosest();
        Record* AddNew();

        Record* GetAt(uint32_t uiIndex) const;
        // m_uiValidResults.
        uint32_t GetSize() const;

        // NiTArray's begin/end run to m_usSize, past this pick's results into the kept records, so a range-for over
        // the results would disagree with GetSize; loop on GetSize instead.
        Record** begin() const = delete;
        Record** end() const = delete;
        NiPick::Record* AddNew(NiAVObject* pObject);

        static void operator delete(void* apMemory);

        uint32_t m_uiValidResults;
    };

    NiPick(uint32_t uiResultsInitialSize = 0, uint32_t uiResultsGrowBy = 8);
    ~NiPick();

    void SetTarget(NiAVObject* pObject);
    bool PickObjects(const NiPoint3& kOrigin, const NiPoint3& kDir, bool bAppend = false);
    // bDeallocateRecords is an int32_t rather than bool so the caller's whole value is passed on; a bool would be
    // zero-extended.
    void ClearResultsArray(int32_t bDeallocateRecords);
    void SetFrontOnly(bool bFrontOnly);
    void SetObserveAppCullFlag(bool bObserveAppCullFlag);
    void SetPickType(PickType ePickType);
    void SetIntersectType(IntersectType eIntersectType);
    Results& GetResults();
    void SetReturnNormal(bool bReturnNormal);
    void SetIsReturnTexture(bool bReturnTexture);

    static void operator delete(void* apMemory);

    PickType m_ePickType;
    SortType m_eSortType;
    IntersectType m_eIntersectType;
    CoordinateType m_eCoordinateType;
    bool m_bFrontOnly;
    bool m_bObserveAppCullFlag;
    NiPointer<NiAVObject> m_spRoot;
    Results m_pickResults;
    Record* m_pLastAddedRecord;
    bool m_bReturnTexture;
    bool m_bReturnNormal;
    bool m_bReturnSmoothNormal;
    bool m_bReturnColor;
};

inline NiAVObject* NiPick::Record::GetAVObject() {
    return m_spObject;
}

inline NiPick::Record* NiPick::Results::GetAt(uint32_t uiIndex) const {
    return NiTPrimitiveArray<Record*>::GetAt(uiIndex);
}

inline void NiPick::SetTarget(NiAVObject* pObject) {
    m_spRoot = pObject;
}
