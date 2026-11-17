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

static char bCalmEffectInitialized =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::CALM, &CalmEffect::Instantiate);

CalmEffect::CalmEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apSpell, apEffect) {}

CalmEffect::~CalmEffect() {}

// Copies the effect: a new instance built from the same caster, spell and effect item, then the field copy.
ActiveEffect* CalmEffect::Copy() {
    ActiveEffect* v2;
    void* v4 = ourMalloc(sizeof(CalmEffect));
    if (v4) {
        v2 = ::new (v4) CalmEffect(pCaster, pSpell, pEffect);
    }
    else {
        v2 = nullptr;
    }
    ActiveEffect* pThis = this;
    pThis->CopyData(v2);
    return v2;
}

void CalmEffect::Update(float afSecondsElapsed) {
    MagicTarget* unk24;
    int8_t v3;
    Actor* targetAsActor;
    ValueModifierEffect::Update(afSecondsElapsed);
    if (ActiveEffect::GetTarget()) {
        unk24 = ActiveEffect::GetTarget();
        targetAsActor = unk24->GetTargetAsActor();
    }
    else {
        targetAsActor = nullptr;
    }
    if (targetAsActor) {
        if (targetAsActor->GetCombatController()) {
            targetAsActor->GetCombatController();
            v3 = 0;
        }
        else {
            v3 = 1;
        }
        if (targetAsActor->IsDead(false) || (targetAsActor->IsDying() || (!v3))) {
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
    uint16_t level;
    float afMagnitude;
    Actor* actor;
    if (apTarget) {
        actor = apTarget->GetTargetAsActor();
    }
    else {
        actor = nullptr;
    }
    uint8_t result = 0;
    if (actor) {
        if (((!actor->IsDead(false)) && (!actor->IsDying())) && actor->IsInCombat()) {
            afMagnitude = fMagnitude;
            level = actor->GetLevel();
            if (MagicFormulas::CheckTargetLevelMagnitude(level, afMagnitude)) {
                return true;
            }
        }
    }
    return (bool)result;
}

void CalmEffect::Start() {
    MagicTarget* unk24;
    CombatController* v2;
    Actor* targetAsActor;
    ValueModifierEffect::Start();
    if (ActiveEffect::GetTarget()) {
        unk24 = ActiveEffect::GetTarget();
        targetAsActor = unk24->GetTargetAsActor();
    }
    else {
        targetAsActor = nullptr;
    }
    if (targetAsActor) {
        if (targetAsActor->GetCombatController()) {
            v2 = targetAsActor->GetCombatController();
            v2->StartCalmed();
        }
    }
}

void CalmEffect::Finish() {
    MagicTarget* unk24;
    CombatController* v2;
    Actor* targetAsActor;
    ValueModifierEffect::Finish();
    if (ActiveEffect::GetTarget()) {
        unk24 = ActiveEffect::GetTarget();
        targetAsActor = unk24->GetTargetAsActor();
    }
    else {
        targetAsActor = nullptr;
    }
    if (targetAsActor) {
        if (targetAsActor->GetCombatController()) {
            v2 = targetAsActor->GetCombatController();
            v2->TESForm::InitializeDataComponent();
        }
    }
}

ActiveEffect* CalmEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    void* apMemory = ourMalloc(sizeof(CalmEffect));
    if (apMemory) {
        return ::new (apMemory) CalmEffect(apCaster, apSpell, apEffect);
    }
    else {
        return nullptr;
    }
}
