#include "Havok/Source/Physics/Utilities/Serialize/Display/hkpSerializedDisplayMarkerList.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Utilities/Serialize/Display/hkpSerializedDisplayMarker.h"

// Each marker loses its reference; the array's storage goes with m_markers.
hkpSerializedDisplayMarkerList::~hkpSerializedDisplayMarkerList() {
    for (hkpSerializedDisplayMarker* pMarker : m_markers) {
        pMarker->removeReference();
    }
}
