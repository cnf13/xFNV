#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnSector.h"
#include "Reconstruction.hpp"

class hkpAgentNnEntry;

// A track of agent entries stored in sectors; the track owns the sectors.
class hkpAgentNnTrack {
public:
	uint32_t m_bytesUsedInLastSector;
	hkInplaceArray<hkpAgentNnSector*, 1, hkContainerHeapAllocator> m_sectors;
};

// Not named in the PDB or IDA; name chosen here. The track's last entry: the last sector's begin plus its used
// bytes, less one entry slot (usercall, track in eax, ret 0).
hkpAgentNnEntry* getLastAgentEntry(hkpAgentNnTrack* r_eax);
