#include "Fallout Shared/Region System/TESRegionObjectList.h"

#include "Fallout Shared/Region System/TESRegionObject.h"

bool TESRegionObjectList::IsInListTOL(TESRegionObject* apObject, bool abIncludeChildren,
	bool abIncludeChildrenOfTextures) const {
	const BSSimpleList<TESRegionObject*>* pList;
	if (!apObject) {
		return 0;
	}
	if (IsInList(apObject)) {
		return 1;
	}
	if (!abIncludeChildren && !abIncludeChildrenOfTextures) {
		return 0;
	}
	if (this) {
		pList = static_cast<const BSSimpleList<TESRegionObject*>*>(this);
	} else {
		pList = 0;
	}
	for (TESRegionObject* const& rpItem : pList->ItemsUntilNull()) {
		if (!abIncludeChildren) {
			if (!abIncludeChildrenOfTextures) {
				continue;
			}
			// The object's virtual GetObject (slot 1), through the base the exe calls it on.
			if (!static_cast<TESRegionObjectBase*>(rpItem)->GetObject() || !rpItem->IsFormLandTexture()) {
				continue;
			}
		}
		// The exe re-zero-extends both flags before passing them on (movzx at 0x4F6F27/0x4F6F2C); clang pushes them
		// zero-extended too.
		if (rpItem->pChildren->IsInListTOL(apObject, abIncludeChildren, abIncludeChildrenOfTextures)) {
			return 1;
		}
	}
	return 0;
}
