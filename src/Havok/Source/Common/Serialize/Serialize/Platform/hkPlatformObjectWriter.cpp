#include "Havok/Source/Common/Serialize/Serialize/Platform/hkPlatformObjectWriter.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Io/Writer/OffsetOnly/hkOffsetOnlyStreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Copier/hkDeepCopier.h"
#include "Havok/Source/Common/Serialize/Copier/hkObjectCopier.h"
#include "Havok/Source/Common/Serialize/Serialize/hkRelocationInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"

hkPlatformObjectWriter::Cache::Cache() {}

// Frees the converted classes; the maps and the array free their storage themselves.
hkPlatformObjectWriter::Cache::~Cache() {
    for (int32_t i = 0; i < m_allocations.m_size; ++i) {
        hkMemoryRouter::easyFree(hkMemoryRouter::getInstance().heap(), m_allocations.m_data[i]);
    }
}

// The class converted for the layout: the cached copy when there is one, else a deep copy of the class (recorded in
// the host-to-platform map), kept in m_allocations and laid out for the layout.
hkClass* hkPlatformObjectWriter::Cache::get(const hkClass* klass, const hkStructureLayout& layout) {
    using ClassMap = hkPointerMap<const void*, void*, hkContainerHeapAllocator>;
    // EXE-INLINED none: hkPointerMap<const void*, void*, hkContainerHeapAllocator>::get
    ClassMap::Storage uiFound;
    if (m_platformClassFromHostClass.m_map.getWithDefaultStatus(ClassMap::ToMapInt(klass), &uiFound) == 0) {
        return ClassMap::FromMapInt<hkClass*>(uiFound);
    }
    hkClass* pKlass = static_cast<hkClass*>(hkDeepCopier::deepCopy(
        klass, hkClassClass, &m_platformClassFromHostClass, nullptr, nullptr,
        hkFlags<hkObjectCopier::ObjectCopierFlagBits, uint32_t>(0)
    ));
    // EXE-INLINED 0x6299C0
    hkMemoryAllocator& kHeap = hkMemoryRouter::getInstance().heap();
    if (m_allocations.m_size == m_allocations.getCapacity()) {
        hkArrayUtil::_reserveMore(kHeap, &m_allocations, sizeof(void*));
    }
    m_allocations.m_data[m_allocations.m_size++] = pKlass;
    layout.computeMemberOffsetsInplace(*pKlass, m_platformClassComputed, 1);
    return pKlass;
}

// The copier converts from the host's layout to layout with flags 2; the cache is made only for a layout other than
// the host's (and is null when the heap has no room for it).
hkPlatformObjectWriter::hkPlatformObjectWriter(const hkStructureLayout& layout)
  : m_copier(new (hkReferencedObject::newObject(sizeof(hkObjectCopier)))
                 hkObjectCopier(hkStructureLayout(hkStructureLayout::HostLayoutRules), layout, 2u)) {
    const hkStructureLayout::LayoutRules& rules = getLayout().m_rules;
    const hkStructureLayout::LayoutRules& host = hkStructureLayout::HostLayoutRules;
    if (rules.m_bytesInPointer == host.m_bytesInPointer && rules.m_littleEndian == host.m_littleEndian &&
        rules.m_reusePaddingOptimization == host.m_reusePaddingOptimization &&
        rules.m_emptyBaseClassOptimization == host.m_emptyBaseClassOptimization) {
        m_cache = nullptr;
    }
    else {
        void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(Cache));
        m_cache = pMemory ? new (pMemory) Cache() : nullptr;
    }
}

// The cache's operator delete (the heap's blockFree) is written out.
hkPlatformObjectWriter::~hkPlatformObjectWriter() {
    m_copier->removeReference();
    if (m_cache) {
        m_cache->~Cache();
        hkMemoryRouter::getInstance().m_heap->blockFree(m_cache, sizeof(Cache));
    }
}

// Copies the object into the stream in the destination layout. With a cache, the class and (for hkClass objects) the
// object are first swapped for their converted copies; the converted object is then sized by copying it into an
// offset-only writer positioned at the stream's offset. Returns 0 (HK_SUCCESS) when the final writer is ok.
hkResult hkPlatformObjectWriter::writeObject(
    hkStreamWriter* apStream,
    const void* apObject,
    const hkClass& arKlass,
    hkRelocationInfo& arReloc
) {
    const hkClass* pKlassOut;
    if (m_cache) {
        pKlassOut = m_cache->get(&arKlass, getLayout());
    }
    else {
        pKlassOut = &arKlass;
    }
    hkOffsetOnlyStreamWriter kWriter;
    hkOffsetOnlyStreamWriter* pWriter = &kWriter;
    hkStreamWriter* pOut;
    if (m_cache && hkString::strCmp(pKlassOut->getName(), "hkClass") == 0) {
        // The object here is an hkClass instance, so it is the class the cache converts.
        const void* pCached = m_cache->get(static_cast<const hkClass*>(apObject), getLayout());
        pOut = apStream;
        // Calls seek directly on the local writer.
        pWriter->hkOffsetOnlyStreamWriter::seek(apStream->tell(), hkStreamWriter::STREAM_SET);
        hkRelocationInfo kReloc;
        kReloc.m_local.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Local>::DONT_DEALLOCATE_FLAG;
        kReloc.m_global.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Global>::DONT_DEALLOCATE_FLAG;
        kReloc.m_finish.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Finish>::DONT_DEALLOCATE_FLAG;
        kReloc.m_imports.m_capacityAndFlags = hkArrayBase<hkRelocationInfo::Import>::DONT_DEALLOCATE_FLAG;
        kReloc.m_local.m_data = nullptr;
        kReloc.m_local.m_size = 0;
        kReloc.m_global.m_data = nullptr;
        kReloc.m_global.m_size = 0;
        kReloc.m_finish.m_data = nullptr;
        kReloc.m_finish.m_size = 0;
        kReloc.m_imports.m_data = nullptr;
        kReloc.m_imports.m_size = 0;
        kReloc.m_pool = nullptr;
        m_copier->copyObject(pCached, arKlass, pOut, *pKlassOut, kReloc);
        pOut = pWriter;
    }
    else {
        pOut = apStream;
    }
    m_copier->copyObject(apObject, arKlass, pOut, *pKlassOut, arReloc);
    hkBool kOk;
    return static_cast<hkResult>(pOut->isOk(&kOk)->m_bool == 0);
}

hkResult hkPlatformObjectWriter::writeRaw(hkStreamWriter* a1, const void* a2, int32_t a3) {
    return static_cast<hkResult>(a1->write(a2, a3) != a3);
}

const hkStructureLayout& hkPlatformObjectWriter::getLayout() const {
    return m_copier->m_layoutOut;
}
