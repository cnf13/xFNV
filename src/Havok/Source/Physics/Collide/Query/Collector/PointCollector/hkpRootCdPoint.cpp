#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "SDK/MSVC/ehvec.h"

namespace hkAlgorithm {} // namespace hkAlgorithm

// The hits arrays' clearAndDeallocate and _clearAndDeallocate: the SDK's inline bodies (hkArray.h).
using hkArray_lhkpRootCdPoint_chkContainerHeapAllocator = hkArray<hkpRootCdPoint, hkContainerHeapAllocator>;

// Whether this point's distance is below b's. This point's distance is read first.
hkBool hkpRootCdPoint::operator<(const hkpRootCdPoint& b) const {
    double dDistance = m_contact.m_position.getNextW();
    return hkBool(b.m_contact.m_position.getNextW() > dDistance);
}
