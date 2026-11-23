#include "Fallout/Magic/ValueAndConditionsEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"

// Whether the value-and-parts archetype took Create; stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::VALUE_AND_PARTS,
    &ValueAndConditionsEffect::Create
);

ValueAndConditionsEffect::ValueAndConditionsEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apItem, apEffect), eTargetBodyPart(ActorValue::VALUE_NONE) {}

ValueAndConditionsEffect::~ValueAndConditionsEffect() {}

// Allocates a copy, constructs it from this effect's caster, spell and effect item, then copies the effect data.
ActiveEffect* ValueAndConditionsEffect::Copy() {
    ValueAndConditionsEffect* pCopy = new ValueAndConditionsEffect(pCaster, pSpell, pEffect);
    CopyData(pCopy);
    return pCopy;
}

// The base's size plus the four-byte target body part.
uint16_t ValueAndConditionsEffect::GetSaveSize(Actor* apActor) {
    return static_cast<uint16_t>(ValueModifierEffect::GetSaveSize(apActor) + 4);
}

// The base's BGS data, then the target body part.
void ValueAndConditionsEffect::SaveGame(BGSSaveFormBuffer* apBuffer) {
    ValueModifierEffect::SaveGame(apBuffer);
    apBuffer->SaveDataEndian(&eTargetBodyPart, sizeof(eTargetBodyPart), 0);
}

// The old-format save: the base's data, then the four-byte target body part.
void ValueAndConditionsEffect::SaveGame(Actor* apActor) {
    ValueModifierEffect::SaveGame(apActor);
    pSaveLoadGameOLD->SaveGameDataOLD(&eTargetBodyPart, sizeof(eTargetBodyPart));
}

// Save form version 8 added the target body part.
void ValueAndConditionsEffect::LoadGame(BGSLoadFormBuffer* apBuffer) {
    ValueModifierEffect::LoadGame(apBuffer);
    if (apBuffer->GetVersion() >= 8) {
        apBuffer->LoadData(&eTargetBodyPart, sizeof(eTargetBodyPart));
    }
}

// The old-format save: the base's data, then the four-byte target body part.
void ValueAndConditionsEffect::LoadGame(Actor* apActor) {
    ValueModifierEffect::LoadGame(apActor);
    pSaveLoadGameOLD->LoadGameDataOLD(&eTargetBodyPart, sizeof(eTargetBodyPart));
}

void ValueAndConditionsEffect::CopyData(ActiveEffect* apDestination) {
    ValueModifierEffect::CopyData(apDestination);
    ValueAndConditionsEffect* pDestination = dynamic_cast<ValueAndConditionsEffect*>(apDestination);
    if (pDestination) {
        pDestination->eTargetBodyPart = eTargetBodyPart;
    }
}

void ValueAndConditionsEffect::ModifyActorValue(
    Actor* apActor,
    float afModifier,
    ActorValue::Index aeOverrideActorValue
) {
    if (eTargetBodyPart == ActorValue::VALUE_NONE) {
        ValueModifierEffect::ModifyActorValue(apActor, afModifier, ActorValue::VALUE_NONE);
        float fPartModifier = afModifier * fMagicVACNoPartTargetedMult.Float();
        for (int32_t i = 25; i < 32; ++i) {
            ValueModifierEffect::ModifyActorValue(apActor, fPartModifier, static_cast<ActorValue::Index>(i));
        }
    }
    else {
        float fValueModifier = afModifier * fMagicVACPartTargetedMult.Float();
        ValueModifierEffect::ModifyActorValue(apActor, afModifier, eTargetBodyPart);
        ValueModifierEffect::ModifyActorValue(apActor, fValueModifier, ActorValue::VALUE_NONE);
    }
}

void ValueAndConditionsEffect::SetTargetBodyPart(ActorValue::Index aeWhichBodyPart) {
    eTargetBodyPart = aeWhichBodyPart;
}

ActiveEffect* ValueAndConditionsEffect::Create(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect) {
    return new ValueAndConditionsEffect(apCaster, apItem, apEffect);
}
