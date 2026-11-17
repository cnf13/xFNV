#pragma once

#include "Reconstruction.hpp"

class BaseFormComponent;
class TESForm;

// The components of one form, gathered so that a call can be run on each of them.
class FormComponentCollection {
public:
	FormComponentCollection();

	// The exe's destructor: the engine's shared empty thiscall function (0x483710).
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

	// 38 slots: ClearFormComponentArray (0x47C540) clears 0x98 bytes, InitFormComponentArray (0x47C070) fills
	// slots 0 to 37, and TESForm::CompareAllComponents keeps two collections 0x98 bytes apart ([ebp-0x13C],
	// [ebp-0xA4]).
	BaseFormComponent* pFCArray[38];
};
