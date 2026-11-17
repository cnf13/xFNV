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
	for (int32_t i = 0; i < m_markers.m_size; i++) {
		m_markers.m_data[i]->removeReference();
	}
}
