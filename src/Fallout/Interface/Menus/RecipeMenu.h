#pragma once

#include "BSMenu/Menu.h"
#include "Fallout/Interface/ListBox.h"
#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/TESRecipe.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// Defined in Fallout/Interface/InterfaceManager.h.
enum MENU_SOUNDS : int32_t;
class TESConditionItem;
class TESObjectREFR;
class TileImage;

// IDA name; not in the PDB. Returns &RecipeMenu::filterlist (0x726F13 mov eax, 0x11D8F08).
BSSimpleArray<TESRecipeCategory*>* get_g_RecipeMenuCategories();

class RecipeMenu : public Menu {
public:
	// IDA: g_recipeMenuCategories.
	using filterlist_t = BSSimpleArray<TESRecipeCategory*, 1024>;

	RecipeMenu();
	~RecipeMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	void DoEnter(int32_t, Tile*) override;
	// The PC inherits DoLeave and DoWheelMove, unlike the Xbox PDB.
	void DoIdle() override;
	int32_t GetClass() override;
	bool DoGamepad(int32_t, float) override;

	void UpdateLists(TESBoundObject* apUpdateObj, TESForm* apCategory);
	// Fills the twelve stat tiles under apDisplayTile with the stats of apCurrItem (thiscall, ret 8; this unused).
	void PopulateItemStatsDisplay(TESForm* apCurrItem, Tile* apDisplayTile);
	bool IsWellFormedMenu();

	static Tile* Create(TESObjectREFR* apVendor, TESForm* apCategory);
	// Updates the lists now when abUpdateNow, else flags them for a
	// refresh.
	static void ForceRecipeUpdate(bool abUpdateNow);
	static int32_t GetClassStatic();
	// How many times the player can make apRecipe; 0 when not at all.
	static int32_t CanMakeRecipe(TESRecipe* apRecipe);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void Close();
	static int32_t FilterSortFunction(const void* apPtr1, const void* apPtr2);
	static void ItemSelectCallback(int32_t aiAmountSelected);
	static int32_t SortFunc(ListBoxItem<TESRecipeComponent*>* apItem1, ListBoxItem<TESRecipeComponent*>* apItem2);

	// The PC has fourteen tiles, rather than the Xbox PDB's twelve.
	TileImage* pTiles[14];
	TESObjectREFR* pCrafterRef;
	// The category Create was given; the exe only stores it and compares it with recipes' categories.
	TESForm* pCategory;
	MENU_SOUNDS eFilterSound;
	// PC listboxes are 0x30 bytes, rather than the Xbox PDB's 0x38 bytes.
	ListBox<TESRecipe*> lbRecipeList;
	ListBox<TESRecipe*>* plbCurrListbox;
	ListBox<TESRecipeComponent*> lbIngredientList;
	ListBox<TESConditionItem*> lbConditionalList;
	bool bTutorial;
	uint8_t padding101[3];

	static inline RecipeMenu* pMe;
	// The gamepad moves through the ingredient list rather than the recipe list.
	static inline bool bControlIngredientList;
	// The recipe the menu shows.
	static inline TESRecipe* pCurrItem;
	static inline int32_t iNumCanMake;
	static filterlist_t filterlist;
	// The flag RecipeMenu::DoIdle tests before running the pulse (IDA .bPulseSelection).
	static inline bool bPulseSelection = true;
	static inline bool bCrossFade;
	static inline Tile* pCrossFadeTile1;
	static inline Tile* pCrossFadeTile2;
	// The strength requirement tile PopulateItemStatsDisplay last filled.
	static inline Tile* pAmmoTile;
	static inline uint32_t uiLastIdleTime;
};
inline constexpr fnvd::Vtable RecipeMenu_vftable{};

inline bool g_bShouldRefreshRecipeMenu;

// IDA names; not in the PDB. What RecipeMenu::DoIdle keeps between calls: whether the pulse is sweeping down, the two
// tiles it fades (0x11D8EAC and 0x11D8EB0), the pulse value, and the time of the last update (afValue is the IDA
// label).
inline float RecipeMenu_afValue;
inline int32_t g_recipeMenuTrait_Title;
inline int32_t g_recipeMenuTrait_Value;
inline int32_t g_recipeMenuTrait_IsRecipeSelected;
