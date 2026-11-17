#pragma once

#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
bool IsTileSelected(void* apThis, [[maybe_unused]] void* apEdx, Tile* a2);

class LevelUpMenu : public Menu {
public:
	// The menu's two pages; the PDB's nested enum (int-sized), the type of eCurrentPage and of SetCurrentPage's
	// parameter.
	enum PageIndex : int32_t {
		PI_SKILLS_PAGE = 0,
		PI_PERKS_PAGE = 1,
		NUM_PAGES = 2,
	};

	// IDA has no name for it; the exe constructs it as a BSSimpleArray<EffectItemData, 1024> (0x788300).
	using PerkEffectList_t = BSSimpleArray<EffectItemData, 1024>;

	LevelUpMenu();
	~LevelUpMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	void DoEnter(int32_t, Tile*) override;
	void DoIdle() override;
	int32_t GetClass() override;
	bool DoGamepad(int32_t, float) override;
	bool DoGamepadUpEvent(int32_t, float) override;

	bool IsWellFormedMenu();
	// Writes the skill tile's clamped base + extra + added value into its _DisplayString
	// trait.
	void SetDisplayString(Tile* apTile);
	// Recomputes the stat changes of the perk on apTile's selected rank.
	void ApplyPerkEffects(Tile* apTile);
	// Switches to aePage when it is at most the last page (the _EndPage trait), else closes the menu; thiscall, ret 4,
	// 0x785830.
	void SetCurrentPage(PageIndex aePage);
	void InitLists();
	void UpdatePointCounter();

	static int32_t GetClassStatic();
	// IDA name Create_; the PDB name is used.
	// Opens the level up menu and returns its tile.
	static Tile* Create();
	// IDA's LevelUpMenu::Create, the body Create (0x706270 above) calls once the interface is
	// up; src's Create already holds the PDB name, so this one's name is chosen here (cdecl, 0x784C80).
	static Tile* CreateMenu();
	static void SetPerkAlphaIfRequirementsNotMet(Tile* a1, BGSPerk* a2);
	// The reset button's traversal of the perks list: deselects apTile and reapplies the perk effects.
	static void ResetPerksFunc(Tile* apTile, BGSPerk* apPerk);
	// SetCurrentPage's traversal of the skills list: refreshes apTile's displayed value.
	static void UpdateSkillsFunc(Tile* apTile, ActorValue::Index aeIndex);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void Close();
	static void ResetSkillsFunc(Tile* apTile, ActorValue::Index a2);
	static void PerkUpdateFunc(Tile* apTile, ActorValue::Index aeIndex);
	// IDA name; not in the PDB. ListBox<BGSPerk *>::Traverse's callback: void (Tile*,
	// BGSPerk*); it sets eax on no path of its own.
	static void IncreaseSelectedPerkRank(Tile* a1, BGSPerk* a2);

	PageIndex eCurrentPage;
	Tile* pTiles[10];
	int32_t iCurrPoints[2];
	int32_t iMaxPoints[2];
	// PC listboxes are 0x30 bytes, rather than the Xbox PDB's 0x38 bytes.
	ListBox<ActorValue::Index> lbSkillsList;
	ListBox<BGSPerk*> lbPerksList;
	BSSimpleList<BGSPerk*> AvailablePerks;

	static inline LevelUpMenu* pMe;
	static inline int32_t iUT_Selected;
	static inline int32_t iUT_AddedValue;
	static PerkEffectList_t PerkEffectList;
};
inline constexpr fnvd::Vtable LevelUpMenu_vftable{};

// The tile trait id of the perk text alpha.
inline int32_t g_levelUpMenu_Trait_TextAlpha;

// The game settings LevelUpMenu.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bHideUnavailablePerks;
inline int32_t g_levelUpMenu_Trait_MaxPoints;
inline int32_t g_levelUpMenu_Trait_CurrPoints;
inline int32_t g_levelUpMenu_Trait_BaseValue;
inline int32_t g_levelUpMenu_Trait_CurrentPage;
inline int32_t g_levelUpMenu_Trait_ExtraValue;
inline int32_t g_levelUpMenu_Trait_Rank;
