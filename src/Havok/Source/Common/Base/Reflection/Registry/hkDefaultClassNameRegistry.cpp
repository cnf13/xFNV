#include "Havok/Source/Common/Base/Reflection/Registry/hkDefaultClassNameRegistry.h"

#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Serialize/Util/hkDynamicClassNameRegistry.h"

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkDefaultClassNameRegistry(
    "hkDefaultClassNameRegistry",
    &hkDefaultClassNameRegistrycreate,
    reinterpret_cast<void**>(&hkDefaultClassNameRegistry::s_instance)
);

// The singleton's create function (HK_SINGLETON_IMPLEMENTATION, `new hkDefaultClassNameRegistry()`).
hkReferencedObject* hkDefaultClassNameRegistrycreate() {
    return new hkDefaultClassNameRegistry;
}
