#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionTraversals.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

uint16_t NiPick::Record::GetTriangleIndex() const {
    return m_usTriangleIndex;
}

void NiPick::Record::Initialize(NiAVObject* pObject) {
    m_spObject = pObject;
    m_spProxyParent = nullptr;
}

NiPick::Results::Results(uint32_t uiMaxSize, uint32_t uiGrowBy)
  : NiTPrimitiveArray<NiPick::Record*>(uiMaxSize, uiGrowBy), m_uiValidResults(0) {}

// Deletes every record, kept or valid; the base frees the buffer.
NiPick::Results::~Results() {
    for (Record* pkRecord : *this) {
        delete pkRecord;
    }
}

void NiPick::Results::ClearArray(int32_t bDeallocateRecords) {
    int32_t v2 = 0;
    m_uiValidResults = 0;
    if (static_cast<uint8_t>(bDeallocateRecords)) {
        int32_t v11;
        if (m_usSize) {
            do {
                NiPick::Record* v4 = m_pBase[v2];
                v4->m_spObject = nullptr;
                v4->m_spProxyParent = nullptr;
                NiPick::Record* v7 = m_pBase[v2];
                if (v7) {
                    NiNode* v8 = v7->m_spProxyParent.m_pObject;
                    if (v8) {
                        // EXE-INLINED none: NiDecRefCount
                        NiDecRefCount(v8);
                    }
                    NiAVObject* v9 = v7->m_spObject.m_pObject;
                    if (v7->m_spObject.m_pObject) {
                        NiDecRefCount(v9);
                    }
                    MemoryManager_Singleton.Deallocate(v7);
                }
                ++v2;
            }
            while (v2 < m_usSize);
        }
        for (int32_t i = 0; static_cast<uint16_t>(i) < m_usSize; m_pBase[v11] = 0) {
            v11 = static_cast<uint16_t>(i++);
        }
        m_usSize = 0;
        m_usESize = 0;
    }
}

void NiPick::Results::SortResults() {
    uint32_t v3;
    uint32_t v1 = 0;
    if (!m_uiValidResults) {
        return;
    }
    do {
        NiPick::Record* v2 = this->m_pBase[v1];
        v3 = v1 + 1;
        if (v1 + 1 >= m_uiValidResults) {
            goto LABEL_15;
        }
        do {
            NiPick::Record** m_pBase = this->m_pBase;
            NiPick::Record* v5 = m_pBase[v3];
            if (!(v2->m_fDistance > static_cast<double>(v5->m_fDistance))) {
                goto LABEL_13;
            }
            if (v3 < m_usSize) {
                if (v5) {
                    goto LABEL_8;
                }
            }
            else {
                m_usSize = v3 + 1;
            }
            ++m_usESize;
        LABEL_8:;
            m_pBase[v3] = v2;
            if (v1 >= m_usSize) {
                m_usSize = v1 + 1;
            LABEL_11:;
                ++m_usESize;
                goto LABEL_12;
            }
            if (!this->m_pBase[v1]) {
                goto LABEL_11;
            }
        LABEL_12:;
            this->m_pBase[v1] = v5;
            v2 = v5;
        LABEL_13:;
            ++v3;
        }
        while (v3 < m_uiValidResults);
        v3 = v1 + 1;
    LABEL_15:;
        v1 = v3;
    }
    while (v3 < m_uiValidResults);
}

void NiPick::Results::FindClosest() {
    uint32_t m_uiValidResults = this->m_uiValidResults;
    if (m_uiValidResults) {
        NiPick::Record** m_pBase = this->m_pBase;
        NiPick::Record* v4 = *m_pBase;
        uint32_t uiClosest = 0;
        uint32_t i = 1;
        NiPick::Record* pRecordClosest = v4;
        NiPick::Record* v7 = v4;
        if (m_uiValidResults > 1) {
            NiPick::Record** v8 = m_pBase + 1;
            do {
                if (v7->m_fDistance > static_cast<double>((*v8)->m_fDistance)) {
                    v7 = *v8;
                    uiClosest = i;
                }
                ++i;
                ++v8;
            }
            while (i < this->m_uiValidResults);
            v4 = pRecordClosest;
        }
        pRecordClosest = v4;
        SetAt(uiClosest, pRecordClosest);
        if (m_usSize) {
            if (v7) {
                NiPick::Record** v10 = this->m_pBase;
                if (!*v10) {
                    ++m_usESize;
                    *v10 = v7;
                    this->m_uiValidResults = 1;
                    return;
                }
            }
            else {
                if (*this->m_pBase) {
                    --m_usESize;
                }
            }
        }
        else {
            m_usSize = 1;
            if (v7) {
                NiPick::Record** v9 = this->m_pBase;
                ++m_usESize;
                *v9 = v7;
                this->m_uiValidResults = 1;
                return;
            }
        }
        *this->m_pBase = v7;
        this->m_uiValidResults = 1;
    }
}

uint32_t NiPick::Results::GetSize() const {
    return m_uiValidResults;
}

void NiPick::Results::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

NiPick::NiPick(uint32_t uiResultsInitialSize, uint32_t uiResultsGrowBy)
  : m_ePickType(FIND_ALL), m_eSortType(SORT), m_eIntersectType(TRIANGLE_INTERSECT),
    m_eCoordinateType(WORLD_COORDINATES), m_bFrontOnly(true), m_bObserveAppCullFlag(false), m_spRoot(nullptr),
    m_pickResults(uiResultsInitialSize, uiResultsGrowBy), m_pLastAddedRecord(nullptr), m_bReturnTexture(false),
    m_bReturnNormal(false), m_bReturnSmoothNormal(false), m_bReturnColor(false) {}

// The results and the root are released by their own destructors.
NiPick::~NiPick() {}

// Collects the hits under the root, then sorts them (find all) or keeps the closest (find first) when they need it.
bool NiPick::PickObjects(const NiPoint3& kOrigin, const NiPoint3& kDir, bool bAppend) {
    if (!bAppend) {
        m_pickResults.m_uiValidResults = 0;
    }
    if (!NiCollisionTraversals::FindIntersections(kOrigin, kDir, *this, m_spRoot.m_pObject)) {
        return false;
    }
    uint32_t uiValidResults = m_pickResults.m_uiValidResults;
    if (!uiValidResults) {
        return false;
    }
    if (m_eSortType != NO_SORT && m_eIntersectType != BOUND_INTERSECT && uiValidResults > 1) {
        if (m_ePickType == FIND_ALL) {
            m_pickResults.SortResults();
        }
        else {
            m_pickResults.FindClosest();
        }
    }
    return true;
}

// Passes all four bytes of bDeallocateRecords on to ClearArray.
void NiPick::ClearResultsArray(int32_t bDeallocateRecords) {
    m_pickResults.ClearArray(bDeallocateRecords);
}

void NiPick::SetFrontOnly(bool bFrontOnly) {
    m_bFrontOnly = (bool)((uint8_t)bFrontOnly);
}

void NiPick::SetObserveAppCullFlag(bool bObserveAppCullFlag) {
    m_bObserveAppCullFlag = (bool)((uint8_t)bObserveAppCullFlag);
}

void NiPick::SetPickType(PickType ePickType) {
    m_ePickType = (NiPick::PickType)((int32_t)ePickType);
}

void NiPick::SetIntersectType(IntersectType eIntersectType) {
    m_eIntersectType = eIntersectType;
}

NiPick::Results& NiPick::GetResults() {
    return m_pickResults;
}

void NiPick::SetReturnNormal(bool bReturnNormal) {
    m_bReturnNormal = (bool)((uint8_t)bReturnNormal);
}

void NiPick::SetIsReturnTexture(bool bReturnTexture) {
    m_bReturnTexture = (bool)((uint8_t)bReturnTexture);
}

void NiPick::operator delete(void* apMemory) {
    ourFree(apMemory);
}
