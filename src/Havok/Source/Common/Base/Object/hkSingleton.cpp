#include "Havok/Source/Common/Base/Object/hkSingleton.h"

#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkCachedHashMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/TraceStream/hkTraceStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"

hkSingletonInitNode* hkSingletonInitNode::findByName(const char* name) {
	hkSingletonInitNode* pThis = this;
	while (1) {
		if (pThis->m_name && !hkString::strCmp(name, pThis->m_name)) {
			return pThis;
		}
		if (!pThis->m_next) {
			break;
		}
		pThis = pThis->m_next;
	}
	return 0;
}

// The SDK's populate: maps every source node by name, then gives each destination node the instance of the source
// node of the same name, with a reference added. The map holds its keys (the names) and values (the nodes) as
// untyped hkUlong words, hence the casts. The exe passes the map's empty allocator as a dword whose low byte it
// clears (0xCF6A04); the upper bytes are no argument (porting.md "Parameter widths").
void hkSingletonInitNode::populate(hkSingletonInitNode* dstBase, const hkSingletonInitNode* srcBase) {
	using SrcMap_t = hkCachedHashMap<hkStringMapOperations, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(SrcMap_t, srcMap, std::nullopt, std::nullopt);
	::new (&srcMap) SrcMap_t(0u);
	for (const hkSingletonInitNode* s = srcBase; s; s = s->m_next) {
		srcMap.insert(reinterpret_cast<hkUlong>(s->m_name), reinterpret_cast<hkUlong>(s));
	}
	for (hkSingletonInitNode* d = dstBase; d; d = d->m_next) {
		const hkSingletonInitNode* s =
			reinterpret_cast<const hkSingletonInitNode*>(srcMap.getWithDefault(reinterpret_cast<hkUlong>(d->m_name),
				0));
		if (s) {
			*d->m_value = *s->m_value;
			if (*d->m_value) {
				static_cast<hkReferencedObject*>(*d->m_value)->addReferenceLockUnchecked();
			}
		}
	}
	// EXE-INLINED none: hkCachedHashMap::~hkCachedHashMap
	srcMap.clearAndDeallocate();
}

// The SDK's depopulate: maps every source node by name, then releases and clears the instance of each destination
// node a source node of the same name has. The allocator dword as in populate.
void hkSingletonInitNode::depopulate(hkSingletonInitNode* dstBase, const hkSingletonInitNode* srcBase) {
	using SrcMap_t = hkCachedHashMap<hkStringMapOperations, hkContainerHeapAllocator>;
	FNVD_INLINED_LOCAL(SrcMap_t, srcMap, std::nullopt, std::nullopt);
	::new (&srcMap) SrcMap_t(0u);
	for (const hkSingletonInitNode* s = srcBase; s; s = s->m_next) {
		srcMap.insert(reinterpret_cast<hkUlong>(s->m_name), reinterpret_cast<hkUlong>(s));
	}
	for (hkSingletonInitNode* d = dstBase; d; d = d->m_next) {
		if (srcMap.getWithDefault(reinterpret_cast<hkUlong>(d->m_name), 0)) {
			if (*d->m_value) {
				static_cast<hkReferencedObject*>(*d->m_value)->removeReferenceLockUnchecked();
			}
			*d->m_value = nullptr;
		}
	}
	// EXE-INLINED none: hkCachedHashMap::~hkCachedHashMap
	srcMap.clearAndDeallocate();
}
