#include "Fallout/Magic/ShieldEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"

// Whether the shield archetype took Instantiate; stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(EffectArchetypes::SHIELD, &ShieldEffect::Instantiate);

ShieldEffect::ShieldEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apItem, apEffect) {
    eWhichActorValue = apEffect->GetActorValueIndex();
}

ShieldEffect::~ShieldEffect() {}

ActiveEffect* ShieldEffect::Copy() {
    ShieldEffect* pCopy = new ShieldEffect(pCaster, pSpell, pEffect);
    CopyData(pCopy);
    return pCopy;
}

void ShieldEffect::Update(float afSecondsElapsed) {
    ValueModifierEffect::Update(afSecondsElapsed);
    if (!pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER)) {
        Actor* pActor = ActiveEffect::GetTarget() ? ActiveEffect::GetTarget()->GetTargetAsActor() : nullptr;
        if (fDuration > 0.0 || eSpellType == MagicSystem::ABILITY) {
            ModifyAdditionalActorValue(pActor, fMagnitude * afSecondsElapsed);
        }
    }
}

// Save version 0x6E added 4 bytes.
uint16_t ShieldEffect::GetSaveSize(Actor* apActor) {
    uint16_t usSize = ValueModifierEffect::GetSaveSize(apActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x6E) {
        usSize += 4;
    }
    return usSize;
}

void ShieldEffect::SaveGame(Actor* apActor) {
    ValueModifierEffect::SaveGame(apActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x6E) {
        pSaveLoadGameOLD->SaveGameDataOLD(&eAdditionalActorValue, sizeof(eAdditionalActorValue));
    }
}

void ShieldEffect::LoadGame(Actor* apActor) {
    ValueModifierEffect::LoadGame(apActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x6E) {
        pSaveLoadGameOLD->LoadGameDataOLD(&eAdditionalActorValue, sizeof(eAdditionalActorValue));
    }
}

void ShieldEffect::CopyData(ActiveEffect* apDestination) {
    ValueModifierEffect::CopyData(apDestination);
    ShieldEffect* pShieldEffect = dynamic_cast<ShieldEffect*>(apDestination);
    if (pShieldEffect) {
        pShieldEffect->eAdditionalActorValue = eAdditionalActorValue;
    }
}

// fDuration is compared with the double 0.0: a duration equal to zero (not NaN) also adds the additional actor value.
void ShieldEffect::Start() {
    ValueModifierEffect::Start();
    if (pEffect->GetEffectSetting()->IsFlagSet(EffectSetting::RECOVER) || fDuration == 0.0) {
        Actor* pActor = GetTarget() ? GetTarget()->GetTargetAsActor() : nullptr;
        ModifyAdditionalActorValue(pActor, fMagnitude);
    }
}

void ShieldEffect::Finish() {
    ValueModifierEffect::Finish();
    if (pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER)) {
        Actor* pActor = ActiveEffect::GetTarget() ? ActiveEffect::GetTarget()->GetTargetAsActor() : nullptr;
        ModifyAdditionalActorValue(pActor, -fMagnitude);
    }
}

void ShieldEffect::ModifyAdditionalActorValue(Actor* apActor, float afModifier) {
    if (apActor && eAdditionalActorValue != ActorValue::ACTOR_VALUE_COUNT) {
        if (eSpellType == MagicSystem::ABILITY) {
            apActor->ModActorBaseValue(eAdditionalActorValue, afModifier);
        }
        else if (pEffect->GetEffectSetting()->IsFlagSet(EffectSetting::RECOVER)) {
            apActor->TempModActorValue(eAdditionalActorValue, afModifier, static_cast<Actor*>(nullptr));
        }
        else {
            apActor->DamageModActorValue(eAdditionalActorValue, afModifier, static_cast<Actor*>(nullptr));
        }
    }
}

ActiveEffect* ShieldEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new ShieldEffect(apCaster, apSpell, apEffect);
}
