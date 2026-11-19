#include "Havok/Source/Common/Serialize/Packfile/hkPackfileData.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkClassNameRegistry.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkTypeInfoRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Serialize/Resource/hkResource.h"
#include "Havok/Source/Common/Serialize/Util/hkBuiltinTypeRegistry.h"

#include <new>

bool baseOrSameClass(const char* typeName, const char* topLevelClassName, const hkClassNameRegistry* classReg) {
    const hkClass* pTypeClass = classReg->getClassByName(typeName);
    const hkClass* pTopLevelClass = classReg->getClassByName(topLevelClassName);
    return pTopLevelClass->isSuperClass(*pTypeClass).m_bool != 0;
}

bool baseOrSameClass_E050C0(const char* name, const char* baseName, const hkClassNameRegistry* reg) {
    const hkClass* v3 = reg->getClassByName(name);
    const hkClass* v4 = reg->getClassByName(baseName);
    return v4->isSuperClass(*v3).m_bool != 0;
}

hkPackfileData::hkPackfileData() : m_topLevelObject(nullptr), m_name(nullptr), m_destructorsEnabled(1) {
    hkClassNameRegistry* pClassRegistry = hkBuiltinTypeRegistry::s_instance->getClassNameRegistry();
    if (pClassRegistry) {
        pClassRegistry->addReference();
    }
    m_packfileClassRegistry.m_pntr = pClassRegistry;
}

// Runs the loaded objects' destructors, then frees the owned blocks, chunks and name; the arrays, maps and registry
// pointer release theirs themselves.
hkPackfileData::~hkPackfileData() {
    hkPackfileData::callDestructors();
    for (void* pMemory : m_memory) {
        hkMemoryRouter::easyFree(hkMemoryRouter::getInstance().heap(), pMemory);
    }
    for (const Chunk& kChunk : m_chunks) {
        hkMemoryRouter::getInstance().m_heap->blockFree(kChunk.pointer, kChunk.numBytes);
    }
    hkMemoryRouter::easyFree(hkMemoryRouter::getInstance().heap(), m_name);
}

// Each tracked object whose type the type map knows is cleaned up by that type's cleanup function, then both maps are
// emptied. The type count is tested with a signed compare against 0.
void hkPackfileData::callDestructors() {
    if (m_destructorsEnabled && m_trackedTypes.m_map.m_numElems > 0) {
        for (int32_t it = m_trackedObjects.getIterator(); m_trackedObjects.isValid(it);
             it = m_trackedObjects.getNext(it)) {
            const hkTypeInfo* pTypeInfo = m_trackedTypes.getWithDefault(m_trackedObjects.getValue(it), nullptr);
            if (pTypeInfo && pTypeInfo->m_cleanupLoadedObjectFunction) {
                pTypeInfo->m_cleanupLoadedObjectFunction(m_trackedObjects.getKey(it));
            }
        }
    }
    m_topLevelObject = nullptr;
    m_trackedObjects.m_map.clear();
    m_trackedTypes.m_map.clear();
}

// Copies the imports and the exports into impOut and expOut.
void hkPackfileData::getImportsExports(
    hkArray<hkResource::Import, hkContainerHeapAllocator>& impOut,
    hkArray<hkResource::Export, hkContainerHeapAllocator>& expOut
) const {
    impOut = m_imports;
    expOut = m_exports;
}

// Returns null without a top-level object, or when typeName's class is not the top-level object's class or a base of
// it. Unless the objects are finished already, each tracked object is finished through finishRegistry and its type
// recorded in the type map; one that the registry does not know makes the result null.
void* hkPackfileData::getContentsPointer(const char* typeName, const hkTypeInfoRegistry* typeRegistry) const {
    if (!m_topLevelObject) {
        return nullptr;
    }
    const char* topLevelClassName = m_trackedObjects.getWithDefault(m_topLevelObject, nullptr);
    if (typeName) {
        // EXE-INLINED 0xE050C0
        const hkClassNameRegistry* pRegistry = m_packfileClassRegistry.val();
        const hkClass* pTopLevelClass = pRegistry->getClassByName(topLevelClassName);
        const hkClass* pTypeClass = pRegistry->getClassByName(typeName);
        if (!pTypeClass->isSuperClass(*pTopLevelClass).m_bool) {
            return nullptr;
        }
    }
    if (!finishedObjects() && typeRegistry) {
        bool bFailed = false;
        for (int32_t it = m_trackedObjects.getIterator(); m_trackedObjects.isValid(it);
             it = m_trackedObjects.getNext(it)) {
            const char* pClassName = m_trackedObjects.getValue(it);
            const hkTypeInfo* pTypeInfo = typeRegistry->finishLoadedObject(m_trackedObjects.getKey(it), pClassName);
            if (pTypeInfo) {
                const_cast<hkPackfileData*>(this)->m_trackedTypes.insert(pClassName, pTypeInfo);
            }
            else {
                bFailed = true;
            }
        }
        if (bFailed) {
            return nullptr;
        }
    }
    return m_topLevelObject;
}

const char* hkPackfileData::getContentsTypeName() const {
    void* m_topLevelObject = this->m_topLevelObject;
    if (m_topLevelObject) {
        return m_trackedObjects.getWithDefault(m_topLevelObject, 0);
    }

    return nullptr;
}

void hkPackfileData::calcContentStatistics(hkStatisticsCollector* a1, const hkClass* a2) const {
    int32_t m_capacityAndFlags = m_memory.m_capacityAndFlags;
    if (m_capacityAndFlags >= 0 || a1->m_includeUnmanagedArrayMemory) {
        int32_t AllocatedSize = hkStatisticsCollector::getAllocatedSize(
            m_memory.m_data, static_cast<int32_t>(sizeof(*m_memory.m_data)) * m_capacityAndFlags
        );
        a1->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "memory", m_memory.m_data,
            static_cast<int32_t>(sizeof(*m_memory.m_data)) * m_memory.m_size, AllocatedSize
        );
    }
    int32_t v6 = m_chunks.m_capacityAndFlags;
    if (v6 >= 0 || a1->m_includeUnmanagedArrayMemory) {
        int32_t v7 = hkStatisticsCollector::getAllocatedSize(
            m_chunks.m_data, static_cast<int32_t>(sizeof(*m_chunks.m_data)) * (v6 & 0x3FFFFFFF)
        );
        a1->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "chunks", m_chunks.m_data,
            static_cast<int32_t>(sizeof(*m_chunks.m_data)) * m_chunks.m_size, v7
        );
    }
    int32_t v8 = m_exports.m_capacityAndFlags;
    if (v8 >= 0 || a1->m_includeUnmanagedArrayMemory) {
        int32_t v9 = hkStatisticsCollector::getAllocatedSize(
            m_exports.m_data, static_cast<int32_t>(sizeof(*m_exports.m_data)) * v8
        );
        a1->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "exports", m_exports.m_data,
            static_cast<int32_t>(sizeof(*m_exports.m_data)) * m_exports.m_size, v9
        );
    }
    int32_t v10 = m_imports.m_capacityAndFlags;
    if (v10 >= 0 || a1->m_includeUnmanagedArrayMemory) {
        int32_t v11 = hkStatisticsCollector::getAllocatedSize(
            m_imports.m_data, static_cast<int32_t>(sizeof(*m_imports.m_data)) * v10
        );
        a1->addChunk(
            hkStatisticsCollector::MEM_ARRAY, "imports", m_imports.m_data,
            static_cast<int32_t>(sizeof(*m_imports.m_data)) * m_imports.m_size, v11
        );
    }
}

void hkPackfileData::setName(const char* n) {
    char* m_name = this->m_name;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkMemoryRouter::easyFree(*Instance->m_heap, m_name);
    this->m_name = hkString::strDup(n);
}

void hkPackfileData::setPackfileClassNameRegistry(const hkClassNameRegistry* classReg) {
    if (classReg) {
        classReg->addReference();
    }
    const hkClassNameRegistry* m_pntr = m_packfileClassRegistry.val();
    if (m_pntr) {
        m_pntr->removeReference();
    }
    m_packfileClassRegistry.m_pntr = classReg;
}

// Every export of object is removed, the loop going on after a match. The last element moves into the freed slot.
void hkPackfileData::removeExport(void* object) {
    for (int32_t i = m_exports.getSize() - 1; i >= 0; --i) {
        if (m_exports.m_data[i].data == object) {
            m_exports.removeAt(i);
        }
    }
}

// The last import of location is removed (the last element moves into the freed slot), then the search stops.
void hkPackfileData::removeImport(void** location) {
    for (int32_t i = m_imports.getSize() - 1; i >= 0; --i) {
        if (m_imports.m_data[i].location == location) {
            m_imports.removeAt(i);
            return;
        }
    }
}

int32_t hkPackfileData::finishedObjects() const {
    return m_trackedTypes.m_map.m_numElems != 0;
}

// Does m_exports.expandOne() and fills the new element with the name and the object: the storage grows by one element
// when full, and the size goes up before the stores.
void hkPackfileData::addExport(const char* symbolName, void* object) {
    // EXE-INLINED none: hkArray<hkResource::Export, hkContainerHeapAllocator>::expandOne
    hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
    if (m_exports.m_size == (m_exports.m_capacityAndFlags & hkArrayBase<hkResource::Export>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*pRouter->m_heap, &m_exports, sizeof(hkResource::Export));
    }
    hkResource::Export& rExport = m_exports.m_data[m_exports.m_size++];
    rExport.name = symbolName;
    rExport.data = object;
}

// Does m_imports.expandOne() and fills the new entry with the name and the location, as in addExport.
void hkPackfileData::addImport(const char* symbolName, void** location) {
    // EXE-INLINED none: hkArray<hkResource::Import, hkContainerHeapAllocator>::expandOne
    hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
    if (m_imports.m_size == (m_imports.m_capacityAndFlags & hkArrayBase<hkResource::Import>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*pRouter->m_heap, &m_imports, sizeof(hkResource::Import));
    }
    hkResource::Import& rImport = m_imports.m_data[m_imports.m_size++];
    rImport.name = symbolName;
    rImport.location = location;
}

// The SDK's m_chunks.pushBack(Chunk(p, n, c)).
void hkPackfileData::addChunk(void* p, int32_t n, HK_MEMORY_CLASS c) {
    m_chunks.pushBack(Chunk{p, n, c});
}

void hkPackfileData::setContentsWithName(void* topLevelObject, const char* typeName) {
    m_topLevelObject = topLevelObject;
}
