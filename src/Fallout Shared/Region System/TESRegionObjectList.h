#pragma once

#include "BSCore/BSSimpleList.h"
#include "Reconstruction.hpp"

class TESRegionObject;

// A region's object list. The PDB derives it from BSSimpleList<TESRegionObject*>, with a virtual destructor; the exe
// puts the vtable pointer in front of that base: IsInListTOL (0x4F6E7D) passes this + 4 to BSSimpleList::IsInList.
class TESRegionObjectList : public BSSimpleList<TESRegionObject*> {
public:
	virtual ~TESRegionObjectList();

	// Whether apObject is in the list or, with abIncludeChildren (with
	// abIncludeChildrenOfTextures, only for items whose form is a TESLandTexture, 0x4F6F70), in an item's child list,
	// searched recursively (thiscall, ret 0xC, 0x4F6E60).
	bool IsInListTOL(TESRegionObject* apObject, bool abIncludeChildren, bool abIncludeChildrenOfTextures) const;

	bool bOwnsTESRegionObjectMemory;
	uint32_t iCount;
};
