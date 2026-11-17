#include "Havok/Source/Common/Base/Reflection/Registry/hkTypeInfoRegistry.h"

#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkTypeInfoRegistry(
    "hkTypeInfoRegistry",
    &hkTypeInfoRegistrycreate,
    reinterpret_cast<void**>(&hkTypeInfoRegistry::s_instance)
);

// The singleton's create function (`new hkTypeInfoRegistry()`).
hkReferencedObject* hkTypeInfoRegistrycreate() {
    // EXE-INLINED 0x56D280
    hkTypeInfoRegistry* pRegistry =
        static_cast<hkTypeInfoRegistry*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkTypeInfoRegistry)));
    pRegistry->m_memSizeAndFlags = sizeof(hkTypeInfoRegistry);
    return ::new (pRegistry) hkTypeInfoRegistry;
}
