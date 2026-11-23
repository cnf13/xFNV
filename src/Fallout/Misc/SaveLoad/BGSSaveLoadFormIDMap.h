#pragma once

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"

class BGSSaveLoadFile;

// The save game's form ID table: each form ID's index and each index's form ID, and the next index to hand out.
class BGSSaveLoadFormIDMap {
public:
    BGSSaveLoadFormIDMap(uint32_t iBucketCount);
    ~BGSSaveLoadFormIDMap();

    // The index of aiFormID, which gets the next index when it has none; a created form ID
    // is returned as is.
    uint32_t AddFormID(uint32_t aiFormID);
    uint32_t GetFormID(uint32_t aiFormIDIndex);
    void Save(BGSSaveLoadFile* apFile);
    // Empties both maps and restarts the indices, index 0 going to form ID 0.
    void Clear();
    // Reads the form ID table from apFile.
    void Load(BGSSaveLoadFile* apFile);
    uint32_t ConvertFormID(uint32_t aiFormID);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    NiTMap<uint32_t, uint32_t> FormIDToIndexMap;
    NiTMap<uint32_t, uint32_t> IndexToFormIDMap;
    uint32_t iCurrentIndex;
};
