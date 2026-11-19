#include "Havok/Source/Common/Serialize/Copier/hkDeepCopier.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkTypeInfoRegistry.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkVtableClassRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkTypeTraits.h"
#include "Havok/Source/Common/Serialize/Serialize/hkRelocationInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"

namespace {
    // Record of one object copied into the buffer: its offset in the buffer, the original object and its class name.
    struct Copy {
        int32_t copyOffset;
        void* original;
        const char* className;
    };
} // namespace
// The class version's loop over the records, which have no destructor: empty. hkArrayUtil's class version has no
// generic body, so both builds need this one.
template <>
void hkArrayUtil::destruct<Copy>(Copy*, int32_t, hkTraitBool<false>) {}

void sub_E4B560() {
    ;
}

// Copies the objects breadth first: each object of the to-do list is copied into buffer, at the offset recorded in
// done, and each pointer of its copy to an object with a class adds that object to the list unless copyFromOriginal
// maps it (the pointer then goes to the mapped copy) or it is done already. The buffer is then moved to a block of its
// own and the copies' pointers are fixed up to point into it.
void* hkDeepCopier::deepCopy(
    const void* dataIn,
    const hkClass& klassIn,
    hkPointerMap<const void*, void*, hkContainerHeapAllocator>* previousCopies,
    hkVtableClassRegistry* vtableClassRegistry,
    hkTypeInfoRegistry* loadedObjectRegistry,
    hkFlags<hkObjectCopier::ObjectCopierFlagBits, uint32_t> flags
) {
    hkArray<char> buffer;
    hkOstream out(buffer);

    hkRelocationInfo reloc;
    reloc.m_local.m_data = nullptr;
    reloc.m_local.m_size = 0;
    reloc.m_local.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Local>::DONT_DEALLOCATE_FLAG;
    reloc.m_global.m_data = nullptr;
    reloc.m_global.m_size = 0;
    reloc.m_global.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Global>::DONT_DEALLOCATE_FLAG;
    reloc.m_finish.m_data = nullptr;
    reloc.m_finish.m_size = 0;
    reloc.m_finish.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Finish>::DONT_DEALLOCATE_FLAG;
    reloc.m_imports.m_data = nullptr;
    reloc.m_imports.m_size = 0;
    reloc.m_imports.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Import>::DONT_DEALLOCATE_FLAG;
    reloc.m_pool = nullptr;

    // The objects still to copy, with their classes; dataIn first.
    hkArray<hkVariant> todo;
    {
        hkVariant& v = todo.expandOne();
        v.m_object = const_cast<void*>(dataIn);
        v.m_class = &klassIn;
    }

    // The offset in buffer of each original object copied.
    using done_t = hkPointerMap<const void*, int32_t>;
    done_t done;

    hkArray<Copy> copies;

    // Equivalent to hkObjectCopier copier(hkStructureLayout::HostLayoutRules, hkStructureLayout::HostLayoutRules,
    // flags), with the two layout temporaries built right to left.
    hkStructureLayout layoutOut(hkStructureLayout::HostLayoutRules);
    hkStructureLayout layoutIn(hkStructureLayout::HostLayoutRules);
    hkObjectCopier copier(layoutIn, layoutOut, flags.get());

    for (int32_t todoIndex = 0; todoIndex < todo.getSize(); ++todoIndex) {
        void* pOriginal = todo.m_data[todoIndex].m_object;
        const hkClass* pKlass = todo.m_data[todoIndex].m_class;
        done.insert(pOriginal, buffer.getSize());
        const int32_t globalsBefore = reloc.m_global.getSize();
        if (pKlass->hasVtable().m_bool) {
            pKlass = vtableClassRegistry->getClassFromVirtualInstance(pOriginal);
        }
        const int32_t copyOffset = buffer.getSize();
        const char* pClassName = pKlass->getName();
        copies.pushBack(Copy{copyOffset, pOriginal, pClassName});
        // Calls hkObjectCopier's own copyObject directly.
        copier.hkObjectCopier::copyObject(pOriginal, *pKlass, out.getStreamWriter(), *pKlass, reloc);

        // The pointers the copy just added: queue each pointed-to object that is neither mapped nor copied.
        for (int32_t i = globalsBefore; i < reloc.m_global.getSize(); ++i) {
            void* pTo = reloc.m_global.m_data[i].m_toAddress;
            if (pTo == nullptr) {
                continue;
            }
            const hkClass* pToClass = reloc.m_global.m_data[i].m_toClass;
            if (pToClass == nullptr) {
                continue;
            }
            void* pAlreadyCopied = previousCopies ? previousCopies->getWithDefault(pTo, nullptr) : nullptr;
            if (pAlreadyCopied) {
                reloc.m_global.m_data[i].m_toAddress = pAlreadyCopied;
            }
            else if (!done.isValid(done.findKey(pTo))) {
                hkVariant& v = todo.expandOne();

                v.m_object = pTo;
                v.m_class = pToClass;
            }
        }
    }

    char* pResult = nullptr;
    if (buffer.getSize() != 0) {
        pResult = static_cast<char*>(hkMemoryRouter::easyAlloc(hkMemoryRouter::getInstance().heap(), buffer.getSize()));
        hkString::memCpy(pResult, buffer.begin(), buffer.getSize());
        // The pointers within the copies. The block is raw bytes: the relocations patch it at byte offsets, with no
        // type of their own.
        for (const hkRelocationInfo::Local& local : reloc.m_local) {
            *reinterpret_cast<char**>(pResult + local.m_fromOffset) = pResult + local.m_toOffset;
        }
        // The pointers to other objects: to their copy where one was made here, else to the object itself.
        for (const hkRelocationInfo::Global& global : reloc.m_global) {
            void* pTo = global.m_toAddress;
            const int32_t offset = done.getWithDefault(pTo, -1);
            if (offset != -1) {
                pTo = pResult + offset;
            }
            *reinterpret_cast<void**>(pResult + global.m_fromOffset) = pTo;
        }
        if (previousCopies) {
            for (const Copy& copy : copies) {
                previousCopies->insert(copy.original, pResult + copy.copyOffset);
            }
        }
        if (loadedObjectRegistry) {
            for (const Copy& copy : copies) {
                loadedObjectRegistry->finishLoadedObject(pResult + copy.copyOffset, copy.className);
            }
        }
    }

    return pResult;
}
