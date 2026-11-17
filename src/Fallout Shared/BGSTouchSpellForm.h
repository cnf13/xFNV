#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

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
	// Stores pFormTouchSpell; the exe's shared one-field setter (thiscall, ret 4, 0x6ECD40).
	void SetFormTouchSpell(MagicItemForm* apEnch);
	// Stores iTouchSpellAnimation; the exe's shared 2-byte setter of the field at +8 (thiscall,
	// ret 4, 0x483170).
	void SetFormTouchSpellAnimation(uint16_t aiAnimGroup);
	// Returns pFormTouchSpell; the exe's shared one-field getter of +4 (thiscall, ret 0,
	// 0x726070).
	MagicItemForm* GetFormTouchSpell();
	// Returns iTouchSpellAnimation; the exe's shared 2-byte getter of +8 (thiscall, ret 0,
	// 0x4A8AE0).
	uint16_t GetFormTouchSpellAnimation();

	MagicItemForm* pFormTouchSpell;
	uint16_t iTouchSpellAnimation;
	uint8_t padding0A[2];
};
inline constexpr fnvd::Vtable BGSTouchSpellForm_vftable{};
