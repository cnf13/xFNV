#pragma once

#include "Fallout Shared/Region System/TESRegionData.h"

class TESFile;
class TESRegionObjectList;

// The objects data of a region: the objects it places. Only the base and the virtuals it adds are declared; the members
// pObjects and bGeneratedObjects are not declared yet.
class TESRegionDataObjects : public TESRegionData {
public:
	// These virtual functions are declared but never defined, and their order follows TESRegionData's nine.
	virtual bool LoadRegionDataObjectsOld(TESFile* apFile);
	virtual bool LoadRegionDataObjects(TESFile* apFile);
	virtual TESRegionObjectList* GetObjectList();
	virtual bool GetGeneratedObjects();
	virtual void SetGeneratedObjects(bool abGeneratedObjects);
};
