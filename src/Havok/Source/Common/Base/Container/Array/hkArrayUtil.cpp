#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "SDK/MSVC/string.h"

#include <cstring>

using hkClassPtrArrayBase_t = hkArrayBase<hkClass*>;

using hkArray_lchar_chkContainerHeapAllocator = hkArray<char, hkContainerHeapAllocator>;

// A fixed capacity array keeps its storage when it grows: a fresh block is allocated, the elements are copied
// into it and the array starts pointing at the block.
void hkArrayUtil::_reserve(hkMemoryAllocator& mem, void* array, int32_t numElem, int32_t sizeElem) {
	hkArrayBase<void>* pArray = static_cast<hkArrayBase<void>*>(array);
	int32_t iReqNumBytes = numElem * sizeElem;
	uint32_t uiCapacityAndFlags = pArray->m_capacityAndFlags;
	if (static_cast<int32_t>(uiCapacityAndFlags) < 0) {
		void* pNewData = mem.bufAlloc(iReqNumBytes);
		std::memcpy(pNewData, pArray->m_data, static_cast<size_t>(sizeElem) * pArray->m_size);
		pArray->m_data = pNewData;
		pArray->m_capacityAndFlags = static_cast<uint32_t>(iReqNumBytes / sizeElem);
	} else {
		pArray->m_data = mem.bufRealloc(pArray->m_data,
			sizeElem * static_cast<int32_t>(uiCapacityAndFlags & hkArrayBase<void>::CAPACITY_MASK), iReqNumBytes);
		pArray->m_capacityAndFlags = static_cast<uint32_t>(iReqNumBytes / sizeElem);
	}
}

// Adding an element to a full array doubles it, or starts it with a single element; the caller gets the capacity
// the array ends up with. apArray is any hkArrayBase<T>, as in the SDK (void* array).
int32_t hkArrayUtil::_reserveMore(hkMemoryAllocator& mem, void* array, int32_t sizeElem) {
	hkArrayBase<void>* pArray = static_cast<hkArrayBase<void>*>(array);
	int32_t iNumElem = pArray->m_size != 0 ? 2 * pArray->m_size : 1;
	int32_t iReqNumBytes = sizeElem * iNumElem;
	int32_t iCapacityAndFlags = pArray->m_capacityAndFlags;
	if (iCapacityAndFlags < 0) {
		void* pNewData = mem.bufAlloc(iReqNumBytes);
		rt::memcpy(pNewData, pArray->m_data, static_cast<size_t>(sizeElem) * pArray->m_size);
		pArray->m_data = pNewData;
		pArray->m_capacityAndFlags = static_cast<uint32_t>(iReqNumBytes / sizeElem);
	} else {
		pArray->m_data = mem.bufRealloc(pArray->m_data,
			sizeElem * (iCapacityAndFlags & static_cast<int32_t>(hkArrayBase<void>::CAPACITY_MASK)), iReqNumBytes);
		pArray->m_capacityAndFlags = static_cast<uint32_t>(iReqNumBytes / sizeElem);
	}
	return iReqNumBytes / sizeElem;
}

// Shrinking into storage the caller provided does not free anything: the elements are copied out and the array's
// old block is handed back before the storage is marked as not ours. apArrayMemory is any hkArrayBase<T>, as in the
// SDK (void* array).
void hkArrayUtil::_reduce(hkMemoryAllocator& mem, void* array, int32_t sizeElem, char* inplaceMem,
	int32_t requestedCapacity) {
	hkArrayBase<void>* apArray = static_cast<hkArrayBase<void>*>(array);
	if (inplaceMem != nullptr && apArray->m_size < requestedCapacity) {
		std::memcpy(inplaceMem, apArray->m_data, static_cast<size_t>(sizeElem) * apArray->m_size);
		mem.bufFree(apArray->m_data,
			sizeElem * static_cast<int32_t>(apArray->m_capacityAndFlags & hkArrayBase<void>::CAPACITY_MASK));
		apArray->m_capacityAndFlags =
			static_cast<uint32_t>(requestedCapacity) | hkArrayBase<void>::DONT_DEALLOCATE_FLAG;
		apArray->m_data = inplaceMem;
		return;
	}
	int32_t iReqNumBytes = sizeElem * requestedCapacity;
	apArray->m_data = mem.bufRealloc(apArray->m_data, apArray->m_capacityAndFlags * sizeElem, iReqNumBytes);
	apArray->m_capacityAndFlags = static_cast<uint32_t>(iReqNumBytes / sizeElem);
}
