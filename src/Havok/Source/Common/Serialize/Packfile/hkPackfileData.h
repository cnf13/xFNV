#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Memory/MemoryClasses/hkMemoryClasses.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Serialize/Resource/hkResource.h"

class hkClassNameRegistry;
class hkStatisticsCollector;
class hkTypeInfoRegistry;

// Whether topLevelClassName's class (looked up with getClassByName) is typeName's class or a superclass of it
// (hkClass::isSuperClass).
bool baseOrSameClass(const char* typeName, const char* topLevelClassName, const hkClassNameRegistry* classReg);

// Owns the allocations, imports, exports and type metadata of a loaded packfile.
class hkPackfileData : public hkResource {
public:
    struct Chunk;

    // Empty, with destructors enabled and the builtin type registry's class registry.
    hkPackfileData();

    ~hkPackfileData() override;

    const char* getName() const override {
        return m_name;
    }
    void callDestructors() override;
    // Havok SDK signature (hkSerialize.lib, tied by bytes,vtable).
    void getImportsExports(
        hkArray<hkResource::Import, hkContainerHeapAllocator>& impOut,
        hkArray<hkResource::Export, hkContainerHeapAllocator>& expOut
    ) const override;
    // Havok SDK signature (hkSerialize.lib, tied by bytes,vtable).
    void* getContentsPointer(const char* typeName, const hkTypeInfoRegistry* typeRegistry) const override;
    const char* getContentsTypeName() const override;
    // Havok SDK signature (hkSerialize.lib, tied by bytes,vtable).
    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;

    void setName(const char* n);
    void setPackfileClassNameRegistry(const hkClassNameRegistry* classReg);
    void removeExport(void* object);
    void removeImport(void** location);
    // Whether the tracked objects have been finished: the type map is not empty.
    int32_t finishedObjects() const;
    void addExport(const char* symbolName, void* object);
    void addImport(const char* symbolName, void** location);
    // Appends the chunk {p, n, c} to m_chunks.
    void addChunk(void* p, int32_t n, HK_MEMORY_CLASS c);
    void setContentsWithName(void* topLevelObject, const char* typeName);

    void* m_topLevelObject;
    char* m_name;
    hkPointerMap<void*, char const*, hkContainerHeapAllocator> m_trackedObjects;
    hkStringMap<hkTypeInfo const*, hkContainerHeapAllocator> m_trackedTypes;
    hkRefPtr<hkClassNameRegistry const> m_packfileClassRegistry;
    int32_t m_destructorsEnabled;
    hkArray<void*, hkContainerHeapAllocator> m_memory;
    hkArray<Chunk, hkContainerHeapAllocator> m_chunks;
    hkArray<hkResource::Export, hkContainerHeapAllocator> m_exports;
    hkArray<hkResource::Import, hkContainerHeapAllocator> m_imports;
};

// A block the packfile data owns: freed with its byte count and memory class.
struct hkPackfileData::Chunk {
    void* pointer;
    int32_t numBytes;
    HK_MEMORY_CLASS memClass;
};
