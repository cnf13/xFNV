#include "Fallout Shared/Region System/TESRegionGrassObjectList.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Region System/TESRegionGrassObject.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLandTexture.h"

TESRegionGrassObjectList::TESRegionGrassObjectList(bool abOwnsTESRegionGrassObjectMemory)
  : BSSimpleList(), bOwnsTESRegionGrassObjectMemory(abOwnsTESRegionGrassObjectMemory), iCount(0) {}

TESRegionGrassObjectList::~TESRegionGrassObjectList() {
    Clear();
}

void TESRegionGrassObjectList::AddTailSimple(TESRegionGrassObject* apObject) {
    static_cast<BSSimpleList<TESRegionGrassObject*>*>(this)->AddTail(apObject);
    ++iCount;
}

void TESRegionGrassObjectList::Clear() {
    for (TESRegionGrassObject* pObject = m_item; pObject; pObject = m_item) {
        RemoveHead();
        if (bOwnsTESRegionGrassObjectMemory) {
            delete pObject;
        }
    }
    iCount = 0;
}

bool TESRegionGrassObjectList::Add(TESRegionGrassObject* apObject) {
    if (!apObject || IsInList(apObject)) {
        return false;
    }
    AddTailSimple(apObject);
    return true;
}

TESRegionGrassObjectList* TESRegionGrassObjectList::Copy(TESRegionGrassObjectList* apCopyInto) {
    TESRegionGrassObjectList* pCopy;
    BSSimpleList<TESRegionGrassObject*>* pCurrent = nullptr;
    if (this) {
        pCurrent = static_cast<BSSimpleList<TESRegionGrassObject*>*>(this);
    }
    if (apCopyInto) {
        pCopy = apCopyInto;
    }
    else {
        pCopy = new TESRegionGrassObjectList(bOwnsTESRegionGrassObjectMemory);
    }
    while (pCurrent && pCurrent->m_item) {
        if (pCopy->bOwnsTESRegionGrassObjectMemory) {
            TESRegionGrassObject* pObjCopy = pCurrent->m_item->Copy(nullptr);
            if (!pCopy->Add(pObjCopy) && pObjCopy) {
                delete pObjCopy;
            }
        }
        else {
            pCopy->Add(pCurrent->m_item);
        }
        pCurrent = pCurrent->m_pkNext;
    }
    return pCopy;
}

void TESRegionGrassObjectList::MergeLists(
    float afPriority,
    TESRegionGrassObjectList* apOtherList,
    float afOtherPriority
) {
    float fPriority;
    float fPriorityOther;
    if (afPriority > 1.0) {
        fPriority = afPriority / 100.0;
    }
    else {
        fPriority = afPriority;
    }
    if (afOtherPriority > 1.0) {
        fPriorityOther = afOtherPriority / 100.0;
    }
    else {
        fPriorityOther = afOtherPriority;
    }
    if (apOtherList && !(fPriority < 0.0) && !(fPriority > 1.0) && !(fPriorityOther < 0.0) && !(fPriorityOther > 1.0)) {
        for (BSSimpleList<TESRegionGrassObject*>* pCurrent =
                 static_cast<BSSimpleList<TESRegionGrassObject*>*>(apOtherList);
             pCurrent && pCurrent->m_item;
             pCurrent = pCurrent->m_pkNext) {
            if (pCurrent->m_item->GetObject()) {
                TESRegionGrassObject* pObjCopy = pCurrent->m_item->Copy(nullptr);
                Add(pObjCopy);
            }
        }
    }
}

TESRegionGrassObject* TESRegionGrassObjectList::Find(uint32_t aiFind, int32_t aiParentFind) {
    BSSimpleList<TESRegionGrassObject*>* pCurrent = nullptr;
    if (this) {
        pCurrent = static_cast<BSSimpleList<TESRegionGrassObject*>*>(this);
    }
    while (pCurrent && pCurrent->m_item) {
        TESRegionGrassObject* pObject = pCurrent->m_item;
        TESForm* pForm = pObject->GetObject();
        if (pForm->iFormID == aiFind) {
            if (aiParentFind >= 0 && pObject->GetParentTexture()) {
                TESLandTexture* pParentTexture = pObject->GetParentTexture();
                if (pParentTexture->iFormID == aiParentFind) {
                    return pObject;
                }
            }

            if (aiParentFind < 0 && !pObject->GetParentTexture()) {
                return pObject;
            }
        }
        pCurrent = pCurrent->m_pkNext;
    }
    return nullptr;
}
