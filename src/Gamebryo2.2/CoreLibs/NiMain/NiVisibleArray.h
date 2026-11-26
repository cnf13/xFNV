#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

#include <cstdint>

class NiGeometry;

// Growable array of the geometry a culling pass found visible.
class NiVisibleArray : public NiMemObject {
public:
    uint32_t GetCount() const {
        return m_uiCurrentSize;
    }
    NiGeometry& GetAt(uint32_t uiIndex) const {
        return *m_ppkArray[uiIndex];
    }

    NiGeometry** m_ppkArray;
    uint32_t m_uiCurrentSize;
    uint32_t m_uiAllocatedSize;
    uint32_t m_uiGrowBy;
};
