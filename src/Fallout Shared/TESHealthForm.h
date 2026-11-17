#pragma once

#include "Fallout Shared/BaseFormComponent.h"

class TESForm;

// The base health of a form.
class TESHealthForm : public BaseFormComponent {
public:
	TESHealthForm();
	~TESHealthForm();

	void InitializeDataComponent() override;
	void ClearDataComponent() override {}
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	virtual uint32_t GetFormHealth() const { return iHealth; }

	// Sets the base health.
	void SetFormHealth(uint32_t aiHealth);

	// The form's base health, or 0 when it has no health component.
	static uint32_t GetFormHealth(TESForm* apForm);
	// The form's health component, or null when it has none.
	static TESHealthForm* GetFormAsHealthForm(TESForm* apForm);

	uint32_t iHealth;
};
