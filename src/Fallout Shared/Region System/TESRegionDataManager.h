#pragma once

#include "Fallout Shared/Region System/TESRegionData.h"

class TESFile;
class TESRegion;
class TESRegionDataGrass;
class TESRegionDataImposter;
class TESRegionDataLandscape;
class TESRegionDataMap;
class TESRegionDataObjects;
class TESRegionDataSound;
class TESRegionDataWeather;

// Reads the region data records of a region from its file and makes the data objects, one kind per record type.
class TESRegionDataManager {
public:
    TESRegionDataManager();

    virtual TESRegion* GetLastLoadedRegion() {
        return pLastLoadedRegion;
    }
    // A new data object of kind aeType (2 objects, 3 weather, 4 map, 5 landscape, 6 grass, 7 sound, 8 imposter);
    // null for any other.
    virtual TESRegionData* ConstructRegionData(REGION_DATA_ID aType);
    // apData if it is that kind of data, else null.
    virtual TESRegionDataObjects* IsRegionDataObjects(TESRegionData* apData);
    virtual TESRegionDataWeather* IsRegionDataWeather(TESRegionData* apData);
    virtual TESRegionDataMap* IsRegionDataMap(TESRegionData* apData);
    virtual TESRegionDataLandscape* IsRegionDataLandscape(TESRegionData* apData);
    virtual TESRegionDataGrass* IsRegionDataGrass(TESRegionData* apData);
    virtual TESRegionDataSound* IsRegionDataSound(TESRegionData* apData);
    virtual TESRegionDataImposter* IsRegionDataImposter(TESRegionData* apData);
    // Reads the chunk apFile is at into apRegion's data; false without a file or a region.
    virtual bool LoadRegionData(TESFile* apFile, TESRegion* apRegion);

    TESRegion* pLastLoadedRegion;
};

inline TESRegionData* pRegData;
