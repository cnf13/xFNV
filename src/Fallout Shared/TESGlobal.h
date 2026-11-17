#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/TESForm.h"
#include "Reconstruction.hpp"

// A global variable form (GLOB).
class TESGlobal : public TESForm {
public:
	TESGlobal();
	~TESGlobal() override;

	bool Load(TESFile* apFile) override;
	void Save() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	// the exe shares this body with TESResponse::GetText (0x616DE0)
	const char* GetFormEditorID() override;
	bool SetFormEditorID(const char* apEditorID) override;

	using TESForm::Save;

	// The global's type character (cType). Copy calls it out of line.
	char GetType();
	// thiscall at 0x46DCE0.
	void SetValue(float afValue);

	// The value, in ST(0); thiscall at 0x526AC0, a float getter the exe shares with
	// BSMultiBoundCapsule::QMultiBoundRadius.
	float GetValue();

	BSStringT<char> strEditorID;
	char cType;
	uint8_t padding21[3];

	// The exe allocates 0x28 bytes (TESDataHandler::CreateFormFromID): the value and the reference ID share 0x24
	// (IDA's union).
	union {
		float fValue;
		uint32_t uRefID;
	};
};
inline constexpr fnvd::Vtable TESGlobal_vftable{};
