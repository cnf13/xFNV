#include "Fallout/Magic/InvisibilityEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicModelHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "SDK/MSVC/rtti.h"

// Whether the invisibility archetype took Instantiate; stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::INVISIBILITY,
    &InvisibilityEffect::Instantiate
);

InvisibilityEffect::InvisibilityEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apItem, apEffect) {}

InvisibilityEffect::~InvisibilityEffect() {}

// Copies the effect: allocates a new one, constructs it from this effect's caster, spell and effect item, then copies
// the shared data into it.
ActiveEffect* InvisibilityEffect::Copy() {
    InvisibilityEffect* pCopy = new InvisibilityEffect(pCaster, pSpell, pEffect);
    static_cast<ActiveEffect*>(this)->CopyData(pCopy);
    return pCopy;
}

void InvisibilityEffect::Update(float afSecondsElapsed) {
    ValueModifierEffect::Update(afSecondsElapsed);
}

void InvisibilityEffect::FinishInitLoadGame(Actor* apActor) {
    ValueModifierEffect::FinishInitLoadGame(apActor);
    if (BSShaderManager::IsRefractionEnabled() &&
        static_cast<const void*>(apActor) == static_cast<const void*>(pPlayer)) {
        apActor->SetAlpha(1.0f);
        apActor->SetRefraction(true, 0.05000000074505806f);
    }
    else {
        apActor->UpdateAlpha();
    }
}

void InvisibilityEffect::Revert(Actor* apActor) {
    ActiveEffect::Revert(apActor);
    if (BSShaderManager::IsRefractionEnabled() && apActor == pPlayer) {
        apActor->SetRefraction(true, 0.0f);
    }
    else {
        apActor->UpdateAlpha();
    }
}

void InvisibilityEffect::CopyData(ActiveEffect* apDestination) {
    ValueModifierEffect::CopyData(apDestination);
}

bool InvisibilityEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pTargetActor = apTarget ? apTarget->GetTargetAsActor() : nullptr;
    return pTargetActor != nullptr;
}

void InvisibilityEffect::Start() {
    Actor* ptargetActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    ValueModifierEffect::Start();
    if (ptargetActor && static_cast<double>(ptargetActor->GetActorValue(eWhichActorValue)) > 0.0) {
        if (BSShaderManager::IsRefractionEnabled() && ptargetActor == pPlayer) {
            ptargetActor->SetAlpha(1.0f);
            ptargetActor->SetRefraction(true, 0.05f);
        }
        else {
            HighProcess* pprocess = dynamic_cast<HighProcess*>(ptargetActor->pCurrentProcess);
            if (pprocess && pprocess->GetFadeState() != 4) {
                pprocess->FadeOut(ptargetActor, nullptr, false);
            }
        }
    }
}

void InvisibilityEffect::Finish() {
    ValueModifierEffect::Finish();
    Actor* ptargetActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    if (ptargetActor && static_cast<double>(ptargetActor->GetActorValue(eWhichActorValue)) == 0.0) {
        if (BSShaderManager::IsRefractionEnabled() && ptargetActor == pPlayer) {
            ptargetActor->SetRefraction(true, 0.0f);
        }
        else {
            HighProcess* pprocess = dynamic_cast<HighProcess*>(ptargetActor->pCurrentProcess);
            if (pprocess && pprocess->GetFadeState() != BaseProcess::FADE_TELEPORT_OUT) {
                pprocess->FadeIn(ptargetActor, false);
            }
        }
        if (static_cast<float>(0.0) > 0.0 && BSShaderManager::IsRefractionEnabled()) {
            float fLerpValue = 1.0 - 0.0f;
            float fMaxRefraction = MagicSystem::GetChameleonMaxRefraction();
            float fMinRefraction = MagicSystem::GetChameleonMinRefraction();
            float fRefractionPower = FLerp(fMinRefraction, fMaxRefraction, 0.0f, 1.0f, fLerpValue);
            ptargetActor->SetRefraction(true, fRefractionPower);
        }
        else {
            ptargetActor->UpdateAlpha();
        }
        MagicHitEffect* peffect = nullptr;
        if (GetEffectSetting()->GetModelLength()) {
            peffect = new MagicModelHitEffect(
                ptargetActor,
                static_cast<TESModel*>(GetEffectSetting())->GetModel(),
                -1.0f
            );
        }
        if (peffect && peffect->Init()) {
            ReferenceProcessLists.AddTempEffect(peffect);
        }
        else if (peffect) {
            delete peffect;
        }
    }
}

ActiveEffect* InvisibilityEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new InvisibilityEffect(apCaster, apSpell, apEffect);
}
