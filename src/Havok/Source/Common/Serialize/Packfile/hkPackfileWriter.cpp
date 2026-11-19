#include "Havok/Source/Common/Serialize/Packfile/hkPackfileWriter.h"

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
#include "Havok/Source/Common/Base/Reflection/Registry/hkVtableClassRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/OffsetOnly/hkOffsetOnlyStreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Data/Dict/hkDataObjectDict.h"
#include "Havok/Source/Common/Serialize/Data/hkDataArray.h"
#include "Havok/Source/Common/Serialize/Packfile/Binary/hkBinaryPackfileWriter.h"
#include "Havok/Source/Common/Serialize/Serialize/Platform/hkPlatformObjectWriter.h"
#include "Havok/Source/Common/Serialize/Serialize/hkRelocationInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkBuiltinTypeRegistry.h"
#include "Havok/Source/Common/Serialize/Util/hkSerializeMultiMap.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "xfnv/Compiler.h"

#include <new>

// The SDK's getExactClass: klass itself when it has no vtable, else the registry's class of the instance (null
// without a registry).
const hkClass* getExactClass(const void* instance, const hkClass& base, const hkVtableClassRegistry* reg) {
    if (!base.hasVtable().m_bool) {
        return &base;
    }
    if (reg) {
        return reg->getClassFromVirtualInstance(instance);
    }
    return nullptr;
}

// The offset-only writer is destroyed before the object writer.
void chasePointers(const void* pointer, const hkClass& klass, hkRelocationInfo& reloc) {
    hkStructureLayout v3;
    hkPlatformObjectWriter apThis(v3);
    hkOffsetOnlyStreamWriter apMemory;
    apThis.hkPlatformObjectWriter::writeObject(&apMemory, pointer, klass, reloc);
}

hkPackfileWriter::hkPackfileWriter()
  : m_contentsPtrPWIndex(-1), m_contentsClassPWIndex(-1), m_numDataInstances(0), m_numClassInstances(0) {
    m_knownObjects.insert(&hkClassEnumClass, -1);
    m_knownObjects.insert(&hkClassMemberClass, -1);
    m_knownObjects.insert(&hkClassClass, -1);
}

// The section tags it duplicated are freed; the containers free their own storage.
hkPackfileWriter::~hkPackfileWriter() {
    for (char* pSection : m_knownSections) {
        hkMemoryRouter::easyFree(hkMemoryRouter::getInstance().heap(), pSection);
    }
}

// The object's exact class (through the builtin vtable class registry when registry is null) is added with its object,
// then the pending-write indices of both are kept. addObject gets the caller's registry, null included, as the game
// does.
void hkPackfileWriter::setContentsWithRegistry(
    const void* pointer,
    const hkClass& klass,
    const hkVtableClassRegistry* classRegistry,
    hkPackfileWriter::AddObjectListener* addListener
) {
    const hkVtableClassRegistry* pRegistry = classRegistry;
    if (!pRegistry) {
        pRegistry = hkBuiltinTypeRegistry::s_instance->getVtableClassRegistry();
    }
    const hkClass* pClass;
    if (klass.hasVtable().m_bool) {
        pClass = pRegistry->getClassFromVirtualInstance(pointer);
    }
    else {
        pClass = &klass;
    }
    addObject(pointer, *pClass, classRegistry, addListener, sectionHint);
    m_contentsPtrPWIndex = m_knownObjects.getWithDefault(pointer, INDEX_IGNORE);
    m_contentsClassPWIndex = m_knownObjects.getWithDefault(pClass, INDEX_IGNORE);
}

void hkPackfileWriter::setContents(
    const void* object,
    const hkClass& klass,
    hkPackfileWriter::AddObjectListener* addListen
) {
    setContentsWithRegistry(object, klass, hkBuiltinTypeRegistry::s_instance->getVtableClassRegistry(), addListen);
}

// The SDK's addImport: a pending write of object goes to no section, object is filed as imported and its symbol
// recorded.
void hkPackfileWriter::addImport(const void* object, const char* id) {
    const int32_t iIndex = m_knownObjects.getWithDefault(object, INDEX_IGNORE);
    if (iIndex >= 0) {
        m_pendingWrites.m_data[iIndex].m_sectionIndex = INDEX_IMPORT;
    }
    m_knownObjects.insert(object, INDEX_IMPORT);
    m_imports.insert(object, id);
}

// Files object under symbolName for export.
void hkPackfileWriter::addExport(const void* object, const char* id) {
    m_exports.insert(object, id);
}

// sectionTagToIndex adds the section when it is new.
void hkPackfileWriter::addSection(const char* sectionTag) {
    sectionTagToIndex(sectionTag);
}

// Writes ptr into the section tagged sectionTag.
void hkPackfileWriter::setSectionForPointer(const void* ptr, const char* sectionTag) {
    int32_t iSection = sectionTagToIndex(sectionTag);
    m_sectionOverrideByPointer.insert(ptr, iSection);
}

// The SDK's setSectionForClass: the section's index (looked up first) as the override of the class's name.
void hkPackfileWriter::setSectionForClass(const hkClass& k, const char* sectionTag) {
    const int32_t iSection = sectionTagToIndex(sectionTag);
    m_sectionOverrideByType.insert(k.getName(), iSection);
}

// A new tag is duplicated, given the next index and appended to the known sections. The index slot is read only when
// the map's get found the tag and wrote it.
uint32_t hkPackfileWriter::sectionTagToIndex(const char* sectionTag) {
    hkUlong ulIndex;
    if (m_sectionTagToIndex.m_map.get(hkUlong(sectionTag), &ulIndex) == HK_SUCCESS) {
        return static_cast<uint32_t>(ulIndex);
    }
    char* pTag = hkString::strDup(sectionTag);
    const int32_t iIndex = m_knownSections.getSize();
    m_sectionTagToIndex.insert(pTag, iIndex);
    m_knownSections.pushBack(pTag);
    return iIndex;
}

// The pointer's own override first, then the overrides of its class and each ancestor by name, then the hint.
uint32_t hkPackfileWriter::findSectionFor(const void* pointer, const hkClass& klass, const char* sectionTag) {
    int32_t iSection = m_sectionOverrideByPointer.getWithDefault(pointer, -1);
    if (iSection == -1) {
        const hkClass* pClass = &klass;
        iSection = m_sectionOverrideByType.getWithDefault(pClass->getName(), -1);
        if (iSection == -1) {
            while ((pClass = pClass->getParent()) != nullptr) {
                iSection = m_sectionOverrideByType.getWithDefault(pClass->getName(), -1);
                if (iSection != -1) {
                    return iSection;
                }
            }
            iSection = sectionTagToIndex(sectionTag);
        }
    }
    return iSection;
}

// Each object goes in once: the listener may swap it or its class first, its pointers are chased with a relocation
// record and every target added in turn (sharing the object's section when the fixup is related), then its class
// record. An object whose exact class is unregistered is warned about, filed as unwritable (-1) and remembered.
void hkPackfileWriter::addObject(
    const void* pointerIn,
    const hkClass& klassIn,
    const hkVtableClassRegistry* classRegistry,
    AddObjectListener* addListener,
    const char* sectionTag
) {
    if (m_knownObjects.isValid(m_knownObjects.findKey(pointerIn))) {
        return;
    }
    const hkClass* pExactClass = getExactClass(pointerIn, klassIn, classRegistry);
    if (pExactClass == nullptr) {
        char acBuf[0x200];
        alignas(hkErrStream) uint8_t aucStream[sizeof(hkErrStream)];
        hkErrStream* pStream = ::new (aucStream) hkErrStream(acBuf, sizeof(acBuf));
        // The name is fetched before the first operator<<.
        const char* pName = klassIn.getName();
        *pStream << "Found an un-registered class derived from " << pName
                 << ". Derived class will not be serialized unless added to class registry.\n"
                 << "All saved pointers to this object will be set to NULL.\n"
                 << "Saved file will not generate warnings (or asserts) on load but NULL pointers may cause "
                    "runtime crashes.";
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAABBA), acBuf, ".\\Packfile\\hkPackfileWriter.cpp", 289
        );
        pStream->~hkErrStream();
        m_knownObjects.insert(pointerIn, -1);
        const hkVariant kEntry = {const_cast<void*>(pointerIn), &klassIn};
        m_objectsWithUnregisteredClass.pushBack(kEntry);
        return;
    }
    const void* pObject = pointerIn;
    const hkClass* pClass = pExactClass;
    if (addListener != nullptr) {
        addListener->addObjectCallback(pObject, pClass);
    }
    if (pObject == nullptr) {
        m_knownObjects.insert(pointerIn, -1);
        return;
    }
    if (!notDuplicateMetaData(pObject, pClass)) {
        return;
    }
    if (pointerIn != pObject && m_knownObjects.isValid(m_knownObjects.findKey(pObject))) {
        return;
    }
    m_knownObjects.insert(pointerIn, m_pendingWrites.getSize());
    if (pObject != pointerIn) {
        m_knownObjects.insert(pObject, m_pendingWrites.getSize());
        m_replacements.insert(pObject, pointerIn);
    }
    addPendingWrite(pObject, *pClass, pointerIn, klassIn, sectionTag);

    hkRelocationInfo kRelocs;
    kRelocs.~hkRelocationInfo();
    ::new (&kRelocs) hkRelocationInfo;
    chasePointers(pObject, *pClass, kRelocs);
    for (const hkRelocationInfo::Global& kGlobal : kRelocs.m_global) {
        const hkClass* pToClass = kGlobal.m_toClass;
        void* pToAddress = kGlobal.m_toAddress;
        if (pToClass == nullptr || pToAddress == nullptr) {
            continue;
        }
        if (kGlobal.m_related != 0 &&
            !m_sectionOverrideByPointer.isValid(m_sectionOverrideByPointer.findKey(pToAddress))) {
            int32_t iSection = m_sectionOverrideByPointer.getWithDefault(pObject, sectionTagToIndex(sectionTag));
            m_sectionOverrideByPointer.insert(pToAddress, iSection);
        }
        addObject(pToAddress, *pToClass, classRegistry, addListener, sectionTag);
        if (pObject != pToAddress) {
            int32_t iPwIndex = m_knownObjects.getWithDefault(pToAddress, -1);
            if (iPwIndex != -1) {
                m_pwIndexesFromReferencedPointer.insert(pObject, iPwIndex);
            }
        }
    }
    addObject(pClass, hkClassClass, classRegistry, addListener, SECTION_TAG_TYPES);
}

uint8_t hkPackfileWriter::addPendingWrite(
    const void* pointer,
    const hkClass& klass,
    const void* pointerIn,
    const hkClass& klassIn,
    const char* sectionTag
) {
    uint32_t SectionFor = findSectionFor(pointerIn, klassIn, sectionTag);
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (m_pendingWrites.m_size ==
        (m_pendingWrites.m_capacityAndFlags & hkArrayBase<hkPackfileWriter::PendingWrite>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, &m_pendingWrites, sizeof(hkPackfileWriter::PendingWrite));
    }
    hkPackfileWriter::PendingWrite* v8 = &m_pendingWrites.m_data[m_pendingWrites.m_size];
    if (v8) {
        v8->m_pointer = pointer;
        v8->m_klass = &klass;
        v8->m_origPointer = pointerIn;
        v8->m_origClass = &klassIn;
        v8->m_sectionIndex = SectionFor;
        v8->m_dataSize = -1;
    }
    ++m_pendingWrites.m_size;
    int32_t v9 = &klass == &hkClassClass;
    m_numClassInstances += v9;
    m_numDataInstances += &klass != &hkClassClass;
    return v9;
}

// The SDK's notDuplicateMetaData: a class record or enum is new (1) unless one of its name is known already; then
// pointer is filed as that one (the known one's pending write and a replacement) and 0 returned. A class record of
// another signature is reported as an error first.
int32_t hkPackfileWriter::notDuplicateMetaData(const void* pointer, const hkClass* klass) {
    if (hkString::strCmp(klass->getName(), "hkClass") == 0) {
        const hkClass* pClass = static_cast<const hkClass*>(pointer);
        const hkClass* pKnownClass = m_knownClasses.getWithDefault(pClass->getName(), nullptr);
        if (pKnownClass == nullptr) {
            m_knownClasses.insert(pClass->getName(), pClass);
            return 1;
        }
        const uint32_t uiKnownSignature = pKnownClass->getSignature(0);
        if (pClass->getSignature(0) != uiKnownSignature) {
            char acBuffer[0x200];
            hkErrStream errStream(acBuffer, 0x200);
            errStream << "Conflicting metadata found for class " << pClass->getName()
                      << ". Perhaps you have called setContents on data which has not been updated to the current "
                         "version.";
            if (hkError::s_instance->message(
                    hkError::MESSAGE_ERROR, 0x2518721C, acBuffer, ".\\Packfile\\hkPackfileWriter.cpp", 178
                )) {
                xfnv::DebugTrap();
            }
        }
        m_knownObjects.insert(pointer, m_knownObjects.getWithDefault(pKnownClass, INDEX_IGNORE));
        m_replacements.insert(pointer, pKnownClass);
        return 0;
    }
    if (hkString::strCmp(klass->getName(), "hkClassEnum") == 0) {
        const hkClassEnum* pEnum = static_cast<const hkClassEnum*>(pointer);
        const hkClassEnum* pKnownEnum = m_knownEnums.getWithDefault(pEnum->m_name, nullptr);
        if (pKnownEnum == nullptr) {
            m_knownEnums.insert(pEnum->m_name, pEnum);
            return 1;
        }
        m_knownObjects.insert(pointer, m_knownObjects.getWithDefault(pKnownEnum, INDEX_IGNORE));
        m_replacements.insert(pointer, pKnownEnum);
        return 0;
    }
    return 1;
}

void hkPackfileWriter::getCurrentVersion(char* buf, int32_t bufLen) {
    hkString::snprintf(buf, bufLen, "Havok-%i.%i.%i-%s", 7, 1, 0, "r1");
}

// The listener interface adds nothing to destroy.
hkPackfileWriter::AddObjectListener::~AddObjectListener() {}
