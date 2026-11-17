#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkVtableClassRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"

hkStatisticsCollector::hkStatisticsCollector(hkVtableClassRegistry* vtblReg)
  : m_vtblRegistry(vtblReg), m_includeUnmanagedArrayMemory(false) {}

hkStatisticsCollector::~hkStatisticsCollector() {
    m_clientData = nullptr;
}

// Reports the named block with its used and allocated sizes as a normal chunk; callers pass an allocated size of 0.
void hkStatisticsCollector::addNormalChunk(const char* name, const void* p, int32_t size, int32_t allocated) {
    int32_t v5 = allocated;
    if ((size > 0) && (!allocated)) {
        v5 = hkStatisticsCollector::getAllocatedSize(p, size);
    }
    addChunk(hkStatisticsCollector::MEM_NORMAL, name, p, size, v5);
}

void hkStatisticsCollector::addObjectChunk(const char* name, const void* p, int32_t size, int32_t allocated) {
    int32_t AllocatedSize = allocated;
    if (size > 0 && !allocated) {
        AllocatedSize = hkStatisticsCollector::getAllocatedSize(p, size);
    }
    addChunk(hkStatisticsCollector::MEM_OBJECT, name, p, size, AllocatedSize);
}

int32_t hkStatisticsCollector::getReferencedObjectSize(const hkReferencedObject* obj) {
    int32_t result = obj->m_memSizeAndFlags & hkReferencedObject::MASK_MEMSIZE;
    if ((obj->m_memSizeAndFlags & hkReferencedObject::MASK_MEMSIZE) == 0 &&
        m_includeUnmanagedArrayMemory != static_cast<uint8_t>(obj->m_memSizeAndFlags)) {
        const hkClass* v3 = this->getVtblClassRegistry()->getClassFromVirtualInstance(obj);
        if (v3) {
            return v3->getObjectSize();
        }
        else {
            return 0;
        }
    }
    return result;
}

// Reports the string the hkStringPtr owns under name as an array chunk.
void hkStatisticsCollector::addStringPtr(const char* name, const hkStringPtr& str) {
    const char* pcString = str.cString();
    if (pcString) {
        int32_t size = hkString::strLen(pcString);
        int32_t allocated = hkStatisticsCollector::getAllocatedSize(pcString, size);
        addChunk(hkStatisticsCollector::MEM_ARRAY, name, pcString, size, allocated);
    }
}

const hkClass* hkStatisticsCollector::findClassType(const hkReferencedObject* obj) {
    if (!obj) {
        return 0;
    }
    const hkClass* ClassType = obj->getClassType();
    if (!ClassType) {
        if (!this->getVtblClassRegistry()) {
            return 0;
        }
        ClassType = this->getVtblClassRegistry()->getClassFromVirtualInstance(obj);
        if (!ClassType) {
            return 0;
        }
    }
    return ClassType;
}

void hkStatisticsCollector::addReferencedObjectMemory(const hkReferencedObject* obj, int32_t flags) {
    if ((flags & 1) == 0) {
        int32_t ReferencedObjectSize = getReferencedObjectSize(obj);
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        int32_t v6 = Instance->m_heap->getAllocatedSize(obj, ReferencedObjectSize);
        addChunk(hkStatisticsCollector::MEM_OBJECT, 0, obj, ReferencedObjectSize, v6);
    }
}

void hkStatisticsCollector::addObjectMemory(const hkClass& cls, const void* obj, int32_t flags) {
    if ((flags & 1) == 0) {
        int32_t ObjectSizeOutOfLine = cls.getObjectSize();
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        int32_t v7 = Instance->m_heap->getAllocatedSize(obj, ObjectSizeOutOfLine);
        addChunk(hkStatisticsCollector::MEM_OBJECT, 0, obj, ObjectSizeOutOfLine, v7);
    }
}

// The heap's figure for a block of size bytes at p; a null block is taken at its requested size.
int32_t hkStatisticsCollector::getAllocatedSize(const void* p, int32_t size) {
    if (p) {
        return hkMemoryRouter::getInstance().heap().getAllocatedSize(p, size);
    }
    return size;
}

const char* hkStatisticsCollector::memoryTypeToCstring(hkStatisticsCollector::MemoryType type) {
    const char* result;
    switch (type) {
        case hkStatisticsCollector::MEM_OBJECT: {
            result = "Object ";
            break;
        }
        case hkStatisticsCollector::MEM_NORMAL: {
            result = "Normal ";
            break;
        }
        case hkStatisticsCollector::MEM_ALLOCATED: {
            result = "Alloc'd";
            break;
        }
        case hkStatisticsCollector::MEM_ARRAY: {
            result = "Array  ";
            break;
        }
        case hkStatisticsCollector::MEM_OTHER: {
            result = "Other  ";
            break;
        }
        default: {
            result = "Unknown";
            break;
        }
    }
    return result;
}

// The KB figure is formatted from a double; the MB one goes through a float local before being passed as a double.
void hkStatisticsCollector::memorySizeToText(int32_t size, hkStringBuf& string) {
    if (size < 1024) {
        string.printf("%i Bytes", size);
        return;
    }
    if (size < 10240) {
        string.printf("%.1f Kb", size * 0.0009765625);
        return;
    }
    if (size < 0x100000) {
        string.printf("%i Kb", size / 1024);
        return;
    }
    float fMegabytes = static_cast<float>(size * 9.5367431640625e-07);
    string.printf("%.2f Mb", fMegabytes);
}
