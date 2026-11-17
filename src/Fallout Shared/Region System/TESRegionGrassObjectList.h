#pragma once

#include "BSCore/BSSimpleList.h"

class TESRegionGrassObject;

// The grass objects of a region data entry: a list of grass object pointers with a flag for whether the list owns them
// and their count.
class TESRegionGrassObjectList : public BSSimpleList<TESRegionGrassObject*> {
public:
	TESRegionGrassObjectList(bool abOwnsTESRegionGrassObjectMemory);
	virtual ~TESRegionGrassObjectList();

	// Adds apObject at the tail without counting it.
	void AddTailSimple(TESRegionGrassObject* apObject);
	void Clear();
	bool Add(TESRegionGrassObject* apObject);
	TESRegionGrassObjectList* Copy(TESRegionGrassObjectList* apCopyInto);
	// Adds a copy of each of apOtherList's objects that has a form.
	void MergeLists(float afPriority, TESRegionGrassObjectList* apOtherList, float afOtherPriority);
	TESRegionGrassObject* Find(uint32_t aiFind, int32_t aiParentFind);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	bool bOwnsTESRegionGrassObjectMemory;
	uint32_t iCount;
};
