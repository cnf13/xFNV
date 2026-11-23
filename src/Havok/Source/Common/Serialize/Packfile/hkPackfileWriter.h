#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkSerializeMultiMap.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"

class hkClassEnum;
class hkRelocationInfo;
class hkStreamWriter;
class hkVtableClassRegistry;

// The class record of instance's most derived class, from its vtable through reg, or null when that class is not
// registered.
const hkClass* getExactClass(const void* instance, const hkClass& base, const hkVtableClassRegistry* reg);
// Adds a global fixup to relocs for each pointer in pointer's members.
void chasePointers(const void* pointer, const hkClass& klass, hkRelocationInfo& reloc);

// Writes a set of objects and their classes as a packfile. Only some of the members are declared.
class hkPackfileWriter : public hkReferencedObject {
public:
    struct AddObjectListener;

    // What save writes the packfile for: the user tag, the destination layout, whether the reflection records go in,
    // and the version string.
    struct Options {
        uint32_t m_userTag;
        hkStructureLayout m_layout;
        hkBool m_writeMetaInfo;
        const char* m_contentsVersion;
    };

    struct PendingWrite;

    // The SDK's pending-write indices with no pending write: not written, imported, failed.
    enum { INDEX_IGNORE = -1, INDEX_IMPORT = -2, INDEX_ERROR = -3 };

    // Empty containers, and the three reflection class records every writer knows.
    hkPackfileWriter();
    ~hkPackfileWriter() override;

    virtual void setContentsWithRegistry(
        const void* pointer,
        const hkClass& klass,
        const hkVtableClassRegistry* classRegistry,
        AddObjectListener* addListener
    );
    virtual void setContents(const void* object, const hkClass& klass, AddObjectListener* addListen);
    virtual void addImport(const void* object, const char* id);
    virtual void addExport(const void* object, const char* id);
    virtual hkResult save(hkStreamWriter* stream, const Options& options) = 0;
    virtual void addSection(const char* sectionTag);
    virtual void setSectionForPointer(const void* ptr, const char* sectionTag);
    virtual void setSectionForClass(const hkClass& k, const char* sectionTag);

protected:
    // The index of the section tagged key, added when new.
    uint32_t sectionTagToIndex(const char* sectionTag);
    // The section of pointer: its override, else its class's or an ancestor's override, else sectionHint's index.
    uint32_t findSectionFor(const void* pointer, const hkClass& klass, const char* sectionTag);
    // Adds pointer, its exact class's record and everything it points at to the pending writes, once each.
    void addObject(
        const void* pointerIn,
        const hkClass& klassIn,
        const hkVtableClassRegistry* classRegistry,
        AddObjectListener* addListener,
        const char* sectionTag
    );
    // Queues pointer (originally origPointer of class origClass) for writing.
    uint8_t addPendingWrite(
        const void* pointer,
        const hkClass& klass,
        const void* pointerIn,
        const hkClass& klassIn,
        const char* sectionTag
    );
    // Whether pointer is not class metadata already written; callers test the whole int32_t return value.
    int32_t notDuplicateMetaData(const void* pointer, const hkClass* klass);

public:
    static void getCurrentVersion(char* buf, int32_t bufLen);

    hkArray<PendingWrite> m_pendingWrites;
    // Each known object's index into m_pendingWrites, or -1 for objects not written.
    hkPointerMap<const void*, int32_t> m_knownObjects;
    // The imported objects and their symbols.
    hkPointerMap<const void*, const char*> m_imports;
    hkPointerMap<const void*, const char*> m_exports;
    // The classes and enums named so far, by name.
    hkStringMap<const hkClass*> m_knownClasses;
    hkStringMap<const hkClassEnum*> m_knownEnums;
    // The objects the add listener replaced, by their replacement.
    hkPointerMap<const void*, const void*> m_replacements;
    // The section tags added so far, and each one's index.
    hkArray<char*> m_knownSections;
    hkStringMap<int32_t> m_sectionTagToIndex;
    hkArray<hkVariant> m_objectsWithUnregisteredClass;
    int32_t m_contentsPtrPWIndex;
    int32_t m_contentsClassPWIndex;
    int32_t m_numDataInstances;
    int32_t m_numClassInstances;
    hkPointerMap<const void*, int32_t> m_sectionOverrideByPointer;
    hkStringMap<int32_t> m_sectionOverrideByType;
    hkSerializeMultiMap<const void*, int32_t> m_pwIndexesFromReferencedPointer;

    // hkPackfileWriter::SECTION_TAG_TYPES: the section tag of the class records.
    static inline const char* SECTION_TAG_TYPES = "__types__";
};

// What setContents calls for every object it adds, so the caller may swap the object or its class first. The class is
// abstract: addObjectCallback is pure.
struct hkPackfileWriter::AddObjectListener : public hkReferencedObject {
    ~AddObjectListener() override;

    virtual void addObjectCallback(const void*& objP, const hkClass*& klassP) = 0;
};

// An object queued for writing: the object and its class as written, the original object and class, its section and its
// data size. addPendingWrite fills it.
struct hkPackfileWriter::PendingWrite {
    const void* m_pointer;
    const hkClass* m_klass;
    const void* m_origPointer;
    const hkClass* m_origClass;
    int32_t m_sectionIndex;
    int32_t m_dataSize;
};

inline char* sectionTag = const_cast<char*>("__classnames__");
inline char* sectionHint = const_cast<char*>("__data__");
