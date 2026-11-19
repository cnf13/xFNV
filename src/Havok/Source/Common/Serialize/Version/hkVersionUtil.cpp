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

// The member fix-up updateMetadataInplace's version 2 step applies to each member (through updateMembersRecursive's
// void (*)(hkClassMember*) callback): rewrites m_type 20 with m_subtype 2 as type 29.
static void convertCharPointerToCstring(hkClassMember* member);
// The version 8 member fix-up, the same way: unpacks an old type 19 into m_type/m_subtype, flag 0x400 in m_flags
// (+0x10), and moves the enum/flags size bits of m_flags into m_subtype.
static void convertZeroMemberToIgnored(hkClassMember* member);
static void updateMetadataInplace(hkClass** a1, int32_t a2);
// The SDK's per-version metadata walk of hkClass::updateMetadataInplace: once per class for flag bit (done is the
// storage map of hkPointerMap<const hkClass*, hkInt32>), walks the classes of klass's declared members, applies fixup
// to each declared member, then goes on with the parent. Returns the last flags read, or 0 at the root.
static uint32_t
updateMembersRecursive(hkClass* klass, hkUlongMapBase* done, int32_t bit, void (*fixup)(hkClassMember*));
// The version 1 step of hkClass::updateMetadataInplace, once per class (flag 1 in done): clears the old record's
// "hasVtable" in each ancestor of klass, counts it as an implemented interface in the root class, walks the struct
// classes of klass's declared members, then goes on with the parent. Returns the last flags read, or 0 at the root.
static uint32_t updateClassVersion1Inplace(hkClass* klass, hkUlongMapBase* done);
static void updateClassVersionInplace(hkClass* a1, hkUlongMapBase* a2, int32_t a3);
// The version 1 member fix-up of hkClass::updateMetadataInplace for one class (r_eax: the class, r_edi: the map of
// classes done).
static void updateConstraintInstance300(hkClass* r_eax, hkUlongMapBase* r_edi);
// hkVersionUtil::copyObjects' worker: a1 objectsIn, a2 objectsOut, a3 the tracker, r_eax the indices of the objects to
// copy; returns the new chunk holding the copies, or null.
static void* copyObjectsToChunk(
    const hkArrayBase<hkVariant>* a1,
    hkArrayBase<hkVariant>* a2,
    hkObjectUpdateTracker* a3,
    const hkArrayBase<int32_t>* r_eax
);

static void updateMetadataInplace(hkClass** a1, int32_t a2) {
    hkUlongMapBase ErrorMessage;
    ErrorMessage.m_elem = nullptr;
    ErrorMessage.m_numElems = 0;
    ErrorMessage.m_hashMod = -1;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    ErrorMessage.reserve(*Instance->m_heap, 0);
    hkClass* v3 = *a1;
    for (int32_t i = 0; v3; ++i) {
        updateClassVersionInplace(v3, &ErrorMessage, a2);
        v3 = a1[i + 1];
    }
    hkMemoryRouter* v5 = &hkMemoryRouter::getInstance();
    ErrorMessage.clearAndDeallocate(*v5->m_heap);
}

// Copies the objects at the indices r_eax lists, each from a1 as its class in a2, into one new heap chunk the tracker
// a3 owns (memory class 5): a packfile copier with the host layout writes them to a buffer, which then moves to the
// chunk, the copies' local pointers fixed in place and their global ones reported to the tracker. Until the chunk
// exists, each copy's m_object in a2 holds its offset in the buffer. Returns the chunk, or null when nothing was
// written.
static void* copyObjectsToChunk(
    const hkArrayBase<hkVariant>* a1,
    hkArrayBase<hkVariant>* a2,
    hkObjectUpdateTracker* a3,
    const hkArrayBase<int32_t>* r_eax
) {
    hkArray<char> buffer;
    hkOstream out(buffer);
    hkRelocationInfo reloc;
    hkPointerMap<void*, int32_t> offsets;
    for (int32_t index : *r_eax) {
        const hkVariant& oldVariant = a1->m_data[index];
        hkStructureLayout layoutOut(hkStructureLayout::HostLayoutRules);
        hkStructureLayout layoutIn(hkStructureLayout::HostLayoutRules);
        PackfileObjectCopier copier(layoutIn, layoutOut);
        hkVariant& newVariant = a2->m_data[index];
        const hkClass* pNewClass = newVariant.m_class;
        // The source's (void*)(hkUlong) offset: the object pointer holds a buffer offset until the chunk exists.
        newVariant.m_object = reinterpret_cast<void*>(static_cast<uintptr_t>(buffer.m_size));
        offsets.insert(oldVariant.m_object, buffer.m_size);
        copier.hkObjectCopier::copyObject(
            oldVariant.m_object, *oldVariant.m_class, out.getStreamWriter(), *pNewClass, reloc
        );
    }
    int32_t size = buffer.m_size;
    hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
    char* pChunk = nullptr;
    if (size) {
        pChunk = static_cast<char*>(pRouter->m_heap->blockAlloc(size));
        a3->addChunk(pChunk, buffer.m_size, 5);
        hkString::memCpy(pChunk, buffer.m_data, buffer.m_size);
        for (int32_t index : *r_eax) {
            hkVariant& variant = a2->m_data[index];
            // The buffer offset the pointer holds becomes the copy's address in the chunk.
            variant.m_object = pChunk + reinterpret_cast<uintptr_t>(variant.m_object);
        }
        for (const hkRelocationInfo::Local& local : reloc.m_local) {
            // The chunk is untyped copied data: a pointer slot at a byte offset.
            *reinterpret_cast<void**>(pChunk + local.m_fromOffset) = pChunk + local.m_toOffset;
        }
        for (const hkRelocationInfo::Global& global : reloc.m_global) {
            a3->objectPointedBy(global.m_toAddress, pChunk + global.m_fromOffset);
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
        hkBufferedStreamWriter* pWriter =
            ::new (aWriterMemory) hkBufferedStreamWriter(obj, newClass.getObjectSize(), memoryIsString);
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
    if (member->m_type.m_storage == 20 && member->m_subtype.m_storage == 2) {
        member->m_type.m_storage = 29;
        member->m_subtype.m_storage = 0;
    }
}

static void convertZeroMemberToIgnored(hkClassMember* member) {
    if (member->m_type.m_storage == 19) {
        member->m_type.m_storage = member->m_subtype.m_storage;
        member->m_subtype.m_storage = 0;
        member->m_flags.m_storage |= 1024u;
    }
    uint8_t m_storage = member->m_type.m_storage;
    if (m_storage == 24 || m_storage == 31) {
        if ((member->m_flags.get() & 8) != 0) {
            member->m_subtype.m_storage = 4;
            member->m_flags.m_storage &= ~8u;
        }
        if ((member->m_flags.get() & 16) != 0) {
            member->m_subtype.m_storage = 6;
            member->m_flags.m_storage &= ~16u;
        }
        if ((member->m_flags.get() & 32) != 0) {
            member->m_subtype.m_storage = 8;
            member->m_flags.m_storage &= ~32u;
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
        hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
        done->insert(*pRouter->m_heap, UpdateFlagFromClassMap::ToMapInt(pClass), uiFlags | bit);
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
    while ((uiFlags & 1) == 0) {
        hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
        done->insert(*pRouter->m_heap, UpdateFlagFromClassMap::ToMapInt(pClass), uiFlags | 1);
        // The fields are written as raw bytes at the offsets the old hkClass record hkClassVersion1Class gives (the
        // class records are updated in place through reflection): a 32-bit value cleared, a byte tested, a 32-bit value
        // incremented.
        // EXE-INLINED none: hkClassMember::getOffset
        int32_t iHasVtable = hkClassVersion1Class.getMemberByName("hasVtable")->m_offset;
        hkClass* pRoot = pClass;
        while (pRoot->getParent()) {
            *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(pRoot) + iHasVtable) = 0;
            pRoot = pRoot->getParent();
        }
        if (reinterpret_cast<uint8_t*>(pRoot)[iHasVtable]) {
            // EXE-INLINED none: hkClassMember::getOffset
            int32_t iInterfaces = hkClassVersion1Class.getMemberByName("numImplementedInterfaces")->m_offset;
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

static void updateClassVersionInplace(hkClass* a1, hkUlongMapBase* a2, int32_t a3) {
    if (a3 == 1) {
        updateClassVersion1Inplace(a1, a2);
        updateConstraintInstance300(a1, a2);
    }
    else {
        if (a3 >= 4) {
            goto LABEL_5;
        }
    }
    updateMembersRecursive(a1, a2, 2, &convertCharPointerToCstring);
LABEL_5:;
    if (a3 < 5) {
        updateMembersRecursive(a1, a2, 8, &convertZeroMemberToIgnored);
    }
}

// The version 1 step of hkClass::updateMetadataInplace for klass: once per class (flag 4 in updatedAlready, a
// hkPointerMap<const hkClass*, hkInt32>), and for hkpConstraintInstance only, sets the field of its declared member 2
// at the offset of hkClassMemberClass's member 4 (read through reflection, as the game does) to TYPE_STRUCT.
static void updateConstraintInstance300(hkClass* r_eax, hkUlongMapBase* r_edi) {
    using UpdateFlagFromClassMap = hkPointerMap<const hkClass*, int32_t>;
    uint32_t uiFlags = r_edi->getWithDefault(UpdateFlagFromClassMap::ToMapInt(r_eax), 0);
    if ((uiFlags & 4) == 0) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        r_edi->insert(*Instance->m_heap, UpdateFlagFromClassMap::ToMapInt(r_eax), uiFlags | 4);
        if (!hkString::strCmp(r_eax->getName(), "hkpConstraintInstance")) {
            const hkClassMember& kMember = r_eax->getDeclaredMember(2);
            const hkClassMember& kField = static_cast<const hkClass&>(hkClassMemberClass).getMember(4);
            // Writes the field as raw bytes at the offset reflection gives (an untyped store into the class's own const
            // member data), as the reflected member accessors do.
            reinterpret_cast<uint8_t*>(const_cast<hkClassMember*>(&kMember))[kField.m_offset] =
                static_cast<uint8_t>(hkClassMember::TYPE_STRUCT);
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
    const hkClass* StructClass;
    const char* v9;
    const char* Name;
    char srca;
    char dsta;
    uint8_t m_storage = src.m_type.m_storage;
    uint8_t v7 = dst.m_type.m_storage;
    bool v18 = m_storage == v7 || (m_storage == 26 && v7 == 22);
    if (m_storage != 29 || (srca = 1, v7 != 33)) {
        srca = 0;
    }
    if (m_storage != 28 ||
        (v7 != 20 ||
         (dst.m_subtype.m_storage != 25 ||
          (!dst.getClass() ||
           (StructClass = &dst.getStructClass(),
            (Name = StructClass->getName(),
             (v9 = hkReferencedObjectClass.getName(), (dsta = 1, hkString::strCmp(v9, Name))))))))) {
        dsta = 0;
    }
    uint8_t v10 = src.m_subtype.m_storage;
    uint8_t v11 = dst.m_subtype.m_storage;
    bool v19 = v10 == v11 || (v10 == 29 && v11 == 33);
    bool v14 = false;
    if ((v10 == 28 && v11 == 20) && (dst.getClass())) {
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
