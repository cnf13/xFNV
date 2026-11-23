#include "Fallout/Magic/FrenzyEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "SDK/MSVC/rtti.h"

// Whether the frenzy archetype took Instantiate: stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(EffectArchetypes::FRENZY, &FrenzyEffect::Instantiate);

FrenzyEffect::FrenzyEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apItem, apEffect), bStartedInCombat(false) {}

FrenzyEffect::~FrenzyEffect() {}

// Copies the flag byte onto the new effect, then copies the base data through a virtual call.
ActiveEffect* FrenzyEffect::Copy() {
    FrenzyEffect* pCopy = new FrenzyEffect(pCaster, pSpell, pEffect);
    pCopy->bStartedInCombat = bStartedInCombat;
    CopyData(pCopy);
    return pCopy;
}

uint16_t FrenzyEffect::GetSaveSize(Actor* apActor) {
    return static_cast<uint16_t>(ValueModifierEffect::GetSaveSize(apActor) + 1);
}

void FrenzyEffect::SaveGame(Actor* apActor) {
    ValueModifierEffect::SaveGame(apActor);
    pSaveLoadGameOLD->SaveGameDataOLD(&bStartedInCombat, sizeof(bStartedInCombat));
}

void FrenzyEffect::LoadGame(Actor* apActor) {
    ValueModifierEffect::LoadGame(apActor);
    pSaveLoadGameOLD->LoadGameDataOLD(&bStartedInCombat, sizeof(bStartedInCombat));
}

bool FrenzyEffect::CheckDeletedCaster(MagicCaster* apCaster) {
    bool bDeleted = ActiveEffect::CheckDeletedCaster(apCaster);
    if (bDeleted) {
        Dispel(true);
    }
    return bDeleted;
}

bool FrenzyEffect::CheckTarget(MagicTarget* apTarget) {
    Character* pCharacter = dynamic_cast<Character*>(apTarget);
    if (pCharacter && !pCharacter->IsDead(false) && !pCharacter->IsDying()) {
        float fEffectMagnitude = fMagnitude;
        uint16_t usLevel = pCharacter->GetLevel();
        if (MagicFormulas::CheckTargetLevelMagnitude(usLevel, fEffectMagnitude)) {
            return true;
        }
    }

    return false;
}

void FrenzyEffect::Start() {
    ValueModifierEffect::Start();
    Actor* pTargetActor = nullptr;
    if (GetTarget()) {
        MagicTarget* pMagicTarget = GetTarget();
        pTargetActor = pMagicTarget->GetTargetAsActor();
    }
    Actor* pCasterActor = nullptr;
    if (GetCaster()) {
        MagicCaster* pMagicCaster = GetCaster();
        pCasterActor = pMagicCaster->GetCasterAsActor();
    }
    if (pTargetActor && pCasterActor && pTargetActor->IsInCombat()) {
        bStartedInCombat = true;
    }
}

void FrenzyEffect::Finish() {
    ValueModifierEffect::Finish();
    if (!bStartedInCombat) {
        Actor* pTargetActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
        if (pTargetActor && pTargetActor->IsInCombat()) {
            pTargetActor->StopCombat(nullptr);
            ProcessArray* pProcessArray = &ReferenceProcessLists.MobProcessArray;
            for (uint32_t uiIndex = 0; uiIndex < pProcessArray->GetProcessLevelTail(PROCESS_TYPE_HIGH); ++uiIndex) {
                Actor* pItem = static_cast<Actor*>(pProcessArray->GetItem(uiIndex));
                if (pItem != pTargetActor) {
                    Actor* pActor = pItem && pItem->IsActor() ? pItem : nullptr;
                    if (pActor) {
                        pActor->RemoveFleeTarget(pTargetActor);
                        if (pActor->IsInCombat()) {
                            pActor->StopCombat(pTargetActor);
                        }
                    }
                }
            }
        }
    }
}

ActiveEffect* FrenzyEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new FrenzyEffect(apCaster, apSpell, apEffect);
}
