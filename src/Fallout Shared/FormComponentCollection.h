#pragma once

class BaseFormComponent;
class TESForm;

// The components of one form, gathered so that a call can be run on each of them.
class FormComponentCollection {
public:
	FormComponentCollection();

	void Destruct();

	// Gathers the components of apForm.
	void InitFormComponentArray(TESForm* apForm);

	// Runs InitializeDataComponent on every gathered component.
	void InitializeDataAllComponents();

	// Runs ClearDataComponent on every gathered component.
	void ClearDataAllComponents();

	// Whether every gathered component equals its counterpart in apFCC.
	bool CompareAllComponents(FormComponentCollection* apFCC);
	void ClearFormComponentArray();
	void CopyAllComponents(FormComponentCollection* apFCC);

	// 38 slots: ClearFormComponentArray clears all of them and InitFormComponentArray fills slots 0 to 37.
	BaseFormComponent* pFCArray[38];
};
