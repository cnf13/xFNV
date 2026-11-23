#pragma once

#include <cstdint>

// A growable array of floats with an explicit used count.
class NiFloatSet {
public:
    // Reallocates to auiSize entries, copying the used ones across; auiSize 0 frees the block.
    void RemoveAll(uint32_t auiSize);

    float* m_pBase;
    uint32_t m_uiAlloced;
    uint32_t m_uiUsed;
};
