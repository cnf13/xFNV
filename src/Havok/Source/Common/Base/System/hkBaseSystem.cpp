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

// The singleton's create function (HK_SINGLETON_IMPLEMENTATION).
hkReferencedObject* hkDummySingletoncreate() {
    return new hkDummySingleton;
}

hkResult
hkBaseSystem::init(hkMemoryRouter* memoryRouter, hkErrorReportFunction errorReportFunction, void* errorReportObject) {
    if (!hkBaseSystemIsInitialized.m_bool) {
        hkBaseSystem::initThread(memoryRouter);
        hkNativeFileSystem* pFileSystem = new hkNativeFileSystem;
        if (hkFileSystem::s_instance) {
            hkFileSystem::s_instance->removeReferenceLockUnchecked();
        }
        hkFileSystem::s_instance = pFileSystem;
        hkDefaultError* pError = new hkDefaultError(errorReportFunction, errorReportObject);
        if (hkError::s_instance) {
            hkError::s_instance->removeReferenceLockUnchecked();
        }
        hkError::s_instance = pError;
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
    failed_t failed;
    hkSingletonInitNode** prevNext = &hkSingletonInitList;
    hkSingletonInitNode* node = hkSingletonInitList;
    while (node) {
        if (*node->m_value == nullptr && node->m_createFunc) {
            hkReferencedObject* pObject = node->m_createFunc();
            if (!pObject) {
                failed.pushBack(node);
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
                failed.removeAt(i);
            }
        }
    }
}

// Collects the list into an inplace array of 128 (heap allocator), then releases the instances from its end.
void hkBaseSystem::quitSingletons() {
    using nodes_t = hkInplaceArray<hkSingletonInitNode*, 128>;
    nodes_t nodes;
    for (hkSingletonInitNode* node = hkSingletonInitList; node; node = node->m_next) {
        nodes.pushBack(node);
    }
    for (int32_t i = nodes.m_size - 1; i >= 0; --i) {
        hkReferencedObject* pObject = static_cast<hkReferencedObject*>(*nodes.m_data[i]->m_value);
        if (pObject) {
            pObject->removeReferenceLockUnchecked();
            *nodes.m_data[i]->m_value = nullptr;
        }
    }
}

hkResult hkBaseSystem::quit() {
    if (hkBaseSystemIsInitialized.m_bool) {
        hkReferencedObject::setLockMode(hkReferencedObject::LOCK_MODE_NONE);
        hkBaseSystem::quitSingletons();
        if (hkSocket::s_platformNetInitialized.m_bool && s_platformNetQuit) {
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
    hkStringBuf apMemory;
    hkStdioStreamWriter* pFile = new hkStdioStreamWriter(s_havokToPlatformConvert(name, apMemory));
    hkBufferedStreamWriter* pBuffered = new hkBufferedStreamWriter(pFile, 0x1000);
    pFile->removeReference();
    return pBuffered;
}
