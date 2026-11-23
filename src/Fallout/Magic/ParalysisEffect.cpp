#include "Fallout/Magic/ParalysisEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkRigidBody.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"

// Whether the paralyze archetype took Instantiate; stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::PARALYZE,
    &ParalysisEffect::Instantiate
);

ParalysisEffect::ParalysisEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apSpell, apEffect) {}

ParalysisEffect::~ParalysisEffect() {}

// Allocates a copy, constructs it from this effect's caster, spell and effect item, then copies the effect data.
ActiveEffect* ParalysisEffect::Copy() {
    ParalysisEffect* pCopy = new ParalysisEffect(pCaster, pSpell, pEffect);
    CopyData(pCopy);
    return pCopy;
}

void ParalysisEffect::Update(float afSecondsElapsed) {
    ValueModifierEffect::Update(afSecondsElapsed);
    Actor* pTargetActor = nullptr;
    if (ActiveEffect::GetTarget()) {
        MagicTarget* pMagicTarget = ActiveEffect::GetTarget();
        pTargetActor = pMagicTarget->GetTargetAsActor();
    }
    if (pTargetActor && pTargetActor->IsDead(false)) {
        ActiveEffect::Dispel(true);
    }
}

void ParalysisEffect::FinishInitLoadGame(Actor* apActor) {
    ValueModifierEffect::FinishInitLoadGame(apActor);
}

void ParalysisEffect::Revert(Actor* apActor) {
    ActiveEffect::Revert(apActor);
    NiNode* pObject = apActor->TESObjectREFR::Get3D();
    if (pObject) {
        bhkRigidBody::SetFixed(pObject, 0, 1);
    }
}

void ParalysisEffect::CopyData(ActiveEffect* apDestination) {
    ValueModifierEffect::CopyData(apDestination);
}

void ParalysisEffect::Start() {
    Actor* pTargetActor = nullptr;
    if (ActiveEffect::GetTarget()) {
        MagicTarget* pMagicTarget = ActiveEffect::GetTarget();
        pTargetActor = pMagicTarget->GetTargetAsActor();
    }
    if (pTargetActor) {
        pTargetActor->MobileObject::StopCurrentDialogue();
        pTargetActor->IsParalyzed();
        ValueModifierEffect::Start();
    }
}

void ParalysisEffect::Finish() {
    ValueModifierEffect::Finish();
}

ActiveEffect* ParalysisEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new ParalysisEffect(apCaster, apSpell, apEffect);
}
