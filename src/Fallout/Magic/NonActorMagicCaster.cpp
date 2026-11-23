#include "Fallout/Magic/NonActorMagicCaster.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "SDK/MSVC/rtti.h"

NonActorMagicCaster::NonActorMagicCaster()
  : BSExtraData(0x32u), MagicCaster(), pCurrentSpell(nullptr), pDesiredTarget(nullptr), pCasterObject(nullptr) {}

NonActorMagicCaster::NonActorMagicCaster(TESObjectREFR* apCasterObject)
  : BSExtraData(0x32u), MagicCaster(), pCurrentSpell(nullptr), pDesiredTarget(nullptr), pCasterObject(apCasterObject) {}

NonActorMagicCaster::~NonActorMagicCaster() {
    if (!pDataHandler->GetClearingData()) {
        if (pCasterObject) {
            pCasterObject->CleanUpTraps();
        }
        CleanUpOtherMobileObjects();
    }
}

void NonActorMagicCaster::CleanUpOtherMobileObjects() {
    ProcessArray* pArray = &ReferenceProcessLists.MobProcessArray;
    for (uint32_t uiIndex = 0; uiIndex < pArray->GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
        MobileObject* pMob = pArray->GetItem(uiIndex);
        Actor* pActor = nullptr;
        if (pMob && pMob->IsActor()) {
            pActor = static_cast<Actor*>(pMob);
        }
        MagicProjectile* pProjectile = nullptr;
        if (pMob && pMob->IsProjectile()) {
            pProjectile = dynamic_cast<MagicProjectile*>(pMob);
        }
        if (pActor) {
            if (this) {
                pActor->CheckDeletedCaster(this);
            }
            else {
                pActor->CheckDeletedCaster(nullptr);
            }
        }
        else if (pProjectile) {
            if (this) {
                pProjectile->CheckDeletedCaster(this);
            }
            else {
                pProjectile->CheckDeletedCaster(nullptr);
            }
        }
    }
    for (uint32_t i = pArray->GetProcessLevelHead(PROCESS_TYPE_MIDDLE_HIGH);
         i < pArray->GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH);
         ++i) {
        MobileObject* pMob = pArray->GetItem(i);
        Actor* pActor = nullptr;
        if (pMob && pMob->IsActor()) {
            pActor = static_cast<Actor*>(pMob);
        }
        MagicProjectile* pProjectile = nullptr;
        if (pMob && pMob->IsProjectile()) {
            pProjectile = dynamic_cast<MagicProjectile*>(pMob);
        }
        if (pActor) {
            if (this) {
                pActor->CheckDeletedCaster(this);
            }
            else {
                pActor->CheckDeletedCaster(nullptr);
            }
        }
        else if (pProjectile) {
            if (this) {
                pProjectile->CheckDeletedCaster(this);
            }
            else {
                pProjectile->CheckDeletedCaster(nullptr);
            }
        }
    }
    if (pPlayer) {
        if (this) {
            pPlayer->CheckDeletedCaster(this);
        }
        else {
            pPlayer->CheckDeletedCaster(nullptr);
        }
    }
}
