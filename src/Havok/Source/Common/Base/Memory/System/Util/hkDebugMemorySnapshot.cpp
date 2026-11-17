#include "Havok/Source/Common/Base/Memory/System/Util/hkDebugMemorySnapshot.h"

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

void hkDebugMemorySnapshot::subtract(hkDebugMemorySnapshot& rhs) {
    ascendingOrder();
    rhs.ascendingOrder();
    void** m_pointers = this->m_pointers;
    void** v4 = &this->m_pointers[m_numPointers];
    void** v5 = rhs.m_pointers;
    void** v6 = &rhs.m_pointers[rhs.m_numPointers];
    if (this->m_pointers < v4) {
        do {
            if (v5 >= v6) {
                return;
            }
            if (*m_pointers == *v5) {
                *m_pointers++ = 0;
            }
            else {
                if (*m_pointers < *v5) {
                    ++m_pointers;
                    continue;
                }
            }
            ++v5;
        }
        while (m_pointers < v4);
    }
}

void hkDebugMemorySnapshot::reset() {
    void** m_pointers = this->m_pointers;
    if (this->m_pointers) {
        int32_t m_numPointers = this->m_numPointers;
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        Instance->m_debug->blockFree(m_pointers, sizeof(void*) * m_numPointers);
        int32_t v5 = this->m_numPointers;
        hkMemoryPointerInfo* m_info = this->m_info;
        hkMemoryRouter* v7 = &hkMemoryRouter::getInstance();
        v7->m_debug->blockFree(m_info, sizeof(hkMemoryPointerInfo) * v5);
    }
    this->m_pointers = 0;
    this->m_info = 0;
    this->m_numPointers = 0;
}

// Sorts the pointers into ascending address order and their records with them: a sorted array of the old slots'
// addresses gives each new position its old slot.
void hkDebugMemorySnapshot::ascendingOrder() {
    if (m_numPointers <= 1) {
        return;
    }
    void*** pppSorted =
        static_cast<void***>(hkMemoryRouter::getInstance().debug().blockAlloc(sizeof(void**) * m_numPointers));
    for (int32_t i = 0; i < m_numPointers; ++i) {
        pppSorted[i] = &m_pointers[i];
    }
    // EXE-INLINED none: hkAlgorithm::quickSort<void**, hkBool (*)(void**, void**)>
    if (m_numPointers > 1) {
        hkAlgorithm::quickSortRecursive<void**, hkBool* (*)(hkBool*, void**, void**)>(
            pppSorted, 0, m_numPointers - 1, &hkDebugMemorySnapshot::_comparePointers
        );
    }
    void** ppPointers =
        static_cast<void**>(hkMemoryRouter::getInstance().debug().blockAlloc(sizeof(void*) * m_numPointers));
    hkMemoryPointerInfo* pInfo = static_cast<hkMemoryPointerInfo*>(
        hkMemoryRouter::getInstance().debug().blockAlloc(sizeof(hkMemoryPointerInfo) * m_numPointers)
    );
    for (int32_t i = 0; i < m_numPointers; ++i) {
        ppPointers[i] = *pppSorted[i];
        pInfo[i] = m_info[pppSorted[i] - m_pointers];
    }
    hkMemoryRouter::getInstance().debug().blockFree(pppSorted, sizeof(void**) * m_numPointers);
    hkMemoryRouter::getInstance().debug().blockFree(m_pointers, sizeof(void*) * m_numPointers);
    hkMemoryRouter::getInstance().debug().blockFree(m_info, sizeof(hkMemoryPointerInfo) * m_numPointers);
    m_pointers = ppPointers;
    m_info = pInfo;
}

hkBool* hkDebugMemorySnapshot::_comparePointers(hkBool* apResult, void** a, void** b) {
    apResult->m_bool = *a < *b;
    return apResult;
}
