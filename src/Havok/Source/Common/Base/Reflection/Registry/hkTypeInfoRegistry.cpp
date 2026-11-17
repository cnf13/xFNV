#include "Havok/Source/Common/Base/Reflection/Registry/hkTypeInfoRegistry.h"

#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkTypeInfoRegistry("hkTypeInfoRegistry", &hkTypeInfoRegistrycreate,
	reinterpret_cast<void**>(&hkTypeInfoRegistry::s_instance));

// The singleton's create function (SDK: HK_SINGLETON_IMPLEMENTATION, `new hkTypeInfoRegistry()`), with the
// allocation and the constructor inlined but the string map's (cdecl, ret 0, 0xDB7A20).
// KEEP (upper:): the exe stores the map's empty Operations object as the byte at [esp+4] and pushes that whole dword
// to the map's constructor (0xDB7A3A-0xDB7A4C); its upper three bytes are the caller's ecx from `push ecx`, which
// src cannot reproduce. Src passes the byte with zero upper bytes.
hkReferencedObject* hkTypeInfoRegistrycreate() {
	// EXE-INLINED 0x56D280
	hkTypeInfoRegistry* pRegistry =
		static_cast<hkTypeInfoRegistry*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkTypeInfoRegistry)));
	pRegistry->m_memSizeAndFlags = sizeof(hkTypeInfoRegistry);
	return ::new (pRegistry) hkTypeInfoRegistry;
}
