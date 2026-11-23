#pragma once

class hkpAgentNnEntry;

// The SDK's sizes of an agent entry slot and of a sector (hkpAgentNnTrack.h): 128 and 512 for 4-byte pointers (the
// exe's), 160 and 960 for 8-byte pointers.
#define HK_AGENT3_AGENT_SIZE 160
#define HK_AGENT3_SECTOR_SIZE 960

// One sector of a track: HK_AGENT3_SECTOR_SIZE bytes of agent entries laid out one after the other. The track's sector
// array holds these, and an entry is the sector's data plus the track's byte offset.
struct hkpAgentNnSector {
    // The first entry and the end of the sector. The data is untyped storage holding the entries.
    hkpAgentNnEntry* getBegin() {
        return reinterpret_cast<hkpAgentNnEntry*>(&m_data[0]);
    }
    hkpAgentNnEntry* getEnd() {
        return reinterpret_cast<hkpAgentNnEntry*>(&m_data[sizeof(m_data)]);
    }

    // The entries; hkAgentNnMachine_AllocateEntry hands out HK_AGENT3_AGENT_SIZE-byte slots of it.
    alignas(16) uint8_t m_data[HK_AGENT3_SECTOR_SIZE];
};
