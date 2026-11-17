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
	} else {
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
	bool bZero = 0;
	if (apActor) {
		if (pLockData || !abMovement) {
			if (pLockData && (!pLockData->IsLocked() && abMovement)) {
				return 1;
			} else {
				TESObjectCELL* ParentCell;
				TESObjectCELL* v5;
				if (apActor != pPlayer &&
					(apActor->GetParentCell() &&
						((ParentCell = apActor->GetParentCell(), ParentCell->GetOwner()) &&
							(v5 = apActor->GetParentCell(), v5->IsActorCellOwner(apActor))))) {
					if (!pTeleportDoorCell ||
						(!pTeleportDoorCell->GetOwner() || pTeleportDoorCell->IsActorCellOwner(apActor))) {
						return 1;
					}
				} else {
					if (apActor == pPlayer || !IsActorAnOwner(apActor, 1)) {
						if (apActor->IsActoraGuard() && apActor->IsAlarmed()) {
							return 1;
						} else {
							if (apActor == pPlayer && pPlayer->IsTresPassing()) {
								if (pTeleportDoorCell) {
									if (pLockData) {
										if (pLockData->cLockLevel != 100 &&
											(!pTeleportDoorCell->GetInterior() || pTeleportDoorCell->GetPublic())) {
											return 1;
										}
									}
								}
							}
						}
					} else {
						return 1;
					}
				}
			}
		} else {
			return 1;
		}
	}
	return bZero;
}

bool DoorLock::CanActorIgnoreDoorLock(const ActorLockData* apActorData, bool abNeedsKey) {
	bool bretval = 0;
	if (apActorData) {
		if (pLockData) {
			if (pLockData->pKey || !abNeedsKey) {
				if (pDoorCell && pDoorCell->IsActorBaseCellOwner(apActorData->pActorBase)) {
					if (!pTeleportDoorCell ||
						(!pTeleportDoorCell->GetOwner() ||
							pTeleportDoorCell->IsActorBaseCellOwner(apActorData->pActorBase))) {
						return 1;
					}
				} else {
					if (IsAnOwner(apActorData, 1)) {
						return 1;
					} else {
						TESClass* pclass = 0;
						if (apActorData->pActorBase) {
							if (apActorData->pActorBase->GetFormType() == NPC__ID) {
								if (apActorData->pActorBase) {
									pclass = static_cast<TESNPC*>(apActorData->pActorBase)->TESNPC::GetClass();
								}
								if (pclass) {
									if (pclass->IsGuard() && apActorData->bIsAlarmed) {
										return 1;
									}
								}
							}
						}
					}
				}
			} else {
				return 1;
			}
		} else {
			return 1;
		}
	}
	return bretval;
}

bool DoorLock::CanActorUnlockDoorLock(const ActorLockData* apActorLockData, TESObjectREFR* apDoorRef,
	bool& abOutMustPick) {
	PACKAGE_OBJECT_TYPE aeMatchId;
	bool v7 = 0;
	aeMatchId = POTYPE_NONE;
	abOutMustPick = 0;
	if (apActorLockData) {
		if (CanActorIgnoreDoorLock(apActorLockData, static_cast<bool>(0))) {
			return 1;
		} else {
			if (pLockData->IsLocked()) {
				if (pLockData->pKey &&
					(apActorLockData->pInventory &&
						apActorLockData->pInventory->ContainerHasObjects(pLockData->pKey, POTYPE_NONE, 1, aeMatchId, 0,
							0))) {
					return 1;
				} else {
					if (pLockData->GetLockLevel(apDoorRef) != 5) {
						if (apActorLockData->pInventory) {
							if (apActorLockData->pInventory->ContainerHasObjects(pLockpick, POTYPE_NONE, 1, aeMatchId,
									0, 0)) {
								v7 = 1;
								abOutMustPick = 1;
							}
						}
					}
				}
			} else {
				return 1;
			}
		}
	}
	return v7;
}

bool DoorLock::IsAnOwner(const ActorLockData* apActorData, bool abUseFaction) {
	bool v6 = 0;
	if (pOwnershipForm) {
		if (apActorData) {
			v6 = 1;
			if (pOwnershipForm != apActorData->pActorBase) {
				TESFaction* apFaction = 0;
				if (pOwnershipForm) {
					if (pOwnershipForm->GetFormType() == 8) {
						apFaction = static_cast<TESFaction*>(pOwnershipForm);
					}
				}
				if ((!apFaction || !abUseFaction) && (!pOwnershipGlobal || pOwnershipGlobal->GetValue() == 0.0)) {
					v6 = 0;
				}
				if (apFaction) {
					if (apActorData->pActorBase) {
						if (apActorData->pActorBase->GetFactionRank(apFaction, 0) < iOwnershipRank) {
							return 0;
						}
					}
				}
			}
		}
	}
	return v6;
}

// The exe's cleanup-only C++ EH frame is left out.
void DoorLock::SetFromDoorRef(TESObjectREFR* apDoorRef) {
	pDoorCell = apDoorRef->GetParentCell();
	DoorTeleportData* Teleport = apDoorRef->GetTeleport();
	if (Teleport) {
		pTeleportDoorCell = Teleport->GetTeleportCell();
	} else {
		pTeleportDoorCell = 0;
	}
	LockData* Lock = apDoorRef->GetLock();
	if (Lock) {
		if (!pLockData) {
			LockData* v2;
			LockData* apMemory = static_cast<LockData*>(ourMalloc(sizeof(LockData)));
			if (apMemory) {
				v2 = ::new (apMemory) LockData;
			} else {
				v2 = 0;
			}
			pLockData = v2;
		}
		pLockData->pKey = Lock->pKey;
		pLockData->cLockLevel = Lock->cLockLevel;
		pLockData->cFlags = Lock->cFlags;
	} else {
		if (pLockData) {
			ourFree(pLockData);
			pLockData = 0;
		}
	}
	pOwnershipForm = apDoorRef->GetOwner();
	iOwnershipRank = apDoorRef->GetOwnershipRank();
	pOwnershipGlobal = apDoorRef->GetOwnershipGlobal();
}

bool DoorLock::IsActorAnOwner(Actor* apActor, bool abUseFaction) {
	bool v9 = 0;
	if (pOwnershipForm) {
		if (apActor) {
			v9 = 1;
			TESBoundObject* ObjectReference = apActor->GetTemplateActorBase();
			if (!ObjectReference) {
				ObjectReference = apActor->GetObjectReference();
			}
			TESActorBase* v8 = 0;
			int32_t FormType = ObjectReference->GetFormType();
			if (FormType >= 42 && FormType <= 43) {
				v8 = dynamic_cast<TESActorBase*>(ObjectReference);
			}
			if (pOwnershipForm != ObjectReference) {
				TESFaction* apFaction = 0;
				if (pOwnershipForm) {
					if (pOwnershipForm->GetFormType() == 8) {
						apFaction = static_cast<TESFaction*>(pOwnershipForm);
					}
				}
				if ((!apFaction || !abUseFaction) && (!pOwnershipGlobal || pOwnershipGlobal->GetValue() == 0.0)) {
					v9 = 0;
				}
				if (apFaction) {
					if (v8) {
						if (v8->GetFactionRank(apFaction, apActor == pPlayer) < iOwnershipRank) {
							return 0;
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
