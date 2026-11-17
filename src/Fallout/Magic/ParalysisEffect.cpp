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
static char bRegistered =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::PARALYZE, &ParalysisEffect::Instantiate);

ParalysisEffect::ParalysisEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apSpell, apEffect) {}

ParalysisEffect::~ParalysisEffect() {}

// Allocates a copy, constructs it from this effect's caster, spell and effect item, then copies the effect data.
ActiveEffect* ParalysisEffect::Copy() {
    void* pMemory = ourMalloc(sizeof(ParalysisEffect));
    ParalysisEffect* pCopy;
    if (pMemory) {
        pCopy = ::new (pMemory) ParalysisEffect(pCaster, pSpell, pEffect);
    }
    else {
        pCopy = nullptr;
    }
    CopyData(pCopy);
    return pCopy;
}

void ParalysisEffect::Update(float afSecondsElapsed) {
    MagicTarget* unk24;
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
        if (targetAsActor->IsDead(false)) {
            ActiveEffect::Dispel(true);
        }
    }
}

void ParalysisEffect::FinishInitLoadGame(Actor* apActor) {
    ValueModifierEffect::FinishInitLoadGame(apActor);
}

void ParalysisEffect::Revert(Actor* apActor) {
    ActiveEffect::Revert(apActor);
    NiNode* apObject = apActor->TESObjectREFR::Get3D();
    if (apObject) {
        bhkRigidBody::SetFixed(apObject, 0, 1);
    }
}

void ParalysisEffect::CopyData(ActiveEffect* apDestination) {
    ValueModifierEffect::CopyData(apDestination);
}

void ParalysisEffect::Start() {
    MagicTarget* unk24;
    Actor* targetAsActor;
    if (ActiveEffect::GetTarget()) {
        unk24 = ActiveEffect::GetTarget();
        targetAsActor = unk24->GetTargetAsActor();
    }
    else {
        targetAsActor = nullptr;
    }
    if (targetAsActor) {
        targetAsActor->MobileObject::StopCurrentDialogue();
        targetAsActor->IsParalyzed();
        ValueModifierEffect::Start();
    }
}

void ParalysisEffect::Finish() {
    ValueModifierEffect::Finish();
}

ActiveEffect* ParalysisEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    void* v5 = ourMalloc(sizeof(ParalysisEffect));
    if (v5) {
        return ::new (v5) ParalysisEffect(apCaster, apSpell, apEffect);
    }
    else {
        return nullptr;
    }
}
