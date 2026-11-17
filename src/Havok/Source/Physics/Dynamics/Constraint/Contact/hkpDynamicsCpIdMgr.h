#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Reconstruction.hpp"

// Maps contact point ids to their indices in the contact constraint's point list: m_values[id] is the index, or
// FREE_VALUE for an unused id.
class hkpDynamicsCpIdMgr {
public:
	enum : uint8_t {
		FREE_VALUE = 0xFF,
	};

	// Shifts every used index above relIndex down by one, after the point at relIndex was removed.
	void decrementValuesGreater(int32_t relIndex);
	void getAllUsedIds(hkArray<uint16_t, hkContainerHeapAllocator>& ids) const;
	int32_t newId(int32_t value);

	// An hkInplaceArray<hkUint8, 8>: the id map lives in the object until it grows past 8 points (IDA's type; the
	// PDB's member is 20 bytes).
	hkInplaceArray<uint8_t, 8, hkContainerHeapAllocator> m_values;
};
