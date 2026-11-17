#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemManager.h"

// A zero size is raised to 1; the exe stores the alignment (4) over its own size argument's stack slot.
void* _NiMalloc(size_t astSizeInBytes) {
	size_t stSizeInBytes = astSizeInBytes;
	if (!stSizeInBytes) {
		stSizeInBytes = 1;
	}
	astSizeInBytes = 4;
	NiAllocator* pkAllocator = NiMemManager::ms_pkMemManager->m_pkAllocator;
	return pkAllocator->Allocate(stSizeInBytes, astSizeInBytes, NI_MALLOC, false, nullptr, -1, nullptr);
}

void _NiFree(void* pvMem) {
	if (pvMem) {
		NiMemManager::ms_pkMemManager->m_pkAllocator->Deallocate(pvMem, NI_FREE, -1);
	}
}

// Frees on a zero size, allocates (a zero size raised to 1) on a null block, reallocates otherwise; the external
// allocator, 4-byte aligned. The exe stores the alignment over the size argument's stack slot.
void* _NiExternalRealloc(void* pvMem, size_t astSizeInBytes) {
	if (!astSizeInBytes && pvMem) {
		NiMemManager::ms_pkMemManager->m_pkAllocator->DeallocateExternal(pvMem, NI_EXTERNAL_FREE, (size_t)-1);
		return nullptr;
	}
	size_t stSizeInBytes = astSizeInBytes;
	if (!pvMem) {
		if (!stSizeInBytes) {
			stSizeInBytes = 1;
		}
		astSizeInBytes = 4;
		NiAllocator* pkAllocator = NiMemManager::ms_pkMemManager->m_pkAllocator;
		return pkAllocator->AllocateExternal(stSizeInBytes, astSizeInBytes, NI_EXTERNAL_MALLOC, false, nullptr, -1,
			nullptr);
	}
	astSizeInBytes = 4;
	NiAllocator* pkAllocator = NiMemManager::ms_pkMemManager->m_pkAllocator;
	return pkAllocator->ReallocateExternal(pvMem, stSizeInBytes, astSizeInBytes, NI_EXTERNAL_REALLOC, false, (size_t)-1,
		nullptr, -1, nullptr);
}

// As _NiExternalRealloc with the caller's alignment; the exe stores it over the size argument's stack slot.
void* _NiExternalAlignedRealloc(void* pvMem, size_t astSizeInBytes, size_t stAlignment) {
	if (!astSizeInBytes && pvMem) {
		NiMemManager::ms_pkMemManager->m_pkAllocator->DeallocateExternal(pvMem, NI_EXTERNAL_ALIGNEDFREE, (size_t)-1);
		return nullptr;
	}
	size_t stSizeInBytes = astSizeInBytes;
	if (!pvMem) {
		if (!stSizeInBytes) {
			stSizeInBytes = 1;
		}
		astSizeInBytes = stAlignment;
		NiAllocator* pkAllocator = NiMemManager::ms_pkMemManager->m_pkAllocator;
		return pkAllocator->AllocateExternal(stSizeInBytes, astSizeInBytes, NI_EXTERNAL_ALIGNEDMALLOC, false, nullptr,
			-1, nullptr);
	}
	astSizeInBytes = stAlignment;
	NiAllocator* pkAllocator = NiMemManager::ms_pkMemManager->m_pkAllocator;
	return pkAllocator->ReallocateExternal(pvMem, stSizeInBytes, astSizeInBytes, NI_EXTERNAL_ALGINEDREALLOC, false,
		(size_t)-1, nullptr, -1, nullptr);
}

void _NiAlignedFree(void* pvMem) {
	if (pvMem) {
		NiMemManager::ms_pkMemManager->m_pkAllocator->Deallocate(pvMem, NI_ALIGNEDFREE, -1);
	}
}

void _NiExternalAlignedFree(void* pvMem) {
	if (pvMem) {
		NiMemManager::ms_pkMemManager->m_pkAllocator->DeallocateExternal(pvMem, NI_EXTERNAL_ALIGNEDFREE, -1);
	}
}

void* _NiAlignedMalloc(size_t stSizeInBytes, size_t stAlignment) {
	uint32_t v2 = stSizeInBytes;
	if (!stSizeInBytes) {
		v2 = 1;
	}
	stSizeInBytes = stAlignment;
	NiAllocator* m_pkAllocator = NiMemManager::ms_pkMemManager->m_pkAllocator;
	stAlignment = v2;
	return m_pkAllocator->Allocate(stAlignment, stSizeInBytes, NI_ALIGNEDMALLOC, 0, 0, -1, 0);
}

void* _NiExternalAlignedMalloc(size_t stSizeInBytes, size_t stAlignment) {
	size_t v2 = stSizeInBytes;
	if (!stSizeInBytes) {
		v2 = 1;
	}
	stSizeInBytes = stAlignment;
	NiAllocator* m_pkAllocator = NiMemManager::ms_pkMemManager->m_pkAllocator;
	stAlignment = v2;
	return m_pkAllocator->AllocateExternal(stAlignment, stSizeInBytes, NI_EXTERNAL_ALIGNEDMALLOC, 0, 0, -1, 0);
}

void _NiExternalFree(void* pvMem) {
	if (pvMem) {
		NiMemManager::ms_pkMemManager->m_pkAllocator->DeallocateExternal(pvMem, NI_EXTERNAL_FREE, 0xFFFFFFFFu);
	}
}
