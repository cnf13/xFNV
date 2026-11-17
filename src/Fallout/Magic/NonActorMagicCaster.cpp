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
    ProcessArray* parray = &ReferenceProcessLists.MobProcessArray;
    uint32_t uiIndex = 0;
    for (uint32_t i = parray->GetProcessLevelTail(PROCESS_TYPE_HIGH); uiIndex < i;
         i = parray->GetProcessLevelTail(PROCESS_TYPE_HIGH)) {
        MagicProjectile* pprojectile;
        Actor* pactor;
        MobileObject* pmob = parray->GetItem(uiIndex);
        if (pmob && pmob->IsActor()) {
            pactor = static_cast<Actor*>(pmob);
        }
        else {
            pactor = nullptr;
        }
        if (pmob && pmob->IsProjectile()) {
            pprojectile = dynamic_cast<MagicProjectile*>(pmob);
        }
        else {
            pprojectile = nullptr;
        }
        if (pactor) {
            if (this) {
                pactor->CheckDeletedCaster(this);
            }
            else {
                pactor->CheckDeletedCaster(nullptr);
            }
        }
        else {
            if (pprojectile) {
                if (this) {
                    pprojectile->CheckDeletedCaster(this);
                }
                else {
                    pprojectile->CheckDeletedCaster(nullptr);
                }
            }
        }
        ++uiIndex;
    }
    for (uint32_t i_2 = parray->GetProcessLevelHead(PROCESS_TYPE_MIDDLE_HIGH);
         i_2 < parray->GetProcessLevelTail(PROCESS_TYPE_MIDDLE_HIGH); ++i_2) {
        MagicProjectile* pprojectile_2;
        Actor* pactor_2;
        MobileObject* pmob_2 = parray->GetItem(i_2);
        if (pmob_2 && pmob_2->IsActor()) {
            pactor_2 = static_cast<Actor*>(pmob_2);
        }
        else {
            pactor_2 = nullptr;
        }
        if (pmob_2 && pmob_2->IsProjectile()) {
            pprojectile_2 = dynamic_cast<MagicProjectile*>(pmob_2);
        }
        else {
            pprojectile_2 = nullptr;
        }
        if (pactor_2) {
            if (this) {
                pactor_2->CheckDeletedCaster(this);
            }
            else {
                pactor_2->CheckDeletedCaster(nullptr);
            }
        }
        else {
            if (pprojectile_2) {
                if (this) {
                    pprojectile_2->CheckDeletedCaster(this);
                }
                else {
                    pprojectile_2->CheckDeletedCaster(nullptr);
                }
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
