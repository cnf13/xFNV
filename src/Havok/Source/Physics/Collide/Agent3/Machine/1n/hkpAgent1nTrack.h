#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Reconstruction.hpp"

// One 512 byte block of the agent data a track holds: a 16 byte header with the bytes in use, then the entries.
struct hkpAgent1nSector {
	enum {
		SECTOR_SIZE = 512,
		NET_SECTOR_SIZE = SECTOR_SIZE - 16,
	};

	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();

	uint32_t m_bytesAllocated;
	uint32_t m_pad0;
	uint32_t m_pad1;
	uint32_t m_pad2;
	uint8_t m_data[NET_SECTOR_SIZE];
};

// The agent data of a list or collection agent: a list of sectors, filled in order.
struct hkpAgent1nTrack {
	hkArray<hkpAgent1nSector*> m_sectors;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpAgent1nSectorTypeInfo;
