#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkStringBuf;
class hkVtableClassRegistry;

// Walks objects and reports the memory they use. Only part of the class is declared: the templates (addArray,
// addSmallArray, addPointerMap ...) and the inline getVtblClassRegistry are not.
class hkStatisticsCollector : public hkReferencedObject {
public:
    // Values from the SDK.
    enum MemoryType : int32_t {
        MEM_OBJECT = 0x0,
        MEM_NORMAL = 0x1,
        MEM_ALLOCATED = 0x2,
        MEM_ARRAY = 0x3,
        MEM_OTHER = 0x4,
        MEM_LAST = 0x5,
    };

    // Values from the SDK.
    enum Flag : int32_t {
        EXCLUDE_OBJECT_MEMORY = 0x1,
    };

    hkStatisticsCollector(hkVtableClassRegistry* vtblReg);
    ~hkStatisticsCollector() override;

    // The flags and allocatedSize arguments have no default; callers pass them.
    virtual void addReferencedObject(const char* fieldName, const hkReferencedObject* obj, int32_t flags) = 0;
    virtual void addObject(const hkClass& cls, const char* fieldName, const void* obj, int32_t flags) = 0;
    virtual void
    addChunk(MemoryType type, const char* name, const void* chunkAddress, int32_t usedSize, int32_t allocatedSize) = 0;
    virtual void pushDir(const char* dirName) = 0;
    virtual void popDir() = 0;

    void addNormalChunk(const char* name, const void* p, int32_t size, int32_t allocated);
    void addObjectChunk(const char* name, const void* p, int32_t size, int32_t allocated);
    hkVtableClassRegistry* getVtblClassRegistry() const {
        return m_vtblRegistry;
    }
    int32_t getReferencedObjectSize(const hkReferencedObject* obj);
    // Reports the string s owns under name as an array chunk.
    void addStringPtr(const char* name, const hkStringPtr& str);
    // The object's class: its own getClassType, else the vtable registry's.
    const hkClass* findClassType(const hkReferencedObject* obj);
    void addReferencedObjectMemory(const hkReferencedObject* obj, int32_t flags);
    void addObjectMemory(const hkClass& cls, const void* obj, int32_t flags);

    static int32_t getAllocatedSize(const void* p, int32_t size);
    static const char* memoryTypeToCstring(MemoryType type);
    // Writes size as text into textOut: bytes below 1 KB, tenths of a KB below 10 KB, whole KB below 1 MB, else
    // hundredths of a MB.
    static void memorySizeToText(int32_t size, hkStringBuf& string);
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

    const void* m_clientData;
    hkVtableClassRegistry* m_vtblRegistry;
    bool m_includeUnmanagedArrayMemory;
};
