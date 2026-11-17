#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"

#include "Havok/Source/Common/Base/Container/Array/hkSmallArrayUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/hkMemUtil.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityActivationListener.h"

// Frees an array of aiCount pointers to the Havok heap (blockFree through the allocator's vtable, slot 2). The
// exe's eax after the call is whatever blockFree left: nothing is returned.
void releasePointerSmallArrayMemory(void** apData, int32_t aiCount) {
	hkMemoryRouter::getInstance().heap().blockFree(apData, aiCount * static_cast<int32_t>(sizeof(void*)));
}
