#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "SDK/MSVC/stdlib.h"

hkThreadLocalData<hkMonitorStream*> hkMonitorStream::m_instance;

void hkMonitorStream::reset() {
    m_end = m_start;
}

// Frees this stream's heap buffer, then the thread's stream object (the one in the TLS slot, read first), and clears
// the slot.
void hkMonitorStream::quit() {
    void* pInstance = TlsGetValue(m_instance.m_slotID);
    char* pStart = m_start;
    if (pStart && m_isBufferAllocatedOnTheHeap.m_bool) {
        char* pCapacity = m_capacity;
        hkMemoryRouter::getInstance().m_debug->bufFree(pStart, pCapacity - pStart);
    }
    hkMemoryRouter::getInstance().m_debug->blockFree(pInstance, sizeof(hkMonitorStream));
    TlsSetValue(m_instance.m_slotID, nullptr);
}

// A new size frees a heap buffer and takes a new one from the debug allocator, the capacity being the size the
// allocator returns; a size of 0 or less frees the whole stream (quit).
void hkMonitorStream::resize(int32_t newSize) {
    char* pStart = m_start;
    char* pCapacity = m_capacity;
    if (newSize == pCapacity - pStart) {
        return;
    }
    if (newSize > 0) {
        if (pStart && m_isBufferAllocatedOnTheHeap.m_bool) {
            hkMemoryRouter::getInstance().m_debug->bufFree(pStart, pCapacity - pStart);
        }
        m_isBufferAllocatedOnTheHeap.m_bool = 1;
        // The SDK's inline _bufAlloc<char>: bufAlloc rounds the size up through its reference.
        int32_t iNumBytes = newSize;
        char* pNewStart = static_cast<char*>(hkMemoryRouter::getInstance().m_debug->bufAlloc(iNumBytes));
        m_start = pNewStart;
        m_end = pNewStart;
        m_capacity = pNewStart + iNumBytes;
        m_capacityMinus16 = m_capacity - 32;
    }
    else {
        quit();
    }
}

void hkMonitorStream::setStaticBuffer(char* buffer, int32_t bufferSize) {
    if (m_isBufferAllocatedOnTheHeap.m_bool) {
        resize(0);
    }
    m_isBufferAllocatedOnTheHeap.m_bool = 0;
    m_start = buffer;
    m_end = buffer;
    m_capacity = &buffer[bufferSize];
    m_capacityMinus16 = &buffer[bufferSize - 32];
}

// Allocates the calling thread's stream from the debug allocator, stores it in the TLS slot and clears it.
void hkMonitorStream::init() {
    hkMonitorStream* pInstance = static_cast<hkMonitorStream*>(
        hkMemoryRouter::getInstance().m_debug->blockAlloc(sizeof(hkMonitorStream))
    );
    TlsSetValue(m_instance.m_slotID, pInstance);
    pInstance->m_isBufferAllocatedOnTheHeap.m_bool = 0;
    pInstance->m_start = nullptr;
    pInstance->m_capacity = nullptr;
    pInstance->m_end = nullptr;
    pInstance->m_capacityMinus16 = nullptr;
}
