#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnSector.h"

class hkpAgentNnEntry;

// A track of agent entries stored in sectors; the track owns the sectors.
class hkpAgentNnTrack {
public:
    int32_t getSectorSize(int32_t sectorIndex) const {
        if (sectorIndex + 1 == m_sectors.m_size) {
            return static_cast<int32_t>(m_bytesUsedInLastSector);
        }
        return static_cast<int32_t>(sizeof(hkpAgentNnSector));
    }

    uint32_t m_bytesUsedInLastSector;
    hkInplaceArray<hkpAgentNnSector*, 1, hkContainerHeapAllocator> m_sectors;
};

// The track's last entry: the last sector's begin plus its used bytes, less one entry slot.
hkpAgentNnEntry* getLastAgentEntry(hkpAgentNnTrack* track);
