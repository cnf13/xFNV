#pragma once

#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESTexture.h"

class TESNPC;

// A hair form (HAIR).
class TESHair : public TESForm, public TESFullName, public TESModel, public TESTexture {
public:
	TESHair();
	~TESHair() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESForm::Save;

	bool IsPlayable();
	bool CanBeMale();
	bool CanBeFemale();
	bool CanBeHairForNPC(TESNPC* apNPC);

	uint8_t cFlags;
	uint8_t padding49[3];
};
