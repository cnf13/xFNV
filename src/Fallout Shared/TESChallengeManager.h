#pragma once

#include "BSCore/BSSimpleList.h"
#include "Reconstruction.hpp"

class TESChallenge;
class TESForm;

// The kinds of challenge (values from the exe's debug info, names from the PDB).
enum CHALLENGE_TYPE : int32_t {
	CHALLENGE_TYPE_KILL_LIST = 0x0,
	CHALLENGE_TYPE_KILL_SPECIFIC = 0x1,
	CHALLENGE_TYPE_KILL_CATEGORY = 0x2,
	CHALLENGE_TYPE_HIT = 0x3,
	CHALLENGE_TYPE_DISCOVER = 0x4,
	CHALLENGE_TYPE_USE_ITEM = 0x5,
	CHALLENGE_TYPE_AQUIRE = 0x6,
	CHALLENGE_TYPE_SKILL = 0x7,
	CHALLENGE_TYPE_DAMAGE = 0x8,
	CHALLENGE_TYPE_USE_ITEM_LIST = 0x9,
	CHALLENGE_TYPE_AQUIRE_LIST = 0xA,
	CHALLENGE_TYPE_MISC_STAT = 0xB,
	// The PC's crafting challenge: RecipeMenu::ItemSelectCallback
	// passes 0xC to UpdateChallenge.
	CHALLENGE_TYPE_CRAFT = 0xC,
	CHALLENGE_TYPE_SCRIPTED = 0xD,
	CHALLENGE_TYPE_COUNT = 0xE,
};

// Keeps the challenges the player can still earn (and those that are done or locked) in one list per challenge type.
// The PDB's source module is TESChallenge.cpp.
class TESChallengeManager {
public:
	using ChallengeListTable = BSSimpleList<TESChallenge*>[CHALLENGE_TYPE_COUNT];

	// Updates the challenges of kind aeType for the forms and special data given.
	static bool UpdateChallenge(CHALLENGE_TYPE cType, int32_t iCount, TESForm* pFormOne, TESForm* pFormTwo,
		uint16_t nSpecialDataOne, uint16_t nSpecialDataTwo, uint16_t nSpecialDataThree);
	// Empties both challenge lists of every challenge type.
	static void UnloadChallenges();
	static void LoadChallenges();
	static bool UnlockChallenge(TESChallenge* pChallenge);
	// Not named in the PDB; IDA's name (TESChallenge::GetProgressValue) is wrong: the exe's one caller,
	// Cmd_IncrementScriptedChallenge, passes the challenge and the count (cdecl, ret 0, 0x5F6110). Adds aiCount to
	// each earnable scripted challenge with apChallenge's form ID; true always.
	static bool IncrementScriptedChallenge(TESChallenge* apChallenge, int32_t aiCount);

	// IDA's kInactiveChallengeList: the completed challenges and those still locked, by challenge type.
	static ChallengeListTable pInactiveChallengeList;
	// IDA's kChallengeList: the challenges that can be earned now, by challenge type.
	static ChallengeListTable pChallengeList;
};
