#pragma once

#include "BSMenu/Menu.h"
#include "Fallout/Interface/ListBox.h"

#include <stdint.h>

class Actor;
class ItemChange;
class TileMenu;

class RepairServicesMenu : public Menu {
public:
    RepairServicesMenu();
    ~RepairServicesMenu() override;

    void AttachTileByID(int32_t, Tile*) override;
    void DoClick(int32_t, Tile*) override;
    void DoEnter(int32_t, Tile*) override;
    void DoLeave(int32_t, Tile*) override;
    int32_t GetClass() override;

    // Repairs apItem.
    void RepairItem(ItemChange* apBrokenItem);
    // True when all 16 tiles are present.
    bool IsWellFormedMenu();
    void InitEntries();

    // Creates the menu for apActor.
    static TileMenu* Create(Actor* apVendor);
    static int32_t GetClassStatic();
    // Ends the repair service and fades the open menu out.
    static void Close();
    static void RepairAllFunc(Tile* a1, ItemChange* a2);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void SetCanRepair(Tile* a1, ItemChange* a2);
    static bool ShouldHideItem(ItemChange* a1);
    // The compare function lbRepairList's ListBox::Sort uses.
    static int32_t Compare(ListBoxItem<ItemChange*>* a1, ListBoxItem<ItemChange*>* a2);

    Tile* pTiles[16];
    ListBox<ItemChange*> lbRepairList;
    int32_t iRepairAllCost;
    // The vendor's Repair skill.
    int32_t iVendorRepairSkill;

    static inline RepairServicesMenu* pMe;
    static inline int32_t iUT_Value;
    static inline Actor* pRepairVendor;
};

// The tile trait id of the can-repair flag.
inline int32_t g_repairServicesMenu_trait_CanRepair;
inline int32_t g_repairServicesMenu_trait_Title;
inline int32_t g_repairServicesMenu_trait_RepairCost;
