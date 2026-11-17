#include "Fallout/Magic/NightEyeEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectCinematic.h"
#include "BSShader/ImageSpaceManager.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"

// Whether the night-eye archetype took Create: stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(EffectArchetypes::NIGHT_EYE, &NightEyeEffect::Create);

NightEyeEffect::NightEyeEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apSpell, apEffect) {}

NightEyeEffect::~NightEyeEffect() {}

// Allocates a copy, constructs it from this effect's caster, spell and effect item, then copies the effect data.
ActiveEffect* NightEyeEffect::Copy() {
    void* pMemory = ourMalloc(sizeof(NightEyeEffect));
    NightEyeEffect* pCopy;
    if (pMemory) {
        pCopy = ::new (pMemory) NightEyeEffect(pCaster, pSpell, pEffect);
    }
    else {
        pCopy = nullptr;
    }
    CopyData(pCopy);
    return pCopy;
}

void NightEyeEffect::Update(float afSecondsElapsed) {
    ValueModifierEffect::Update(afSecondsElapsed);
}

// Calls ActiveEffect's body; ValueModifierEffect does not override it.
void NightEyeEffect::InitLoadGame(Actor* apActor) {
    ActiveEffect::InitLoadGame(apActor);
    if (apActor == pPlayer) {
        ApplyVisualEffect();
    }
}

void NightEyeEffect::Revert(Actor* apActor) {
    ActiveEffect::Revert(apActor);
    if (pSaveLoadGameOLD->IsUnk048Zero() && apActor == pPlayer) {
        NightEyeEffect::ApplyVisualEffect();
    }
}

void NightEyeEffect::CopyData(ActiveEffect* apDestination) {
    ValueModifierEffect::CopyData(apDestination);
}

bool NightEyeEffect::CheckTarget(MagicTarget* apTarget) {
    Actor* pTargetActor;
    if (apTarget) {
        pTargetActor = apTarget->GetTargetAsActor();
    }
    else {
        pTargetActor = nullptr;
    }
    return pTargetActor != nullptr;
}

// Runs at full magnitude and turns the night eye view on when the target is the player.
void NightEyeEffect::Start() {
    SetMagnitude(1.0f);
    ValueModifierEffect::Start();
    MagicTarget* pMagicTarget = GetTarget();
    if (pMagicTarget->GetTargetStatsObject() == pPlayer) {
        ApplyVisualEffect();
    }
}

// Calls ApplyVisualEffect on finishing too when the target is the player.
void NightEyeEffect::Finish() {
    ValueModifierEffect::Finish();
    MagicTarget* pMagicTarget = GetTarget();
    if (pMagicTarget->GetTargetStatsObject() == pPlayer) {
        ApplyVisualEffect();
    }
}

void NightEyeEffect::ApplyVisualEffect() {
    ImageSpaceManager* pmanager = BSShaderManager::GetImageSpaceManager();
    ImageSpaceEffectCinematic* peffect =
        static_cast<ImageSpaceEffectCinematic*>(pmanager->GetEffect(ImageSpaceManager::EFFECT_CINEMATIC));
    if (pPlayer->GetActorValue(ActorValue::NIGHT_EYE_BONUS) <= 0) {
        BSShaderManager::fMinAmbient = 0.0f;
        peffect->SetNightEye(false);
    }
    else {
        BSShaderManager::fMinAmbient = fMagicNightEyeAmbient.Float();
        peffect->SetNightEye(true);
    }
}

ActiveEffect* NightEyeEffect::Create(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    void* v5 = ourMalloc(sizeof(NightEyeEffect));
    if (v5) {
        return ::new (v5) NightEyeEffect(apCaster, apSpell, apEffect);
    }
    else {
        return nullptr;
    }
}
