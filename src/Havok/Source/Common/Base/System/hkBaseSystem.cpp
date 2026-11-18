#include "Havok/Source/Common/Base/System/hkBaseSystem.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Base/System/Error/hkDefaultError.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Hardware/hkHardwareInfo.h"
#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"
#include "Havok/Source/Common/Base/System/Io/FileSystem/hkNativeFileSystem.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Socket/Bsd/hkBsdSocket.h"
#include "Havok/Source/Common/Base/System/Io/Socket/hkSocket.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Buffered/hkBufferedStreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Stdio/hkStdioStreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "SDK/Windows/winbase.h"

#include <thread>

s_platformNetQuit_t s_platformNetQuit{&hkBsdNetworkQuit};

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkDummySingleton(
    "hkDummySingleton",
    &hkDummySingletoncreate,
    reinterpret_cast<void**>(&hkDummySingleton::s_instance)
);

void showHavokBuild() {
    ;
}

// The processor count is held as an int and compared signed, and the thread count is capped at 4.
void hkGetHardwareInfo(hkHardwareInfo& info) {
    // GetSystemInfo's dwNumberOfProcessors: the logical processors.
    const int32_t iNumProcessors = static_cast<int32_t>(std::thread::hardware_concurrency());
    if (iNumProcessors >= 4) {
        info.m_numThreads = 4;
    }
    else {
        info.m_numThreads = iNumProcessors;
    }
}

// The singleton's create function (HK_SINGLETON_IMPLEMENTATION: new hkDummySingleton()), with the constructor written
// out.
hkReferencedObject* hkDummySingletoncreate() {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkDummySingleton* result = static_cast<hkDummySingleton*>(Instance->m_heap->blockAlloc(sizeof(hkDummySingleton)));
    result->m_memSizeAndFlags = sizeof(hkDummySingleton);
    return ::new (result) hkDummySingleton;
}

hkResult
hkBaseSystem::init(hkMemoryRouter* memoryRouter, hkErrorReportFunction errorReportFunction, void* errorReportObject) {
    if (!hkBaseSystemIsInitialized.m_bool) {
        hkBaseSystem::initThread(memoryRouter);
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkNativeFileSystem* v4 =
            static_cast<hkNativeFileSystem*>(Instance->m_heap->blockAlloc(sizeof(hkNativeFileSystem)));
        v4->m_memSizeAndFlags = sizeof(hkNativeFileSystem);
        ::new (v4) hkNativeFileSystem;
        if (hkFileSystem::s_instance) {
            hkFileSystem::s_instance->removeReferenceLockUnchecked();
        }
        hkFileSystem::s_instance = v4;
        hkMemoryRouter* v5 = &hkMemoryRouter::getInstance();
        hkDefaultError* v6 = static_cast<hkDefaultError*>(v5->m_heap->blockAlloc(sizeof(hkDefaultError)));
        v6->m_memSizeAndFlags = sizeof(hkDefaultError);
        hkDefaultError* v7 = ::new (v6) hkDefaultError(errorReportFunction, errorReportObject);
        if (hkError::s_instance) {
            hkError::s_instance->removeReferenceLockUnchecked();
        }
        hkError::s_instance = (hkError*)v7;
        hkBaseSystem::initSingletons();
        hkDummySingleton::s_instance->forceLinkage();
        hkBaseSystemIsInitialized.m_bool = 1;
    }
    return HK_SUCCESS;
}

hkResult hkBaseSystem::initThread(hkMemoryRouter* memoryRouter) {
    hkMemoryRouter::replaceInstance(memoryRouter);
    hkMonitorStream::init();
    return HK_SUCCESS;
}

// Frees the thread's monitor stream, then detaches its memory router. Returns HK_SUCCESS (0).
hkResult hkBaseSystem::quitThread() {
    hkMonitorStream* pStream = static_cast<hkMonitorStream*>(TlsGetValue(hkMonitorStream::m_instance.m_slotID));
    if (pStream) {
        pStream->quit();
    }
    if (hkMemoryRouter::getInstancePtr()) {
        hkMemoryRouter::replaceInstance(nullptr);
    }
    return HK_SUCCESS;
}

// Walks the list through the address of the link that points at the current node, so that a node whose create
// function fails is unlinked in place (its m_next cleared) and kept in a temp-allocator array. The failed ones are
// retried from the last back to the first, again and again until the array is empty; each that succeeds is linked
// back in after the last linked node.
void hkBaseSystem::initSingletons() {
    using failed_t = hkArray<hkSingletonInitNode*, hkContainerTempAllocator>;
    XFNV_INLINED_LOCAL(failed_t, failed, std::nullopt, std::nullopt);
    failed.m_data = nullptr;
    failed.m_size = 0;
    failed.m_capacityAndFlags = hkArrayBase<hkSingletonInitNode*>::DONT_DEALLOCATE_FLAG;
    hkSingletonInitNode** prevNext = &hkSingletonInitList;
    hkSingletonInitNode* node = hkSingletonInitList;
    while (node) {
        if (*node->m_value == nullptr && node->m_createFunc) {
            hkReferencedObject* pObject = node->m_createFunc();
            if (!pObject) {
                // EXE-INLINED none: hkArray<hkSingletonInitNode*, hkContainerTempAllocator>::pushBack
                hkMemoryRouter& router = hkMemoryRouter::getInstance();
                if (failed.m_size == (failed.m_capacityAndFlags & hkArrayBase<hkSingletonInitNode*>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(*router.m_temp, &failed, sizeof(hkSingletonInitNode*));
                }
                failed.m_data[failed.m_size] = node;
                failed.m_size++;
                hkSingletonInitNode* next = node->m_next;
                (*prevNext)->m_next = nullptr;
                *prevNext = next;
                node = next;
                continue;
            }
            *node->m_value = pObject;
        }
        prevNext = &node->m_next;
        node = *prevNext;
    }
    while (failed.m_size != 0) {
        for (int32_t i = failed.m_size - 1; i >= 0; --i) {
            hkSingletonInitNode* retry = failed.m_data[i];
            hkReferencedObject* pObject = retry->m_createFunc();
            if (pObject) {
                *retry->m_value = pObject;
                *prevNext = retry;
                prevNext = &retry->m_next;
                // EXE-INLINED none: hkArrayBase<hkSingletonInitNode*>::removeAt
                const uint32_t last = --failed.m_size;
                if (last != static_cast<uint32_t>(i)) {
                    failed.m_data[i] = failed.m_data[last];
                }
            }
        }
    }
    hkMemoryAllocator& temp = hkMemoryRouter::getInstance().temp();
    const int32_t capacityAndFlags = failed.m_capacityAndFlags;
    failed.m_size = 0;
    if (capacityAndFlags >= 0) {
        temp.bufFree(
            failed.m_data,
            (capacityAndFlags & hkArrayBase<hkSingletonInitNode*>::CAPACITY_MASK) * sizeof(hkSingletonInitNode*)
        );
    }
}

// Collects the list into an inplace array of 128 (heap allocator), then releases the instances from its end.
void hkBaseSystem::quitSingletons() {
    using nodes_t = hkInplaceArray<hkSingletonInitNode*, 128>;
    XFNV_INLINED_LOCAL(nodes_t, nodes, std::nullopt, std::nullopt);
    nodes.m_data = nodes.m_storage;
    nodes.m_size = 0;
    nodes.m_capacityAndFlags = 128 | hkArrayBase<hkSingletonInitNode*>::DONT_DEALLOCATE_FLAG;
    for (hkSingletonInitNode* node = hkSingletonInitList; node; node = node->m_next) {
        // EXE-INLINED none: hkArray<hkSingletonInitNode*>::pushBack
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (nodes.m_size == (nodes.m_capacityAndFlags & hkArrayBase<hkSingletonInitNode*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*router.m_heap, &nodes, sizeof(hkSingletonInitNode*));
        }
        nodes.m_data[nodes.m_size] = node;
        nodes.m_size++;
    }
    for (int32_t i = nodes.m_size - 1; i >= 0; --i) {
        hkReferencedObject* pObject = static_cast<hkReferencedObject*>(*nodes.m_data[i]->m_value);
        if (pObject) {
            pObject->removeReferenceLockUnchecked();
            *nodes.m_data[i]->m_value = nullptr;
        }
    }
    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
    const int32_t capacityAndFlags = nodes.m_capacityAndFlags;
    nodes.m_size = 0;
    if (capacityAndFlags >= 0) {
        heap.bufFree(
            nodes.m_data,
            (capacityAndFlags & hkArrayBase<hkSingletonInitNode*>::CAPACITY_MASK) * sizeof(hkSingletonInitNode*)
        );
    }
}

hkResult hkBaseSystem::quit() {
    if (hkBaseSystemIsInitialized.m_bool) {
        hkReferencedObject::setLockMode(hkReferencedObject::LOCK_MODE_NONE);
        hkBaseSystem::quitSingletons();
        if ((hkSocket::s_platformNetInitialized.m_bool) && (s_platformNetQuit)) {
            s_platformNetQuit();
            hkSocket::s_platformNetInitialized.m_bool = 0;
        }

        if (hkError::s_instance) {
            hkError::s_instance->removeReferenceLockUnchecked();
        }
        hkError::s_instance = nullptr;
        if (hkFileSystem::s_instance) {
            hkFileSystem::s_instance->removeReferenceLockUnchecked();
        }
        hkFileSystem::s_instance = nullptr;
        hkBaseSystem::quitThread();
        hkBaseSystemIsInitialized.m_bool = 0;
    }
    return HK_SUCCESS;
}

// Copies the initialised flag into apResult and returns it.
hkBool hkBaseSystem::isInitialized() {
    return hkBaseSystemIsInitialized.m_bool;
}

hkStreamWriter* hkNativeFileSystem::openWriter(const char* name) {
    XFNV_INLINED_LOCAL(hkStringBuf, apMemory, std::nullopt, std::nullopt);
    ::new (&apMemory) hkStringBuf;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkStdioStreamWriter* v3 =
        static_cast<hkStdioStreamWriter*>(Instance->m_heap->blockAlloc(sizeof(hkStdioStreamWriter)));
    v3->m_memSizeAndFlags = sizeof(hkStdioStreamWriter);
    const char* nameIn = s_havokToPlatformConvert(name, apMemory);
    hkStdioStreamWriter* hkStdioStreamWriter_ = ::new (v3) hkStdioStreamWriter(nameIn);
    hkMemoryRouter* Instance_1 = &hkMemoryRouter::getInstance();
    hkBufferedStreamWriter* v7 =
        static_cast<hkBufferedStreamWriter*>(Instance_1->m_heap->blockAlloc(sizeof(hkBufferedStreamWriter)));
    v7->m_memSizeAndFlags = sizeof(hkBufferedStreamWriter);
    hkBufferedStreamWriter* hkBufferedStreamWriter_ = ::new (v7) hkBufferedStreamWriter(hkStdioStreamWriter_, 0x1000);
    hkStdioStreamWriter_->removeReference();
    hkMemoryAllocator* m_temp = (&hkMemoryRouter::getInstance())->m_temp;
    apMemory.m_string.m_size = 0;
    if (apMemory.m_string.m_capacityAndFlags >= 0) {
        m_temp->bufFree(
            apMemory.m_string.begin(), apMemory.m_string.m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK
        );
    }
    return hkBufferedStreamWriter_;
}
