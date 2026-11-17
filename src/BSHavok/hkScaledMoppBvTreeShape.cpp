#include "BSHavok/hkScaledMoppBvTreeShape.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"

// The exe's destructor (sub_D54960): the reference on the unscaled code, then hkpMoppBvTreeShape's.
hkScaledMoppBvTreeShape::~hkScaledMoppBvTreeShape() {
	if (pOriginalCode) {
		pOriginalCode->removeReference();
	}
}
