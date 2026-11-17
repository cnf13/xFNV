#include "Fallout Shared/DoorLock.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTaskManager.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESKey.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "SDK/MSVC/rtti.h"

DoorLock::DoorLock()
  : pDoorCell(nullptr), pTeleportDoorCell(nullptr), pLockData(nullptr), pOwnershipForm(nullptr), iOwnershipRank(-1),
    pOwnershipGlobal(nullptr) {}

// A copy of the door's lock data, its cells and its ownership.
DoorLock::DoorLock(TESObjectREFR* apDoorRef) : pDoorCell(apDoorRef->TESObjectREFR::GetParentCell()) {
    DoorTeleportData* pTeleport = apDoorRef->GetTeleport();
    pTeleportDoorCell = pTeleport ? pTeleport->GetTeleportCell() : nullptr;
    LockData* pLock = apDoorRef->GetLock();
    if (pLock) {
        pLockData = new LockData();
        pLockData->pKey = pLock->pKey;
        pLockData->cLockLevel = pLock->cLockLevel;
        pLockData->cFlags = pLock->cFlags;
    }
    else {
        pLockData = nullptr;
    }
    pOwnershipForm = apDoorRef->GetOwner();
    iOwnershipRank = apDoorRef->GetOwnershipRank();
    pOwnershipGlobal = apDoorRef->GetOwnershipGlobal();
}

DoorLock::~DoorLock() {
    if (pLockData) {
        delete pLockData;
    }
}

bool DoorLock::CanActorIgnoreDoorLock(Actor* apActor, bool abActivate, bool abMovement) {
    bool bZero = false;
    if (apActor) {
        if (pLockData || !abMovement) {
            if (pLockData && (!pLockData->IsLocked() && abMovement)) {
                return true;
            }
            else {
                TESObjectCELL* ParentCell;
                TESObjectCELL* v5;
                if (apActor != pPlayer &&
                    (apActor->GetParentCell() &&
                     ((ParentCell = apActor->GetParentCell(), ParentCell->GetOwner()) &&
                      (v5 = apActor->GetParentCell(), v5->IsActorCellOwner(apActor))))) {
                    if (!pTeleportDoorCell ||
                        (!pTeleportDoorCell->GetOwner() || pTeleportDoorCell->IsActorCellOwner(apActor))) {
                        return true;
                    }
                }
                else {
                    if (apActor == pPlayer || !IsActorAnOwner(apActor, true)) {
                        if (apActor->IsActoraGuard() && apActor->IsAlarmed()) {
                            return true;
                        }
                        else {
                            if (apActor == pPlayer && pPlayer->IsTresPassing()) {
                                if (pTeleportDoorCell) {
                                    if (pLockData) {
                                        if (pLockData->cLockLevel != 100 &&
                                            (!pTeleportDoorCell->GetInterior() || pTeleportDoorCell->GetPublic())) {
                                            return true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else {
                        return true;
                    }
                }
            }
        }
        else {
            return true;
        }
    }
    return bZero;
}

bool DoorLock::CanActorIgnoreDoorLock(const ActorLockData* apActorData, bool abNeedsKey) {
    bool bretval = false;
    if (apActorData) {
        if (pLockData) {
            if (pLockData->pKey || !abNeedsKey) {
                if (pDoorCell && pDoorCell->IsActorBaseCellOwner(apActorData->pActorBase)) {
                    if (!pTeleportDoorCell ||
                        (!pTeleportDoorCell->GetOwner() ||
                         pTeleportDoorCell->IsActorBaseCellOwner(apActorData->pActorBase))) {
                        return true;
                    }
                }
                else {
                    if (IsAnOwner(apActorData, true)) {
                        return true;
                    }
                    else {
                        TESClass* pclass = nullptr;
                        if (apActorData->pActorBase) {
                            if (apActorData->pActorBase->GetFormType() == NPC__ID) {
                                if (apActorData->pActorBase) {
                                    pclass = static_cast<TESNPC*>(apActorData->pActorBase)->TESNPC::GetClass();
                                }
                                if (pclass) {
                                    if (pclass->IsGuard() && apActorData->bIsAlarmed) {
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else {
                return true;
            }
        }
        else {
            return true;
        }
    }
    return bretval;
}

bool DoorLock::CanActorUnlockDoorLock(
    const ActorLockData* apActorLockData,
    TESObjectREFR* apDoorRef,
    bool& abOutMustPick
) {
    PACKAGE_OBJECT_TYPE aeMatchId;
    bool v7 = false;
    aeMatchId = POTYPE_NONE;
    abOutMustPick = false;
    if (apActorLockData) {
        if (CanActorIgnoreDoorLock(apActorLockData, false)) {
            return true;
        }
        else {
            if (pLockData->IsLocked()) {
                if (pLockData->pKey &&
                    (apActorLockData->pInventory &&
                     apActorLockData->pInventory->ContainerHasObjects(
                         pLockData->pKey, POTYPE_NONE, 1, aeMatchId, 0, nullptr
                     ))) {
                    return true;
                }
                else {
                    if (pLockData->GetLockLevel(apDoorRef) != 5) {
                        if (apActorLockData->pInventory) {
                            if (apActorLockData->pInventory->ContainerHasObjects(
                                    pLockpick, POTYPE_NONE, 1, aeMatchId, 0, nullptr
                                )) {
                                v7 = true;
                                abOutMustPick = true;
                            }
                        }
                    }
                }
            }
            else {
                return true;
            }
        }
    }
    return v7;
}

bool DoorLock::IsAnOwner(const ActorLockData* apActorData, bool abUseFaction) {
    bool v6 = false;
    if (pOwnershipForm) {
        if (apActorData) {
            v6 = true;
            if (pOwnershipForm != apActorData->pActorBase) {
                TESFaction* apFaction = nullptr;
                if (pOwnershipForm) {
                    if (pOwnershipForm->GetFormType() == 8) {
                        apFaction = static_cast<TESFaction*>(pOwnershipForm);
                    }
                }
                if ((!apFaction || !abUseFaction) && (!pOwnershipGlobal || pOwnershipGlobal->GetValue() == 0.0)) {
                    v6 = false;
                }
                if (apFaction) {
                    if (apActorData->pActorBase) {
                        if (apActorData->pActorBase->GetFactionRank(apFaction, false) < iOwnershipRank) {
                            return false;
                        }
                    }
                }
            }
        }
    }
    return v6;
}

void DoorLock::SetFromDoorRef(TESObjectREFR* apDoorRef) {
    pDoorCell = apDoorRef->GetParentCell();
    DoorTeleportData* Teleport = apDoorRef->GetTeleport();
    if (Teleport) {
        pTeleportDoorCell = Teleport->GetTeleportCell();
    }
    else {
        pTeleportDoorCell = nullptr;
    }
    LockData* Lock = apDoorRef->GetLock();
    if (Lock) {
        if (!pLockData) {
            LockData* v2;
            LockData* apMemory = static_cast<LockData*>(ourMalloc(sizeof(LockData)));
            if (apMemory) {
                v2 = ::new (apMemory) LockData;
            }
            else {
                v2 = nullptr;
            }
            pLockData = v2;
        }
        pLockData->pKey = Lock->pKey;
        pLockData->cLockLevel = Lock->cLockLevel;
        pLockData->cFlags = Lock->cFlags;
    }
    else {
        if (pLockData) {
            ourFree(pLockData);
            pLockData = nullptr;
        }
    }
    pOwnershipForm = apDoorRef->GetOwner();
    iOwnershipRank = apDoorRef->GetOwnershipRank();
    pOwnershipGlobal = apDoorRef->GetOwnershipGlobal();
}

bool DoorLock::IsActorAnOwner(Actor* apActor, bool abUseFaction) {
    bool v9 = false;
    if (pOwnershipForm) {
        if (apActor) {
            v9 = true;
            TESBoundObject* ObjectReference = apActor->GetTemplateActorBase();
            if (!ObjectReference) {
                ObjectReference = apActor->GetObjectReference();
            }
            TESActorBase* v8 = nullptr;
            int32_t FormType = ObjectReference->GetFormType();
            if (FormType >= 42 && FormType <= 43) {
                v8 = dynamic_cast<TESActorBase*>(ObjectReference);
            }
            if (pOwnershipForm != ObjectReference) {
                TESFaction* apFaction = nullptr;
                if (pOwnershipForm) {
                    if (pOwnershipForm->GetFormType() == 8) {
                        apFaction = static_cast<TESFaction*>(pOwnershipForm);
                    }
                }
                if ((!apFaction || !abUseFaction) && (!pOwnershipGlobal || pOwnershipGlobal->GetValue() == 0.0)) {
                    v9 = false;
                }
                if (apFaction) {
                    if (v8) {
                        if (v8->GetFactionRank(apFaction, apActor == pPlayer) < iOwnershipRank) {
                            return false;
                        }
                    }
                }
            }
        }
    }
    return v9;
}

ActorLockData& ActorLockData::operator=(const ActorLockData& aData) {
    pActorBase = aData.pActorBase;
    pInventory = aData.pInventory;
    bIsAlarmed = aData.bIsAlarmed;
    return *this;
}
