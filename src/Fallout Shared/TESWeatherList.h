#pragma once

#include "BSCore/BSSimpleList.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class TESFile;
class TESForm;
class TESGlobal;
class TESWeather;
struct WeatherEntry;

enum CHUNK_ID : uint32_t;

// One entry of the WLST chunk as Save writes it and LoadWeatherListChunk reads it: the weather's and the global's form
// IDs and the chance.
struct tmpWeatherEntry {
    tmpWeatherEntry();

    // Swaps the byte order of uiNumericID, uiChance and uiGlobalID.
    void Endian();

    uint32_t uiNumericID;
    uint32_t uiChance;
    uint32_t uiGlobalID;
};

// A weather a region or climate can have, how likely it is and the global that scales the chance.
struct WeatherEntry {
    WeatherEntry();

    TESWeather* pWeather;
    uint32_t uiChance;
    TESGlobal* pChanceVar;
};

// A list of weathers.
class TESWeatherList : public BSSimpleList<WeatherEntry*> {
public:
    TESWeatherList();
    ~TESWeatherList();

    // Copies apEntry into the list.
    void CopyComponent(const TESWeatherList* apCopyWL, bool abAppend);
    // A random weather of the list.
    TESWeather* GetRandomWeather();
    // Writes the list's entries as chunk aeChunkID.
    void Save(CHUNK_ID aeChunkID);
    // Frees every entry (ourFree; WeatherEntry has no destructor) and then the list's nodes.
    void FlushList();
    bool CompareComponent(TESWeatherList* apCompareWL);

    static int32_t SortCompare(WeatherEntry* p1, WeatherEntry* p2);
    // Reads the list's entries from the WLST chunk of apFile, warning about the weather forms
    // that do not resolve for pForm.
    static void LoadWeatherListChunk(TESWeatherList* apWeatherList, TESFile* apFile, TESForm* pForm);
};
