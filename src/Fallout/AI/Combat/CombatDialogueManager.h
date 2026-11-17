#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/TESTopic.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class CombatController;
class TESObjectREFR;

// Starts the combat dialogue (taunts, warnings and the like) of the actors in combat. Only the members the
// reconstructed functions use are declared.
class CombatDialogueManager {
public:
	// Has apCombatController's attacker say the combat topic to its target. PDB: int aiDialogueType; the exe passes
	// DIALOGUE_TYPE values on to the speaker's dialogue (0x61A2D0). The topic stays the PDB's int: its enum depends
	// on the type (DIALOGUE_COMBAT for DT_COMBAT, the detection topics for DT_DETECTION).
	bool StartDialogue(CombatController* apCombatController, DIALOGUE_TYPE aeDialogueType, int32_t aiDialogueTopic,
		bool abInterrupt);
	// Has apSpeaker say the combat topic to apTarget (thiscall, ret 0x18, 0x9839B0). PDB: int aiDialogueType; the
	// exe passes it on as DIALOGUE_TYPE (0x61A2D0).
	bool StartDialogue(TESObjectREFR* apSpeaker, TESObjectREFR* apTarget, DIALOGUE_TYPE aeDialogueType,
		int32_t aiDialogueTopic, bool abInterrupt, CombatController* apCombatController);

	// Allocates the singleton once (the class has no data; the exe allocates 1 byte and runs no constructor).
	static void Create();
	// Frees the singleton (IDA's Free).
	static void Destroy();

	static inline int32_t uiLastCombatAttackSoundTime;
};

inline CombatDialogueManager* pCombatDialogueManager;

// The game settings CombatDialogueManager.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bDisableCombatDialogue;
