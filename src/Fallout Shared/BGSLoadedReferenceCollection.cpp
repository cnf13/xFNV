#include "Fallout Shared/BGSLoadedReferenceCollection.h"

#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/FindObjectIterator.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/InventoryIterator.h"
#include "Fallout Shared/LoadedReferenceMap.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectSTAT.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout/AI/Actor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

BGSLoadedReferenceCollection::BGSLoadedReferenceCollection() {}

BGSLoadedReferenceCollection::~BGSLoadedReferenceCollection() {}

LoadedReferenceMap* BGSLoadedReferenceCollection::GetFormTypeMap(ENUM_FORM_ID aeFormType) {
    if (((int32_t)aeFormType) == 40) {
        return &WeaponsMap;
    }
    if (((int32_t)aeFormType) == 41) {
        return &AmmoMap;
    }
    return nullptr;
}

void BGSLoadedReferenceCollection::AddReference(TESObjectREFR* apRef) {
    AutoMemContext apMemory(
        MEM_CONTEXT_GLOBAL_LISTS, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSLoadedReferenceCollection.cpp", 104
    );
    TESBoundObject* ObjectReference = apRef->GetObjectReference();
    ENUM_FORM_ID aeFormType = ObjectReference->GetFormType();
    if (TESContainer::ContainerCanHoldType(aeFormType)) {
        ItemsMap.AddReference(apRef);
        LoadedReferenceMap* FormTypeMap = GetFormTypeMap(aeFormType);
        if (FormTypeMap) {
            FormTypeMap->AddReference(apRef);
        }
    }
    if (aeFormType == CONT_ID) {
        ContainersMap.AddReference(apRef);
    }
    if (aeFormType == NPC__ID || aeFormType == CREA_ID) {
        ActorsMap.AddReference(apRef);
    }
    if (apRef->GetObjectReference() == pRadiationMarker) {
        RadiationMap.AddReference(apRef);
    }
    if ((aeFormType == EXPL_ID) && (apRef->GetExtraData(93u))) {
        RadiationMap.AddReference(apRef);
    }

    if ((aeFormType == DOOR_ID) && (apRef->GetTeleport())) {
        TeleportDoorMap.AddReference(apRef);
    }
}

void BGSLoadedReferenceCollection::RemoveReference(TESObjectREFR* apRef) {
    TESBoundObject* ObjectReference = apRef->GetObjectReference();
    ENUM_FORM_ID aeFormType = ObjectReference->GetFormType();
    if (TESContainer::ContainerCanHoldType(aeFormType)) {
        ItemsMap.RemoveReference(apRef);
        LoadedReferenceMap* FormTypeMap = GetFormTypeMap(aeFormType);
        if (FormTypeMap) {
            FormTypeMap->RemoveReference(apRef);
        }
    }
    if (aeFormType == CONT_ID) {
        ContainersMap.RemoveReference(apRef);
    }
    if (aeFormType == NPC__ID || aeFormType == CREA_ID) {
        ActorsMap.RemoveReference(apRef);
    }
    if (apRef->GetObjectReference() == pRadiationMarker) {
        RadiationMap.RemoveReference(apRef);
    }
    if (aeFormType == EXPL_ID) {
        RadiationMap.RemoveReference(apRef);
    }
    if ((aeFormType == DOOR_ID) && (apRef->GetTeleport())) {
        TeleportDoorMap.RemoveReference(apRef);
    }
}

bool BGSLoadedReferenceCollection::GetNextObject(
    FindObjectIterator& arIterator,
    TESObjectREFR*& arpRef,
    ItemChange*& arpItemChange,
    uint32_t aiFindFlags
) {
LABEL_1:;
    while (arIterator.pCurrentMap) {
        ItemChange* pitemChange;
        if (!arIterator.pCurrentRef) {
            goto LABEL_12;
        }
        pitemChange = nullptr;
        while (arIterator.pCurrentInventory->GetNextItem(arIterator.InvIterator, pitemChange, 1u)) {
            if (arIterator.pBoundObject) {
                if (pitemChange->pContainerObj == arIterator.pBoundObject) {
                    goto LABEL_9;
                }
            }
            else {
                if (pitemChange->pContainerObj->GetFormType() == arIterator.eFormType) {
                LABEL_9:;
                    if (pitemChange->iNumber) {
                        arpRef = arIterator.pCurrentRef;
                        arpItemChange = pitemChange;
                        return true;
                    }
                }
            }
        }
        arIterator.InvIterator.Reset();
        arIterator.pCurrentInventory = nullptr;
        arIterator.pCurrentRef = nullptr;
    LABEL_12:;
        while (!arIterator.MapIterator) {
            uint8_t cMapIndex = arIterator.cMapIndex;
            if (cMapIndex) {
                if (cMapIndex == 1) {
                    if ((aiFindFlags & 6) != 0) {
                        arIterator.pCurrentMap = &ActorsMap;
                    }
                }
                else {
                    if (cMapIndex == 2) {
                        arIterator.pCurrentMap = nullptr;
                        return false;
                    }
                }
            }
            else {
                if ((aiFindFlags & 1) != 0) {
                    arIterator.pCurrentMap = &ContainersMap;
                }
            }
            ++arIterator.cMapIndex;
            if (arIterator.pCurrentMap) {
                arIterator.MapIterator = arIterator.pCurrentMap->GetFirstPos();
            }
        }
        bool bcontainerMap = arIterator.cMapIndex != 0;
        bool bspecificMap = arIterator.pCurrentMap != &ItemsMap;
        while (arIterator.MapIterator) {
            uint32_t iformID = 0;
            TESObjectREFR* pref = nullptr;
            arIterator.pCurrentMap->GetNext(arIterator.MapIterator, iformID, pref);
            if (!pref->GetDisabled() && !pref->GetDelete() && !pref->IsQuestObject()) {
                TESBoundObject* pBaseForm = pref->GetObjectReference();
                if (!pBaseForm->GetQuestObject() && (!pref->TESForm::GetObstacle() || (aiFindFlags & 8) != 0)) {
                    if (bcontainerMap) {
                        if (arIterator.pCurrentMap == &ActorsMap) {
                            if ((aiFindFlags & 2) != 0 == ((aiFindFlags & 4) != 0) ||
                                pref->IsDead(false) != ((aiFindFlags & 2) == 0)) {
                                goto LABEL_50;
                            }
                        }
                        else {
                            TESObjectCONT* pcontainer;
                            if (pref->GetObjectReference()->GetFormType() != CONT_ID ||
                                (pcontainer = static_cast<TESObjectCONT*>(pref->GetObjectReference()),
                                 pcontainer->CanActivate(pref, arIterator.pActivatingRef, false))) {
                            LABEL_50:;
                                arIterator.pCurrentInventory = (&pref->m_Extra)->GetContainerChanges();
                                if (arIterator.pCurrentInventory) {
                                    arIterator.pCurrentRef = pref;
                                }
                                goto LABEL_1;
                            }
                        }
                    }
                    else {
                        if (arIterator.pBoundObject) {
                            if (arIterator.pBoundObject == pref->GetObjectReference()) {
                                goto LABEL_43;
                            }
                        }
                        else {
                            if (bspecificMap || arIterator.eFormType == pref->GetObjectReference()->GetFormType()) {
                            LABEL_43:;
                                arpRef = pref;
                                arpItemChange = nullptr;
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

void BGSLoadedReferenceCollection::Destroy() {
    if (pLoadedReferenceCollection) {
        delete pLoadedReferenceCollection;
    }
    pLoadedReferenceCollection = nullptr;
}

void BGSLoadedReferenceCollection::Create() {
    BGSLoadedReferenceCollection* v0;
    AutoMemContext apMemory(
        MEM_CONTEXT_GLOBAL_LISTS, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSLoadedReferenceCollection.cpp", 88
    );
    BGSLoadedReferenceCollection* v1 =
        static_cast<BGSLoadedReferenceCollection*>(ourMalloc(sizeof(BGSLoadedReferenceCollection)));
    if (v1) {
        v0 = ::new (v1) BGSLoadedReferenceCollection;
    }
    else {
        v0 = nullptr;
    }
    pLoadedReferenceCollection = v0;
}

FindObjectIterator::FindObjectIterator(ENUM_FORM_ID aeFormType, TESObjectREFR* apActivatingRef)
  : eFormType(aeFormType), pBoundObject(nullptr), pActivatingRef(apActivatingRef) {
    Reset();
}

FindObjectIterator::FindObjectIterator(TESForm* apForm, TESObjectREFR* apRef)
  : eFormType(apForm->GetFormType()), pBoundObject(static_cast<TESBoundObject*>(apForm)), pActivatingRef(apRef) {
    Reset();
}

FindObjectIterator::~FindObjectIterator() {}

void FindObjectIterator::Reset() {
    pCurrentRef = nullptr;
    pCurrentInventory = nullptr;
    cMapIndex = 0;
    pCurrentMap = pLoadedReferenceCollection->GetFormTypeMap(eFormType);
    if (!pCurrentMap) {
        pCurrentMap = &pLoadedReferenceCollection->ItemsMap;
    }
    MapIterator = pCurrentMap->GetFirstPos();
}

InventoryIterator::InventoryIterator()
  : pContainerObjectList(nullptr), pInventoryChangeList(nullptr), bFinished(false) {}

// The instance list is borrowed from the change being walked: cleared so that the change's destructor keeps it.
InventoryIterator::~InventoryIterator() {
    TempItemChange.pExtraObjectList = nullptr;
}

void InventoryIterator::Reset() {
    pContainerObjectList = (BSSimpleList<ContainerObject*>*)nullptr;
    pInventoryChangeList = (BSSimpleList<ItemChange*>*)nullptr;
    bFinished = false;
}

LoadedReferenceMap::LoadedReferenceMap()
  : NiTMap<uint32_t, TESObjectREFR*>(0x25u), iCount(0), iLastChangedTimeStamp(0) {}

LoadedReferenceMap::~LoadedReferenceMap() {
    RemoveAll();
}

bool LoadedReferenceMap::AddReference(TESObjectREFR* apRef) {
    TESObjectREFR* pRef = nullptr;
    if (GetAt(apRef->GetFormID(), pRef)) {
        return false;
    }
    SetAt(apRef->GetFormID(), apRef);
    ++iCount;
    iLastChangedTimeStamp = appTimer.GetTime();
    return true;
}

bool LoadedReferenceMap::RemoveReference(TESObjectREFR* apRef) {
    if (!RemoveAt(apRef->GetFormID())) {
        return false;
    }
    --iCount;
    iLastChangedTimeStamp = appTimer.GetTime();
    return true;
}
