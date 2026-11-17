#pragma once

#include "BSMenu/Menu.h"
#include "Fallout/Interface/ListBox.h"

#include <stdint.h>

class ItemChange;
class ItemModMenu;

class ItemModMenu : public Menu {
public:
	ItemModMenu();
	~ItemModMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	void DoEnter(int32_t, Tile*) override;
	void DoLeave(int32_t, Tile*) override;
	int32_t GetClass() override;
	bool DoGamepadUpEvent(int32_t, float) override;

	// True when all 14 tiles at +0x28 are set.
	bool IsWellFormedMenu();
	char ItemModItem(ItemChange* a1);

	// Builds the menu from item_mod_menu.xml and returns its tile, or null when it is malformed.
	static Tile* Create();
	static int32_t GetClassStatic();
	// Opens the item mod menu for apBrokenItem.
	static void InitItemModMenu(ItemChange* apBrokenItem, bool bCheckEquip);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void ResetMenu();
	static void Update();
	static void SetTileAlpha(Tile* a1, ItemChange* a2);
	// lbItemModList's ListBox::Sort compare: reads each item's tile (ListBoxItem::tile).
	static int32_t CompareForSorting(ListBoxItem<ItemChange*>* a1, ListBoxItem<ItemChange*>* a2);

	Tile* pTiles[14];
	ListBox<ItemChange*> lbItemModList;

	static inline ItemChange* pBrokenItem;
	static inline bool bIsEquipped;
	static inline ItemModMenu* pMe;
};

inline int32_t g_ItemModMenuTrait_Title;
inline int32_t g_ItemModMenuTrait_Value;
inline int32_t g_ItemModMenuTrait_enabled;
inline int32_t g_ItemModMenuTrait_IsItemModItem;
