#include "Fallout/Magic/CalmEffect.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"

static char bCalmEffectInitialized = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::CALM,
    &CalmEffect::Instantiate
);

CalmEffect::CalmEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apSpell, apEffect) {}

CalmEffect::~CalmEffect() {}

// Copies the effect: a new instance built from the same caster, spell and effect item, then the field copy.
ActiveEffect* CalmEffect::Copy() {
    ActiveEffect* pCopy = new CalmEffect(pCaster, pSpell, pEffect);
    ActiveEffect* pThis = this;
    pThis->CopyData(pCopy);
    return pCopy;
}

void CalmEffect::Update(float afSecondsElapsed) {
    ValueModifierEffect::Update(afSecondsElapsed);
    Actor* pTargetAsActor = nullptr;
    if (ActiveEffect::GetTarget()) {
        MagicTarget* pTarget = ActiveEffect::GetTarget();
        pTargetAsActor = pTarget->GetTargetAsActor();
    }
    if (pTargetAsActor) {
        int8_t bNoCombatController;
        if (pTargetAsActor->GetCombatController()) {
            pTargetAsActor->GetCombatController();
            bNoCombatController = 0;
        }
        else {
            bNoCombatController = 1;
        }
        if (pTargetAsActor->IsDead(false) || pTargetAsActor->IsDying() || !bNoCombatController) {
            ActiveEffect::Dispel(false);
        }
    }
}

bool CalmEffect::CheckDeletedCaster(MagicCaster* apCaster) {
    bool bDeleted = ActiveEffect::CheckDeletedCaster(apCaster);
    if (bDeleted) {
        Dispel(true);
    }
    return bDeleted;
}

bool CalmEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pActor = apTarget ? apTarget->GetTargetAsActor() : nullptr;
    if (pActor && !pActor->IsDead(false) && !pActor->IsDying() && pActor->IsInCombat()) {
        float fEffectMagnitude = fMagnitude;
        uint16_t usLevel = pActor->GetLevel();
        if (MagicFormulas::CheckTargetLevelMagnitude(usLevel, fEffectMagnitude)) {
            return true;
        }
    }

    return false;
}

void CalmEffect::Start() {
    ValueModifierEffect::Start();
    Actor* pTargetAsActor = nullptr;
    if (ActiveEffect::GetTarget()) {
        MagicTarget* pTarget = ActiveEffect::GetTarget();
        pTargetAsActor = pTarget->GetTargetAsActor();
    }
    if (pTargetAsActor && pTargetAsActor->GetCombatController()) {
        CombatController* pCombatController = pTargetAsActor->GetCombatController();
        pCombatController->StartCalmed();
    }
}

void CalmEffect::Finish() {
    ValueModifierEffect::Finish();
    Actor* pTargetAsActor = nullptr;
    if (ActiveEffect::GetTarget()) {
        MagicTarget* pTarget = ActiveEffect::GetTarget();
        pTargetAsActor = pTarget->GetTargetAsActor();
    }
    if (pTargetAsActor && pTargetAsActor->GetCombatController()) {
        CombatController* pCombatController = pTargetAsActor->GetCombatController();
        pCombatController->TESForm::InitializeDataComponent();
    }
}

ActiveEffect* CalmEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new CalmEffect(apCaster, apSpell, apEffect);
}
