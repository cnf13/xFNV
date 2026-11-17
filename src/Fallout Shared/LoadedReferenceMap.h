#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Reconstruction.hpp"

class TESObjectREFR;

// A map from form ID to loaded reference with a count and the time of the last change (the PDB's iCount and
// iLastChangedTimeStamp). The PDB lists the class under BGSLoadedReferenceCollection.cpp, whose header does not
// declare it.
class LoadedReferenceMap : public NiTMap<uint32_t, TESObjectREFR*> {
public:
	LoadedReferenceMap();
	~LoadedReferenceMap() override;

	// Maps apRef under its form ID unless the ID is already mapped; stamps the change time when added.
	bool AddReference(TESObjectREFR* apRef);
	// Unmaps apRef's form ID; stamps the change time when it was mapped.
	bool RemoveReference(TESObjectREFR* apRef);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	uint32_t iCount;
	uint32_t iLastChangedTimeStamp;
};
inline constexpr fnvd::Vtable LoadedReferenceMap_vftable{};
