#include "Fallout Shared/NiFormArray.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "SDK/MSVC/string.h"

NiFormArray::NiFormArray() : NiTLargePrimitiveArray<TESForm*>(0, 1) {}

TESForm* NiFormArray::GetFormAt(uint32_t aiIndex) {
    TESForm* v4 = nullptr;
    bool vCondition1;
    vCondition1 = aiIndex < NiTLargeArray<TESForm*, NiTMallocInterface<TESForm*>>::GetSize();
    if (vCondition1) {
        return NiTLargeArray<TESForm*, NiTMallocInterface<TESForm*>>::GetAt(aiIndex);
    }
    return v4;
}

void NiFormArray::InsertFormAt(uint32_t aiIndex, TESForm* apForm) {
    Compact();
    if (aiIndex <= m_uiSize) {
        uint32_t newsize = m_uiSize + 1;
        if (m_uiMaxSize < newsize) {
            SetSize(m_uiGrowBy + newsize);
        }
        std::memmove(&m_pBase[aiIndex + 1], &m_pBase[aiIndex], sizeof(TESForm*) * (m_uiSize - aiIndex));
        m_uiESize = ++m_uiSize;
        m_pBase[aiIndex] = apForm;
    }
    else {
        SetAtGrow(aiIndex, apForm);
    }
}

uint32_t NiFormArray::GetIndexForForm(TESForm* apForm) {
    uint32_t i;
    uint32_t v5;
    uint32_t v6 = 0xFFFFFFFFu;
    if (apForm) {
        v5 = NiTLargeArray<TESForm*, NiTMallocInterface<TESForm*>>::GetSize();
        if (v5) {
            for (i = 0; i < v5; ++i) {
                bool vCondition1;
                vCondition1 = NiTLargeArray<TESForm*, NiTMallocInterface<TESForm*>>::GetAt(i) == apForm;
                if (vCondition1) {
                    return i;
                }
            }
        }
    }
    return v6;
}

void NiFormArray::operator delete(void* apMemory) {
    ourFree(apMemory);
}
