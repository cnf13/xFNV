#pragma once

#include "BSMenu/Menu.h"
#include "BSCore/BSSimpleList.h"
#include "Fallout/Interface/HotKeysWheel.h"
#include "Fallout/Interface/ListBox.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class ItemChange;
class TESBoundObject;

bool IsDrBagUsable();
int32_t InventoryMenu_SortFunc(ListBoxItem<ItemChange*>* a1, ListBoxItem<ItemChange*>* a2);
void InventoryMenu_InitFunc(Tile* a1, ItemChange* a2);

// The inventory menu: the item list, the equip and drop tiles, and the item listbox.
class InventoryMenu : public Menu {
public:
	// The menu's tiles (pTiles) up to NUM_TILES; from TI_WEAPONS_TAB on, ids of tiles it builds. Values from the
	// Xbox PDB.
	enum TILE_INDEX : int32_t {
		TI_PLAYER_CAPS_INFO = 0,
		TI_PLAYER_HP_INFO = 1,
		TI_PLAYER_DR_INFO = 2,
		TI_PLAYER_WG_INFO = 3,
		TI_INVENTORY_LIST = 4,
		TI_HOTKEY_WHEEL = 5,
		TI_EQUIP_BTN = 6,
		TI_DROP_BTN = 7,
		TI_REPAIR_BTN = 8,
		TI_HOTKEY_BTN = 9,
		TI_CANCEL_BTN = 10,
		TI_ITEM_ICON = 11,
		TI_ITEM_INFO = 12,
		TI_TABLINE = 13,
		TI_DAM = 14,
		TI_DPS = 15,
		TI_STRREQ = 16,
		TI_DR = 17,
		TI_DT = 18,
		TI_MOD_BTN = 19,
		TI_ITEM_BADGE = 20,
		TI_PLAYER_DT_INFO = 21,
		TI_SKILLREQ = 22,
		NUM_TILES = 23,
		TI_WEAPONS_TAB = 24,
		TI_APPAREL_TAB = 25,
		TI_AID_TAB = 26,
		TI_MISC_TAB = 27,
		TI_AMMO_TAB = 28,
		TI_INVENTORY_ITEM = 29,
		TI_KEYRING = 30,
	};

	// A tab's saved list position.
	struct sPageState {
		int32_t iHighlightIndex;
		int32_t iScrollIndex;
	};

	InventoryMenu();
	~InventoryMenu() override;

	// Attaches one of the menu's tiles by its id, when the id is in range.
	void AttachTileByID(int32_t aiID, Tile* apTile) override;
	void DoClick(int32_t aiID, Tile* apTarget) override;
	void DoEnter(int32_t aiID, Tile* apTarget) override;
	void DoLeave(int32_t aiID, Tile* apTarget) override;
	void DoIdle() override;
	int32_t GetClass() override;
	bool DoGamepad(int32_t aiButton, float afValue) override;
	bool DoGamepadUpEvent(int32_t aiButton, float afValue) override;

	// Sets the button tiles' strings and visibility for the current item.
	void UpdateButtons();
	// Clears the selected item and hides the item data tile.
	void ClearCurrentItem();
	// PDB name by its shape and its place after DropCurrentItem (IDA HandleEquipOrUnEquip).
	// Equips or unequips the selected item (thiscall, ret 0, 0x780D60).
	void SelectCurrentItem();
	// Whether all 0x17 menu tiles are attached.
	bool IsWellFormedMenu();
	// The exe's SetVisibleFunc, which the item list's Traverse is passed by address.
	bool IsItemRepairable(ItemChange* apItem);

	// Rebuilds the item list and the player's stat lines of the open menu.
	static void UpdateList();
	// Fills the item data tile apTile (its last 13 children are the stat rows) with
	// apItem's stats; also used by the barter and container menus.
	// Inferred type: apItem: ItemChange* from 0x72e4cf (BarterMenu::pCurrItem); apTile: Tile* from 0x75cee1
	// (ContainerMenu::pTiles (this of ContainerMenu::DoEnter)), its members used at 0x707ec1
	static void SetupItemDataTile(ItemChange* apItem, Tile* apTile);
	// Remembers the list position of the current tab.
	static void SavePageState();
	// Whether the open inventory menu shows the keyring.
	static bool IsKeyringOpen();
	// PDB name by its parameter and its place after DoClick (IDA DropObjectCallback). Drops
	// aiAmountChosen of the selected item; also the quantity menu's callback (cdecl, ret 0, 0x780C50).
	static void DropCurrentItem(int32_t aiAmountChosen);
	// Selects tab aiTab (at most 4) in the open inventory menu.
	static void SetCurrentTab(int32_t aiTab);
	// SetCurrentTab for a byte-sized tab.
	static void SetCurrentInventoryPage(uint8_t aucTab);
	// The inventory menu's class id; static, as the exe calls it without a this (the PDB's GetClassStatic shape).
	static int32_t GetClassStatic();
	// Builds the inventory menu from its XML file.
	static Tile* Create();
	static void SetKnobPosition(bool abPlaySounds);
	static uint8_t GetCurrentInventoryPage();
	// ListBox::Traverse visitor: adds a worn item's object to pInstance's ItemUpdateList;
	// the tile is not read.
	static void AddToChangedItemsListIfWorn(Tile* apTile, ItemChange* apItem);
	// Puts the item list back at the position saved for the current tab.
	static void LoadPageState();
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	// IDA's TileUnkContChange::IsKey; the PDB's KeyFilterFunc by its place after FilterFunc.
	// True unless apItem's object is a key (TESKey); true for a null apItem (cdecl, ret 0, 0x782810).
	static bool KeyFilterFunc(ItemChange* apItem);

protected:
	// Shows an item tile when its trait 4012 is not negative, else hides it; the list's Traverse passes the item too.
	static void SetVisibleFunc(Tile* apTile, ItemChange* apItem);

public:
	static bool FilterFunc(ItemChange* apItem);

	// The menu's tiles; AttachTileByID, DoEnter and IsWellFormedMenu index it by the tile id.
	Tile* pTiles[NUM_TILES];
	// The item category filter (enum kWeapons, kApparel, kAid, kMisc, kAmmo).
	uint32_t iCurrTab;
	// The saved list position of each tab; the keyring is the last.
	sPageState pPageStatesA[6];
	ListBox<ItemChange*> lbInventoryList;
	// PDB name (IDA kHotkeyWheel, type HotKeyWheel): the constructor 0x77FA00 builds it after the list box.
	HotKeysWheel HKWheel;
	// PDB name (IDA kChangedItemsList): the objects whose worn state changed while the menu was open; the destructor
	// 0x77FB80 destroys it first.
	BSSimpleList<TESBoundObject*> ItemUpdateList;

	static inline ItemChange* pCurrItem;
	static inline InventoryMenu* pMe;
	static inline int32_t iUT_CurrentTab;
	static inline int32_t iUT_KeyringOpen;
	// The user trait the stat lines show their value in.
	static inline int32_t uiValueTrait;
	static inline int32_t iUT_ItemType;
	static inline int32_t iUT_EquippableItem;
};

inline int32_t ItemChangeListBox_Filter_iUT_num_filtered;
inline int32_t g_lastInventoryMenuSelectionListIndex;
inline constexpr fnvd::Vtable InventoryMenu_vftable{};
