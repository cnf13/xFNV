#include "Havok/Source/Common/Base/Container/Array/hkSmallArrayUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

#include <cstring>

// Growing a small array allocates a fresh block, copies the elements into it, hands the old block back unless the
// array does not own it, and keeps the locked flag in the new capacity word. apArrayMemory is any hkSmallArray<T>, as
// in the SDK (void* array).
int16_t hkSmallArrayUtil::_reserve(void* array, int32_t numElem, int32_t sizeElem) {
    hkSmallArray<void*>* apArray = static_cast<hkSmallArray<void*>*>(array);
    hkMemoryAllocator* pHeap = &hkMemoryRouter::getInstance().heap();
    void* pNewData = pHeap->blockAlloc(numElem * sizeElem);
    std::memcpy(pNewData, apArray->m_data, static_cast<size_t>(sizeElem) * apArray->m_size);
    if ((apArray->m_capacityAndFlags & hkSmallArray<void*>::DONT_DEALLOCATE_FLAG) == 0) {
        hkMemoryAllocator* pHeapForFree = &hkMemoryRouter::getInstance().heap();
        pHeapForFree->blockFree(
            apArray->m_data,
            sizeElem * (apArray->m_capacityAndFlags & hkSmallArray<void*>::CAPACITY_MASK)
        );
    }
    int16_t iCapacityAndFlags = static_cast<int16_t>(
        static_cast<uint16_t>(numElem) | (apArray->m_capacityAndFlags & hkSmallArray<void*>::LOCKED_FLAG)
    );
    apArray->m_data = static_cast<void**>(pNewData);
    apArray->m_capacityAndFlags = static_cast<uint16_t>(iCapacityAndFlags);
    return iCapacityAndFlags;
}

// Adding an element to a full array doubles it, or starts it with a single element when it is empty. apArrayMemory is
// any hkSmallArray<T>, as in the SDK (void* array).
int16_t hkSmallArrayUtil::_reserveMore(void* array, int32_t sizeElem) {
    hkSmallArray<void*>* apArray = static_cast<hkSmallArray<void*>*>(array);
    int32_t iNumElem = apArray->m_size != 0 ? 2 * apArray->m_size : 1;
    uint16_t usNumElem = static_cast<uint16_t>(iNumElem);
    hkMemoryAllocator* pHeap = &hkMemoryRouter::getInstance().heap();
    void* pNewData = pHeap->blockAlloc(iNumElem * sizeElem);
    std::memcpy(pNewData, apArray->m_data, static_cast<size_t>(sizeElem) * apArray->m_size);
    if ((apArray->m_capacityAndFlags & hkSmallArray<void*>::DONT_DEALLOCATE_FLAG) == 0) {
        hkMemoryAllocator* pHeapForFree = &hkMemoryRouter::getInstance().heap();
        pHeapForFree->blockFree(
            apArray->m_data,
            sizeElem * (apArray->m_capacityAndFlags & hkSmallArray<void*>::CAPACITY_MASK)
        );
    }
    int16_t iCapacityAndFlags = static_cast<int16_t>(
        usNumElem | (apArray->m_capacityAndFlags & hkSmallArray<void*>::LOCKED_FLAG)
    );
    apArray->m_data = static_cast<void**>(pNewData);
    apArray->m_capacityAndFlags = static_cast<uint16_t>(iCapacityAndFlags);
    return iCapacityAndFlags;
}
