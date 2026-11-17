#pragma once

#include "Fallout Shared/Region System/TESRegionData.h"
#include "Fallout Shared/TESWeatherList.h"

class TESFile;
class TESRegion;

// A region's weather list (RDWT).
class TESRegionDataWeather : public TESRegionData {
public:
	TESRegionDataWeather();
	// The copy constructor; reads the source's weatherList.
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
