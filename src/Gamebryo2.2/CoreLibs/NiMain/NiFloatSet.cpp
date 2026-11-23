#include "Gamebryo2.2/CoreLibs/NiMain/NiFloatSet.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

// Frees the old block even when it is null, and copies the used entries one at a time.
void NiFloatSet::RemoveAll(uint32_t auiSize) {
    if (auiSize == m_uiAlloced) {
        return;
    }
    float* pfNew;
    if (auiSize) {
        pfNew = static_cast<float*>(_NiMalloc(sizeof(float) * auiSize));
        for (uint32_t i = 0; i < m_uiUsed; ++i) {
            pfNew[i] = m_pBase[i];
        }
    }
    else {
        pfNew = nullptr;
    }
    _NiFree(m_pBase);
    m_pBase = pfNew;
    m_uiAlloced = auiSize;
}
