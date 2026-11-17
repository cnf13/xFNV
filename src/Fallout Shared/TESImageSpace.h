#pragma once

#include "Fallout Shared/TESForm.h"
#include "BSShader/ImageSpaceManager.h"
#include "Reconstruction.hpp"

// An image space form (IMGS).
class TESImageSpace : public TESForm {
public:
	TESImageSpace();
	~TESImageSpace() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	// Resets the image space to the engine's defaults; thiscall at 0x52B780.
	void MakeDefault();

	ImageSpaceParameterData Data;
};
inline constexpr fnvd::Vtable TESImageSpace_vftable{};
