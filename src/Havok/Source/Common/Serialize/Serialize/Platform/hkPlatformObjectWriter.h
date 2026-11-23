#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Serialize/hkObjectWriter.h"

#include <stdint.h>

class hkObjectCopier;
class hkRelocationInfo;
class hkStreamWriter;
class hkStructureLayout;

// Writes reflected objects in the destination platform's object layout.
class hkPlatformObjectWriter : public hkObjectWriter {
public:
    // The objects and classes already converted to the destination layout.
    struct Cache {
        Cache();
        ~Cache();

        // The class allocator: the heap allocator, by size.
        static void* operator new(size_t aSize) noexcept {
            return hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(aSize));
        }
        static void operator delete(void* apMemory, size_t aSize) {
            hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
        }

        // The converted copy of the class for the layout, made and laid out on its first request.
        hkClass* get(const hkClass* klass, const hkStructureLayout& layout);

        hkPointerMap<const void*, void*, hkContainerHeapAllocator> m_platformClassFromHostClass;
        hkPointerMap<const hkClass*, int32_t, hkContainerHeapAllocator> m_platformClassComputed;
        // The converted classes, freed with the cache.
        hkArray<void*, hkContainerHeapAllocator> m_allocations;
    };

    hkPlatformObjectWriter(const hkStructureLayout& layout);
    ~hkPlatformObjectWriter() override;

    hkResult writeObject(hkStreamWriter*, const void*, const hkClass&, hkRelocationInfo&) override;
    // Havok SDK signature (hkSerialize.lib, tied by bytes,vtable).
    hkResult writeRaw(hkStreamWriter* writer, const void* data, int32_t dataLen) override;

    // The copier's destination layout.
    const hkStructureLayout& getLayout() const;

    hkObjectCopier* m_copier;
    Cache* m_cache;
};
