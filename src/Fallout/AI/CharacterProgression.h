#pragma once

#include "Fallout Shared/ActorValue.h"
#include "Reconstruction.hpp"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;

// The player's level-up state: whether a level-up is due, the experience needed for the next one, and whether the
// callback that announces it is suppressed.
struct CharacterProgressionData {
	CharacterProgressionData();

	void Initialize();
	// Whether the progression is ready to reach level aiLevel.
	bool CheckCanLevelUp(int32_t aiLevel);
	// Writes the ready flag and the next threshold to the save.
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	// Reads the ready flag and the next threshold from the save.
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// Clears the ready flag and sets the next threshold to level 2's (thiscall, ret 4, 0x8D5720). The exe ignores
	// apLoadGameBuffer.
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer);

	bool bReadyToLevelUp;
	uint8_t padding01[3];
	int32_t iNextThreshold;
	bool bIgnoreCallback;
	uint8_t padding09[3];
};

// The player's level-up progression data.
class CharacterProgression {
public:
	CharacterProgression();

	// Resets the progression data after the data files are loaded.
	void InitializeData();
	char GetReadyToLevelUp();
	void DeleteData();
	// Adds aiAmount experience, capped at the maximum level.
	void RewardExperience(int32_t aiAmount);
	// The exe's shared empty stub (thiscall, ret 8), which PlayerCharacter::UseSkill calls out of line. aiAction is a
	// SKILL_ACTION.
	void UseSkill(ActorValue::Index aiSkill, uint32_t aiAction) {}
	// The exe's shared empty stub (thiscall, ret 8), which PlayerCharacter::FinishInitLoadGame calls out of line.
	void FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {}
	// pData; the exe calls the shared one-field getter 0x559450 (NiPointer::operator->).
	CharacterProgressionData* GetData();
	// Plays the level-up sound, raises the player's level and opens the level-up menu, once a level-up is due.
	void BeginLevelUp();
	// The old-format save: writes the progression data's ready flag and next threshold through pPlayer; the exe
	// ignores the flags argument (thiscall, ret 4, 0x8D5510).
	void SaveGame(uint32_t aiFlags);
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	void AllocateData();
	// Reads the ready flag and the next threshold from the old-format save.
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags);
	// Reverts the progression data.
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer);
	void FinishLevelUp();
	void ForceLevel(int32_t aiLevel);

	// a1: its one caller, PlayerCharacter::GetSaveSize, passes
	// its uint32_t save flags.
	static int32_t GetSaveSize(uint32_t a1);

	CharacterProgressionData* pData;
};
