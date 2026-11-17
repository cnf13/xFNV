#pragma once

#include "BSCore/BSSimpleList.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

enum CHUNK_ID : uint32_t;
class TESFile;
class TESForm;
class TESGlobal;
class TESWeather;
struct WeatherEntry;

// One entry of the WLST chunk as Save (0x582380) writes it and LoadWeatherListChunk (0x582940) reads it: the
// weather's and the global's form IDs and the chance (PDB names; the PDB's source is TESWeatherList.cpp).
struct tmpWeatherEntry {
	tmpWeatherEntry();

	// The PDB's tmpWeatherEntry::Endian: swaps the three dwords.
	void Endian();

	uint32_t uiNumericID;
	uint32_t uiChance;
	uint32_t uiGlobalID;
};

// A weather a region or climate can have, how likely it is and the global that scales the chance. The PDB's source
// module is TESClimate.cpp.
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
