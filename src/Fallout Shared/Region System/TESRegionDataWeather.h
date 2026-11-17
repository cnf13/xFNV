#pragma once

#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/TESWeatherList.h"
#include "Reconstruction.hpp"

class TESFile;
class TESRegion;

// A region's weather list (RDWT).
class TESRegionDataWeather : public TESRegionData {
public:
	TESRegionDataWeather();
	// The copy constructor (thiscall, ret 4, 0x4F5140). PDB: TESRegionDataWeather(const TESRegionDataWeather&); the exe
	// reads the source's weatherList (+0x8).
	TESRegionDataWeather(TESRegionDataWeather* apSource);
	~TESRegionDataWeather() override;

	void Save() override;
	REGION_DATA_ID GetID() const override;
	TESRegionData* Copy() override;
	TESRegionData* Blend(const TESRegionData*) override;
	void BlendInto(const TESRegionData*, uint32_t) override;
	bool Validate() override;
	virtual void LoadRegionDataWeather(TESFile* apFile, TESRegion* apRegion);

	// A random weather of weatherList.
	TESWeather* GetRandomWeather();

	TESWeatherList weatherList;
};
inline constexpr fnvd::Vtable TESRegionDataWeather_vftable{};
