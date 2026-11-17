#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Reconstruction.hpp"

class TESFile;
class TESForm;

// The display name of a form.
class TESFullName : public BaseFormComponent {
public:
	TESFullName();
	~TESFullName();

	void InitializeDataComponent() override;
	// The engine's shared empty thiscall function (0x483710), which the exe calls.
	void ClearDataComponent() override;
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	// Saves the name as a FULL chunk when it has one.
	void Save();
	// thiscall at 0x489100.
	void SetFullName(const char* apFullName);

	const char* GetFullName();
	uint32_t GetFullNameLength();

	// apObj's name, or the shared empty string when it has none.
	static const char* GetFullName(TESForm* apObj);

	// Reads the FULL chunk of apFile into apFullName.
	static void LoadFullNameChunk(TESFullName* apFullName, TESFile* apFile);

	BSStringT<char> cFullName;
};
inline constexpr fnvd::Vtable TESFullName_vftable{};
