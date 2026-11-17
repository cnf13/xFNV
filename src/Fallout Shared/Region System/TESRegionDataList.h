#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/Region System/TESRegionData.h"

// The list of data entries of a region; bOwnsDataMemory tells whether the list owns the entries it holds.
class TESRegionDataList : public BSSimpleList<TESRegionData*> {
public:
	TESRegionDataList(bool abOwnsDataMemory);
	~TESRegionDataList();

	// The data of the kind aeFind, or null.
	TESRegionData* Find(const REGION_DATA_ID aFind);
	bool Add(TESRegionData* apData);
	// Whether every entry validates.
	bool Validate();
	TESRegionDataList* Copy(TESRegionDataList* apCopyIntoList);
	void Clear();

	bool bOwnsDataMemory;
};
