#include "Havok/Source/Common/Serialize/Serialize/hkRelocationInfo.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStorageStringMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

hkRelocationInfo::hkRelocationInfo() : m_pool(nullptr) {}

// The pool's class has no destructor of its own here: the SDK's (clear, then its map's) and its operator delete
// (the heap's blockFree) are written out, as the exe inlines them.
hkRelocationInfo::~hkRelocationInfo() {
	if (m_pool) {
		m_pool->clear();
		m_pool->~hkStorageStringMap();
		hkMemoryRouter::getInstance().m_heap->blockFree(m_pool,
			sizeof(hkStorageStringMap<int32_t, hkContainerHeapAllocator>));
	}
}

// IDA name .text_E02800; the SDK's hkRelocationInfo::addImport, whose header declares it out of line. The exe
// inlines the pool's operator new, hkStorageStringMap::insert(name, 0) and m_imports.pushBack. The empty allocator the
// map's constructor takes by value is pushed as the exe's prologue `push ecx` slot (this) with its low byte cleared.
// isValid's hkBool goes to the exe's name argument slot (name is kept in a register); src uses a local. The map keys
// are hkUlong words holding the string pointers, hence the pointer/integer reinterpret_casts.
void hkRelocationInfo::addImport(int32_t off, const char* name) {
	if (!m_pool) {
		// EXE-INLINED none: hkStorageStringMap::operator new
		void* pMemory = hkMemoryRouter::getInstance()
							.m_heap->blockAlloc(sizeof(hkStorageStringMap<int32_t, hkContainerHeapAllocator>));
		hkStorageStringMap<int32_t, hkContainerHeapAllocator>* pPool = nullptr;
		if (pMemory) {
			const uint32_t uiAllocator = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this)) & 0xFFFFFF00u;
			::new (pMemory) hkCachedHashMap<hkStringMapOperations, hkContainerHeapAllocator>(uiAllocator);
			pPool = static_cast<hkStorageStringMap<int32_t, hkContainerHeapAllocator>*>(pMemory);
		}
		m_pool = pPool;
	}
	hkStorageStringMap<int32_t, hkContainerHeapAllocator>* pPool = m_pool;
	// EXE-INLINED none: hkStorageStringMap::insert
	auto it = pPool->m_map.findKey(reinterpret_cast<hkUlong>(name));
	hkBool valid;
	pPool->m_map.isValid(&valid, it);
	const char* pcName;
	if (!valid.m_bool) {
		// EXE-INLINED 0x4A4840
		pcName = hkString::strDup(name, *hkMemoryRouter::getInstance().m_heap);
		pPool->m_map.insert(reinterpret_cast<hkUlong>(pcName), 0);
	} else {
		pcName = reinterpret_cast<const char*>(pPool->m_map.getKey(it));
		pPool->m_map.setValue(it, 0);
	}
	// EXE-INLINED none: hkArray<hkRelocationInfo::Import>::pushBack
	hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
	if (m_imports.m_size == (m_imports.m_capacityAndFlags & hkArrayBase<Import>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(*kRouter.m_heap, &m_imports, sizeof(Import));
	}
	Import& rImport = m_imports.m_data[m_imports.m_size];
	rImport.m_fromOffset = off;
	rImport.m_identifier = pcName;
	m_imports.m_size += 1;
}

// The SDK's applyLocalAndGlobal: each local fixup stores the buffer address of its target, each global one its
// target's address, at its offset in the buffer. The buffer is raw packfile data, untyped for the exe too: the
// fixup slots are pointers written through casts.
void hkRelocationInfo::applyLocalAndGlobal(void* buffer) {
	char* pBuffer = static_cast<char*>(buffer);
	for (int32_t i = 0; i < m_local.m_size; ++i) {
		*reinterpret_cast<void**>(pBuffer + m_local.m_data[i].m_fromOffset) = pBuffer + m_local.m_data[i].m_toOffset;
	}
	for (int32_t i = 0; i < m_global.m_size; ++i) {
		*reinterpret_cast<void**>(pBuffer + m_global.m_data[i].m_fromOffset) = m_global.m_data[i].m_toAddress;
	}
}
