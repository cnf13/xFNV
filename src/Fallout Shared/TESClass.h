#pragma once

#include "Fallout Shared/TESAttributes.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESTexture.h"
#include "Reconstruction.hpp"

// The class record's data (DATA): the tag skills, flags, services and trainer data.
struct CLASS_DATA {
	// Swaps the service flags and the tag skills read from a big-endian file.
	void Endian();

	int32_t eTagSkills[4];
	uint8_t cClassFlags;
	uint8_t padding11[3];
	uint32_t iServiceFlags;
	int8_t cTrainingSkill;
	uint8_t cTrainingLevel;
};

// The class data as older files store it, with three tag skills.
struct OLD_CLASS_DATA {
	// Swaps the service flags and the tag skills read from a big-endian file.
	void Endian();

	int32_t eTagSkills[3];
	uint8_t cClassFlags;
	uint8_t padding0D[3];
	uint32_t iServiceFlags;
	int8_t cTrainingSkill;
	uint8_t cTrainingLevel;
};

// An NPC class form (CLAS).
class TESClass : public TESForm, public TESFullName, public TESDescription, public TESTexture, public TESAttributes {
public:
	TESClass();
	~TESClass() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESForm::LoadGame;
	using TESForm::Revert;
	using TESForm::Save;
	using TESForm::SaveGame;

	// Sets the skill the class trains.
	void SetTrainingSkill(ActorValue::Index aeSkill);
	// Whether aiSkill is one of the class's four tag skills.
	bool IsTagSkill(ActorValue::Index aiSkill);
	// Whether the class is flagged guard (flag 2 of the class flags; thiscall, ret 0,
	// 0x5F6E60).
	bool IsGuard();
	void SetMajorSkill(int32_t aiIndex, ActorValue::Index aeSkill);
	ActorValue::Index GetTrainingSkill();
	bool CheckData();
	void SaveGameTES();
	// Reads the class data and the full name and icon strings back
	// from the old-format save. Hides TESAttributes::LoadGameTES, which no src code calls through
	// a TESClass.
	void LoadGameTES();
	int16_t GetSaveSize();

	static int32_t GetTagSkillCount();

	CLASS_DATA data;
};
inline constexpr fnvd::Vtable TESClass_vftable{};
inline constexpr fnvd::Vtable TESClass_vftable_TESAttributes{};
inline constexpr fnvd::Vtable TESClass_vftable_TESDescription{};
inline constexpr fnvd::Vtable TESClass_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESClass_vftable_TESTexture{};
