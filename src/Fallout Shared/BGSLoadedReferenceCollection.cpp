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
    if (aeFormType == WEAP_ID) {
        return &WeaponsMap;
    }
    if (aeFormType == AMMO_ID) {
        return &AmmoMap;
    }
    return nullptr;
}

void BGSLoadedReferenceCollection::AddReference(TESObjectREFR* apRef) {
    AutoMemContext kMemContext(
        MEM_CONTEXT_GLOBAL_LISTS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSLoadedReferenceCollection.cpp",
        104
    );
    ENUM_FORM_ID eFormType = apRef->GetObjectReference()->GetFormType();
    if (TESContainer::ContainerCanHoldType(eFormType)) {
        ItemsMap.AddReference(apRef);
        LoadedReferenceMap* pFormTypeMap = GetFormTypeMap(eFormType);
        if (pFormTypeMap) {
            pFormTypeMap->AddReference(apRef);
        }
    }
    if (eFormType == CONT_ID) {
        ContainersMap.AddReference(apRef);
    }
    if (eFormType == NPC__ID || eFormType == CREA_ID) {
        ActorsMap.AddReference(apRef);
    }
    if (apRef->GetObjectReference() == pRadiationMarker) {
        RadiationMap.AddReference(apRef);
    }
    if (eFormType == EXPL_ID && apRef->GetExtraData(EXTRA_RADIATION)) {
        RadiationMap.AddReference(apRef);
    }

    if (eFormType == DOOR_ID && apRef->GetTeleport()) {
        TeleportDoorMap.AddReference(apRef);
    }
}

void BGSLoadedReferenceCollection::RemoveReference(TESObjectREFR* apRef) {
    ENUM_FORM_ID eFormType = apRef->GetObjectReference()->GetFormType();
    if (TESContainer::ContainerCanHoldType(eFormType)) {
        ItemsMap.RemoveReference(apRef);
        LoadedReferenceMap* pFormTypeMap = GetFormTypeMap(eFormType);
        if (pFormTypeMap) {
            pFormTypeMap->RemoveReference(apRef);
        }
    }
    if (eFormType == CONT_ID) {
        ContainersMap.RemoveReference(apRef);
    }
    if (eFormType == NPC__ID || eFormType == CREA_ID) {
        ActorsMap.RemoveReference(apRef);
    }
    if (apRef->GetObjectReference() == pRadiationMarker) {
        RadiationMap.RemoveReference(apRef);
    }
    if (eFormType == EXPL_ID) {
        RadiationMap.RemoveReference(apRef);
    }
    if (eFormType == DOOR_ID && apRef->GetTeleport()) {
        TeleportDoorMap.RemoveReference(apRef);
    }
}

bool BGSLoadedReferenceCollection::GetNextObject(
    FindObjectIterator& arIterator,
    TESObjectREFR*& arpRef,
    ItemChange*& arpItemChange,
    uint32_t aiFindFlags
) {
    while (arIterator.pCurrentMap) {
        if (arIterator.pCurrentRef) {
            ItemChange* pItemChange = nullptr;
            while (arIterator.pCurrentInventory->GetNextItem(arIterator.InvIterator, pItemChange, 1u)) {
                if (arIterator.pBoundObject ? pItemChange->pContainerObj == arIterator.pBoundObject
                                            : pItemChange->pContainerObj->GetFormType() == arIterator.eFormType) {
                    if (pItemChange->iNumber) {
                        arpRef = arIterator.pCurrentRef;
                        arpItemChange = pItemChange;
                        return true;
                    }
                }
            }
            arIterator.InvIterator.Reset();
            arIterator.pCurrentInventory = nullptr;
            arIterator.pCurrentRef = nullptr;
        }
        while (!arIterator.MapIterator) {
            uint8_t cMapIndex = arIterator.cMapIndex;
            if (cMapIndex) {
                if (cMapIndex == 1) {
                    if ((aiFindFlags & (FIND_ON_DEAD_ACTORS | FIND_ON_LIVING_ACTORS)) != 0) {
                        arIterator.pCurrentMap = &ActorsMap;
                    }
                }
                else if (cMapIndex == 2) {
                    arIterator.pCurrentMap = nullptr;
                    return false;
                }
            }
            else if ((aiFindFlags & FIND_IN_CONTAINERS) != 0) {
                arIterator.pCurrentMap = &ContainersMap;
            }
            ++arIterator.cMapIndex;
            if (arIterator.pCurrentMap) {
                arIterator.MapIterator = arIterator.pCurrentMap->GetFirstPos();
            }
        }
        bool bContainerMap = arIterator.cMapIndex != 0;
        bool bSpecificMap = arIterator.pCurrentMap != &ItemsMap;
        while (arIterator.MapIterator) {
            uint32_t uiFormID = 0;
            TESObjectREFR* pRef = nullptr;
            arIterator.pCurrentMap->GetNext(arIterator.MapIterator, uiFormID, pRef);
            if (!pRef->GetDisabled() && !pRef->GetDelete() && !pRef->IsQuestObject()) {
                TESBoundObject* pBaseForm = pRef->GetObjectReference();
                if (!pBaseForm->GetQuestObject() &&
                    (!pRef->TESForm::GetObstacle() || (aiFindFlags & FIND_OBSTACLES) != 0)) {
                    if (bContainerMap) {
                        if (arIterator.pCurrentMap == &ActorsMap
                                ? (aiFindFlags & FIND_ON_DEAD_ACTORS) != 0 ==
                                        ((aiFindFlags & FIND_ON_LIVING_ACTORS) != 0) ||
                                    pRef->IsDead(false) != ((aiFindFlags & FIND_ON_DEAD_ACTORS) == 0)
                                : pRef->GetObjectReference()->GetFormType() != CONT_ID ||
                                    static_cast<TESObjectCONT*>(pRef->GetObjectReference())
                                        ->CanActivate(pRef, arIterator.pActivatingRef, false)) {
                            arIterator.pCurrentInventory = pRef->m_Extra.GetContainerChanges();
                            if (arIterator.pCurrentInventory) {
                                arIterator.pCurrentRef = pRef;
                            }
                            break;
                        }
                    }
                    else if (
                        arIterator.pBoundObject
                            ? arIterator.pBoundObject == pRef->GetObjectReference()
                            : bSpecificMap || arIterator.eFormType == pRef->GetObjectReference()->GetFormType()
                    ) {
                        arpRef = pRef;
                        arpItemChange = nullptr;
                        return true;
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
    AutoMemContext kMemContext(
        MEM_CONTEXT_GLOBAL_LISTS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSLoadedReferenceCollection.cpp",
        88
    );
    pLoadedReferenceCollection = new BGSLoadedReferenceCollection;
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
    pContainerObjectList = nullptr;
    pInventoryChangeList = nullptr;
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
