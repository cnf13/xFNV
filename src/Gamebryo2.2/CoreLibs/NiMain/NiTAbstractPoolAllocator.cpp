#include "Gamebryo2.2/CoreLibs/NiMain/NiTAbstractPoolAllocator.h"

#include "BSCore/BSSpinLock.h"

void NiTAbstractPoolAllocator<uint32_t>::Deallocate(NiTAbstractPoolAllocator<uint32_t>::AllocNode* pkDel) {
    pkDel->m_element &= 0xFFFFFF00u;
    NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.Lock(nullptr);
    pkDel->m_pkData = nullptr;
    pkDel->m_pkNext = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
    bool v1 = NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.uiLockCount-- == 1;
    NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem = pkDel;
    if (v1) {
        NiTAbstractPoolAllocator<uint32_t>::ms_kSpinLock.uiOwningThread = 0;
    }
}
