#include "Fallout/Magic/DarknessEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"

// Whether the darkness archetype took Instantiate; holds RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::DARKNESS,
    &DarknessEffect::Instantiate
);

DarknessEffect::DarknessEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apSpell, apEffect) {}

// Copies the effect: allocates a new one, constructs it from this effect's caster, spell and effect item, then copies
// the shared data into it. CopyData is called through the vtable with this effect as `this`.
ActiveEffect* DarknessEffect::Copy() {
    DarknessEffect* pCopy = new DarknessEffect(pCaster, pSpell, pEffect);
    static_cast<ActiveEffect*>(this)->CopyData(pCopy);
    return pCopy;
}

void DarknessEffect::FinishInitLoadGame(Actor* apActor) {
    ValueModifierEffect::FinishInitLoadGame(apActor);
    float fNewDarkness = 1.0 - apActor->GetActorFloatValue(eWhichActorValue) / 100.0;
    if (fNewDarkness < 0.0) {
        fNewDarkness = 0.0f;
    }
    if (fNewDarkness > 1.0) {
        fNewDarkness = 1.0f;
    }
    if (apActor->TESObjectREFR::Get3D()) {
        BSShaderManager::RecursiveSetForcedDarkness(apActor->TESObjectREFR::Get3D(), fNewDarkness);
    }
}

// Reverts the base effect, then lifts the forced darkness (1.0) from the actor's 3D. The base Revert and
// TESObjectREFR::Get3D are called directly, not virtually.
void DarknessEffect::Revert(Actor* apActor) {
    ValueModifierEffect::Revert(apActor);
    if (apActor->TESObjectREFR::Get3D()) {
        BSShaderManager::RecursiveSetForcedDarkness(apActor->TESObjectREFR::Get3D(), 1.0f);
    }
}

void DarknessEffect::CopyData(ActiveEffect* apDestination) {
    ValueModifierEffect::CopyData(apDestination);
}

bool DarknessEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pTargetActor = apTarget ? apTarget->GetTargetAsActor() : nullptr;
    return pTargetActor != nullptr;
}

void DarknessEffect::Start() {
    Actor* pTargetActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    ValueModifierEffect::Start();
    if (pTargetActor && pTargetActor->Get3D()) {
        float fNewDarkness = 1.0 - pTargetActor->GetActorFloatValue(eWhichActorValue) / 100.0;
        if (fNewDarkness < 0.0) {
            fNewDarkness = 0.0f;
        }
        if (fNewDarkness > 1.0) {
            fNewDarkness = 1.0f;
        }
        BSShaderManager::RecursiveSetForcedDarkness(pTargetActor->Get3D(), fNewDarkness);
    }
}

void DarknessEffect::Finish() {
    ValueModifierEffect::Finish();
    Actor* pTargetActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    if (pTargetActor && pTargetActor->Get3D()) {
        float fNewDarkness = 1.0 - pTargetActor->GetActorFloatValue(eWhichActorValue) / 100.0;
        if (fNewDarkness < 0.0) {
            fNewDarkness = 0.0f;
        }
        if (fNewDarkness > 1.0) {
            fNewDarkness = 1.0f;
        }
        BSShaderManager::RecursiveSetForcedDarkness(pTargetActor->Get3D(), fNewDarkness);
    }
}

ActiveEffect* DarknessEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new DarknessEffect(apCaster, apSpell, apEffect);
}
