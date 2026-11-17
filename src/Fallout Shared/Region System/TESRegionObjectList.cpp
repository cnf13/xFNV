#include "Fallout Shared/Region System/TESRegionObjectList.h"

#include "Fallout Shared/Region System/TESRegionObject.h"

bool TESRegionObjectList::IsInListTOL(
    TESRegionObject* apObject,
    bool abIncludeChildren,
    bool abIncludeChildrenOfTextures
) const {
    const BSSimpleList<TESRegionObject*>* pList;
    if (!apObject) {
        return false;
    }
    if (IsInList(apObject)) {
        return true;
    }
    if (!abIncludeChildren && !abIncludeChildrenOfTextures) {
        return false;
    }
    if (this) {
        pList = static_cast<const BSSimpleList<TESRegionObject*>*>(this);
    }
    else {
        pList = nullptr;
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
            return true;
        }
    }
    return false;
}
