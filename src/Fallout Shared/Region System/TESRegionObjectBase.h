#pragma once

#include <cstdint>

class TESForm;

// What a region object places: GetID's value.
enum REGION_OBJECT_ID : int32_t {
	REGION_OBJECT_OBJECT_ID = 0,
	REGION_OBJECT_GRASS_ID = 1,
	REGION_OBJECT_COUNT = 2,
};

// The base of the objects a region data entry places in the world. It has no destructor of its own: both of its
// virtuals are pure, and a derived class's destructor takes its own slot after them.
class TESRegionObjectBase {
public:
	TESRegionObjectBase();

	virtual int32_t GetID() = 0;
	virtual TESForm* GetObject() = 0;
};
