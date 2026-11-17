#pragma once

#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

class TESForm;

// The weight of a form.
class TESWeightForm : public BaseFormComponent {
public:
	TESWeightForm();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// Same function as DecalData::GetDecalMaxWidth (0x6B9130): fWeight. Not in the PDB, and IDA has no name for it.
	float GetWeight() { return fWeight; }
	void SetFormWeight(float afWeight);

	// The weight of apForm's object, as hardcore mode counts it with abIsHardcore.
	static float GetFormWeight(TESForm* apForm, bool abIsHardcore);

	float fWeight;
};
inline constexpr fnvd::Vtable TESWeightForm_vftable{};
