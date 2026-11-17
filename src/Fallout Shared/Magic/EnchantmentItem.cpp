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
	ENDIAN(eCastingType, false);
	ENDIAN(iChargeOverride, false);
	ENDIAN(iCostOverride, false);
}

EnchantmentItem::EnchantmentItem() : MagicItemForm() {
	SetFormType(ENCH_ID);
	EnchantmentItem::InitializeData();
}

EnchantmentItem::~EnchantmentItem() {
	EnchantmentItem::ClearData();
}

void EnchantmentItem::InitializeData() {
	data.iCostOverride = (-1);
	data.iChargeOverride = (-1);
	data.iFlags = (uint8_t)0;
	data.eCastingType = MagicSystem::WHEN_STRIKES;
}

void EnchantmentItem::ClearData() {
	EffectItemList::RemoveAllEffects();
}

bool EnchantmentItem::IsAutoCalc() {
	return !GetFlag(1u);
}

void EnchantmentItem::SetAutoCalc(bool abValue) {
	SetFlag(COST_OVERRIDE, !abValue);
}

float EnchantmentItem::GetCost(Actor* apActor) {
	if (GetFlag(1u)) {
		return static_cast<float>(data.iCostOverride);
	} else {
		return static_cast<EffectItemList*>(this)->EffectItemList::GetCost(0);
	}
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
	EnchantmentItem* v7 = dynamic_cast<EnchantmentItem*>(apCompare);
	if (!v7) {
		return 1;
	}
	int32_t ConversationData = v7->data.iChargeOverride;
	bool result = true;
	if (ConversationData == data.iChargeOverride) {
		int32_t EnchantmentSource = v7->data.iCostOverride;
		if (EnchantmentSource == data.iCostOverride) {
			MagicSystem::CastingType CastingType = v7->GetCastingType();
			if (CastingType == GetCastingType() && v7->data.iFlags == data.iFlags) {
				return false;
			}
		}
	}
	return result;
}

void EnchantmentItem::CopyMagicItemData(MagicItem* apCopy) {
	EnchantmentItem* v3 = dynamic_cast<EnchantmentItem*>(apCopy);
	if (v3) {
		data.iChargeOverride = v3->data.iChargeOverride;
		data.iCostOverride = v3->data.iCostOverride;
		data.eCastingType = v3->GetCastingType();
		data.iFlags = v3->data.iFlags;
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
		data.iFlags = static_cast<uint8_t>(data.iFlags | aiFlag);
	} else {
		data.iFlags = static_cast<uint8_t>(data.iFlags & ~aiFlag);
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
	case CREA_ID:
		return GetCastingType() == MagicSystem::WHEN_STRIKES;
	case EXPL_ID:
		return true;
	default:
		return false;
	}
}

bool EnchantmentItem::IsValidEffectForEnchantment(TESBoundObject* apEnchantableItem, ExtraDataList* apExtra,
	EffectSetting* apEffectSetting, bool abFromMenus) {
	bool result;
	if (!apEnchantableItem || !apEffectSetting) {
		return 0;
	}
	if (apEnchantableItem->GetFormType() == 43) {
		return 1;
	}
	if (!dynamic_cast<TESEnchantableForm*>(apEnchantableItem)) {
		return 0;
	}
	switch (apEnchantableItem->GetFormType()) {
	case 24u:
	case 26u: {
		if (apEffectSetting->IsFlagSet(EffectSetting::RANGE_SELF)) {
			if (apEffectSetting->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE)) {
				result = 0;
			} else {
				if (apEffectSetting->IsFlagSet(EffectSetting::NO_DURATION)) {
					result = 0;
				} else {
					if (apEffectSetting->IsFlagSet(EffectSetting::NO_MAGNITUDE)) {
						result = 0;
					} else {
						if (apExtra && apExtra->GetCanNotWear()) {
							result = 0;
						} else {
							if (dynamic_cast<TESBipedModelForm*>(apEnchantableItem)) {
								if (!apEffectSetting->IsFlagSet(EffectSetting::USE_ASSOCIATED_ARMOR)) {
									goto LABEL_30;
								}
								result = 0;
							} else {
								result = 0;
							}
						}
					}
				}
			}
		} else {
			result = 0;
		}
		break;
	}
	case 40u:
	case 41u: {
		if (apEffectSetting->IsFlagSet(EffectSetting::RANGE_TOUCH)) {
			if (!apExtra || !apExtra->GetCanNotWear()) {
				goto LABEL_30;
			}
			result = 0;
		} else {
			result = 0;
		}
		break;
	}
	default: {
	LABEL_30:;
		result = 1;
		break;
	}
	}
	return result;
}

MagicItemForm::MagicItemForm() : TESForm(), MagicItem() {}

MagicItemForm::~MagicItemForm() {}
