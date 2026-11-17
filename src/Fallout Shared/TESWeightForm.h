#pragma once

#include "Fallout Shared/BaseFormComponent.h"

class TESForm;

// The weight of a form.
class TESWeightForm : public BaseFormComponent {
public:
	TESWeightForm();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	float GetWeight() { return fWeight; }
	void SetFormWeight(float afWeight);

	// The weight of apForm's object, as hardcore mode counts it with abIsHardcore.
	static float GetFormWeight(TESForm* apForm, bool abIsHardcore);

	float fWeight;
};
