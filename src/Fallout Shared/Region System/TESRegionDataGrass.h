#pragma once

#include "Fallout Shared/Region System/TESRegionData.h"

class TESFile;
class TESRegionGrassObjectList;

// A region's grass list (RDGS).
class TESRegionDataGrass : public TESRegionData {
public:
	TESRegionDataGrass();
	// Constructs a copy of apSource in apMemory.
	TESRegionDataGrass(TESRegionDataGrass* apSource);
	~TESRegionDataGrass() override;

	void Save() override;
	REGION_DATA_ID GetID() const override;
	TESRegionData* Copy() override;
	TESRegionData* Blend(const TESRegionData*) override;
	void BlendInto(const TESRegionData*, uint32_t) override;
	bool Validate() override;
	virtual bool LoadRegionDataGrass(TESFile* apFile);
	virtual TESRegionGrassObjectList* GetObjectList() { return pObjects; }

	TESRegionGrassObjectList* pObjects;
};
