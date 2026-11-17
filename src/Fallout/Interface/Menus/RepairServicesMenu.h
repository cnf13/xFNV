#pragma once

#include "BSMenu/Menu.h"
#include "Fallout/Interface/ListBox.h"
#include "Reconstruction.hpp"

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
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void SetCanRepair(Tile* a1, ItemChange* a2);
	static bool ShouldHideItem(ItemChange* a1);
	// IDA name; the PDB's SortFunc(ListBoxItem<ItemChange*>* apItem1, ListBoxItem<ItemChange*>*
	// apItem2), lbRepairList's ListBox::Sort compare.
	static int32_t Compare(ListBoxItem<ItemChange*>* a1, ListBoxItem<ItemChange*>* a2);

	Tile* pTiles[16];
	// PC listboxes are 0x30 bytes, rather than the Xbox PDB's 0x38 bytes.
	ListBox<ItemChange*> lbRepairList;
	int32_t iRepairAllCost;
	// The vendor's Repair skill: every access is a dword (0x7B7704 store, 0x7B7FC8/0x7B85E0 fild, 0x7B8C5A load).
	int32_t iVendorRepairSkill;

	static inline RepairServicesMenu* pMe;
	static inline int32_t iUT_Value;
	static inline Actor* pRepairVendor;
};
inline constexpr fnvd::Vtable RepairServicesMenu_vftable{};

// The tile trait id of the can-repair flag.
inline int32_t g_repairServicesMenu_trait_CanRepair;
inline int32_t g_repairServicesMenu_trait_Title;
inline int32_t g_repairServicesMenu_trait_RepairCost;
