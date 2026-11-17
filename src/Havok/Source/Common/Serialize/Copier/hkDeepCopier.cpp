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
void hkArrayUtil::destruct<Copy>(Copy*, int32_t, hkTraitBool<0>) {}

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
    // hkArray<char> buffer; hkOstream out(buffer): the lines below are the array's constructor.
    FNVD_INLINED_LOCAL(hkArray<char>, buffer, std::nullopt, std::nullopt);
    buffer.m_data = nullptr;
    buffer.m_size = 0;
    buffer.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
    // Destroyed by hand below, in the game's order.
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
    FNVD_INLINED_LOCAL(hkArray<hkVariant>, todo, std::nullopt, std::nullopt);
    todo.m_data = nullptr;
    todo.m_size = 0;
    todo.m_capacityAndFlags = hkArrayBase<hkVariant>::DONT_DEALLOCATE_FLAG;
    {
        // EXE-INLINED none: hkArray<hkVariant>::expandOne
        hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
        if (todo.m_size == todo.getCapacity()) {
            hkArrayUtil::_reserveMore(kHeap, &todo, sizeof(hkVariant));
        }
        hkVariant& v = todo.m_data[todo.m_size++];
        v.m_object = const_cast<void*>(dataIn);
        v.m_class = &klassIn;
    }

    // The offset in buffer of each original object copied.
    using done_t = hkPointerMap<const void*, int32_t>;
    FNVD_INLINED_LOCAL(done_t, done, std::nullopt, std::nullopt);
    // EXE-INLINED 0x626560
    done.m_map.m_elem = nullptr;
    done.m_map.m_numElems = 0;
    done.m_map.m_hashMod = -1;
    done.m_map.hkUlongMapBase::reserve(hkMemoryRouter::getInstance().heap(), 0);

    FNVD_INLINED_LOCAL(hkArray<Copy>, copies, std::nullopt, std::nullopt);
    copies.m_data = nullptr;
    copies.m_size = 0;
    copies.m_capacityAndFlags = hkArrayBase<Copy>::DONT_DEALLOCATE_FLAG;

    // Equivalent to hkObjectCopier copier(hkStructureLayout::HostLayoutRules, hkStructureLayout::HostLayoutRules,
    // flags), with the two layout temporaries built right to left.
    hkStructureLayout layoutOut(hkStructureLayout::HostLayoutRules);
    hkStructureLayout layoutIn(hkStructureLayout::HostLayoutRules);
    // Destroyed by hand below, in the game's order.
    hkObjectCopier copier(layoutIn, layoutOut, flags.get());

    for (int32_t todoIndex = 0; todoIndex < todo.getSize(); ++todoIndex) {
        void* pOriginal = todo.m_data[todoIndex].m_object;
        const hkClass* pKlass = todo.m_data[todoIndex].m_class;
        done.insert(pOriginal, buffer.getSize());
        const int32_t globalsBefore = reloc.m_global.getSize();
        hkBool result;
        if (pKlass->hasVtable(&result)->m_bool) {
            pKlass = vtableClassRegistry->getClassFromVirtualInstance(pOriginal);
        }
        const int32_t copyOffset = buffer.getSize();
        const char* pClassName = pKlass->getName();
        {
            // EXE-INLINED none: hkArray<Copy>::pushBack
            hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
            if (copies.m_size == copies.getCapacity()) {
                hkArrayUtil::_reserveMore(kHeap, &copies, sizeof(Copy));
            }
            // The record is constructed in place (placement new), which is tested for null.
            Copy* pCopy = copies.m_data + copies.m_size;
            if (pCopy) {
                pCopy->copyOffset = copyOffset;
                pCopy->original = pOriginal;
                pCopy->className = pClassName;
            }
            ++copies.m_size;
        }
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
                // EXE-INLINED none: hkArray<hkVariant>::expandOne
                hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
                if (todo.m_size == todo.getCapacity()) {
                    hkArrayUtil::_reserveMore(kHeap, &todo, sizeof(hkVariant));
                }
                hkVariant& v = todo.m_data[todo.m_size++];
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
        for (int32_t i = 0; i < reloc.m_local.getSize(); ++i) {
            const hkRelocationInfo::Local& local = reloc.m_local.m_data[i];
            *reinterpret_cast<char**>(pResult + local.m_fromOffset) = pResult + local.m_toOffset;
        }
        // The pointers to other objects: to their copy where one was made here, else to the object itself.
        for (int32_t i = 0; i < reloc.m_global.getSize(); ++i) {
            void* pTo = reloc.m_global.m_data[i].m_toAddress;
            const int32_t offset = done.getWithDefault(pTo, -1);
            if (offset != -1) {
                pTo = pResult + offset;
            }
            *reinterpret_cast<void**>(pResult + reloc.m_global.m_data[i].m_fromOffset) = pTo;
        }
        if (previousCopies) {
            for (int32_t i = 0; i < copies.getSize(); ++i) {
                previousCopies->insert(copies.m_data[i].original, pResult + copies.m_data[i].copyOffset);
            }
        }
        if (loadedObjectRegistry) {
            for (int32_t i = 0; i < copies.getSize(); ++i) {
                loadedObjectRegistry->finishLoadedObject(
                    pResult + copies.m_data[i].copyOffset, copies.m_data[i].className
                );
            }
        }
    }

    // The locals' destructors, in the order the game runs them; ~hkArray is the code of clearAndDeallocate.
    copies.clearAndDeallocate();
    // EXE-INLINED 0x6264F0
    // ~hkMap: the table is freed; ~hkMapBase is the exe's empty function 0xA29680.
    done.m_map.clearAndDeallocate(hkMemoryRouter::getInstance().heap());
    todo.clearAndDeallocate();
    buffer.clearAndDeallocate();
    return pResult;
}
