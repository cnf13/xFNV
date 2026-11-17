#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "Reconstruction.hpp"

class TESFile;

// The map data of a region: the name shown for it on the map.
class TESRegionDataMap : public TESRegionData {
public:
	TESRegionDataMap();
	// Constructs a copy of apSource in apMemory.
	TESRegionDataMap(TESRegionDataMap* apSource);
	~TESRegionDataMap() override;

	void Save() override;
	REGION_DATA_ID GetID() const override;
	TESRegionData* Copy() override;
	TESRegionData* Blend(const TESRegionData*) override;
	void BlendInto(const TESRegionData*, uint32_t) override;
	bool Validate() override;
	virtual bool LoadRegionDataMap(TESFile*);
	// Overloads sit in the vtable in reverse declaration order (MSVC ABI); the exe has the string version first.
	// Both const, as the PDB's ?GetMapName@TESRegionDataMap@@UBAPBDXZ and UBAXAAV?$BSStringT@D@@@Z.
	virtual const char* GetMapName() const;
	// Copies the map name into the string.
	virtual void GetMapName(BSStringT<char>*) const;
	// Replaces the map name; false when it is the same.
	virtual bool SetMapName(const char*);

	BSStringT<char> MapName;
};
inline constexpr fnvd::Vtable TESRegionDataMap_vftable{};
