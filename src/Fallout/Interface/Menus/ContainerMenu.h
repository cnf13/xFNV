#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSMenu/Menu.h"
#include "BSCore/BSSimpleList.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/ListBox.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Actor;
class InventoryChanges;
class ItemChange;
// Defined in Fallout/Interface/InterfaceManager.h.
enum MENU_SOUNDS : int32_t;
class TESBoundObject;
class TESObjectREFR;

// The container menu: the player's inventory beside a container's or a companion's.
class ContainerMenu : public Menu {
public:
	// The item category a list shows; DoClick steps through them and wraps from FM_AMMO to FM_ALL. Values from the
	// Xbox PDB.
	enum FILTER_MODE : int32_t {
		FM_ALL = 0,
		FM_WEAPONS = 1,
		FM_APPAREL = 2,
		FM_AID = 3,
		FM_MISC = 4,
		FM_AMMO = 5,
		FM_COUNT = 6,
	};

	// The menu's tiles, indexed by TILE_INDEX up to NUM_TILES; from TI_ITEM on, ids of tiles it builds. Values from
	// the Xbox PDB.
	enum TILE_INDEX : int32_t {
		TI_ITEMS_FILTER_LEFT = 0,
		TI_ITEMS_TITLE = 1,
		TI_ITEMS_FILTER_RIGHT = 2,
		TI_WEIGHT_INFO = 3,
		TI_ITEMS_LIST = 4,
		TI_CONTAINER_FILTER_LEFT = 5,
		TI_CONTAINER_TITLE = 6,
		TI_CONTAINER_FILTER_RIGHT = 7,
		TI_CONTAINER_LIST = 8,
		TI_ARROW_ICON = 9,
		TI_TAKE_ALL_BTN = 10,
		TI_EXIT_BTN = 11,
		TI_ITEM_ICON = 12,
		TI_ITEM_INFO = 13,
		TI_DAM = 14,
		TI_DPS = 15,
		TI_STRREQ = 16,
		TI_DR = 17,
		TI_DT = 18,
		NUM_TILES = 19,
		TI_ITEM = 20,
		TI_ITEMS_RECT = 21,
		TI_CONTAINER_RECT = 22,
	};

	ContainerMenu();
	~ContainerMenu() override;

	int32_t GetClass() override { return 0x3F0; }
	void AttachTileByID(int32_t aiID, Tile* apTile) override;
	void DoClick(int32_t aiID, Tile* apTarget) override;
	void DoEnter(int32_t aiID, Tile* apTarget) override;
	void DoIdle() override;
	bool DoGamepad(int32_t aiButton, float afValue) override;
	bool DoKey(int32_t aiKey) override;
	void DoLeave(int32_t aiID, Tile* apTarget) override;
	bool DoGamepadUpEvent(int32_t aiButton, float afValue) override;
	void DoWheelMove(int32_t aiID, Tile* apTarget) override;

	// Refreshes the item lists; apUpdateObj is the changed item.
	void UpdateLists(TESBoundObject* apUpdateObj);
	// The exe reads a1's extra data list at +0 and its value.
	int32_t GetStolenValue(ItemChange* a1, int32_t a2, bool a3);
	void PlayReactivityBark(TESTopic* aRelatedTopic);
	void DisplayContainerMenuSubtitle(BSStringT<char> asText, BSSoundHandle ahSound);

protected:
	// Rolls a pickpocket attempt on apActor for an item worth aiStealValue; on failure raises the alarm and shows
	// the fail message. Returns false when caught.
	bool PickpocketCheck(Actor* apActor, int32_t aiStealValue);

public:
	bool IsWellFormedMenu();
	void PlayContainerSound(TESObjectREFR* apContainer, bool abOpenSound);
	void InitContainerMode();
	void UpdateItemInList(TESBoundObject* apObj, InventoryChanges* apInventory, ListBox<ItemChange*>* apListbox);
	bool IsPlacingMine(ItemChange* apItem, TESObjectREFR* apRefFrom, TESObjectREFR* apRefTo);
	void SwitchContainer(Tile* apTile, ItemChange* apItem, int32_t aiAmountSwitched);
	void SetFilterModeTitle(Tile* apTitleTile);
	void SetContainerTitle(const char* astrTitle);

	// Opens the container menu for apContainer.
	static TileMenu* Create(TESObjectREFR* apContainer, Interface::CONTAINER_MENU_MODE aeContainerMode);
	// cdecl; closes the container menu that is up.
	static void Close();
	static uint32_t GetClassStatic();
	static void ForceContainerUpdate();
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	// IDA name; the PDB's TakeAllFn(Tile*, ItemChange*), lbContainer's ListBox::Traverse
	// visitor (the exe reads a1's child list at +4 and a2's extra data list).
	static void TakeAllFn(Tile* a1, ItemChange* a2);
	static bool FirstPassFilterFunc(ItemChange* apItem);
	static int32_t SortFunc(ListBoxItem<ItemChange*>* apItem1, ListBoxItem<ItemChange*>* apItem2);
	static void InitFunc(Tile* apTile, ItemChange* apItem);
	static void ItemSelectCallback(int32_t aiAmountSelected);
	static bool FilterModeFunc(ItemChange* apItem);

	// The menu's tiles; AttachTileByID, DoEnter and DoLeave index it by the tile id.
	Tile* pTiles[NUM_TILES];
	TESObjectREFR* pContainerRef;
	BSSimpleList<TESBoundObject*> ActiveMineList;
	Interface::CONTAINER_MENU_MODE eContainerMode;
	int32_t iValueStolen;
	// IDA name; the PDB's Xbox offsets list the two pickpocket flags in the other order.
	bool bPickpocketFail;
	bool bPickPocketTallied;
	uint8_t padding8A[2];
	FILTER_MODE eItemsFilterMode;
	FILTER_MODE eContainerFilterMode;
	MENU_SOUNDS eFilterSound;
	ListBox<ItemChange*> lbItems;
	ListBox<ItemChange*> lbContainer;
	ListBox<ItemChange*>* plbCurrListbox;
	uint32_t m_nTimeToClearSubtitle;
	BSSoundHandle m_hCurrentReactivityBark;

	static inline ItemChange* pCurrItem;
	static inline ContainerMenu* pMe;
	static inline char bMouseOverEnabled = 1;
};
inline constexpr fnvd::Vtable ContainerMenu_vftable{};
