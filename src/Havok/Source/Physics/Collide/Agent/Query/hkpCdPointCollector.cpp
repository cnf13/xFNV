#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"

#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

// HK_DECLARE_CLASS_ALLOCATOR's operator delete: the block goes back to the thread's Havok heap.
void hkpCdPointCollector::operator delete(void* p) {
    if (p) {
        hkMemoryRouter::getInstance().heap().blockFree(p, sizeof(hkpCdPointCollector));
    }
}
