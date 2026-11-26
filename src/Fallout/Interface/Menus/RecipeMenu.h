#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/NewOverloads.h"
#include "BSMenu/Menu.h"
#include "Fallout Shared/TESRecipe.h"
#include "Fallout/Interface/ListBox.h"

#include <stdint.h>

class TESConditionItem;
class TESObjectREFR;
class TileImage;

// Defined in Fallout/Interface/InterfaceManager.h.
enum MENU_SOUNDS : int32_t;

// Returns &RecipeMenu::filterlist.
BSSimpleArray<TESRecipeCategory*>* get_g_RecipeMenuCategories();

class RecipeMenu : public Menu {
public:
    using filterlist_t = BSSimpleArray<TESRecipeCategory*, 1024>;

    RecipeMenu();
    ~RecipeMenu() override;

    void AttachTileByID(int32_t, Tile*) override;
    void DoClick(int32_t, Tile*) override;
    void DoEnter(int32_t, Tile*) override;
    // DoLeave and DoWheelMove are inherited, not overridden.
    void DoIdle() override;
    int32_t GetClass() override;
    bool DoGamepad(int32_t, float) override;

    void UpdateLists(TESBoundObject* apUpdateObj, TESForm* apCategory);
    // Fills the twelve stat tiles under apDisplayTile with the stats of apCurrItem; this is unused.
    void PopulateItemStatsDisplay(TESForm* apCurrItem, Tile* apDisplayTile);
    bool IsWellFormedMenu();

    static Tile* Create(TESObjectREFR* apVendor, TESForm* apCategory);
    // Updates the lists now when abUpdateNow, else flags them for a
    // refresh.
    static void ForceRecipeUpdate(bool abUpdateNow);
    static int32_t GetClassStatic();
    // How many times the player can make apRecipe; 0 when not at all.
    static int32_t CanMakeRecipe(TESRecipe* apRecipe);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void Close();
    static int32_t FilterSortFunction(const void* apPtr1, const void* apPtr2);
    static void ItemSelectCallback(int32_t aiAmountSelected);
    static int32_t SortFunc(ListBoxItem<TESRecipeComponent*>* apItem1, ListBoxItem<TESRecipeComponent*>* apItem2);

    // The PC version has fourteen tiles, where the Xbox version has twelve.
    TileImage* pTiles[14];
    TESObjectREFR* pCrafterRef;
    // The category Create was given; it is only stored and compared with recipes' categories.
    TESForm* pCategory;
    MENU_SOUNDS eFilterSound;
    ListBox<TESRecipe*> lbRecipeList;
    ListBox<TESRecipe*>* plbCurrListbox;
    ListBox<TESRecipeComponent*> lbIngredientList;
    ListBox<TESConditionItem*> lbConditionalList;
    bool bTutorial;

    static inline RecipeMenu* pMe;
    // The gamepad moves through the ingredient list rather than the recipe list.
    static inline bool bControlIngredientList;
    // The recipe the menu shows.
    static inline TESRecipe* pCurrItem;
    static inline int32_t iNumCanMake;
    static filterlist_t filterlist;
    // The flag RecipeMenu::DoIdle tests before running the pulse.
    static inline bool bPulseSelection = true;
    static inline bool bCrossFade;
    static inline Tile* pCrossFadeTile1;
    static inline Tile* pCrossFadeTile2;
    // The strength requirement tile PopulateItemStatsDisplay last filled.
    static inline Tile* pAmmoTile;
    // The value of the cross-fade pulse DoIdle runs.
    static float fPulseValue;
    static inline uint32_t uiLastIdleTime;
};

inline float RecipeMenu::fPulseValue;

inline bool g_bShouldRefreshRecipeMenu;
inline int32_t g_recipeMenuTrait_Title;
inline int32_t g_recipeMenuTrait_Value;
inline int32_t g_recipeMenuTrait_IsRecipeSelected;
