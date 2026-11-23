#include "Fallout/Magic/DemoralizeEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/Creature.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "SDK/MSVC/rtti.h"

// Whether the demoralize archetype took Create: stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::DEMORALIZE,
    &DemoralizeEffect::Create
);

DemoralizeEffect::DemoralizeEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ActiveEffect(apCaster, apItem, apEffect), bTurned(false) {}

DemoralizeEffect::~DemoralizeEffect() {}

// Copies the flag byte onto the new effect, then copies the base data.
ActiveEffect* DemoralizeEffect::Copy() {
    DemoralizeEffect* pCopy = new DemoralizeEffect(pCaster, pSpell, pEffect);
    pCopy->bTurned = bTurned;
    CopyData(pCopy);
    return pCopy;
}

uint16_t DemoralizeEffect::GetSaveSize(Actor* apActor) {
    return static_cast<uint16_t>(ActiveEffect::GetSaveSize(apActor) + 1);
}

void DemoralizeEffect::SaveGame(Actor* apActor) {
    ActiveEffect::SaveGame(apActor);
    pSaveLoadGameOLD->SaveGameDataOLD(&bTurned, sizeof(bTurned));
}

void DemoralizeEffect::LoadGame(Actor* apActor) {
    ActiveEffect::LoadGame(apActor);
    pSaveLoadGameOLD->LoadGameDataOLD(&bTurned, sizeof(bTurned));
}

bool DemoralizeEffect::CheckDeletedCaster(MagicCaster* apCaster) {
    bool bDeleted = ActiveEffect::CheckDeletedCaster(apCaster);
    if (bDeleted) {
        Dispel(true);
    }
    return bDeleted;
}

bool DemoralizeEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pTargetActor = apTarget ? apTarget->GetTargetAsActor() : nullptr;
    if (!pTargetActor) {
        return false;
    }
    if (pTargetActor->IsDead(false)) {
        return false;
    }
    if (pTargetActor->IsDying()) {
        return false;
    }
    float fEffectMagnitude = fMagnitude;
    uint16_t usLevel = pTargetActor->GetLevel();
    if (!MagicFormulas::CheckTargetLevelMagnitude(usLevel, fEffectMagnitude)) {
        return false;
    }
    return dynamic_cast<Creature*>(apTarget) || !pTargetActor->IsReanimated();
}

void DemoralizeEffect::Start() {
    Actor* pTargetActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    Actor* pCasterActor = pCaster ? pCaster->GetCasterAsActor() : nullptr;
    if (pTargetActor && pCasterActor) {
        if (pTargetActor->GetCombatController()) {
            pTargetActor->GetCombatController()->ForceFlee(pCasterActor, false);
            bTurned = true;
        }
        else if (!pTargetActor->IsFleeing(false)) {
            pTargetActor->EndInterruptPackage(false);
            pTargetActor->InitiateFlee(pCasterActor, true, true, true, nullptr, nullptr, -1.0f, -1.0f);
            bTurned = true;
        }
    }
}

void DemoralizeEffect::Finish() {
    if (bTurned) {
        Actor* pTargetActor = nullptr;
        if (ActiveEffect::GetTarget()) {
            pTargetActor = ActiveEffect::GetTarget()->GetTargetAsActor();
        }
        Actor* pCasterActor = nullptr;
        if (ActiveEffect::GetCaster()) {
            pCasterActor = ActiveEffect::GetCaster()->GetCasterAsActor();
        }
        if (pTargetActor) {
            if (pTargetActor->IsInCombat()) {
                pTargetActor->StopCombat(pCasterActor);
                if (pCasterActor && pTargetActor->MobileObject::GetCurrentAIProcess()) {
                    pTargetActor->MobileObject::GetCurrentAIProcess()->EnterCombat(
                        pTargetActor,
                        pCasterActor,
                        true,
                        false,
                        nullptr,
                        false,
                        false,
                        false,
                        false,
                        false,
                        false,
                        true,
                        nullptr
                    );
                }
            }
            else if (pTargetActor->IsFleeing(false)) {
                pTargetActor->RemoveFleeTarget(pCasterActor);
            }
        }
    }
}

ActiveEffect* DemoralizeEffect::Create(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect) {
    return new DemoralizeEffect(apCaster, apItem, apEffect);
}
