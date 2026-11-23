#include "Fallout/Magic/TurnUndeadEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Package.h"
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

// Whether the turn-undead archetype took Create; stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::TURN_UNDEAD,
    &TurnUndeadEffect::Create
);

TurnUndeadEffect::TurnUndeadEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ActiveEffect(apCaster, apItem, apEffect), bTurned(false) {}

TurnUndeadEffect::~TurnUndeadEffect() {}

// Copies the flag byte onto the new effect, then copies the base data through the virtual copy function.
ActiveEffect* TurnUndeadEffect::Copy() {
    TurnUndeadEffect* pCopy;
    pCopy = new TurnUndeadEffect(pCaster, pSpell, pEffect);
    pCopy->bTurned = bTurned;
    CopyData(pCopy);
    return pCopy;
}

// The base's size plus the one-byte bTurned.
uint16_t TurnUndeadEffect::GetSaveSize(Actor* apActor) {
    return static_cast<uint16_t>(ActiveEffect::GetSaveSize(apActor) + 1);
}

// The old-format save: the base's data, then bTurned.
void TurnUndeadEffect::SaveGame(Actor* apActor) {
    ActiveEffect::SaveGame(apActor);
    pSaveLoadGameOLD->SaveGameDataOLD(&bTurned, sizeof(bTurned));
}

// The old-format load: the base's data, then bTurned.
void TurnUndeadEffect::LoadGame(Actor* apActor) {
    ActiveEffect::LoadGame(apActor);
    pSaveLoadGameOLD->LoadGameDataOLD(&bTurned, sizeof(bTurned));
}

// A deleted caster dispels the effect at once.
bool TurnUndeadEffect::CheckDeletedCaster(MagicCaster* apCaster) {
    bool bDeleted = ActiveEffect::CheckDeletedCaster(apCaster);
    if (bDeleted) {
        Dispel(true);
    }
    return bDeleted;
}

bool TurnUndeadEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pTargetActor = apTarget ? apTarget->GetTargetAsActor() : nullptr;
    if (pTargetActor && !pTargetActor->IsDead(false) && !pTargetActor->IsDying()) {
        float fMagnitude = GetMagnitude();
        uint16_t usLevel = pTargetActor->GetLevel();
        if (MagicFormulas::CheckTargetLevelMagnitude(usLevel, fMagnitude) &&
            !dynamic_cast<Creature*>(apTarget) &&
            pTargetActor->IsReanimated()) {
            return true;
        }
    }

    return false;
}

void TurnUndeadEffect::Start() {
    Actor* pTargetActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    Actor* pCasterActor = pCaster ? pCaster->GetCasterAsActor() : nullptr;
    if (pTargetActor && pCasterActor && pTargetActor->GetCombatController()) {
        pTargetActor->GetCombatController()->ForceFlee(pCasterActor, 0);
        bTurned = true;
    }
}

void TurnUndeadEffect::Finish() {
    if (bTurned) {
        Actor* pTargetActor;
        if (ActiveEffect::GetTarget()) {
            MagicTarget* pMagicTarget = ActiveEffect::GetTarget();
            pTargetActor = pMagicTarget->GetTargetAsActor();
        }
        else {
            pTargetActor = nullptr;
        }
        Actor* pCasterActor;
        if (ActiveEffect::GetCaster()) {
            MagicCaster* pMagicCaster = ActiveEffect::GetCaster();
            pCasterActor = pMagicCaster->GetCasterAsActor();
        }
        else {
            pCasterActor = nullptr;
        }
        if (pTargetActor && pTargetActor->IsInCombat()) {
            pTargetActor->StopCombat(pCasterActor);
            if (pCasterActor && pTargetActor->MobileObject::GetCurrentAIProcess()) {
                BaseProcess* pProcess = pTargetActor->MobileObject::GetCurrentAIProcess();
                pProcess->EnterCombat(
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
    }
}

ActiveEffect* TurnUndeadEffect::Create(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect) {
    return new TurnUndeadEffect(apCaster, apItem, apEffect);
}
