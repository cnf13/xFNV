#pragma once

#include "Fallout Shared/BGSPerkEntry.h"
#include "Reconstruction.hpp"

class TESQuest;

// The DATA chunk of a quest perk entry (8 bytes, padding included): the quest and the stage to set it to.
struct QuestPerkEntryData {
	// Swaps the first dword.
	void Endian();

	TESQuest* pQuest;
	uint8_t cQuestStage;
	uint8_t padding05[3];
};

// A perk entry that sets a quest to a stage when the perk is applied.
class BGSQuestPerkEntry : public BGSPerkEntry {
public:
	BGSQuestPerkEntry();
	~BGSQuestPerkEntry() override;

	PERK_ENTRY_TYPE GetType() override;
	void InitItem(TESForm* apOwner) override;
	BGSPerkEntry* Duplicate() override;
	bool Compare(BGSPerkEntry* apCompare) override;
	void Save() override;
	bool Load(TESFile* apFile) override;
	void ApplyPerkEntry(Actor* apActor, bool bAlt) override;
	void RemovePerkEntry(Actor* apActor, bool bAlt) override {}

	QuestPerkEntryData Data;
};
inline constexpr fnvd::Vtable BGSQuestPerkEntry_vftable{};
