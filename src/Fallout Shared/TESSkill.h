#pragma once

#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESTexture.h"

// A skill's specialization (SKILL_DATA::eSpecialization).
enum SPECIALIZATION : int32_t {
	SPECIALIZATION_NONE = -1,
	SPECIALIZATION_COMBAT = 0,
	SPECIALIZATION_MAGIC = 1,
	SPECIALIZATION_STEALTH = 2,
	SPECIALIZATION_COUNT = 3,
};
// A skill's DATA chunk: the actor value it raises, its governing attribute, its specialization and the two use
// values.
struct SKILL_DATA {
	// Byte-swaps the record read from a big-endian file.
	void Endian();

	ActorValue::Index eIndex;
	ActorValue::Index eAttribute;
	SPECIALIZATION eSpecialization;
	float fActionValue[2];
};

// A skill (SKIL): the actor value it raises, its governing attribute and the use values.
class TESSkill : public TESForm, public TESDescription, public TESTexture {
public:
	TESSkill();
	~TESSkill() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	// Restores the skill's default data before it is loaded again.
	void ResetData();

	SKILL_DATA kData;
	TESDescription kDescriptions[4];
};
