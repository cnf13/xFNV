#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/TESForm.h"

// A global variable form (GLOB).
class TESGlobal : public TESForm {
public:
	TESGlobal();
	~TESGlobal() override;

	bool Load(TESFile* apFile) override;
	void Save() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	const char* GetFormEditorID() override;
	bool SetFormEditorID(const char* apEditorID) override;

	using TESForm::Save;

	// The global's type character (cType).
	char GetType();
	void SetValue(float afValue);

	float GetValue();

	BSStringT<char> strEditorID;
	char cType;
	uint8_t padding21[3];

	// The value and the reference ID share the same storage.
	union {
		float fValue;
		uint32_t uRefID;
	};
};
