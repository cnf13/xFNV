#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

class TESForm;
class TESRace;

// The race of an NPC.
class TESRaceForm : public BaseFormComponent {
public:
	TESRaceForm();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// The shared one-field getter at 0x726070.
	TESRace* GetFormRace() const;
	// Resolves the loaded race form ID for apOwner.
	void InitItem(TESForm* apOwner);
	// The shared one-field setter at 0x6ECD40 (thiscall, ret 4).
	void SetFormRace(TESRace* apRace);
	void SaveSubRecord();

	// The race of apForm when it has a race component, else null.
	static TESRace* GetFormRace(TESForm* apForm);

	TESRace* pFormRace;
};
inline constexpr fnvd::Vtable TESRaceForm_vftable{};
