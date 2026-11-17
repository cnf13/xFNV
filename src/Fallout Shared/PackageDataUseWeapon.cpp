#include "Fallout Shared/PackageDataUseWeapon.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>

UseWeaponActorPackageData::UseWeaponActorPackageData() : pAttackTarget(nullptr) {}

UseWeaponActorPackageData::~UseWeaponActorPackageData() {}

// Clears the attack target when it is the deleted form.
void UseWeaponActorPackageData::HandleDeletedReference(TESObjectREFR* pDeletedObj) {
	pAttackTarget = (pAttackTarget == pDeletedObj) ? nullptr : pAttackTarget;
}

PACKAGE_TYPE UseWeaponActorPackageData::GetPackageType() {
	return PACKAGE_TYPE_USE_WEAPON;
}

void UseWeaponActorPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	pAttackTarget = nullptr;
}

void UseWeaponActorPackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	pAttackTarget = nullptr;
}

void UseWeaponActorPackageData::operator delete(void* apMemory) {
	ourFree(apMemory);
}

void TESUseWeaponPackageData::Copy(TESPackageData* apCopy) {
	TESUseWeaponPackageData* pcopy = dynamic_cast<TESUseWeaponPackageData*>(apCopy);
	if (pcopy) {
		if (pAttackTarget) {
			delete pAttackTarget;
		}
		pAttackTarget = 0;
		if (pcopy->pAttackTarget) {
			PackageTarget* v3;
			PackageTarget* v6 = static_cast<PackageTarget*>(ourMalloc(sizeof(PackageTarget)));
			if (v6) {
				v3 = ::new (v6) PackageTarget;
			} else {
				v3 = 0;
			}
			pAttackTarget = v3;
			pAttackTarget->Copy(pcopy->pAttackTarget);
		}
		pWeapon = pcopy->pWeapon;
		if (pTargetLocation) {
			delete pTargetLocation;
		}
		pTargetLocation = 0;
		if (pcopy->pTargetLocation) {
			PackageLocation* v2;
			PackageLocation* v5 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
			if (v5) {
				v2 = ::new (v5) PackageLocation;
			} else {
				v2 = 0;
			}
			pTargetLocation = v2;
			pTargetLocation->Copy(pcopy->pTargetLocation, 0);
		}
		bAlwaysHit = pcopy->bAlwaysHit;
		bDoNoDamage = pcopy->bDoNoDamage;
		bCrouch = pcopy->bCrouch;
		bHoldFire = pcopy->bHoldFire;
		bVolleyFire = pcopy->bVolleyFire;
		bRepeatFire = pcopy->bRepeatFire;
		sBurstCount = pcopy->sBurstCount;
		sVolleyShotsMin = pcopy->sVolleyShotsMin;
		sVolleyShotsMax = pcopy->sVolleyShotsMax;
		fVolleyWaitMin = pcopy->fVolleyWaitMin;
		fVolleyWaitMax = pcopy->fVolleyWaitMax;
	}
}

bool TESUseWeaponPackageData::Compare(TESPackageData* apCompare) {
	TESUseWeaponPackageData* pcomp = dynamic_cast<TESUseWeaponPackageData*>(apCompare);
	if (!pcomp) {
		return 1;
	}
	if (pWeapon != pcomp->pWeapon) {
		return 1;
	}
	if (pAttackTarget) {
		if (pAttackTarget->Compare(pcomp->pAttackTarget)) {
			return 1;
		}
	} else {
		if (pcomp->pAttackTarget) {
			return 1;
		}
	}
	if (pTargetLocation) {
		if (pTargetLocation->Compare(pcomp->pTargetLocation)) {
			return 1;
		}
	} else {
		if (pcomp->pTargetLocation) {
			return 1;
		}
	}
	if (bAlwaysHit != pcomp->bAlwaysHit) {
		return 1;
	}
	if (bDoNoDamage != pcomp->bDoNoDamage) {
		return 1;
	}
	if (bCrouch != pcomp->bCrouch) {
		return 1;
	}
	if (bHoldFire != pcomp->bHoldFire) {
		return 1;
	}
	if (bVolleyFire != pcomp->bVolleyFire) {
		return 1;
	}
	if (bRepeatFire != pcomp->bRepeatFire) {
		return 1;
	}
	if (sBurstCount != pcomp->sBurstCount) {
		return 1;
	}
	if (sVolleyShotsMin != pcomp->sVolleyShotsMin) {
		return 1;
	}
	if (sVolleyShotsMax != pcomp->sVolleyShotsMax) {
		return 1;
	}
	float v4 = fVolleyWaitMin - pcomp->fVolleyWaitMin;
	if (std::fabsf(v4) > 0.01) {
		return 1;
	}
	float v3 = fVolleyWaitMax - pcomp->fVolleyWaitMax;
	return std::fabsf(v3) > 0.01;
}

void TESUseWeaponPackageData::Save() {
	uint32_t iWeapon;
	PACK_USE_WEAPON_DATA_PKW3 kData;
	ourMemset(&kData, 0, sizeof(PACK_USE_WEAPON_DATA_PKW3));
	kData.bAlwaysHit = bAlwaysHit;
	kData.bDoNoDamage = bDoNoDamage;
	kData.bCrouch = bCrouch;
	kData.bHoldFire = bHoldFire;
	kData.bVolleyFire = bVolleyFire;
	kData.bRepeatFire = bRepeatFire;
	kData.sBurstCount = sBurstCount;
	kData.sVolleyShotsMin = sVolleyShotsMin;
	kData.sVolleyShotsMax = sVolleyShotsMax;
	kData.fVolleyWaitMin = fVolleyWaitMin;
	kData.fVolleyWaitMax = fVolleyWaitMax;
	if (pWeapon) {
		iWeapon = pWeapon->iFormID;
	} else {
		iWeapon = 0;
	}
	kData.iWeapon = iWeapon;
	if (TESForm::QEndianSwapOnSave()) {
		kData.Endian();
	}
	TESForm::__AddChunkData(PKW3_ID, &kData, sizeof(PACK_USE_WEAPON_DATA_PKW3));
	if (TESForm::QEndianSwapOnSave()) {
		kData.Endian();
	}
	if (pAttackTarget) {
		pAttackTarget->Save(PTD2_ID);
	}
	if (pTargetLocation) {
		pTargetLocation->Save(PLD2_ID);
	}
}

void TESUseWeaponPackageData::InitItem(TESForm* apOwner) {
	// apOwner is the package that owns this data (TESPackage::InitItem passes itself).
	PackageLocation* ppackLoc = static_cast<TESPackage*>(apOwner)->GetLocation();
	char v11 = 1;
	if (!pAttackTarget) {
		if (ppackLoc) {
			if (ppackLoc->GetLocType() == PLT_REFERENCE) {
				TESObjectREFR* plocRef = ppackLoc->GetLocReference();
				if (plocRef) {
					TESBoundObject* pboundObj = plocRef->GetObjectReference();
					if (pboundObj == pXMarker || pboundObj == pXMarkerHeading) {
						ExtraDataList* pextra = &plocRef->m_Extra;
						if (pextra) {
							TESObjectREFR* pattackTarget = pextra->GetXTarget();
							if (pattackTarget) {
								PackageTarget* v3;
								PackageTarget* v5 = static_cast<PackageTarget*>(ourMalloc(sizeof(PackageTarget)));
								if (v5) {
									v3 = ::new (v5) PackageTarget;
								} else {
									v3 = 0;
								}
								pAttackTarget = v3;
								pAttackTarget->SetTargType(PTT_REFERENCE);
								pAttackTarget->SetTargReference(pattackTarget);
								v11 = 0;
							}
						}
					}
				}
			}
		}
	}
	if (pAttackTarget) {
		if (v11) {
			pAttackTarget->InitItem(apOwner);
		}
	}
	TESForm* FormByNumericID = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pWeapon)));
	pWeapon = dynamic_cast<TESObjectWEAP*>(FormByNumericID);
}

void TESUseWeaponPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	char v3;
	char apData;
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bAlwaysHit), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bDoNoDamage), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bCrouch), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHoldFire), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bVolleyFire), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bRepeatFire), 1u, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&sBurstCount), 2u, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&sVolleyShotsMin), 2u, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&sVolleyShotsMax), 2u, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fVolleyWaitMin), 4u, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fVolleyWaitMax), 4u, 0);
	apSaveGameBuffer->SaveFormID(pWeapon, static_cast<uint32_t>(0));
	apData = pAttackTarget != 0;
	apSaveGameBuffer->SaveDataEndian(&apData, 1u, 0);
	if (apData) {
		pAttackTarget->SaveGame(apSaveGameBuffer);
	}
	v3 = pTargetLocation != 0;
	apSaveGameBuffer->SaveDataEndian(&v3, 1u, 0);
	if (v3) {
		pTargetLocation->SaveGame(apSaveGameBuffer);
	}
}

// The flags, counts and waits, the weapon, then a byte for each of the attack target and the target location saying
// whether it follows.
void TESUseWeaponPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bAlwaysHit), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bDoNoDamage), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bCrouch), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHoldFire), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bVolleyFire), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bRepeatFire), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&sBurstCount), 2);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&sVolleyShotsMin), 2);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&sVolleyShotsMax), 2);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fVolleyWaitMin), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fVolleyWaitMax), 4);
	pWeapon = dynamic_cast<TESObjectWEAP*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
	uint8_t bHasTarget = 0;
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHasTarget), 1);
	if (bHasTarget) {
		void* pMemory = ourMalloc(sizeof(PackageTarget));
		pAttackTarget = pMemory ? ::new (pMemory) PackageTarget : nullptr;
		pAttackTarget->LoadGame(apLoadGameBuffer);
	}
	uint8_t bHasLocation = 0;
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHasLocation), 1);
	if (bHasLocation) {
		void* pMemory = ourMalloc(sizeof(PackageLocation));
		pTargetLocation = pMemory ? ::new (pMemory) PackageLocation : nullptr;
		pTargetLocation->LoadGame(apLoadGameBuffer);
	}
}

void TESUseWeaponPackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	if (pAttackTarget) {
		pAttackTarget->InitLoadGame(apLoadGameBuffer);
	}
	if (pTargetLocation) {
		pTargetLocation->InitLoadGame(apLoadGameBuffer);
	}
}

void TESUseWeaponPackageData::SetAttackTarget(PackageTarget* apTarg) {
	if (apTarg) {
		if (!this->pAttackTarget) {
			PackageTarget* v2;
			PackageTarget* v5 = static_cast<PackageTarget*>(ourMalloc(sizeof(PackageTarget)));
			if (v5) {
				v2 = ::new (v5) PackageTarget;
			} else {
				v2 = 0;
			}
			this->pAttackTarget = v2;
		}
		this->pAttackTarget->Copy(apTarg);
	} else {
		PackageTarget* pAttackTarget = this->pAttackTarget;
		if (pAttackTarget) {
			delete pAttackTarget;
		}
		this->pAttackTarget = 0;
	}
}

ActorPackageData::ActorPackageData() {}

ActorPackageData::~ActorPackageData() {}
