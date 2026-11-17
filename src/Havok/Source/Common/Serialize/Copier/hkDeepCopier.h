#pragma once

#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Copier/hkObjectCopier.h"

class hkTypeInfoRegistry;
class hkVtableClassRegistry;

void sub_E4B560();

namespace hkDeepCopier {
// Copies dataIn of class klassIn and every object it points to that has a class into one new block from the heap,
// with the pointers between them fixed up; an object copyFromOriginal already maps is pointed to, not copied again,
// and the new copies are added to it. Objects with a vtable are copied as the class vtableClassRegistry gives them,
// and loadedObjectRegistry finishes each copy. Returns the copy of dataIn, or null when nothing was written.
// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
void* deepCopy(const void* dataIn, const hkClass& klassIn,
	hkPointerMap<const void*, void*, hkContainerHeapAllocator>* previousCopies,
	hkVtableClassRegistry* vtableClassRegistry, hkTypeInfoRegistry* loadedObjectRegistry,
	hkFlags<hkObjectCopier::ObjectCopierFlagBits, uint32_t> flags);
}  // namespace hkDeepCopier
