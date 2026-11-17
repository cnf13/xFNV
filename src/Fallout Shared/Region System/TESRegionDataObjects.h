#pragma once

#include "Fallout Shared/Region System/TESRegionData.h"
#include "Reconstruction.hpp"

class TESFile;
class TESRegionObjectList;

// The objects data of a region: the objects it places. Only the base and the virtuals it adds are modelled so far;
// the PDB's pObjects and bGeneratedObjects are not declared yet.
class TESRegionDataObjects : public TESRegionData {
public:
	// The PC exe has no RTTI or vtable of this class, so these are declared, never defined: the slots follow the
	// PDB's order after TESRegionData's nine, which the exe's call of slot 11 (+0x2C, GetObjectList) at 0x550009
	// (TESObjectCELL::GetScaleBasedOnLocation) confirms.
	virtual bool LoadRegionDataObjectsOld(TESFile* apFile);
	virtual bool LoadRegionDataObjects(TESFile* apFile);
	virtual TESRegionObjectList* GetObjectList();
	virtual bool GetGeneratedObjects();
	virtual void SetGeneratedObjects(bool abGeneratedObjects);
};
