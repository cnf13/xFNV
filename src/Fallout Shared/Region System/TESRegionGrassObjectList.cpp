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
    for (TESRegionGrassObject* i = m_item; i; i = m_item) {
        RemoveHead();
        if (bOwnsTESRegionGrassObjectMemory) {
            delete i;
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
    TESRegionGrassObjectList* pcopy;
    BSSimpleList<TESRegionGrassObject*>* pcurrent;
    if (this) {
        pcurrent = static_cast<BSSimpleList<TESRegionGrassObject*>*>(this);
    }
    else {
        pcurrent = nullptr;
    }
    BSSimpleList<TESRegionGrassObject*>* pcurrent_2 = pcurrent;
    if (apCopyInto) {
        pcopy = apCopyInto;
    }
    else {
        TESRegionGrassObjectList* pcopy_2;
        TESRegionGrassObjectList* apMemory =
            static_cast<TESRegionGrassObjectList*>(ourMalloc(sizeof(TESRegionGrassObjectList)));
        if (apMemory) {
            pcopy_2 = ::new (apMemory) TESRegionGrassObjectList(bOwnsTESRegionGrassObjectMemory);
        }
        else {
            pcopy_2 = nullptr;
        }
        pcopy = pcopy_2;
    }
    while (pcurrent_2 && pcurrent_2->m_item) {
        if (pcopy->bOwnsTESRegionGrassObjectMemory) {
            TESRegionGrassObject** v2 = &pcurrent_2->m_item;
            TESRegionGrassObject* pobjCopy = (*v2)->Copy(nullptr);
            if ((!pcopy->Add(pobjCopy)) && (pobjCopy)) {
                delete pobjCopy;
            }
        }
        else {
            TESRegionGrassObject** v3 = &pcurrent_2->m_item;
            pcopy->Add(*v3);
        }
        pcurrent_2 = pcurrent_2->m_pkNext;
    }
    return pcopy;
}

void TESRegionGrassObjectList::MergeLists(
    float afPriority,
    TESRegionGrassObjectList* apOtherList,
    float afOtherPriority
) {
    float fpriority;
    float fpriorityOther;
    if (!(afPriority > 1.0)) {
        fpriority = afPriority;
    }
    else {
        fpriority = afPriority / 100.0;
    }
    if (!(afOtherPriority > 1.0)) {
        fpriorityOther = afOtherPriority;
    }
    else {
        fpriorityOther = afOtherPriority / 100.0;
    }
    if ((apOtherList) &&
        (!(fpriority < 0.0) && !(fpriority > 1.0) && !(fpriorityOther < 0.0) && !(fpriorityOther > 1.0))) {
        for (BSSimpleList<TESRegionGrassObject*>* pcurrent =
                 static_cast<BSSimpleList<TESRegionGrassObject*>*>(apOtherList);
             pcurrent && pcurrent->m_item; pcurrent = pcurrent->m_pkNext) {
            TESRegionGrassObject* m_item = pcurrent->m_item;
            if (m_item->GetObject()) {
                TESRegionGrassObject* pobjCopy = pcurrent->m_item->Copy(nullptr);
                Add(pobjCopy);
            }
        }
    }
}

TESRegionGrassObject* TESRegionGrassObjectList::Find(uint32_t aiFind, int32_t aiParentFind) {
    BSSimpleList<TESRegionGrassObject*>* v7;
    if (this) {
        v7 = static_cast<BSSimpleList<TESRegionGrassObject*>*>(this);
    }
    else {
        v7 = nullptr;
    }
    BSSimpleList<TESRegionGrassObject*>* ecx0 = v7;
    TESRegionGrassObject* v9 = nullptr;
    while (ecx0 && ecx0->m_item) {
        TESRegionGrassObject** v3 = &ecx0->m_item;
        TESRegionGrassObject* v10 = *v3;
        TESForm* v4 = (*v3)->GetObject();
        if (v4->iFormID == aiFind) {
            if ((aiParentFind >= 0) && (v10->GetParentTexture())) {
                TESLandTexture* v5 = v10->GetParentTexture();
                if (v5->iFormID == aiParentFind) {
                    return v10;
                }
            }

            if (aiParentFind < 0 && !v10->GetParentTexture()) {
                return v10;
            }
        }
        ecx0 = ecx0->m_pkNext;
    }
    return v9;
}
