#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "SDK/MSVC/ehvec.h"

namespace hkAlgorithm {}  // namespace hkAlgorithm

// The hits arrays' clearAndDeallocate and _clearAndDeallocate: the SDK's inline bodies (hkArray.h).
using hkArray_lhkpRootCdPoint_chkContainerHeapAllocator = hkArray<hkpRootCdPoint, hkContainerHeapAllocator>;

// Havok SDK (inline): whether this point's distance is below b's. The exe's out-of-line copy reads each distance
// through 0x9B3100 out of line, this point's first, and builds the hkBool in the hidden result (thiscall, ret 8).
// That copy lies among the game's functions; the body is defined with its class.
hkBool hkpRootCdPoint::operator<(const hkpRootCdPoint& b) const {
	double dDistance = m_contact.m_position.getNextW();
	return hkBool(b.m_contact.m_position.getNextW() > dDistance);
}
