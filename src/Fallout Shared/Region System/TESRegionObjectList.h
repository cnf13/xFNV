#pragma once

#include "BSCore/BSSimpleList.h"

class TESRegionObject;

// A region's object list. It derives from BSSimpleList<TESRegionObject*>, with a virtual destructor.
class TESRegionObjectList : public BSSimpleList<TESRegionObject*> {
public:
	virtual ~TESRegionObjectList();

	// Whether apObject is in the list or, with abIncludeChildren (with abIncludeChildrenOfTextures, only for items
	// whose form is a TESLandTexture), in an item's child list, searched recursively.
	bool IsInListTOL(TESRegionObject* apObject, bool abIncludeChildren, bool abIncludeChildrenOfTextures) const;

	bool bOwnsTESRegionObjectMemory;
	uint32_t iCount;
};
