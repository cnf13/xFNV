#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"

#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"

using hkPointerMap_const_void_const_char_hkContainerHeapAllocator =
	hkPointerMap<const void*, const char*, hkContainerHeapAllocator>;

using hkPointerMap_lint_cint_chkContainerHeapAllocator = hkPointerMap<int32_t, int32_t, hkContainerHeapAllocator>;
