#pragma once

#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/TESCondition.h"
#include "Reconstruction.hpp"

class TESObjectREFR;

// A quest target's QSTA chunk as a file stores it: the target reference's 4-byte slot, then the flags byte. Not
// named in the PDB; name chosen here.
struct QSTA_DATA {
	// Byte-swaps the reference read from a big-endian file. Called out of line, as the exe does.
	void Endian();

	uint32_t uiTargetRef;
	uint8_t cFlags;
	uint8_t padding05[3];
};

// A reference a quest objective points the player at.
class TESQuestTarget {
public:
	TESQuestTarget();
	~TESQuestTarget();

	// The target reference; with abAllowPickUpActor, a deleted (picked-up) target resolves to
	// the reference its extra data points at.
	TESObjectREFR* GetReference(bool abAllowPickUpActor);
	bool GetIgnoreLocks();
	// Reads the target's QSTA chunk: the reference and the flags byte.
	void Load(TESFile* apFile);
	// Same function as TESQuestStage::GetStageItemList (0x717E50), declared there.
	TESCondition* GetConditionList() { return &objConditions; }
	// Resolves the target's form IDs for apOwner.
	void InitItem(TESForm* apOwner);
	// The reference the target's path leads to: the first door of m_TargetPath when it has
	// one, else m_pTargetRef.
	TESObjectREFR* QTrackingRef();
	// &m_TargetPath, through the exe's shared folded accessor that returns this + 0x10
	// (0x460140).
	TeleportPath* QTargetPath();

	// The exe's deleting destructor (0x5EC4D0) frees through ourFree.
	static void operator delete(void* apMemory);

	uint8_t cFlags;
	uint8_t padding01[3];
	TESCondition objConditions;
	TESObjectREFR* m_pTargetRef;
	TeleportPath m_TargetPath;
};
