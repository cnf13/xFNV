#include "Fallout Shared/Region System/TESRegionObjectList.h"

#include "Fallout Shared/Region System/TESRegionObject.h"

bool TESRegionObjectList::IsInListTOL(
    TESRegionObject* apObject,
    bool abIncludeChildren,
    bool abIncludeChildrenOfTextures
) const {
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
    }
    else {
        pList = 0;
    }
    for (TESRegionObject* const& rpItem : pList->ItemsUntilNull()) {
        if (!abIncludeChildren) {
            if (!abIncludeChildrenOfTextures) {
                continue;
            }
            if (!static_cast<TESRegionObjectBase*>(rpItem)->GetObject() || !rpItem->IsFormLandTexture()) {
                continue;
            }
        }
        if (rpItem->pChildren->IsInListTOL(apObject, abIncludeChildren, abIncludeChildrenOfTextures)) {
            return 1;
        }
    }
    return 0;
}
