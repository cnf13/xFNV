#include "Fallout/Magic/LimbConditionEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"

// Whether the limb-condition archetype took Create: stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::LIMB_CONDITION,
    &LimbConditionEffect::Create
);

LimbConditionEffect::LimbConditionEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apSpell, apEffect), eTargetBodyPart(ActorValue::VALUE_NONE) {}

LimbConditionEffect::~LimbConditionEffect() {}

// Copies the effect: allocates a new one, constructs it from this effect's caster, spell and effect item, then copies
// the shared data into it.
ActiveEffect* LimbConditionEffect::Copy() {
    LimbConditionEffect* pCopy = new LimbConditionEffect(pCaster, pSpell, pEffect);
    static_cast<ActiveEffect*>(this)->CopyData(pCopy);
    return pCopy;
}

uint16_t LimbConditionEffect::GetSaveSize(Actor* apActor) {
    return static_cast<uint16_t>(ValueModifierEffect::GetSaveSize(apActor) + 4);
}

// The base's BGS data, then the target body part.
void LimbConditionEffect::SaveGame(BGSSaveFormBuffer* apBuffer) {
    ActiveEffect::SaveGame(apBuffer);
    apBuffer->SaveDataEndian(reinterpret_cast<char*>(&eTargetBodyPart), sizeof(eTargetBodyPart), 0);
}

// The old-format save: the base's data, then the four-byte target body part.
void LimbConditionEffect::SaveGame(Actor* apActor) {
    ValueModifierEffect::SaveGame(apActor);
    pSaveLoadGameOLD->SaveGameDataOLD(&eTargetBodyPart, sizeof(eTargetBodyPart));
}

// Save form version 8 added the target body part.
void LimbConditionEffect::LoadGame(BGSLoadFormBuffer* apBuffer) {
    ActiveEffect::LoadGame(apBuffer);
    if (apBuffer->GetVersion() >= 8) {
        apBuffer->LoadData(reinterpret_cast<char*>(&eTargetBodyPart), sizeof(eTargetBodyPart));
    }
}

// The old-format save: the base's data, then the four-byte target body part.
void LimbConditionEffect::LoadGame(Actor* apActor) {
    ValueModifierEffect::LoadGame(apActor);
    pSaveLoadGameOLD->LoadGameDataOLD(&eTargetBodyPart, sizeof(eTargetBodyPart));
}

void LimbConditionEffect::CopyData(ActiveEffect* apDestination) {
    ValueModifierEffect::CopyData(apDestination);
    LimbConditionEffect* pDestination = dynamic_cast<LimbConditionEffect*>(apDestination);
    if (pDestination) {
        pDestination->eTargetBodyPart = eTargetBodyPart;
    }
}

void LimbConditionEffect::ModifyActorValue(Actor* apActor, float afModifier, ActorValue::Index aeOverrideActorValue) {
    if (eTargetBodyPart == ActorValue::VALUE_NONE) {
        float fNoPartModifier = afModifier * fMagicVACNoPartTargetedMult.Float();
        for (int32_t i = ActorValue::CONDITION_START; i < ActorValue::CONDITION_END; ++i) {
            ValueModifierEffect::ModifyActorValue(apActor, fNoPartModifier, static_cast<ActorValue::Index>(i));
        }
    }
    else {
        ValueModifierEffect::ModifyActorValue(apActor, afModifier, eTargetBodyPart);
    }
}

void LimbConditionEffect::SetTargetBodyPart(uint32_t aeWhichBodyPart) {
    eTargetBodyPart = static_cast<ActorValue::Index>(aeWhichBodyPart);
}

ActiveEffect* LimbConditionEffect::Create(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new LimbConditionEffect(apCaster, apSpell, apEffect);
}
