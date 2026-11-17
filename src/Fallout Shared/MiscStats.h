#pragma once

#include "BSMain/Setting.h"
#include "Fallout/Interface/ListBox.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSLoadGameBuffer;
class BGSSaveGameBuffer;

// One statistic: the setting holding its display name and its value.
class MiscStat {
public:
	MiscStat(const char* apStatName, uint32_t aiIndex, int32_t aiVal);
	~MiscStat();

	// The exe's deleting destructor (0x4D5DD0) frees through ourFree.
	static void operator delete(void* apMemory);

	SettingT<GameSettingCollection>* pDisplayName;
	int32_t iVal;
};

// The player's miscellaneous statistics shown on the pip-boy's general stats page.
class MiscStatManager {
public:
	enum MiscStatID : int32_t {
		MS_QUESTS_COMPLETED = 0,
		MS_LOCATIONS_DISCOVERED = 1,
		MS_HUMANS_KILLED = 2,
		MS_CREATURES_KILLED = 3,
		MS_LOCKS_PICKED = 4,
		MS_COMPUTERS_HACKED = 5,
		MS_STIMPAKS_TAKEN = 6,
		MS_RADX_TAKEN = 7,
		MS_RADAWAY_TAKEN = 8,
		MS_CHEMS_TAKEN = 9,
		MS_ADDICTIONS = 10,
		MS_MINES_DISARMED = 11,
		MS_SPEECH_SUCCESSES = 12,
		MS_POCKETS_PICKED = 13,
		MS_PANTS_EXPLODED = 14,
		MS_BOOKS_READ = 15,
		MS_STIMPAK_HEALTH_RESTORED = 16,
		MS_WEAPONS_CONSTRUCTED = 17,
		MS_FOOD_HEALTH_RESTORED = 18,
		MS_WATER_CONSUMED = 19,
		MS_SANDMAN_KILLS = 20,
		MS_PARALYZING_PUNCHES = 21,
		MS_ROBOTS_DISABLED = 22,
		MS_TIMES_SLEPT = 23,
		MS_CORPSES_EATEN = 24,
		MS_STRANGER_ASSISTS = 25,
		MS_DOCTORBAG_USED = 26,
		MS_CHALLENGES_COMPLETED = 27,
		MS_MISS_FORTUNE = 28,
		MS_DISINTEGRATIONS = 29,
		MS_HAVE_LIMBS_CRIPPLED = 30,
		MS_SPEECH_FAILURES = 31,
		MS_ITEMS_CRAFTED = 32,
		MS_WEAPONS_MODDED = 33,
		MS_ITEMS_REPAIRED = 34,
		MS_TOTAL_THINGS_KILLED = 35,
		MS_DISMEMBERMENTS = 36,
		MS_CARAVAN_GAMES_WON = 37,
		MS_CARAVAN_GAMES_LOST = 38,
		MS_BARTER_AMOUNT_TRADED = 39,
		MS_ROULETTE_GAMES_PLAYED = 40,
		MS_BLACKJACK_GAMES_PLAYED = 41,
		MS_SLOTS_GAMES_PLAYED = 42,
		MS_EQUIPMENT_DAMAGE_REPAIRED = 43,
		MS_COUNT = 44,
	};

	// The statistics, one per MiscStatID below MS_EQUIPMENT_DAMAGE_REPAIRED: the exe holds 43 (0x2B) of them (the
	// constructor builds 43, Revert clears 0xAC bytes) and the per-TU statics of MiscStats.cpp start at +0xAC.
	using StatListArray = MiscStat* [MS_EQUIPMENT_DAMAGE_REPAIRED];
	// The statistics' names, one per MiscStatID below MS_EQUIPMENT_DAMAGE_REPAIRED: 43 pointers up to the per-TU
	// copies of _bad_alloc_Message at 0x118932C.
	using StatNameArray = const char* [MS_EQUIPMENT_DAMAGE_REPAIRED];

	// The constructor (the PDB's MiscStatManager()): unless StatList[0] is set, builds the
	// first 0x2B statistics, each a new MiscStat of its StatNames entry (0x1189280), its index and 0 (thiscall, ret 0,
	// 0x4D5C80; the static initializer 0xF3F660 runs it on the global at 0x11C6E04).
	MiscStatManager();

	static const char* GetDisplayText(MiscStatID aeID);
	static int32_t GetVal(MiscStatID aeID);
	static void Increment(MiscStatID aeID);
	static void ModVal(MiscStatID aeID, int32_t aiVal);
	static void Decrement(MiscStatManager::MiscStatID aeID);
	// Saves the statistic count, then every value (0 for a missing statistic).
	static void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	// Reads what SaveGame writes; saves before version 0xB carry no count and hold 21 statistics.
	static void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	// Zeroes every statistic on a revert (IDA's RevertGlobalData::MiscStats).
	static void Revert(BGSLoadGameBuffer* apLoadGameBuffer);
	// The destructor (the PDB's ~MiscStatManager(); thiscall, ret 0, 0x4D5D40).
	static void Destruct(MiscStatManager* apThis);
	static void ReleaseMemory();
	// Not named in the PDB or IDA; name chosen here. Sets the values of the first 43 stats from apStats (the exe's
	// caller, PlayerCharacter::LoadGame, passes a stack array it read from the save).
	static void SetAll(const int32_t* apStats);
	// Not named in the PDB or IDA; name chosen here. Copies the values of the first 43 stats into apStats (the exe's
	// caller, PlayerCharacter::SaveGame, passes a stack array of MS_COUNT values).
	static void GetAll(int32_t* apStats);

	static inline StatListArray StatList{};
	static inline StatNameArray StatNames{"Quests Completed", "Locations Discovered", "People Killed",
		"Creatures Killed", "Locks Picked", "Computers Hacked", "Stimpaks Taken", "Rad-X Taken", "RadAway Taken",
		"Chems Taken", "Times Addicted", "Mines Disarmed", "Speech Successes", "Pockets Picked", "Pants Exploded",
		"Books Read", "Health From Stimpaks", "Weapons Created", "Health From Food", "Water Consumed", "Sandman Kills",
		"Paralyzing Punches", "Robots Disabled", "Times Slept", "Corpses Eaten", "Mysterious Stranger Visits",
		"Doctor Bags Used", "Challenges Completed", "Miss Fortunate Occurrences", "Disintegrations",
		"Have Limbs Crippled", "Speech Failures", "Items Crafted", "Weapon Modifications", "Items Repaired",
		"Total Things Killed", "Dismembered Limbs", "Caravan Games Won", "Caravan Games Lost", "Barter Amount Traded",
		"Roulette Games Played", "Blackjack Games Played", "Slots Games Played"};
};

// Steps the statistic to the next one; returns arID (cdecl, 0x7DC0A0, no PDB name).
MiscStatManager::MiscStatID& operator++(MiscStatManager::MiscStatID& arID);
// Inferred type: a1: char* from 0x4d5edf (passed to BSstricmp 1)
int32_t GetMiscStatIDFromName(char* a1);

inline int32_t MiscStatListBox_IsEnabled_iUT_Enabled;
inline int32_t MiscStatListBox_UpdateHighlightBoxPos_iUT_SelectedHeight;
inline int32_t MiscStatListBox_UpdateHighlightBoxPos_iUT_Highlight_Y;
inline int32_t MiscStatListBox_NumUnfiltered_iUT_num_filtered;
