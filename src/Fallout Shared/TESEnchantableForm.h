#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Reconstruction.hpp"

class EnchantmentItem;

// The enchantment a form carries, with its charge and casting type.
class TESEnchantableForm : public BaseFormComponent {
public:
	TESEnchantableForm();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	virtual MagicSystem::CastingType GetCastingType() { return eCastingType; }

	// Resolves the enchantment's form ID for apForm.
	void InitItem(TESForm* apOwner);
	// Stores eCastingType.
	void SetCastingType(MagicSystem::CastingType aeCastingType) { eCastingType = aeCastingType; }
	// Writes the enchantment's EITM chunk and the amount's EAMT chunk; the exe shares the code with the touch
	// spell's (both components keep a form at +4 and a word at +8).
	void SaveSubRecord();
	// Stores iAmountofEnchantment from an ANAM/EAMT chunk.
	void SetFormEnchantmentAmount(uint16_t aiAmount);
	// The enchantment (pFormEnchanting); the exe's shared one-field getter 0x726070
	// (thiscall, ret 0).
	EnchantmentItem* GetFormEnchanting();
	// Stores pFormEnchanting; the exe's shared one-field setter (thiscall, ret 4, 0x6ECD40).
	void SetFormEnchanting(EnchantmentItem* apEnch);

	// The enchantment of apForm when it is enchantable, else null.
	static EnchantmentItem* GetFormEnchanting(TESForm* apForm);

	EnchantmentItem* pFormEnchanting;
	uint16_t iAmountofEnchantment;
	uint8_t padding0A[2];
	MagicSystem::CastingType eCastingType;
};
inline constexpr fnvd::Vtable TESEnchantableForm_vftable{};
