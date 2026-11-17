#pragma once

#include "Fallout Shared/BaseFormComponent.h"

class MagicItemForm;
class TESForm;

// The spell cast by an actor's unarmed attack.
class BGSTouchSpellForm : public BaseFormComponent {
public:
	BGSTouchSpellForm();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// Resolves the touch spell loaded for apOwner.
	void InitItem(TESForm* apOwner);
	void SaveSubRecord();
	void SetFormTouchSpell(MagicItemForm* apEnch);
	// Stores iTouchSpellAnimation.
	void SetFormTouchSpellAnimation(uint16_t aiAnimGroup);
	MagicItemForm* GetFormTouchSpell();
	uint16_t GetFormTouchSpellAnimation();

	MagicItemForm* pFormTouchSpell;
	uint16_t iTouchSpellAnimation;
	uint8_t padding0A[2];
};
