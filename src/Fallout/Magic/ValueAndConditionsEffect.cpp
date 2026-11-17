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
static char bRegistered =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::VALUE_AND_PARTS, &ValueAndConditionsEffect::Create);

ValueAndConditionsEffect::ValueAndConditionsEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apItem, apEffect), eTargetBodyPart(static_cast<ActorValue::Index>(-1)) {}

ValueAndConditionsEffect::~ValueAndConditionsEffect() {}

// Allocates a copy, constructs it from this effect's caster, spell and effect item, then copies the effect data.
ActiveEffect* ValueAndConditionsEffect::Copy() {
    void* pMemory = ourMalloc(sizeof(ValueAndConditionsEffect));
    ValueAndConditionsEffect* pCopy;
    if (pMemory) {
        pCopy = ::new (pMemory) ValueAndConditionsEffect(pCaster, pSpell, pEffect);
    }
    else {
        pCopy = nullptr;
    }
    CopyData(pCopy);
    return pCopy;
}

// The base's size plus the four-byte target body part.
uint16_t ValueAndConditionsEffect::GetSaveSize(Actor* apActor) {
    return (uint16_t)(((int16_t)ValueModifierEffect::GetSaveSize(apActor)) + 4);
}

// The base's BGS data, then the target body part.
void ValueAndConditionsEffect::SaveGame(BGSSaveFormBuffer* apBuffer) {
    ValueModifierEffect::SaveGame(apBuffer);
    apBuffer->SaveDataEndian(&eTargetBodyPart, 4, 0);
}

// The old-format save: the base's data, then the four-byte target body part.
void ValueAndConditionsEffect::SaveGame(Actor* apActor) {
    ValueModifierEffect::SaveGame(apActor);
    pSaveLoadGameOLD->SaveGameDataOLD(&eTargetBodyPart, 4);
}

// Save form version 8 added the target body part.
void ValueAndConditionsEffect::LoadGame(BGSLoadFormBuffer* apBuffer) {
    ValueModifierEffect::LoadGame(apBuffer);
    if (apBuffer->GetVersion() >= 8) {
        apBuffer->LoadData(&eTargetBodyPart, 4);
    }
}

// The old-format save: the base's data, then the four-byte target body part.
void ValueAndConditionsEffect::LoadGame(Actor* apActor) {
    ValueModifierEffect::LoadGame(apActor);
    pSaveLoadGameOLD->LoadGameDataOLD(&eTargetBodyPart, 4);
}

void ValueAndConditionsEffect::CopyData(ActiveEffect* apDestination) {
    ValueModifierEffect::CopyData(apDestination);
    ValueAndConditionsEffect* v3 = dynamic_cast<ValueAndConditionsEffect*>(apDestination);
    if (v3) {
        v3->eTargetBodyPart = static_cast<ActorValue::Index>((int32_t)eTargetBodyPart);
    }
}

void ValueAndConditionsEffect::ModifyActorValue(
    Actor* apActor,
    float afModifier,
    ActorValue::Index aeOverrideActorValue
) {
    int32_t i;
    float v6;
    float v7;
    if (((int32_t)eTargetBodyPart) == (-1)) {
        ValueModifierEffect::ModifyActorValue(apActor, afModifier, static_cast<ActorValue::Index>(-1));
        v6 = afModifier * (&fMagicVACNoPartTargetedMult)->Float();
        for (i = 25; i < 32; ++i) {
            ValueModifierEffect::ModifyActorValue(apActor, v6, static_cast<ActorValue::Index>(i));
        }
    }
    else {
        v7 = afModifier * (&fMagicVACPartTargetedMult)->Float();
        ValueModifierEffect::ModifyActorValue(apActor, afModifier, eTargetBodyPart);
        ValueModifierEffect::ModifyActorValue(apActor, v7, static_cast<ActorValue::Index>(-1));
    }
}

void ValueAndConditionsEffect::SetTargetBodyPart(ActorValue::Index aeWhichBodyPart) {
    eTargetBodyPart = aeWhichBodyPart;
}

ActiveEffect* ValueAndConditionsEffect::Create(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect) {
    ValueAndConditionsEffect* v5 = static_cast<ValueAndConditionsEffect*>(ourMalloc(sizeof(ValueAndConditionsEffect)));
    if (v5) {
        return ::new (v5) ValueAndConditionsEffect(apCaster, apItem, apEffect);
    }

    return nullptr;
}
