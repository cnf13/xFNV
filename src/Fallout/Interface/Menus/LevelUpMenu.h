#pragma once

#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout/Interface/ListBox.h"

#include <stdint.h>

bool IsTileSelected(void* apThis, [[maybe_unused]] void* apEdx, Tile* a2);

class LevelUpMenu : public Menu {
public:
    // The menu's two pages; the type of eCurrentPage and of SetCurrentPage's parameter.
    enum PageIndex : int32_t {
        PI_SKILLS_PAGE = 0,
        PI_PERKS_PAGE = 1,
        NUM_PAGES = 2,
    };

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
    // Switches to aePage when it is at most the last page (the _EndPage trait), else closes the menu.
    void SetCurrentPage(PageIndex aePage);
    void InitLists();
    void UpdatePointCounter();

    static int32_t GetClassStatic();
    // Opens the level up menu and returns its tile.
    static Tile* Create();
    // The body Create calls once the interface is up.
    static Tile* CreateMenu();
    static void SetPerkAlphaIfRequirementsNotMet(Tile* a1, BGSPerk* a2);
    // The reset button's traversal of the perks list: deselects apTile and reapplies the perk effects.
    static void ResetPerksFunc(Tile* apTile, BGSPerk* apPerk);
    // SetCurrentPage's traversal of the skills list: refreshes apTile's displayed value.
    static void UpdateSkillsFunc(Tile* apTile, ActorValue::Index aeIndex);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void Close();
    static void ResetSkillsFunc(Tile* apTile, ActorValue::Index a2);
    static void PerkUpdateFunc(Tile* apTile, ActorValue::Index aeIndex);
    // ListBox<BGSPerk *>::Traverse's callback.
    static void IncreaseSelectedPerkRank(Tile* a1, BGSPerk* a2);

    PageIndex eCurrentPage;
    Tile* pTiles[10];
    int32_t iCurrPoints[2];
    int32_t iMaxPoints[2];
    ListBox<ActorValue::Index> lbSkillsList;
    ListBox<BGSPerk*> lbPerksList;
    BSSimpleList<BGSPerk*> AvailablePerks;

    static inline LevelUpMenu* pMe;
    static inline int32_t iUT_Selected;
    static inline int32_t iUT_AddedValue;
    static PerkEffectList_t PerkEffectList;
};

// The tile trait id of the perk text alpha.
inline int32_t g_levelUpMenu_Trait_TextAlpha;

extern SettingT<INISettingCollection> bHideUnavailablePerks;
inline int32_t g_levelUpMenu_Trait_MaxPoints;
inline int32_t g_levelUpMenu_Trait_CurrPoints;
inline int32_t g_levelUpMenu_Trait_BaseValue;
inline int32_t g_levelUpMenu_Trait_CurrentPage;
inline int32_t g_levelUpMenu_Trait_ExtraValue;
inline int32_t g_levelUpMenu_Trait_Rank;
