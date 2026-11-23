#pragma once

#include "BSMenu/Menu.h"
#include "Fallout/Interface/ListBox.h"

#include <stdint.h>

class ItemChange;
class RepairMenu;

class RepairMenu : public Menu {
public:
    RepairMenu();
    ~RepairMenu() override;

    void AttachTileByID(int32_t, Tile*) override;
    void DoClick(int32_t, Tile*) override;
    void DoEnter(int32_t, Tile*) override;
    void DoLeave(int32_t, Tile*) override;
    int32_t GetClass() override;
    bool DoGamepadUpEvent(int32_t, float) override;

    void ShowErrorMessage(const char* astrMessage);
    bool IsWellFormedMenu();
    void RepairItem(ItemChange* apSparePart);

    // Builds the menu from repair_menu.xml and returns its tile, or null when it is malformed.
    static Tile* Create();
    static int32_t GetClassStatic();
    // Opens the repair menu for apBrokenItem.
    static void InitRepairMenu(ItemChange* apBrokenItem);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void ResetMenu();
    static void InitFunc(Tile* apTile, ItemChange* apItem);
    static int32_t SortFunc(ListBoxItem<ItemChange*>* apItem1, ListBoxItem<ItemChange*>* apItem2);
    static void Update();

    Tile* pTiles[13];
    ListBox<ItemChange*> lbRepairList;

    static inline int32_t iUT_Title;
    static inline int32_t iUT_Value;
    static int32_t iUT_enabled;
    static inline RepairMenu* pMe;
    static inline ItemChange* pBrokenItem;
    static inline int32_t iUT_IsRepairItem;
};

inline int32_t RepairMenu::iUT_enabled;
