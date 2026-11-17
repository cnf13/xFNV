#include "Gamebryo2.2/CoreLibs/NiMain/NiTAbstractPoolAllocator.h"

#include "BSCore/BSSpinLock.h"

void NiTAbstractPoolAllocator<uint32_t>::Deallocate(NiTAbstractPoolAllocator<uint32_t>::AllocNode* a1) {
    a1->m_element = static_cast<uint32_t>(
        (static_cast<uint32_t>(a1->m_element) & 0xFFFFFF00u) | (static_cast<uint32_t>(static_cast<uint8_t>(0)))
    );
    NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.Lock(nullptr);
    a1->m_pkData = nullptr;
    a1->m_pkNext = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
    bool v1 = NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.uiLockCount-- == 1;
    NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem = a1;
    if (v1) {
        NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.uiOwningThread = 0;
    }
}
