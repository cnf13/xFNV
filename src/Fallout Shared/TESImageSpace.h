#pragma once

#include "BSShader/ImageSpaceManager.h"
#include "Fallout Shared/TESForm.h"

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

	// Resets the image space to the engine's defaults.
	void MakeDefault();

	ImageSpaceParameterData Data;
};
