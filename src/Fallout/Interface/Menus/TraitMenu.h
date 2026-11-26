#pragma once

#include "BSCore/NewOverloads.h"
#include "BSMenu/Menu.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout/Interface/ListBox.h"

#include <stdint.h>

class BGSPerk;

// TraitMenu::DoClick hands it to ListBox<BGSPerk *>::Traverse as the traversal callback; it reads only the tile.
void ClearSelectedTrait(Tile* apTile, BGSPerk* apPerk);
void UpdatePerkTextAlpha(Tile* apTile, BGSPerk* apPerk);

class TraitMenu : public Menu {
public:
    using PerkEffectList_t = BSSimpleArray<EffectItemData, 1024>;

    TraitMenu();
    ~TraitMenu() override;

    void AttachTileByID(int32_t, Tile*) override;
    void DoClick(int32_t, Tile*) override;
    void DoEnter(int32_t, Tile*) override;
    void DoIdle() override;
    int32_t GetClass() override;
    bool DoGamepad(int32_t, float) override;
    bool DoGamepadUpEvent(int32_t, float) override;

    // True when all nine pTiles are attached.
    bool IsWellFormedMenu();
    // Gathers the effects of apTile's perk at its rank, reading lbPerksList.
    void ApplyPerkEffects(Tile* apTile);
    // Fills lbPerksList from AvailablePerks.
    void InitLists();
    // Shows iMaxPoints - iCurrPoints in pTiles[4]; called by TraitMenu::Create and DoClick.
    void UpdatePointCounter();

    // Creates the menu's tile.
    static Tile* Create();
    // Applies the chosen traits and closes the open menu.
    static void Close();
    // The menu's class ID (1084), for Create.
    static int32_t GetClassStatic();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    // ListBox<BGSPerk *>::Traverse's callback.
    static void IncreaseSelectedTraitRank(Tile* apTile, BGSPerk* apPerk);
    static int32_t SortPerksFunc(ListBoxItem<BGSPerk*>* apItem1, ListBoxItem<BGSPerk*>* apItem2);

    Tile* pTiles[9];
    int32_t iCurrPoints;
    int32_t iMaxPoints;
    ListBox<BGSPerk*> lbPerksList;
    BSSimpleList<BGSPerk*> AvailablePerks;

    static inline int32_t iUT_TextAlpha;
    // The open menu: Create stores the new TraitMenu here and reads its fields (+0x38 tile, +0x4C, +0x50).
    static inline TraitMenu* pMe;
    static PerkEffectList_t PerkEffectList;
    static inline int32_t iUT_AddedValue;
    static inline int32_t iUT_ExtraValue;
    static inline int32_t iUT_BaseValue;
};

inline int32_t g_traitMenuSelected;
inline int32_t g_traitMenuCurrPoints;
inline int32_t g_traitMenuMaxPoints;
