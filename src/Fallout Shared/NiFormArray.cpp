#include "Fallout Shared/NiFormArray.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTLargeArray.h"
#include "SDK/MSVC/string.h"

NiFormArray::NiFormArray() : NiTLargePrimitiveArray<TESForm*>(0, 1) {}

TESForm* NiFormArray::GetFormAt(uint32_t aiIndex) {
    if (aiIndex < NiTLargeArray<TESForm*, NiTMallocInterface<TESForm*>>::GetSize()) {
        return NiTLargeArray<TESForm*, NiTMallocInterface<TESForm*>>::GetAt(aiIndex);
    }
    return nullptr;
}

void NiFormArray::InsertFormAt(uint32_t aiIndex, TESForm* apForm) {
    Compact();
    if (aiIndex <= m_uiSize) {
        uint32_t uiNewSize = m_uiSize + 1;
        if (m_uiMaxSize < uiNewSize) {
            SetSize(m_uiGrowBy + uiNewSize);
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
    if (apForm) {
        uint32_t uiSize = NiTLargeArray<TESForm*, NiTMallocInterface<TESForm*>>::GetSize();
        if (uiSize) {
            for (uint32_t i = 0; i < uiSize; ++i) {
                if (NiTLargeArray<TESForm*, NiTMallocInterface<TESForm*>>::GetAt(i) == apForm) {
                    return i;
                }
            }
        }
    }
    return 0xFFFFFFFFu;
}

void NiFormArray::operator delete(void* apMemory) {
    ourFree(apMemory);
}
