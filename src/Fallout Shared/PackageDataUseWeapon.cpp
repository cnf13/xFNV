#include "Fallout Shared/PackageDataUseWeapon.h"

#include "BSCore/NewOverloads.h"
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
    pAttackTarget = pAttackTarget == pDeletedObj ? nullptr : pAttackTarget;
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
        pAttackTarget = nullptr;
        if (pcopy->pAttackTarget) {
            pAttackTarget = new PackageTarget;
            pAttackTarget->Copy(pcopy->pAttackTarget);
        }
        pWeapon = pcopy->pWeapon;
        if (pTargetLocation) {
            delete pTargetLocation;
        }
        pTargetLocation = nullptr;
        if (pcopy->pTargetLocation) {
            pTargetLocation = new PackageLocation;
            pTargetLocation->Copy(pcopy->pTargetLocation, false);
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
        return true;
    }
    if (pWeapon != pcomp->pWeapon) {
        return true;
    }
    if (pAttackTarget) {
        if (pAttackTarget->Compare(pcomp->pAttackTarget)) {
            return true;
        }
    }
    else if (pcomp->pAttackTarget) {
        return true;
    }
    if (pTargetLocation) {
        if (pTargetLocation->Compare(pcomp->pTargetLocation)) {
            return true;
        }
    }
    else if (pcomp->pTargetLocation) {
        return true;
    }
    if (bAlwaysHit != pcomp->bAlwaysHit) {
        return true;
    }
    if (bDoNoDamage != pcomp->bDoNoDamage) {
        return true;
    }
    if (bCrouch != pcomp->bCrouch) {
        return true;
    }
    if (bHoldFire != pcomp->bHoldFire) {
        return true;
    }
    if (bVolleyFire != pcomp->bVolleyFire) {
        return true;
    }
    if (bRepeatFire != pcomp->bRepeatFire) {
        return true;
    }
    if (sBurstCount != pcomp->sBurstCount) {
        return true;
    }
    if (sVolleyShotsMin != pcomp->sVolleyShotsMin) {
        return true;
    }
    if (sVolleyShotsMax != pcomp->sVolleyShotsMax) {
        return true;
    }
    float fDiff = fVolleyWaitMin - pcomp->fVolleyWaitMin;
    if (std::fabsf(fDiff) > 0.01) {
        return true;
    }
    fDiff = fVolleyWaitMax - pcomp->fVolleyWaitMax;
    return std::fabsf(fDiff) > 0.01;
}

void TESUseWeaponPackageData::Save() {
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
    kData.iWeapon = pWeapon ? pWeapon->iFormID : 0;
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
    char bInitTarget = 1;
    if (!pAttackTarget && ppackLoc && ppackLoc->GetLocType() == PLT_REFERENCE) {
        TESObjectREFR* plocRef = ppackLoc->GetLocReference();
        if (plocRef) {
            TESBoundObject* pboundObj = plocRef->GetObjectReference();
            if (pboundObj == pXMarker || pboundObj == pXMarkerHeading) {
                ExtraDataList* pextra = &plocRef->m_Extra;
                if (pextra) {
                    TESObjectREFR* pattackTarget = pextra->GetXTarget();
                    if (pattackTarget) {
                        pAttackTarget = new PackageTarget;
                        pAttackTarget->SetTargType(PTT_REFERENCE);
                        pAttackTarget->SetTargReference(pattackTarget);
                        bInitTarget = 0;
                    }
                }
            }
        }
    }

    if (pAttackTarget && bInitTarget) {
        pAttackTarget->InitItem(apOwner);
    }

    TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pWeapon)));
    pWeapon = dynamic_cast<TESObjectWEAP*>(pForm);
}

void TESUseWeaponPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    char bHasTarget;
    char bHasLocation;
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bAlwaysHit), sizeof(bAlwaysHit), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bDoNoDamage), sizeof(bDoNoDamage), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bCrouch), sizeof(bCrouch), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bHoldFire), sizeof(bHoldFire), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bVolleyFire), sizeof(bVolleyFire), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bRepeatFire), sizeof(bRepeatFire), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&sBurstCount), sizeof(sBurstCount), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&sVolleyShotsMin), sizeof(sVolleyShotsMin), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&sVolleyShotsMax), sizeof(sVolleyShotsMax), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fVolleyWaitMin), sizeof(fVolleyWaitMin), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fVolleyWaitMax), sizeof(fVolleyWaitMax), 0);
    apSaveGameBuffer->SaveFormID(pWeapon, static_cast<uint32_t>(0));
    bHasTarget = pAttackTarget != nullptr;
    apSaveGameBuffer->SaveDataEndian(&bHasTarget, sizeof(bHasTarget), 0);
    if (bHasTarget) {
        pAttackTarget->SaveGame(apSaveGameBuffer);
    }
    bHasLocation = pTargetLocation != nullptr;
    apSaveGameBuffer->SaveDataEndian(&bHasLocation, sizeof(bHasLocation), 0);
    if (bHasLocation) {
        pTargetLocation->SaveGame(apSaveGameBuffer);
    }
}

// The flags, counts and waits, the weapon, then a byte for each of the attack target and the target location saying
// whether it follows.
void TESUseWeaponPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bAlwaysHit), sizeof(bAlwaysHit));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bDoNoDamage), sizeof(bDoNoDamage));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bCrouch), sizeof(bCrouch));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHoldFire), sizeof(bHoldFire));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bVolleyFire), sizeof(bVolleyFire));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bRepeatFire), sizeof(bRepeatFire));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&sBurstCount), sizeof(sBurstCount));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&sVolleyShotsMin), sizeof(sVolleyShotsMin));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&sVolleyShotsMax), sizeof(sVolleyShotsMax));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fVolleyWaitMin), sizeof(fVolleyWaitMin));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fVolleyWaitMax), sizeof(fVolleyWaitMax));
    pWeapon = dynamic_cast<TESObjectWEAP*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    uint8_t bHasTarget = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHasTarget), sizeof(bHasTarget));
    if (bHasTarget) {
        pAttackTarget = new PackageTarget;
        pAttackTarget->LoadGame(apLoadGameBuffer);
    }
    uint8_t bHasLocation = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bHasLocation), sizeof(bHasLocation));
    if (bHasLocation) {
        pTargetLocation = new PackageLocation;
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
            this->pAttackTarget = new PackageTarget;
        }
        this->pAttackTarget->Copy(apTarg);
    }
    else {
        PackageTarget* pOldTarget = this->pAttackTarget;
        if (pOldTarget) {
            delete pOldTarget;
        }
        this->pAttackTarget = nullptr;
    }
}

ActorPackageData::ActorPackageData() {}

ActorPackageData::~ActorPackageData() {}
