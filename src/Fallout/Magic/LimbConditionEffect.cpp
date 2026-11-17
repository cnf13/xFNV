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
static char bRegistered =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::LIMB_CONDITION, &LimbConditionEffect::Create);

LimbConditionEffect::LimbConditionEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apSpell, apEffect), eTargetBodyPart(static_cast<ActorValue::Index>(-1)) {}

LimbConditionEffect::~LimbConditionEffect() {}

// Copies the effect: allocates a new one, constructs it from this effect's caster, spell and effect item, then copies
// the shared data into it.
ActiveEffect* LimbConditionEffect::Copy() {
    LimbConditionEffect* pCopy = static_cast<LimbConditionEffect*>(ourMalloc(sizeof(LimbConditionEffect)));
    if (pCopy) {
        pCopy = ::new (pCopy) LimbConditionEffect(pCaster, pSpell, pEffect);
    }
    else {
        pCopy = nullptr;
    }
    static_cast<ActiveEffect*>(this)->CopyData(pCopy);
    return pCopy;
}

uint16_t LimbConditionEffect::GetSaveSize(Actor* apActor) {
    return (uint16_t)(((int16_t)ValueModifierEffect::GetSaveSize(apActor)) + 4);
}

// The base's BGS data, then the target body part.
void LimbConditionEffect::SaveGame(BGSSaveFormBuffer* apBuffer) {
    ActiveEffect::SaveGame(apBuffer);
    apBuffer->SaveDataEndian(reinterpret_cast<char*>(&eTargetBodyPart), 4, 0);
}

// The old-format save: the base's data, then the four-byte target body part.
void LimbConditionEffect::SaveGame(Actor* apActor) {
    ValueModifierEffect::SaveGame(apActor);
    pSaveLoadGameOLD->SaveGameDataOLD(&eTargetBodyPart, 4);
}

// Save form version 8 added the target body part.
void LimbConditionEffect::LoadGame(BGSLoadFormBuffer* apBuffer) {
    ActiveEffect::LoadGame(apBuffer);
    if (apBuffer->GetVersion() >= 8) {
        apBuffer->LoadData(reinterpret_cast<char*>(&eTargetBodyPart), 4);
    }
}

// The old-format save: the base's data, then the four-byte target body part.
void LimbConditionEffect::LoadGame(Actor* apActor) {
    ValueModifierEffect::LoadGame(apActor);
    pSaveLoadGameOLD->LoadGameDataOLD(&eTargetBodyPart, 4);
}

void LimbConditionEffect::CopyData(ActiveEffect* apDestination) {
    ValueModifierEffect::CopyData(apDestination);
    LimbConditionEffect* result = dynamic_cast<LimbConditionEffect*>(apDestination);
    LimbConditionEffect* v4 = result;
    if (result) {
        v4->eTargetBodyPart = eTargetBodyPart;
        return;
    }
}

void LimbConditionEffect::ModifyActorValue(Actor* apActor, float afModifier, ActorValue::Index aeOverrideActorValue) {
    int32_t i;
    float v6;
    float v7;
    if (eTargetBodyPart == ((-1))) {
        v6 = afModifier * ((&fMagicVACNoPartTargetedMult)->Float());
        for (i = 25; i < 32; ++i) {
            ValueModifierEffect::ModifyActorValue(apActor, v6, static_cast<ActorValue::Index>(i));
        }
    }
    else {
        v7 = afModifier * ((&fMagicVACPartTargetedMult)->Float());
        ValueModifierEffect::ModifyActorValue(apActor, afModifier, static_cast<ActorValue::Index>(eTargetBodyPart));
    }
}

void LimbConditionEffect::SetTargetBodyPart(uint32_t aeWhichBodyPart) {
    eTargetBodyPart = static_cast<ActorValue::Index>(aeWhichBodyPart);
}

ActiveEffect* LimbConditionEffect::Create(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    void* v5 = ourMalloc(sizeof(LimbConditionEffect));
    if (v5) {
        return ::new (v5) LimbConditionEffect(apCaster, apSpell, apEffect);
    }
    else {
        return nullptr;
    }
}
