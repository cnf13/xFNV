#include "Fallout Shared/Havok/TESTrapListener.h"

#include "BSCore/BSList.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkEntityListener.h"
#include "BSHavok/bhkTTNode.h"
#include "BSHavok/bhkTrapListener.h"
#include "BSHavok/bhkWorld.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/Havok/TrapEntry.h"
#include "Fallout Shared/Havok/TriggerEntry.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpAllCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpRootCdBodyPair.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomType.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h"

#include <cstdint>
#include <xmmintrin.h>

TrapTargetEntry::TrapTargetEntry(TESObjectREFR* apRefr, const hkpCollidable* apCollidable)
  : TargetEntry(apRefr, apCollidable) {}

void TrapTargetEntry::Run(ZoneEntry* apTrap) {
    pRef->DoTrap(static_cast<TrapEntry*>(apTrap), this);
}

TESTrapListener::TESTrapListener() : pTriggers(nullptr) {}

TESTrapListener::~TESTrapListener() {
    EmptyTriggerList();
}

ZoneEntry* TESTrapListener::CreateZoneEntry(TESObjectREFR* apRef, const hkpCollidable* apCol) {
    return new TrapEntry(apRef, apCol);
}

void TESTrapListener::Update(float afDelta) {
    for (TriggerEntry* ptriggers = pTriggers; ptriggers; ptriggers = ptriggers->QNextEntry()) {
        ptriggers->Update(afDelta);
    }
    for (ZoneEntry* pTrap = pZones; pTrap; pTrap = pTrap->QNextEntry()) {
        pTrap->Run();
    }
}

void TESTrapListener::entityRemovedCallback(hkpEntity* apEntity) {
    CFilter ifilt;
    const hkpCollidable* pCol = apEntity->getCollidable();
    ifilt.iFilter = pCol->getCollisionFilterInfo();
    COL_LAYER eLayer = ifilt.GetLayer();
    if (eLayer == L_TRAP || eLayer == L_CLOUDTRAP || eLayer == L_CHARCONTROLLER) {
        TESObjectREFR* pRef = TESHavokUtilities::FindCollidableRef(pCol);
        if (pRef) {
            if (eLayer == L_TRAP || eLayer == L_CLOUDTRAP) {
                RemoveZone(pRef);
            }
            else {
                RemoveTarget(pRef);
            }
        }
    }
}

void TESTrapListener::phantomRemovedCallback(hkpPhantom* apPhantom) {
    CFilter iflt;
    const hkpCollidable* pCol = apPhantom->getCollidable();
    iflt.iFilter = pCol->getCollisionFilterInfo();
    if (pTriggers) {
        COL_LAYER eLayer = iflt.GetLayer();
        if (eLayer == L_TRIGGER || eLayer == L_ACTORZONE || eLayer == L_GASTRAP) {
            TriggerEntry* pTrigger = pTriggers->GetTriggerEntry(pCol);
            if (pTrigger) {
                pTriggers = static_cast<TriggerEntry*>(pTrigger->RemoveFromList(pTriggers));
                delete pTrigger;
            }
        }
    }
}

void TESTrapListener::phantomAddedCallback(hkpPhantom* apPhantom) {
    CFilter iflt;
    const hkpCollidable* pCol = apPhantom->getCollidable();
    iflt.iFilter = pCol->getCollisionFilterInfo();
    COL_LAYER eLayer = iflt.GetLayer();
    if (eLayer == L_TRIGGER || eLayer == L_ACTORZONE || eLayer == L_GASTRAP) {
        TESObjectREFR* pRef = TESHavokUtilities::FindCollidableRef(pCol);
        if (pRef) {
            TriggerEntry* pEntry;
            pEntry = new TriggerEntry(pRef, pCol);
            pEntry->pNext = pTriggers;
            pTriggers = pEntry;
        }
    }
}

bool TESTrapListener::contactPointAddedCallback(hkpRootCdPoint& arPoint) {
    bool bRes = false;
    TESObjectREFR* pTrapRef = TESHavokUtilities::FindCollidableRef(arPoint.m_rootCollidableB);
    TESObjectREFR* pTargetRef = TESHavokUtilities::FindCollidableRef(arPoint.m_rootCollidableA);
    if (pTrapRef && pTargetRef && pTargetRef->IsActor()) {
        return AddZoneTargetPair(arPoint, pTrapRef, pTargetRef, 0, true) != nullptr;
    }

    return bRes;
}

bool TESTrapListener::contactPointRemovedCallback(hkpRootCdPoint& arPoint) {
    bool bRes = false;
    TESObjectREFR* pTrapRef = TESHavokUtilities::FindCollidableRef(arPoint.m_rootCollidableB);
    TESObjectREFR* pTargetRef = TESHavokUtilities::FindCollidableRef(arPoint.m_rootCollidableA);
    if (pTrapRef && pTargetRef && pTargetRef->IsActor()) {
        RemoveTargetFromZone(arPoint.m_rootCollidableB, pTargetRef);
        return true;
    }

    return bRes;
}

TESTrapListener* TESTrapListener::GetListener(TESObjectREFR* apRef) {
    if (apRef) {
        TESObjectCELL* pParentCell = apRef->GetParentCell();
        if (pParentCell) {
            bhkWorld* pWorld = pParentCell->GetbhkWorld();
            if (pWorld) {
                // The world holds the listener by its bhkTrapListener base.
                return static_cast<TESTrapListener*>(pWorld->pTrapListener);
            }
        }
    }
    return nullptr;
}

void TESTrapListener::ClearCurrentRefs(TESObjectREFR* apRef) {
    for (TriggerEntry* pEntry = pTriggers; pEntry; pEntry = pEntry->QNextEntry()) {
        if (static_cast<const void*>(pEntry->GetRef()) == static_cast<const void*>(apRef)) {
            pEntry->ClearCurrentRefs();
            return;
        }
    }
}

// Finds the trigger's entry; a trigger the listener does not know holds nothing.
bool TESTrapListener::IsRefInTrigger(TESObjectREFR* apRef, TESObjectREFR* apTrigger) {
    TriggerEntry* pEntry = pTriggers->GetZoneEntry(apTrigger);
    if (!pEntry) {
        return false;
    }
    return pEntry->IsInTrigger(apRef);
}

bool TESTrapListener::IsCurrentTrigger(TESObjectREFR* apRef) {
    for (TriggerEntry* pEntry = pTriggers; pEntry; pEntry = pEntry->QNextEntry()) {
        if (pEntry->pRef == apRef) {
            return true;
        }
    }
    return false;
}

// Reads each entry's next before deleting it, and null-tests the entry again before the delete.
void TESTrapListener::EmptyTriggerList() {
    TriggerEntry* pEntry = pTriggers;
    while (pEntry) {
        TriggerEntry* pNext = pEntry->QNextEntry();
        if (pEntry) {
            delete pEntry;
        }
        pEntry = pNext;
    }
    pTriggers = nullptr;
}

bhkTrapListener::bhkTrapListener() : bhkEntityListener(0) {}

bhkTrapListener::~bhkTrapListener() {}

TrapEntry::TrapEntry(TESObjectREFR* apRef, const hkpCollidable* apCol) : ZoneEntry(apRef, apCol) {}

// Runs every target entry of the trap. The node base sits at +0x10: each next node steps back to its entry.
void TrapEntry::Run() {
    for (TargetEntry* pEntry = pTargets; pEntry; pEntry = static_cast<TargetEntry*>(pEntry->pNext)) {
        pEntry->Run(this);
    }
}

TriggerEntry::RefList::RefList() : iArraySize(0), iArrayMaxSize(20) {
    pRefArray = static_cast<RefStatus*>(ourMalloc(sizeof(RefStatus) * iArrayMaxSize));
}

bool TriggerEntry::RefList::Add(TESObjectREFR* apRef) {
    uint32_t uiNewID = apRef->iFormID;
    for (int32_t iRefIdx = 0; iRefIdx < iArraySize; ++iRefIdx) {
        if (uiNewID == pRefArray[iRefIdx].RefID) {
            if (pRefArray[iRefIdx].bInsideTrigger) {
                return false;
            }
            pRefArray[iRefIdx].bInsideTrigger = true;
            return true;
        }
    }
    if (iArraySize == iArrayMaxSize) {
        int32_t iNewArraySize = 2 * iArrayMaxSize;
        // The allocation size for `new RefStatus[iNewArraySize]`: the count times the entry, all ones when that
        // overflows.
        uint32_t uiCount = static_cast<uint32_t>(iNewArraySize);
        TriggerEntry::RefList::RefStatus* pNewArray = static_cast<TriggerEntry::RefList::RefStatus*>(ourMalloc(
            uiCount > SIZE_MAX / sizeof(TriggerEntry::RefList::RefStatus)
                ? SIZE_MAX
                : sizeof(TriggerEntry::RefList::RefStatus) * uiCount
        ));
        for (int32_t iCopyIdx = 0; iCopyIdx < iArrayMaxSize; ++iCopyIdx) {
            pNewArray[iCopyIdx].RefID = pRefArray[iCopyIdx].RefID;
            pNewArray[iCopyIdx].bInsideTrigger = pRefArray[iCopyIdx].bInsideTrigger;
        }
        iArrayMaxSize = iNewArraySize;
        ourFree(pRefArray);
        pRefArray = pNewArray;
    }
    pRefArray[iArraySize].RefID = uiNewID;
    pRefArray[iArraySize++].bInsideTrigger = true;
    TriggerEntry::TriggerEvent* pNewEvent = static_cast<TriggerEntry::TriggerEvent*>(
        ourMalloc(sizeof(TriggerEntry::TriggerEvent))
    );
    pNewEvent->eStatus = TriggerEntry::TRIGGER_STATUS_ENTER;
    pNewEvent->RefID = uiNewID;
    EventList.AddTail(pNewEvent);
    return false;
}

TriggerEntry::TriggerEvent* TriggerEntry::RefList::GetNewEvent() {
    if (EventList.IsEmpty()) {
        return nullptr;
    }

    return EventList.RemoveHead();
}

void TriggerEntry::RefList::UpdateExitList() {
    int32_t iRefIdx = 0;
    while (iRefIdx < iArraySize) {
        if (pRefArray[iRefIdx].bInsideTrigger) {
            pRefArray[iRefIdx++].bInsideTrigger = false;
        }
        else {
            TriggerEntry::TriggerEvent* pNewEvent = static_cast<TriggerEntry::TriggerEvent*>(
                ourMalloc(sizeof(TriggerEntry::TriggerEvent))
            );
            pNewEvent->eStatus = TriggerEntry::TRIGGER_STATUS_EXIT;
            pNewEvent->RefID = pRefArray[iRefIdx].RefID;
            EventList.AddTail(pNewEvent);
            for (int32_t iShrinkIdx = iRefIdx; iShrinkIdx < iArraySize - 1; ++iShrinkIdx) {
                pRefArray[iShrinkIdx].RefID = pRefArray[iShrinkIdx + 1].RefID;
                pRefArray[iShrinkIdx].bInsideTrigger = pRefArray[iShrinkIdx + 1].bInsideTrigger;
            }
            --iArraySize;
        }
    }
}

TriggerEntry::TriggerEntry(TESObjectREFR* apRef, const hkpCollidable* apCol) : bhkTTNode<TESObjectREFR>(apRef, apCol) {}

TriggerEntry::~TriggerEntry() {}

void TriggerEntry::ClearCurrentRefs() {
    if (!pSaveLoadGame->GetSaveGameLoading()) {
        CurrentRefs.ForceLeaveTrigger(pRef);
    }
    CurrentRefs.Reset();
}

TriggerEntry* TriggerEntry::QNextEntry() {
    return static_cast<TriggerEntry*>(pNext);
}

TriggerEntry* TriggerEntry::GetZoneEntry(TESObjectREFR* apRef) {
    return static_cast<TriggerEntry*>(GetEntry(apRef));
}

TESObjectREFR* TriggerEntry::GetRef() {
    return pRef;
}

// The world's read locks around the phantom queries are empty in this build, so nothing is written for them. The action
// flags passed to Script::SetActionFlag are 0x10000000 inside, 0x20000000 entering and 0x40000000 leaving; they have no
// names.
bool TriggerEntry::Update(float afDelta) {
    bool bTriggered = false;
    TESObjectREFR* pTriggerRef = pRef;
    if (!pTriggerRef || pTriggerRef->GetDisabled() || pTriggerRef->GetDelete() || !pTriggerRef->Get3D()) {
        return bTriggered;
    }
    hkpPhantom* pPhantom = static_cast<hkpPhantom*>(pCollidable->getOwner());
    if (pPhantom->getType() == HK_PHANTOM_SIMPLE_SHAPE) {
        hkpSimpleShapePhantom* pShapePhantom = static_cast<hkpSimpleShapePhantom*>(pPhantom);
        if (!pShapePhantom->getCollisionDetails().isEmpty()) {
            hkpAllCdBodyPairCollector kCollector;
            kCollector.hkpAllCdBodyPairCollector::reset();
            pShapePhantom->getPenetrations(kCollector, nullptr);
            const hkArray<hkpRootCdBodyPair>& rCols = kCollector.getHits();
            if (!rCols.isEmpty()) {
                int32_t iSize = rCols.getSize();
                for (int32_t i = 0; i < iSize; ++i) {
                    const hkpCollidable* pCol = rCols[i].m_rootCollidableB;
                    if (pCol) {
                        TESObjectREFR* pTriggeredRef = TESHavokUtilities::FindCollidableRef(pCol);
                        if (pTriggeredRef) {
                            bool bActorLimb = pTriggeredRef->IsActor() && hkpGetRigidBody(pCol);
                            bool bActorRagdollin = pTriggeredRef->IsDead(false) || pTriggeredRef->IsKnockedOut();
                            if ((!bActorLimb || bActorRagdollin) && CurrentRefs.Add(pTriggeredRef)) {
                                ExtraDataList* pExtraList = pRef->GetExtra();
                                bTriggered = Script::SetActionFlag(
                                                 pTriggeredRef,
                                                 pExtraList,
                                                 SCRIPT_ACTION_ONTRIGGER
                                             ) ||
                                    bTriggered;
                            }
                        }
                    }
                }
            }
        }
    }
    else if (pPhantom->getType() == HK_PHANTOM_AABB) {
        hkpAabbPhantom* pAabbPhantom = static_cast<hkpAabbPhantom*>(pPhantom);
        hkArray<hkpCollidable*>& rCols = pAabbPhantom->m_overlappingCollidables;
        if (!rCols.isEmpty()) {
            int32_t iSize = rCols.getSize();
            for (int32_t i = 0; i < iSize; ++i) {
                hkpCollidable* pCol = rCols[i];
                if (pCol) {
                    TESObjectREFR* pTriggeredRef = TESHavokUtilities::FindCollidableRef(pCol);
                    if (pTriggeredRef) {
                        bool bActorLimb = pTriggeredRef->IsActor() && hkpGetRigidBody(pCol);
                        bool bActorRagdollin = pTriggeredRef->IsDead(false) || pTriggeredRef->IsKnockedOut();
                        if ((!bActorLimb || bActorRagdollin) && CurrentRefs.Add(pTriggeredRef)) {
                            ExtraDataList* pExtraList = pRef->GetExtra();
                            bTriggered = Script::SetActionFlag(pTriggeredRef, pExtraList, SCRIPT_ACTION_ONTRIGGER) ||
                                bTriggered;
                        }
                    }
                }
            }
        }
    }
    CurrentRefs.UpdateExitList();
    TriggerEntry::TriggerEvent* pCurrentEvent = CurrentRefs.GetNewEvent();
    if (pCurrentEvent) {
        uint32_t uiFlag = pCurrentEvent->eStatus == TriggerEntry::TRIGGER_STATUS_ENTER ? SCRIPT_ACTION_ONTRIGGERENTER
                                                                                       : SCRIPT_ACTION_ONTRIGGERLEAVE;
        TESObjectREFR* pEventRef = static_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(pCurrentEvent->RefID));
        ourFree(pCurrentEvent);
        if (pEventRef) {
            ExtraDataList* pExtraList = pRef->GetExtra();
            bTriggered = Script::SetActionFlag(pEventRef, pExtraList, uiFlag) || bTriggered;
            pRef->SetActionRef(pEventRef);
        }
    }
    return bTriggered;
}

void TriggerEntry::operator delete(void* apMemory) {
    ourFree(apMemory);
}

hkpCdBodyPairCollector::hkpCdBodyPairCollector() : m_earlyOut(false) {}

hkpCdBodyPairCollector::~hkpCdBodyPairCollector() {}

void hkpCdBodyPairCollector::operator delete(void* apMemory) {
    hkpCdPointCollector::operator delete(apMemory);
}

hkpAllCdBodyPairCollector::hkpAllCdBodyPairCollector() : m_hits(0) {
    hkpAllCdBodyPairCollector::reset();
}

// Clears the hits, then the base's early-out flag.
void hkpAllCdBodyPairCollector::reset() {
    m_hits.clear();
    hkpCdBodyPairCollector::reset();
}

hkpAllCdBodyPairCollector::~hkpAllCdBodyPairCollector() {}

const hkArray<hkpRootCdBodyPair, hkContainerHeapAllocator>& hkpAllCdBodyPairCollector::getHits() const {
    return m_hits;
}

void hkpAllCdBodyPairCollector::operator delete(void* p) {
    if (!p) {
        return;
    }
    hkMemoryAllocator& kAllocator = hkMemoryRouter::getInstance().heap();
    kAllocator.blockFree(p, sizeof(hkpAllCdBodyPairCollector));
}

hkpPhantomListener::hkpPhantomListener() {}

hkpPhantomListener::~hkpPhantomListener() {}

void hkpPhantomListener::phantomShapeSetCallback(hkpPhantom* phantom) {
    if (phantom->hkpWorldObject::getWorld()) {
        phantomRemovedCallback(phantom);
        phantomAddedCallback(phantom);
    }
}

void hkpPhantomListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}
