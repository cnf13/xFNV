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
    data.iCostOverride = -1;
    data.eLevelOverride = SkillLevel::NOVICE;
    data.iFlags = 0;
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
    return SpellItem::GetFlag(IGNORE_RESISTANCE);
}

bool SpellItem::GetNoAbsorbOrReflect() {
    return SpellItem::GetFlag(NO_ABSORB_REFLECT);
}

float SpellItem::GetCost(Actor* apActor) {
    float fCost = 0.0f;
    if (GetSpellType() == MagicSystem::POWER || GetSpellType() == MagicSystem::LESSER_POWER) {
        apActor = nullptr;
    }
    if (GetFlag(COST_OVERRIDE)) {
        fCost = static_cast<float>(data.iCostOverride);
        if (apActor) {
            int32_t iLuck = apActor->GetClampedActorValue(ActorValue::LUCK);
            ActorValue::Index eSkill = static_cast<EffectItemList*>(this)->GetAssociatedSkill();
            int32_t iSkill = apActor->GetClampedActorValue(eSkill);
            fCost = MagicFormulas::AdjustMagicSpellCost(fCost, iSkill, iLuck);
        }
    }
    else {
        fCost = static_cast<EffectItemList*>(this)->EffectItemList::GetCost(apActor);
    }
    return fCost;
}

SkillLevel::Value SpellItem::GetLevel() {
    if (GetFlag(COST_OVERRIDE)) {
        return data.eLevelOverride;
    }
    float fSpellCost = static_cast<EffectItemList*>(this)->GetCost(nullptr);
    return MagicFormulas::GetMagicSpellLevel(fSpellCost);
}

void SpellItem::Endian() {
    data.Endian();
}

CHUNK_ID SpellItem::GetChunkID() {
    return SPIT_ID;
}

uint32_t SpellItem::GetDataSize() {
    return sizeof(data);
}

bool SpellItem::CompareMagicItemData(MagicItem* apCompare) {
    SpellItem* pSpell = dynamic_cast<SpellItem*>(apCompare);
    if (!pSpell) {
        return true;
    }
    MagicSystem::SpellType eSpellType = pSpell->GetSpellType();
    if (eSpellType == static_cast<MagicItem*>(this)->GetSpellType()) {
        if (pSpell->data.iCostOverride == data.iCostOverride) {
            if (pSpell->data.eLevelOverride == data.eLevelOverride && pSpell->data.iFlags == data.iFlags) {
                return false;
            }
        }
    }
    return true;
}

void SpellItem::CopyMagicItemData(MagicItem* apCopy) {
    SpellItem* pSpell = dynamic_cast<SpellItem*>(apCopy);
    if (pSpell) {
        data.iSpellType = pSpell->GetSpellType();
        data.iCostOverride = pSpell->data.iCostOverride;
        data.eLevelOverride = pSpell->data.eLevelOverride;
        data.iFlags = pSpell->data.iFlags;
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
    return (aiValue & data.iFlags) != 0;
}

bool SpellItem::IsPCStartSpell() {
    return GetFlag(PC_STARTSPELL);
}

// Sets or clears the aiFlag bits of the flags byte.
void SpellItem::SetFlag(SpellFlag aiFlag, bool abValue) {
    if (abValue) {
        data.iFlags |= aiFlag;
    }
    else {
        data.iFlags &= ~aiFlag;
    }
}
