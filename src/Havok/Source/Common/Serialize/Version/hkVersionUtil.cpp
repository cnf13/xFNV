#include "Havok/Source/Common/Serialize/Version/hkVersionUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMemberAccessor.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Buffered/hkBufferedStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Copier/hkObjectCopier.h"
#include "Havok/Source/Common/Serialize/Serialize/hkRelocationInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "Havok/Source/Common/Serialize/Version/hkObjectUpdateTracker.h"
#include "Havok/Source/Common/Serialize/Version/hkVersionPatchManager.h"

// The bits the in-place metadata update keeps per class in its done map: which steps have updated the class.
enum MetadataUpdateFlags : uint32_t {
    UPDATED_VERSION_1 = 0x1,
    UPDATED_CHAR_POINTER_TO_CSTRING = 0x2,
    UPDATED_CONSTRAINT_INSTANCE_300 = 0x4,
    UPDATED_ZERO_MEMBER_TO_IGNORED = 0x8,
};

// The member fix-up updateMetadataInplace's UPDATED_CHAR_POINTER_TO_CSTRING step applies to each member (through
// updateMembersRecursive's void (*)(hkClassMember*) callback): rewrites m_type 20 with m_subtype 2 as type 29.
static void convertCharPointerToCstring(hkClassMember* member);
// The UPDATED_ZERO_MEMBER_TO_IGNORED member fix-up, the same way: unpacks an old type 19 into m_type/m_subtype, sets
// SERIALIZE_IGNORED in m_flags, and moves the enum/flags size bits of m_flags into m_subtype.
static void convertZeroMemberToIgnored(hkClassMember* member);
static void updateMetadataInplace(hkClass** c, int32_t sourceVersion);
// The SDK's per-version metadata walk of hkClass::updateMetadataInplace: once per class for flag bit (done is the
// storage map of hkPointerMap<const hkClass*, hkInt32>), walks the classes of klass's declared members, applies fixup
// to each declared member, then goes on with the parent. Returns the last flags read, or 0 at the root.
static uint32_t
updateMembersRecursive(hkClass* klass, hkUlongMapBase* done, int32_t bit, void (*fixup)(hkClassMember*));
// The version 1 step of hkClass::updateMetadataInplace, once per class (UPDATED_VERSION_1 in done): clears the old
// record's "hasVtable" in each ancestor of klass, counts it as an implemented interface in the root class, walks the
// struct classes of klass's declared members, then goes on with the parent. Returns the last flags read, or 0 at the
// root.
static uint32_t updateClassVersion1Inplace(hkClass* klass, hkUlongMapBase* done);
static void updateClassVersionInplace(hkClass* c, hkUlongMapBase* updatedAlready, int32_t sourceVersion);
// The version 1 member fix-up of hkClass::updateMetadataInplace for one class (klass: the class, done: the map of
// classes done).
static void updateConstraintInstance300(hkClass* klass, hkUlongMapBase* done);
// hkVersionUtil::copyObjects' worker: objectsIn, objectsOut, the tracker, and indices, the indices of the objects to
// copy; returns the new chunk holding the copies, or null.
static void* copyObjectsToChunk(
    const hkArrayBase<hkVariant>* objectsIn,
    hkArrayBase<hkVariant>* objectsOut,
    hkObjectUpdateTracker* tracker,
    const hkArrayBase<int32_t>* indices
);

static void updateMetadataInplace(hkClass** c, int32_t sourceVersion) {
    hkUlongMapBase ErrorMessage;
    ErrorMessage.m_elem = nullptr;
    ErrorMessage.m_numElems = 0;
    ErrorMessage.m_hashMod = -1;
    ErrorMessage.reserve(*hkMemoryRouter::getInstance().m_heap, 0);
    hkClass* v3 = *c;
    for (int32_t i = 0; v3; ++i) {
        updateClassVersionInplace(v3, &ErrorMessage, sourceVersion);
        v3 = c[i + 1];
    }
    ErrorMessage.clearAndDeallocate(*hkMemoryRouter::getInstance().m_heap);
}

// Copies the objects at the indices listed in indices, each from objectsIn as its class in objectsOut, into one new
// heap chunk tracker owns (memory class 5): a packfile copier with the host layout writes them to a buffer, which then
// moves to the chunk, the copies' local pointers fixed in place and their global ones reported to the tracker. Until
// the chunk exists, each copy's m_object in objectsOut holds its offset in the buffer. Returns the chunk, or null when
// nothing was written.
static void* copyObjectsToChunk(
    const hkArrayBase<hkVariant>* objectsIn,
    hkArrayBase<hkVariant>* objectsOut,
    hkObjectUpdateTracker* tracker,
    const hkArrayBase<int32_t>* indices
) {
    hkArray<char> buffer;
    hkOstream out(buffer);
    hkRelocationInfo reloc;
    hkPointerMap<void*, int32_t> offsets;
    for (int32_t index : *indices) {
        const hkVariant& oldVariant = objectsIn->m_data[index];
        hkStructureLayout layoutOut(hkStructureLayout::HostLayoutRules);
        hkStructureLayout layoutIn(hkStructureLayout::HostLayoutRules);
        PackfileObjectCopier copier(layoutIn, layoutOut);
        hkVariant& newVariant = objectsOut->m_data[index];
        const hkClass* pNewClass = newVariant.m_class;
        // The source's (void*)(hkUlong) offset: the object pointer holds a buffer offset until the chunk exists.
        newVariant.m_object = reinterpret_cast<void*>(static_cast<uintptr_t>(buffer.m_size));
        offsets.insert(oldVariant.m_object, buffer.m_size);
        copier.hkObjectCopier::copyObject(
            oldVariant.m_object,
            *oldVariant.m_class,
            out.getStreamWriter(),
            *pNewClass,
            reloc
        );
    }
    int32_t size = buffer.m_size;
    hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
    char* pChunk = nullptr;
    if (size) {
        pChunk = static_cast<char*>(pRouter->m_heap->blockAlloc(size));
        tracker->addChunk(pChunk, buffer.m_size, 5);
        hkString::memCpy(pChunk, buffer.m_data, buffer.m_size);
        for (int32_t index : *indices) {
            hkVariant& variant = objectsOut->m_data[index];
            // The buffer offset the pointer holds becomes the copy's address in the chunk.
            variant.m_object = pChunk + reinterpret_cast<uintptr_t>(variant.m_object);
        }
        for (const hkRelocationInfo::Local& local : reloc.m_local) {
            // The chunk is untyped copied data: a pointer slot at a byte offset.
            *reinterpret_cast<void**>(pChunk + local.m_fromOffset) = pChunk + local.m_toOffset;
        }
        for (const hkRelocationInfo::Global& global : reloc.m_global) {
            tracker->objectPointedBy(global.m_toAddress, pChunk + global.m_fromOffset);
        }
    }
    return pChunk;
}

namespace hkVersionUtil {
    // Writes the new class's default value of every member the old class does not have, at the member's offset. The
    // writer covers the object itself, so the defaults land in it.
    void copyDefaults(void* obj, const hkClass& oldClass, const hkClass& newClass) {
        alignas(hkBufferedStreamWriter) uint8_t aWriterMemory[sizeof(hkBufferedStreamWriter)];
        hkBool memoryIsString;
        memoryIsString.m_bool = 0;
        hkBufferedStreamWriter* pWriter = ::new (aWriterMemory)
            hkBufferedStreamWriter(obj, newClass.getObjectSize(), memoryIsString);
        for (int32_t i = 0; i < newClass.getNumMembers(); ++i) {
            const hkClassMember& member = newClass.getMember(i);
            if (!oldClass.getMemberByName(member.m_name)) {
                pWriter->hkBufferedStreamWriter::seek(member.m_offset, hkStreamWriter::STREAM_SET);
                newClass.getDefault(i, pWriter);
            }
        }
        pWriter->~hkBufferedStreamWriter();
    }
} // namespace hkVersionUtil

static void convertCharPointerToCstring(hkClassMember* member) {
    if (member->m_type.m_storage == hkClassMember::TYPE_POINTER &&
        member->m_subtype.m_storage == hkClassMember::TYPE_CHAR) {
        member->m_type.m_storage = hkClassMember::TYPE_CSTRING;
        member->m_subtype.m_storage = hkClassMember::TYPE_VOID;
    }
}

static void convertZeroMemberToIgnored(hkClassMember* member) {
    if (member->m_type.m_storage == hkClassMember::TYPE_ZERO) {
        member->m_type.m_storage = member->m_subtype.m_storage;
        member->m_subtype.m_storage = hkClassMember::TYPE_VOID;
        member->m_flags.m_storage |= hkClassMember::SERIALIZE_IGNORED;
    }
    uint8_t m_storage = member->m_type.m_storage;
    if (m_storage == hkClassMember::TYPE_ENUM || m_storage == hkClassMember::TYPE_FLAGS) {
        if ((member->m_flags.get() & hkClassMember::DEPRECATED_ENUM_8) != 0) {
            member->m_subtype.m_storage = hkClassMember::TYPE_UINT8;
            member->m_flags.m_storage &= ~hkClassMember::DEPRECATED_ENUM_8;
        }
        if ((member->m_flags.get() & hkClassMember::DEPRECATED_ENUM_16) != 0) {
            member->m_subtype.m_storage = hkClassMember::TYPE_UINT16;
            member->m_flags.m_storage &= ~hkClassMember::DEPRECATED_ENUM_16;
        }
        if ((member->m_flags.get() & hkClassMember::DEPRECATED_ENUM_32) != 0) {
            member->m_subtype.m_storage = hkClassMember::TYPE_UINT32;
            member->m_flags.m_storage &= ~hkClassMember::DEPRECATED_ENUM_32;
        }
    }
}

static uint32_t
updateMembersRecursive(hkClass* klass, hkUlongMapBase* done, int32_t bit, void (*fixup)(hkClassMember*)) {
    using UpdateFlagFromClassMap = hkPointerMap<const hkClass*, int32_t>;
    hkClass* pClass = klass;
    uint32_t uiFlags = done->getWithDefault(UpdateFlagFromClassMap::ToMapInt(pClass), 0);
    // The SDK's tail call for the parent class, made a loop by the compiler.
    while ((bit & uiFlags) == 0) {
        done->insert(*hkMemoryRouter::getInstance().m_heap, UpdateFlagFromClassMap::ToMapInt(pClass), uiFlags | bit);
        for (int32_t i = 0; i < pClass->getNumDeclaredMembers(); ++i) {
            const hkClassMember& kMember = pClass->getDeclaredMember(i);
            const hkClass* pMemberClass = kMember.getClass();
            // The SDK updates the class records in place through const_casts.
            if (pMemberClass) {
                updateMembersRecursive(const_cast<hkClass*>(pMemberClass), done, bit, fixup);
            }
            fixup(const_cast<hkClassMember*>(&kMember));
        }
        if (!pClass->getParent()) {
            return 0;
        }
        pClass = pClass->getParent();
        uiFlags = done->getWithDefault(UpdateFlagFromClassMap::ToMapInt(pClass), 0);
    }
    return uiFlags;
}

static uint32_t updateClassVersion1Inplace(hkClass* klass, hkUlongMapBase* done) {
    using UpdateFlagFromClassMap = hkPointerMap<const hkClass*, int32_t>;
    hkClass* pClass = klass;
    uint32_t uiFlags = done->getWithDefault(UpdateFlagFromClassMap::ToMapInt(pClass), 0);
    // The SDK's tail call for the parent class, made a loop by the compiler.
    while ((uiFlags & UPDATED_VERSION_1) == 0) {
        done->insert(
            *hkMemoryRouter::getInstance().m_heap,
            UpdateFlagFromClassMap::ToMapInt(pClass),
            uiFlags | UPDATED_VERSION_1
        );
        // The fields are written as raw bytes at the offsets the old hkClass record hkClassVersion1Class gives (the
        // class records are updated in place through reflection): a 32-bit value cleared, a byte tested, a 32-bit value
        // incremented.
        int32_t iHasVtable = hkClassVersion1Class.getMemberByName("hasVtable")->getOffset();
        hkClass* pRoot = pClass;
        while (pRoot->getParent()) {
            *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pRoot) + iHasVtable) = 0;
            pRoot = pRoot->getParent();
        }
        if (reinterpret_cast<uint8_t*>(pRoot)[iHasVtable]) {
            int32_t iInterfaces = hkClassVersion1Class.getMemberByName("numImplementedInterfaces")->getOffset();
            ++*reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pRoot) + iInterfaces);
        }
        *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pRoot) + iHasVtable) = 0;
        for (int32_t i = 0; i < pClass->getNumDeclaredMembers(); ++i) {
            const hkClassMember& kMember = pClass->getDeclaredMember(i);
            if (kMember.m_class) {
                // The SDK updates the class records in place through const_casts.
                updateClassVersion1Inplace(const_cast<hkClass*>(&kMember.getStructClass()), done);
            }
        }
        if (!pClass->getParent()) {
            return 0;
        }
        pClass = pClass->getParent();
        uiFlags = done->getWithDefault(UpdateFlagFromClassMap::ToMapInt(pClass), 0);
    }
    return uiFlags;
}

static void updateClassVersionInplace(hkClass* c, hkUlongMapBase* updatedAlready, int32_t sourceVersion) {
    if (sourceVersion == 1) {
        updateClassVersion1Inplace(c, updatedAlready);
        updateConstraintInstance300(c, updatedAlready);
    }
    if (sourceVersion < 4) {
        updateMembersRecursive(c, updatedAlready, UPDATED_CHAR_POINTER_TO_CSTRING, &convertCharPointerToCstring);
    }
    if (sourceVersion < 5) {
        updateMembersRecursive(c, updatedAlready, UPDATED_ZERO_MEMBER_TO_IGNORED, &convertZeroMemberToIgnored);
    }
}

// The version 1 step of hkClass::updateMetadataInplace for klass: once per class (UPDATED_CONSTRAINT_INSTANCE_300 in
// done, a hkPointerMap<const hkClass*, hkInt32>), and for hkpConstraintInstance only, sets the field of its declared
// member 2 at the offset of hkClassMemberClass's member 4 (read through reflection) to TYPE_STRUCT.
static void updateConstraintInstance300(hkClass* klass, hkUlongMapBase* done) {
    using UpdateFlagFromClassMap = hkPointerMap<const hkClass*, int32_t>;
    uint32_t uiFlags = done->getWithDefault(UpdateFlagFromClassMap::ToMapInt(klass), 0);
    if ((uiFlags & UPDATED_CONSTRAINT_INSTANCE_300) == 0) {
        done->insert(
            *hkMemoryRouter::getInstance().m_heap,
            UpdateFlagFromClassMap::ToMapInt(klass),
            uiFlags | UPDATED_CONSTRAINT_INSTANCE_300
        );
        if (!hkString::strCmp(klass->getName(), "hkpConstraintInstance")) {
            const hkClassMember& kMember = klass->getDeclaredMember(2);
            const hkClassMember& kField = static_cast<const hkClass&>(hkClassMemberClass).getMember(4);
            // Writes the field as raw bytes at the offset reflection gives (an untyped store into the class's own const
            // member data), as the reflected member accessors do.
            reinterpret_cast<uint8_t*>(const_cast<hkClassMember*>(&kMember))[kField.m_offset] = static_cast<uint8_t>(
                hkClassMember::TYPE_STRUCT
            );
        }
    }
}

void hkVersionUtil::renameMember(hkVariant& oldObj, const char* oldName, hkVariant& newObj, const char* newName) {
    hkClassMemberAccessor apMemory(oldObj, oldName);
    hkClassMemberAccessor v9(newObj, newName);
    if (apMemory.isOk().m_bool && v9.isOk().m_bool) {
        const hkClassMember* ClassMember = &v9.getClassMember();
        int32_t SizeInBytes = ClassMember->getSizeInBytes();
        void* v6 = apMemory.asRaw();
        void* v5 = v9.asRaw();
        hkString::memCpy(v5, v6, SizeInBytes);
    }
}

char* hkVersionUtil::getCurrentVersion() {
    return const_cast<char*>("Havok-7.1.0-r1");
}

// Copies every object of objectsIn, by index, into one chunk the tracker owns.
void* hkVersionUtil::copyObjects(
    const hkArrayBase<hkVariant>& objectsIn,
    hkArray<hkVariant, hkContainerHeapAllocator>& objectsOut,
    hkObjectUpdateTracker& tracker
) {
    int32_t numObjects = objectsIn.m_size;
    hkArray<int32_t, hkContainerTempAllocator> indices;
    indices.setSize(numObjects);

    for (int32_t i = 0; i < objectsIn.m_size; ++i) {
        indices.m_data[i] = i;
    }
    void* pResult = copyObjectsToChunk(&objectsIn, &objectsOut, &tracker, &indices);
    return pResult;
}

PackfileObjectCopier::PackfileObjectCopier(const hkStructureLayout& layoutIn, const hkStructureLayout& layoutOut)
  : hkObjectCopier(layoutIn, layoutOut, 0) {}

int32_t PackfileObjectCopier::areMembersCompatible(const hkClassMember& src, const hkClassMember& dst) {
    char srca;
    char dsta;
    uint8_t m_storage = src.m_type.m_storage;
    uint8_t v7 = dst.m_type.m_storage;
    bool v18 = m_storage == v7 || (m_storage == hkClassMember::TYPE_SIMPLEARRAY && v7 == hkClassMember::TYPE_ARRAY);
    if (m_storage == hkClassMember::TYPE_CSTRING && v7 == hkClassMember::TYPE_STRINGPTR) {
        srca = 1;
    }
    else {
        srca = 0;
    }
    dsta = 0;
    if (m_storage == hkClassMember::TYPE_VARIANT &&
        v7 == hkClassMember::TYPE_POINTER &&
        dst.m_subtype.m_storage == hkClassMember::TYPE_STRUCT &&
        dst.getClass()) {
        const hkClass* StructClass = &dst.getStructClass();
        const char* Name = StructClass->getName();
        const char* v9 = hkReferencedObjectClass.getName();
        if (!hkString::strCmp(v9, Name)) {
            dsta = 1;
        }
    }
    uint8_t v10 = src.m_subtype.m_storage;
    uint8_t v11 = dst.m_subtype.m_storage;
    bool v19 = v10 == v11 || (v10 == hkClassMember::TYPE_CSTRING && v11 == hkClassMember::TYPE_STRINGPTR);
    bool v14 = false;
    if ((v10 == hkClassMember::TYPE_VARIANT && v11 == hkClassMember::TYPE_POINTER) && dst.getClass()) {
        const hkClass* v12 = &dst.getStructClass();
        const char* v17 = v12->getName();
        const char* v13 = hkReferencedObjectClass.getName();
        if (!hkString::strCmp(v13, v17)) {
            v14 = true;
        }
    }

    return hkObjectCopier::areMembersCompatible(src, dst) || (srca || (dsta || (v18 && (v19 || v14))));
}

// The classes' metadata is brought up to date in place, then every class's member offsets are laid out for the host,
// each class once. The const_cast is there because updateMetadataInplace updates the classes the array points to.
void hkVersionUtil::recomputeClassMemberOffsets(hkClass* const* classes, int32_t classVersion) {
    updateMetadataInplace(const_cast<hkClass**>(classes), classVersion);
    hkStructureLayout layout;
    hkPointerMap<const hkClass*, int32_t> classesDone;
    hkClass* klass = *classes;
    for (int32_t i = 0; klass; ++i) {
        layout.computeMemberOffsetsInplace(*klass, classesDone, 1);
        klass = classes[i + 1];
    }
}
