#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnSector.h"

class hkpAgentNnEntry;

// A track of agent entries stored in sectors; the track owns the sectors.
class hkpAgentNnTrack {
public:
	uint32_t m_bytesUsedInLastSector;
	hkInplaceArray<hkpAgentNnSector*, 1, hkContainerHeapAllocator> m_sectors;
};

// The track's last entry: the last sector's begin plus its used bytes, less one entry slot.
hkpAgentNnEntry* getLastAgentEntry(hkpAgentNnTrack* r_eax);
