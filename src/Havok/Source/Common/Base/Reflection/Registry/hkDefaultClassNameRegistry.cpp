#include "Havok/Source/Common/Base/Reflection/Registry/hkDefaultClassNameRegistry.h"

#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Serialize/Util/hkDynamicClassNameRegistry.h"

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkDefaultClassNameRegistry("hkDefaultClassNameRegistry",
	&hkDefaultClassNameRegistrycreate, reinterpret_cast<void**>(&hkDefaultClassNameRegistry::s_instance));

// The singleton's create function (SDK: HK_SINGLETON_IMPLEMENTATION, `new hkDefaultClassNameRegistry()`), with the
// allocation and the constructors inlined but the string map's (cdecl, ret 0, 0xDB79C0).
// KEEP (upper:): the exe stores the map's empty Operations object as the byte at [esp+4] and pushes that whole dword
// to the map's constructor (0xDB79DA-0xDB79EC); its upper three bytes are the caller's ecx from `push ecx`, which
// src cannot reproduce. Src passes the byte with zero upper bytes.
hkReferencedObject* hkDefaultClassNameRegistrycreate() {
	// EXE-INLINED 0x56D280
	hkDefaultClassNameRegistry* pRegistry = static_cast<hkDefaultClassNameRegistry*>(hkMemoryRouter::getInstance()
			.heap()
			.blockAlloc(sizeof(hkDefaultClassNameRegistry)));
	pRegistry->m_memSizeAndFlags = sizeof(hkDefaultClassNameRegistry);
	return ::new (pRegistry) hkDefaultClassNameRegistry;
}
