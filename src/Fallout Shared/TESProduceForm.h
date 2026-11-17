#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout/Misc/Calendar.h"
#include "Reconstruction.hpp"

class IngredientItem;
class TESForm;

// The ingredient a plant produces and its chance per season.
class TESProduceForm : public BaseFormComponent {
public:
	TESProduceForm();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// Writes the ingredient's form ID and the four production chances.
	void Save();
	// Copies the four production chances from apiChances, when given.
	void SetAllProductionChances(const uint8_t* apiChances);
	// pFormIngredient; the exe's shared one-field setter (thiscall, ret 4, 0x6ECD40).
	void SetFormIngredient(IngredientItem* apIngr);
	void InitItem(TESForm* apOwner);
	uint8_t GetProductionChance(SEASON aSeason);

	IngredientItem* pFormIngredient;
	uint8_t iProduceChance[4];
};
inline constexpr fnvd::Vtable TESProduceForm_vftable{};
