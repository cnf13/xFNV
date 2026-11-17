#include "Havok/Source/Common/Base/Reflection/Registry/hkVtableClassRegistry.h"

#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkVtableClassRegistry("hkVtableClassRegistry",
	&hkVtableClassRegistrycreate, reinterpret_cast<void**>(&hkVtableClassRegistry::s_instance));

// The singleton's create function (SDK: HK_SINGLETON_IMPLEMENTATION, `new hkVtableClassRegistry()`), with the
// allocation and the constructor inlined.
hkReferencedObject* hkVtableClassRegistrycreate() {
	// EXE-INLINED 0x56D280
	hkVtableClassRegistry* pRegistry = static_cast<hkVtableClassRegistry*>(hkMemoryRouter::getInstance()
			.heap()
			.blockAlloc(sizeof(hkVtableClassRegistry)));
	pRegistry->m_memSizeAndFlags = sizeof(hkVtableClassRegistry);
	return ::new (pRegistry) hkVtableClassRegistry;
}

// The map frees its storage in its own destructor.
hkVtableClassRegistry::~hkVtableClassRegistry() {}

// Files klass under the vtable address.
void hkVtableClassRegistry::registerVtable(const void* vtable, const hkClass* klass) {
	m_map.insert(vtable, klass);
}

// The class filed under obj's vtable pointer (its first word), or null.
const hkClass* hkVtableClassRegistry::getClassFromVirtualInstance(const void* obj) const {
	return m_map.getWithDefault(*static_cast<const void* const*>(obj), nullptr);
}

// Registers each type info's vtable with the class at the same index, up to the first null info or class; an info
// with no vtable is skipped.
void hkVtableClassRegistry::registerList(const hkTypeInfo* const* infos, const hkClass* const* classes) {
	for (int32_t i = 0; infos[i]; ++i) {
		if (!classes[i]) {
			break;
		}
		const void* pVtable = infos[i]->m_vtable;
		if (pVtable) {
			registerVtable(pVtable, classes[i]);
		}
	}
}

void hkVtableClassRegistry::merge(const hkVtableClassRegistry& mergeFrom) {
	int32_t m_hashMod = mergeFrom.m_map.m_map.m_hashMod;
	int32_t v4 = 0;
	if (m_hashMod >= 0) {
		hkUlongMapBase::Pair* m_elem = mergeFrom.m_map.m_map.m_elem;
		do {
			if (m_elem->key != -1) {
				break;
			}
			++v4;
			++m_elem;
		} while (v4 <= m_hashMod);
	}
	if (v4 <= m_hashMod) {
		int32_t v10;
		int32_t v11;
		hkPointerMap<const void*, const hkClass*, hkContainerHeapAllocator>* p_m_map = &m_map;
		do {
			hkUlongMapBase::Pair* v6 = mergeFrom.m_map.m_map.m_elem;
			uintptr_t val = v6[v4].val;
			uintptr_t key = v6[v4].key;
			hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
			p_m_map->m_map.insert(*Instance->m_heap, key, val);
			v10 = mergeFrom.m_map.m_map.m_hashMod;
			v11 = v4 + 1;
			if (v4 + 1 <= v10) {
				hkUlongMapBase::Pair* v12 = &mergeFrom.m_map.m_map.m_elem[v11];
				do {
					if (v12->key != -1) {
						break;
					}
					++v11;
					++v12;
				} while (v11 <= v10);
			}
			v4 = v11;
		} while (v11 <= v10);
	}
}
