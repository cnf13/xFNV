#include "Fallout Shared/DoorLock.h"

#include "BSCore/NewOverloads.h"
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
            if (pLockData && !pLockData->IsLocked() && abMovement) {
                return true;
            }

            if (apActor != pPlayer &&
                apActor->GetParentCell() &&
                apActor->GetParentCell()->GetOwner() &&
                apActor->GetParentCell()->IsActorCellOwner(apActor)) {
                if (!pTeleportDoorCell ||
                    !pTeleportDoorCell->GetOwner() ||
                    pTeleportDoorCell->IsActorCellOwner(apActor)) {
                    return true;
                }
            }
            else if (apActor == pPlayer || !IsActorAnOwner(apActor, true)) {
                if (apActor->IsActoraGuard() && apActor->IsAlarmed()) {
                    return true;
                }

                if (apActor == pPlayer &&
                    pPlayer->IsTresPassing() &&
                    pTeleportDoorCell &&
                    pLockData &&
                    pLockData->cLockLevel != 100 &&
                    (!pTeleportDoorCell->GetInterior() || pTeleportDoorCell->GetPublic())) {
                    return true;
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
    return bZero;
}

bool DoorLock::CanActorIgnoreDoorLock(const ActorLockData* apActorData, bool abNeedsKey) {
    bool bRetVal = false;
    if (apActorData) {
        if (pLockData) {
            if (pLockData->pKey || !abNeedsKey) {
                if (pDoorCell && pDoorCell->IsActorBaseCellOwner(apActorData->pActorBase)) {
                    if (!pTeleportDoorCell ||
                        !pTeleportDoorCell->GetOwner() ||
                        pTeleportDoorCell->IsActorBaseCellOwner(apActorData->pActorBase)) {
                        return true;
                    }
                }
                else {
                    if (IsAnOwner(apActorData, true)) {
                        return true;
                    }

                    TESClass* pClass = nullptr;
                    if (apActorData->pActorBase && apActorData->pActorBase->GetFormType() == NPC__ID) {
                        if (apActorData->pActorBase) {
                            pClass = static_cast<TESNPC*>(apActorData->pActorBase)->TESNPC::GetClass();
                        }
                        if (pClass && pClass->IsGuard() && apActorData->bIsAlarmed) {
                            return true;
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
    return bRetVal;
}

bool DoorLock::CanActorUnlockDoorLock(
    const ActorLockData* apActorLockData,
    TESObjectREFR* apDoorRef,
    bool& abOutMustPick
) {
    bool bCanUnlock = false;
    PACKAGE_OBJECT_TYPE eMatchId = POTYPE_NONE;
    abOutMustPick = false;
    if (apActorLockData) {
        if (CanActorIgnoreDoorLock(apActorLockData, false)) {
            return true;
        }

        if (pLockData->IsLocked()) {
            if (pLockData->pKey &&
                apActorLockData->pInventory &&
                apActorLockData->pInventory
                    ->ContainerHasObjects(pLockData->pKey, POTYPE_NONE, 1, eMatchId, 0, nullptr)) {
                return true;
            }

            if (pLockData->GetLockLevel(apDoorRef) != 5 &&
                apActorLockData->pInventory &&
                apActorLockData->pInventory->ContainerHasObjects(pLockpick, POTYPE_NONE, 1, eMatchId, 0, nullptr)) {
                bCanUnlock = true;
                abOutMustPick = true;
            }
        }
        else {
            return true;
        }
    }
    return bCanUnlock;
}

bool DoorLock::IsAnOwner(const ActorLockData* apActorData, bool abUseFaction) {
    bool bIsOwner = false;
    if (pOwnershipForm && apActorData) {
        bIsOwner = true;
        if (pOwnershipForm != apActorData->pActorBase) {
            TESFaction* pFaction = nullptr;
            if (pOwnershipForm && pOwnershipForm->GetFormType() == FACT_ID) {
                pFaction = static_cast<TESFaction*>(pOwnershipForm);
            }

            if ((!pFaction || !abUseFaction) && (!pOwnershipGlobal || pOwnershipGlobal->GetValue() == 0.0)) {
                bIsOwner = false;
            }
            if (pFaction &&
                apActorData->pActorBase &&
                apActorData->pActorBase->GetFactionRank(pFaction, false) < iOwnershipRank) {
                return false;
            }
        }
    }

    return bIsOwner;
}

void DoorLock::SetFromDoorRef(TESObjectREFR* apDoorRef) {
    pDoorCell = apDoorRef->GetParentCell();
    DoorTeleportData* pTeleport = apDoorRef->GetTeleport();
    pTeleportDoorCell = pTeleport ? pTeleport->GetTeleportCell() : nullptr;
    LockData* pLock = apDoorRef->GetLock();
    if (pLock) {
        if (!pLockData) {
            pLockData = new LockData;
        }
        pLockData->pKey = pLock->pKey;
        pLockData->cLockLevel = pLock->cLockLevel;
        pLockData->cFlags = pLock->cFlags;
    }
    else if (pLockData) {
        ourFree(pLockData);
        pLockData = nullptr;
    }
    pOwnershipForm = apDoorRef->GetOwner();
    iOwnershipRank = apDoorRef->GetOwnershipRank();
    pOwnershipGlobal = apDoorRef->GetOwnershipGlobal();
}

bool DoorLock::IsActorAnOwner(Actor* apActor, bool abUseFaction) {
    bool bIsOwner = false;
    if (pOwnershipForm && apActor) {
        bIsOwner = true;
        TESBoundObject* pBase = apActor->GetTemplateActorBase();
        if (!pBase) {
            pBase = apActor->GetObjectReference();
        }
        TESActorBase* pActorBase = nullptr;
        int32_t iFormType = pBase->GetFormType();
        if (iFormType >= NPC__ID && iFormType <= CREA_ID) {
            pActorBase = dynamic_cast<TESActorBase*>(pBase);
        }
        if (pOwnershipForm != pBase) {
            TESFaction* pFaction = nullptr;
            if (pOwnershipForm && pOwnershipForm->GetFormType() == FACT_ID) {
                pFaction = static_cast<TESFaction*>(pOwnershipForm);
            }

            if ((!pFaction || !abUseFaction) && (!pOwnershipGlobal || pOwnershipGlobal->GetValue() == 0.0)) {
                bIsOwner = false;
            }
            if (pFaction && pActorBase && pActorBase->GetFactionRank(pFaction, apActor == pPlayer) < iOwnershipRank) {
                return false;
            }
        }
    }

    return bIsOwner;
}

ActorLockData& ActorLockData::operator=(const ActorLockData& aData) {
    pActorBase = aData.pActorBase;
    pInventory = aData.pInventory;
    bIsAlarmed = aData.bIsAlarmed;
    return *this;
}
