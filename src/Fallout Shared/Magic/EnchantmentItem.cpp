#include "Fallout Shared/Magic/EnchantmentItem.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicItemForm.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Actor.h"
#include "SDK/MSVC/rtti.h"

void EnchantmentItemData::Endian() {
    BSEndian(eCastingType, false);
    BSEndian(iChargeOverride, false);
    BSEndian(iCostOverride, false);
}

EnchantmentItem::EnchantmentItem() : MagicItemForm() {
    SetFormType(ENCH_ID);
    EnchantmentItem::InitializeData();
}

EnchantmentItem::~EnchantmentItem() {
    EnchantmentItem::ClearData();
}

void EnchantmentItem::InitializeData() {
    data.iCostOverride = -1;
    data.iChargeOverride = -1;
    data.iFlags = 0;
    data.eCastingType = MagicSystem::WHEN_STRIKES;
}

void EnchantmentItem::ClearData() {
    EffectItemList::RemoveAllEffects();
}

bool EnchantmentItem::IsAutoCalc() {
    return !GetFlag(COST_OVERRIDE);
}

void EnchantmentItem::SetAutoCalc(bool abValue) {
    SetFlag(COST_OVERRIDE, !abValue);
}

float EnchantmentItem::GetCost(Actor* apActor) {
    if (GetFlag(COST_OVERRIDE)) {
        return static_cast<float>(data.iCostOverride);
    }

    return static_cast<EffectItemList*>(this)->EffectItemList::GetCost(nullptr);
}

void EnchantmentItem::Endian() {
    data.Endian();
}

CHUNK_ID EnchantmentItem::GetChunkID() {
    return ENIT_ID;
}

uint32_t EnchantmentItem::GetDataSize() {
    return sizeof(EnchantmentItemData);
}

bool EnchantmentItem::CompareMagicItemData(MagicItem* apCompare) {
    EnchantmentItem* pCompare = dynamic_cast<EnchantmentItem*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (pCompare->data.iChargeOverride == data.iChargeOverride) {
        if (pCompare->data.iCostOverride == data.iCostOverride) {
            MagicSystem::CastingType eCastingType = pCompare->GetCastingType();
            if (eCastingType == GetCastingType() && pCompare->data.iFlags == data.iFlags) {
                return false;
            }
        }
    }
    return true;
}

void EnchantmentItem::CopyMagicItemData(MagicItem* apCopy) {
    EnchantmentItem* pCopy = dynamic_cast<EnchantmentItem*>(apCopy);
    if (pCopy) {
        data.iChargeOverride = pCopy->data.iChargeOverride;
        data.iCostOverride = pCopy->data.iCostOverride;
        data.eCastingType = pCopy->GetCastingType();
        data.iFlags = pCopy->data.iFlags;
    }
}

// Writes the data chunk, byte swapped around the write when saves are swapped.
void EnchantmentItem::SaveMagicItemData() {
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::__AddChunkData(GetChunkID(), &data, sizeof(data));
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
}

bool EnchantmentItem::GetFlag(uint32_t aiValue) {
    return (aiValue & data.iFlags) != 0;
}

void EnchantmentItem::SetFlag(EnchantmentFlag aiFlag, bool abValue) {
    if (abValue) {
        data.iFlags |= aiFlag;
    }
    else {
        data.iFlags &= ~aiFlag;
    }
}

MagicSystem::CastingType EnchantmentItem::GetCastingType() {
    return data.eCastingType;
}

// The clothing case tests the biped form's stub before the associated-armour effect.
bool EnchantmentItem::IsValidEnchantForObject(TESForm* apOwner) {
    TESObject* pObject = dynamic_cast<TESObject*>(apOwner);
    if (!pObject) {
        return false;
    }
    switch (pObject->GetFormType()) {
        case ARMO_ID:
            return !EffectItemList::HasEffect(EffectSetting::USE_ASSOCIATED_ARMOR) &&
                GetCastingType() == MagicSystem::CONSTANT_EFFECT;
        case BOOK_ID:
            if (!static_cast<TESObjectBOOK*>(pObject)->IsScroll()) {
                return false;
            }
            return GetCastingType() == MagicSystem::CAST_ONCE;
        case CLOT_ID: {
            TESBipedModelForm* pBiped = dynamic_cast<TESBipedModelForm*>(apOwner);
            if (!pBiped) {
                return false;
            }
            if (!pBiped->IsJewelry() && EffectItemList::HasEffect(EffectSetting::USE_ASSOCIATED_ARMOR)) {
                return false;
            }
            return GetCastingType() == MagicSystem::CONSTANT_EFFECT;
        }
        case WEAP_ID:
            if (!dynamic_cast<TESObjectWEAP*>(pObject)) {
                return false;
            }
            return GetCastingType() == MagicSystem::WHEN_STRIKES;
        case AMMO_ID:
        case NPC__ID:
        case CREA_ID: return GetCastingType() == MagicSystem::WHEN_STRIKES;
        case EXPL_ID: return true;
        default: return false;
    }
}

bool EnchantmentItem::IsValidEffectForEnchantment(
    TESBoundObject* apEnchantableItem,
    ExtraDataList* apExtra,
    EffectSetting* apEffectSetting,
    bool abFromMenus
) {
    bool bResult;
    if (!apEnchantableItem || !apEffectSetting) {
        return false;
    }
    if (apEnchantableItem->GetFormType() == 43) {
        return true;
    }
    if (!dynamic_cast<TESEnchantableForm*>(apEnchantableItem)) {
        return false;
    }
    switch (apEnchantableItem->GetFormType()) {
        case 24u:
        case 26u: {
            if (apEffectSetting->IsFlagSet(EffectSetting::RANGE_SELF)) {
                if (apEffectSetting->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE)) {
                    bResult = false;
                }
                else if (apEffectSetting->IsFlagSet(EffectSetting::NO_DURATION)) {
                    bResult = false;
                }
                else if (apEffectSetting->IsFlagSet(EffectSetting::NO_MAGNITUDE)) {
                    bResult = false;
                }
                else if (apExtra && apExtra->GetCanNotWear()) {
                    bResult = false;
                }
                else if (dynamic_cast<TESBipedModelForm*>(apEnchantableItem)) {
                    bResult = !apEffectSetting->IsFlagSet(EffectSetting::USE_ASSOCIATED_ARMOR);
                }
                else {
                    bResult = false;
                }
            }
            else {
                bResult = false;
            }
            break;
        }
        case 40u:
        case 41u: {
            if (apEffectSetting->IsFlagSet(EffectSetting::RANGE_TOUCH)) {
                bResult = !apExtra || !apExtra->GetCanNotWear();
            }
            else {
                bResult = false;
            }
            break;
        }
        default: {
            bResult = true;
            break;
        }
    }
    return bResult;
}

MagicItemForm::MagicItemForm() : TESForm(), MagicItem() {}

MagicItemForm::~MagicItemForm() {}
