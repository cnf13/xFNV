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
    ShieldEffect* v2;
    void* v4 = ourMalloc(sizeof(ShieldEffect));
    if (v4) {
        v2 = ::new (v4) ShieldEffect(pCaster, pSpell, pEffect);
    }
    else {
        v2 = nullptr;
    }
    CopyData(v2);
    return v2;
}

void ShieldEffect::Update(float afSecondsElapsed) {
    MagicTarget* unk24;
    float v4;
    Actor* targetAsActor;
    ValueModifierEffect::Update(afSecondsElapsed);
    EffectSetting* time = pEffect->pEffectSetting;
    if (!time->IsFlagSet(EffectSetting::RECOVER)) {
        if (ActiveEffect::GetTarget()) {
            unk24 = ActiveEffect::GetTarget();
            targetAsActor = unk24->GetTargetAsActor();
        }
        else {
            targetAsActor = nullptr;
        }
        if ((fDuration > 0.0) || (((int32_t)eSpellType) == 4)) {
            v4 = fMagnitude * afSecondsElapsed;
            ModifyAdditionalActorValue(targetAsActor, v4);
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
        pSaveLoadGameOLD->SaveGameDataOLD(&eAdditionalActorValue, 4);
    }
}

void ShieldEffect::LoadGame(Actor* apActor) {
    ValueModifierEffect::LoadGame(apActor);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x6E) {
        pSaveLoadGameOLD->LoadGameDataOLD(&eAdditionalActorValue, 4);
    }
}

void ShieldEffect::CopyData(ActiveEffect* apDestination) {
    ValueModifierEffect::CopyData(apDestination);
    ShieldEffect* v3 = dynamic_cast<ShieldEffect*>(apDestination);
    if (v3) {
        v3->eAdditionalActorValue = static_cast<ActorValue::Index>((int32_t)eAdditionalActorValue);
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
    MagicTarget* unk24;
    float v3;
    Actor* targetAsActor;
    ValueModifierEffect::Finish();
    EffectSetting* time = pEffect->pEffectSetting;
    if (time->IsFlagSet(EffectSetting::RECOVER)) {
        if (ActiveEffect::GetTarget()) {
            unk24 = ActiveEffect::GetTarget();
            targetAsActor = unk24->GetTargetAsActor();
        }
        else {
            targetAsActor = nullptr;
        }
        v3 = -fMagnitude;
        ModifyAdditionalActorValue(targetAsActor, v3);
    }
}

void ShieldEffect::ModifyAdditionalActorValue(Actor* apActor, float afModifier) {
    if (apActor) {
        if (eAdditionalActorValue != ActorValue::ACTOR_VALUE_COUNT) {
            if (eSpellType == MagicSystem::ABILITY) {
                ActorValue::Index Sound1 = eAdditionalActorValue;
                apActor->ModActorBaseValue(Sound1, afModifier);
            }
            else {
                EffectSetting* Time = pEffect->GetEffectSetting();
                if (Time->IsFlagSet(EffectSetting::RECOVER)) {
                    ActorValue::Index v5 = eAdditionalActorValue;
                    apActor->TempModActorValue(v5, afModifier, static_cast<Actor*>(nullptr));
                }
                else {
                    ActorValue::Index v6 = eAdditionalActorValue;
                    apActor->DamageModActorValue(v6, afModifier, static_cast<Actor*>(nullptr));
                }
            }
        }
    }
}

ActiveEffect* ShieldEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    void* v5 = ourMalloc(sizeof(ShieldEffect));
    if (v5) {
        return ::new (v5) ShieldEffect(apCaster, apSpell, apEffect);
    }
    else {
        return nullptr;
    }
}
