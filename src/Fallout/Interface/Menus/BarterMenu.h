#pragma once

#include "BSMenu/Menu.h"
#include "BSCore/BSSimpleList.h"
#include "Fallout/Interface/ListBox.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class ItemChange;
// Defined in Fallout/Interface/InterfaceManager.h.
enum MENU_SOUNDS : int32_t;
class TESBoundObject;
class TESObjectREFR;

int32_t BSstrupr(char* apString1, int32_t aiSize);

class BarterMenu : public Menu {
public:
	// The item category a list shows; DoClick steps through them and wraps from FM_AMMO to FM_ALL.
	enum FILTER_MODE : int32_t {
		FM_ALL = 0,
		FM_WEAPONS = 1,
		FM_APPAREL = 2,
		FM_AID = 3,
		FM_MISC = 4,
		FM_AMMO = 5,
		FM_COUNT = 6,
	};

	BarterMenu();
	~BarterMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	void DoEnter(int32_t, Tile*) override;
	void DoLeave(int32_t, Tile*) override;
	void DoWheelMove(int32_t, Tile*) override;
	void DoIdle() override;
	int32_t GetClass() override;
	bool DoGamepad(int32_t, float) override;

	// Rebuilds both item lists, keeping apUpdateObj selected.
	void UpdateLists(TESBoundObject* apUpdateObj);
	void UpdateCapsLabels();
	// The exe's one check of 22 tile pointers from 0x28, folded with the map menu's.
	bool AreAllTilesNonNull();
	ItemChange* GetTransactItem(BSSimpleList<ItemChange*>* apItemList, ItemChange* apCompareItem);
	void AddTransactItem(int32_t aiAmountSelected);
	void RemoveTransactItem(int32_t aiAmountSelected);
	void CompleteTransaction();
	void SetFilterModeTitle(Tile* apTitleTile);

	// Refreshes the barter lists now, or at the next idle when abNow is false. The exe tests the whole byte; bool
	// would assume 0/1.
	static void ForceBarterUpdate(bool abNow);
	// Opens the barter menu with apVendor (cdecl, ret 0, 0x72D250); IDA types the second
	// argument int.
	static Tile* Create(TESObjectREFR* apVendor, int32_t aiDiscount);
	static int32_t GetClassStatic();
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void Close();
	static void CancelConfirmFunc();
	static bool PlayerFirstPassFilterFunc(ItemChange* apItem);
	static bool VendorFirstPassFilterFunc(ItemChange* apItem);
	static void RemoveStolenItems(Tile* apTile, ItemChange* apItem);
	static float GetSortValue(Tile* apTile, ItemChange* apItem);
	static int32_t SortFunc(ListBoxItem<ItemChange*>* apItem1, ListBoxItem<ItemChange*>* apItem2);
	static void InitFunc(Tile* apTile, ItemChange* apItem);
	static void ItemSelectCallback(int32_t aiAmountSelected);
	static bool FilterModeFunc(ItemChange* apItem);

	Tile* pTiles[22];
	TESObjectREFR* pVendorRef;
	float fTransactionAmount;
	int32_t iDiscount;
	int32_t iPlayerCaps;
	int32_t iVendorCaps;
	float fBuyMult;
	float fSellMult;
	FILTER_MODE ePlayerFilterMode;
	FILTER_MODE eVendorFilterMode;
	// The sound a filter change plays (MENU_SOUND_NONE while the menu changes the filter itself).
	MENU_SOUNDS eFilterSound;
	// PC listboxes occupy 0x30 bytes, rather than the Xbox PDB's 0x38 bytes.
	ListBox<ItemChange*> lbPlayerItems;
	ListBox<ItemChange*> lbVendorItems;
	ListBox<ItemChange*>* plbCurrListbox;
	// The items of the pending transaction, on each side (IDA: leftBarter and rightBarter).
	BSSimpleList<ItemChange*> ItemsToBuy;
	BSSimpleList<ItemChange*> ItemsToSell;
	// The PDB's bItemsTraded, at 0x11C on the PC because its listboxes are 8 bytes smaller each. The exe reads and
	// writes the byte only (Close 0x72D722, Construct 0x72CFDA, CompleteTransaction 0x730182).
	bool bItemsTraded;
	uint8_t padding11D[3];

	static inline int32_t iUT_IsBarterSelected;
	static inline BarterMenu* pMe;
	static inline bool bForceUpdate;
	static inline bool bCrossFade = true;
	static inline ItemChange* pCurrItem;
};

inline int32_t g_BarterMenu_Trait_Title;
inline int32_t g_BarterMenu_Trait_Value;
inline int32_t g_BarterMenu_Trait_IsContainerListSelected;
inline int32_t g_BarterMenu_Trait_ArrowDirection;
inline constexpr fnvd::Vtable BarterMenu_vftable{};
