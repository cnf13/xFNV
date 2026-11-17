#include "Fallout Shared/Magic/SpellItem.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicItemForm.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/SkillLevel.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/AI/Actor.h"
#include "SDK/MSVC/rtti.h"

void SpellItemData::Endian() {
    ENDIAN(iSpellType, false);
    ENDIAN(iCostOverride, false);
    ENDIAN(eLevelOverride, false);
}

SpellItem::SpellItem() : MagicItemForm() {
    SetFormType(20u);
    SpellItem::InitializeData();
}

SpellItem::~SpellItem() {
    SpellItem::ClearData();
}

void SpellItem::InitializeData() {
    data.iSpellType = MagicSystem::SPELL;
    data.iCostOverride = (uint32_t)((-1));
    data.eLevelOverride = SkillLevel::NOVICE;
    data.iFlags = (uint8_t)0;
}

void SpellItem::ClearData() {
    EffectItemList::RemoveAllEffects();
}

bool SpellItem::IsAutoCalc() {
    return !GetFlag(COST_OVERRIDE);
}

void SpellItem::SetAutoCalc(bool abValue) {
    SetFlag(COST_OVERRIDE, !abValue);
}

bool SpellItem::IgnoreResistance() {
    return SpellItem::GetFlag(0x20);
}

bool SpellItem::GetNoAbsorbOrReflect() {
    return SpellItem::GetFlag(0x40);
}

float SpellItem::GetCost(Actor* apActor) {
    float fcost = 0.0f;
    if (GetSpellType() == MagicSystem::POWER || GetSpellType() == MagicSystem::LESSER_POWER) {
        apActor = 0;
    }
    if (GetFlag(1u)) {
        fcost = static_cast<float>(data.iCostOverride);
        if (apActor) {
            int32_t ClampedActorValue = apActor->GetClampedActorValue(ActorValue::LUCK);
            ActorValue::Index AssociatedSkill = static_cast<EffectItemList*>(this)->GetAssociatedSkill();
            int32_t v3 = apActor->GetClampedActorValue(AssociatedSkill);
            fcost = MagicFormulas::AdjustMagicSpellCost(fcost, v3, ClampedActorValue);
        }
    }
    else {
        fcost = static_cast<EffectItemList*>(this)->EffectItemList::GetCost(apActor);
    }
    return fcost;
}

SkillLevel::Value SpellItem::GetLevel() {
    if (GetFlag(1u)) {
        return data.eLevelOverride;
    }
    float afSpellCost = static_cast<EffectItemList*>(this)->GetCost(0);
    return MagicFormulas::GetMagicSpellLevel(afSpellCost);
}

void SpellItem::Endian() {
    data.Endian();
}

CHUNK_ID SpellItem::GetChunkID() {
    return static_cast<CHUNK_ID>(1414090835);
}

uint32_t SpellItem::GetDataSize() {
    return sizeof(data);
}

bool SpellItem::CompareMagicItemData(MagicItem* apCompare) {
    SpellItem* pspell = dynamic_cast<SpellItem*>(apCompare);
    if (!pspell) {
        return 1;
    }
    MagicSystem::SpellType v3 = pspell->GetSpellType();
    bool result = true;
    if (v3 == static_cast<MagicItem*>(this)->GetSpellType()) {
        int32_t ConversationData = pspell->data.iCostOverride;
        if (ConversationData == data.iCostOverride) {
            SkillLevel::Value EnchantmentSource = pspell->data.eLevelOverride;
            if (EnchantmentSource == data.eLevelOverride && pspell->data.iFlags == data.iFlags) {
                return false;
            }
        }
    }
    return result;
}

void SpellItem::CopyMagicItemData(MagicItem* apCopy) {
    SpellItem* pspell = dynamic_cast<SpellItem*>(apCopy);
    if (pspell) {
        data.iSpellType = pspell->GetSpellType();
        data.iCostOverride = pspell->data.iCostOverride;
        data.eLevelOverride = pspell->data.eLevelOverride;
        data.iFlags = pspell->data.iFlags;
    }
}

// Writes the data chunk, byte swapped around the write when saves are swapped (the MagicItem vtable's slot).
void SpellItem::SaveMagicItemData() {
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::__AddChunkData(GetChunkID(), &data, sizeof(data));
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
}

bool SpellItem::GetFlag(uint32_t aiValue) {
    return ((aiValue & data.iFlags) != 0);
}

bool SpellItem::IsPCStartSpell() {
    return (bool)((uint8_t)GetFlag(4u));
}

// Sets or clears the aiFlag bits of the flags byte.
void SpellItem::SetFlag(SpellFlag aiFlag, bool abValue) {
    if (abValue) {
        data.iFlags = static_cast<uint8_t>(data.iFlags | aiFlag);
    }
    else {
        data.iFlags = static_cast<uint8_t>(data.iFlags & ~aiFlag);
    }
}
