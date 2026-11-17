#pragma once

#include <cstdint>

class TESForm;

// The kind of a region data entry.
enum REGION_DATA_ID : int32_t {
    REGION_DATA_NONE = 0x0,
    REGION_DATA_GENERAL_ID = 0x1,
    REGION_DATA_OBJECTS_ID = 0x2,
    REGION_DATA_WEATHER_ID = 0x3,
    REGION_DATA_MAP_ID = 0x4,
    REGION_DATA_LANDSCAPE_ID = 0x5,
    REGION_DATA_GRASS_ID = 0x6,
    REGION_DATA_SOUND_ID = 0x7,
    REGION_DATA_IMPOSTER_ID = 0x8,
    REGION_DATA_COUNT = 0x9,
};

// The RDAT chunk of one region data entry: its REGION_DATA_ID, override flag and priority (zeroed, 8 bytes).
struct REGION_DATA_BASE {
    // Swaps iDataTypeID.
    void Endian();

    REGION_DATA_ID iDataTypeID;
    bool bOverride;
    uint8_t cPriority;
    uint8_t padding06[2];
};

class TESRegionData {
public:
    TESRegionData();
    TESRegionData(TESRegionData* arData);
    virtual ~TESRegionData();

    // Writes the RDAT chunk: the data type, override flag and priority (REGION_DATA_BASE).
    virtual void Save();
    virtual bool LoadRegionData(REGION_DATA_BASE*);
    // The PC initializer takes an owner pointer, omitted from the exported PC vtable annotation.
    virtual void InitItem(TESForm*) {}
    virtual REGION_DATA_ID GetID() const = 0;
    virtual TESRegionData* Copy() = 0;
    virtual TESRegionData* Blend(const TESRegionData*) = 0;
    virtual void BlendInto(const TESRegionData*, uint32_t) = 0;
    virtual bool Validate() = 0;

    bool SetPriority(const uint8_t acPriority);
    // Stores the argument's low byte at +4.
    void SetOverride(bool abNewOverride) {
        bOverride = abNewOverride;
    }
    // Returns bOverride.
    bool GetOverride();
    // Returns cPriority.
    uint8_t GetPriority();

    static void operator delete(void* apMemory);

    bool bOverride;
    bool bIgnore;
    uint8_t cPriority;
    uint8_t padding07[1];
};
