#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemManager.h"

// A zero size is raised to 1.
void* NiMemObject::operator new(size_t astSizeInBytes) noexcept {
    size_t stSizeInBytes = astSizeInBytes;
    if (!stSizeInBytes) {
        stSizeInBytes = 1;
    }
    astSizeInBytes = 4;
    NiAllocator* pkAllocator = NiMemManager::ms_pkMemManager->m_pkAllocator;
    return pkAllocator->Allocate(stSizeInBytes, astSizeInBytes, NI_OPER_NEW, true, nullptr, -1, nullptr);
}

void NiMemObject::operator delete(void* apvMem, size_t astElementSize) {
    if (apvMem) {
        NiMemManager::ms_pkMemManager->m_pkAllocator->Deallocate(apvMem, NI_OPER_DELETE, astElementSize);
    }
}

// As operator new, with event NI_OPER_NEW_ARRAY and no accurate size on deallocation.
void* NiMemObject::operator new[](size_t astSizeInBytes) noexcept {
    size_t stSizeInBytes = astSizeInBytes;
    if (!stSizeInBytes) {
        stSizeInBytes = 1;
    }
    astSizeInBytes = 4;
    NiAllocator* pkAllocator = NiMemManager::ms_pkMemManager->m_pkAllocator;
    return pkAllocator->Allocate(stSizeInBytes, astSizeInBytes, NI_OPER_NEW_ARRAY, false, nullptr, -1, nullptr);
}

// The block goes to the current allocator with event NI_OPER_DELETE_ARRAY and no size; the element size is not read.
void NiMemObject::operator delete[](void* pvMem, size_t /*astElementSize*/) {
    if (pvMem) {
        NiAllocator* pkAllocator = NiMemManager::ms_pkMemManager->m_pkAllocator;
        pkAllocator->Deallocate(pvMem, NI_OPER_DELETE_ARRAY, static_cast<size_t>(-1));
    }
}
