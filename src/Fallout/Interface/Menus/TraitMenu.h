#pragma once

#include "BSCore/MemoryManager.h"
#include "BSMenu/Menu.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSPerk;

// Not named in the PDB or IDA; name chosen here. TraitMenu::DoClick (0x7E739C) hands it to
// ListBox<BGSPerk *>::Traverse as the traversal callback, void (Tile*, BGSPerk*); it reads only the tile and sets
// eax on no path of its own.
void ClearSelectedTrait(Tile* a1, BGSPerk* a2);
void UpdatePerkTextAlpha(Tile* a1, BGSPerk* a2);

class TraitMenu : public Menu {
public:
	// IDA has no name for it; the exe constructs it as a BSSimpleArray<EffectItemData, 1024> (0x788300).
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

	// True when all nine pTiles are attached (thiscall, ret 0, 0x7E6DD0; Create tests al).
	bool IsWellFormedMenu();
	// PDB name (IDA Tile::sub_7E7850): gathers the effects of apTile's perk at its rank
	// (thiscall, ret 4, 0x7E7850: reads lbPerksList at +0x54). Returns nothing; IDA's NiNode* is a leftover eax.
	void ApplyPerkEffects(Tile* apTile);
	// Fills lbPerksList from AvailablePerks.
	void InitLists();
	// Shows iMaxPoints - iCurrPoints in pTiles[4] (thiscall, ret 0, 0x7E71B0: reads this + 0x4C, + 0x50 and the
	// tile at + 0x38; called by TraitMenu::Create and DoClick on the TraitMenu).
	void UpdatePointCounter();

	// Creates the menu's tile.
	static Tile* Create();
	// Applies the chosen traits and closes the open menu.
	static void Close();
	// The menu's class ID (1084), for Create.
	static int32_t GetClassStatic();
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	// IDA name; not in the PDB. ListBox<BGSPerk *>::Traverse's callback (0x7E6E10 hands it
	// over): void (Tile*, BGSPerk*); it sets eax on no path of its own.
	static void IncreaseSelectedTraitRank(Tile* a1, BGSPerk* a2);
	static int32_t SortPerksFunc(ListBoxItem<BGSPerk*>* apItem1, ListBoxItem<BGSPerk*>* apItem2);

	Tile* pTiles[9];
	int32_t iCurrPoints;
	int32_t iMaxPoints;
	// PC listboxes are 0x30 bytes, rather than the Xbox PDB's 0x38 bytes.
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
inline constexpr fnvd::Vtable TraitMenu_vftable{};

inline int32_t g_traitMenuSelected;
inline int32_t g_traitMenuCurrPoints;
inline int32_t g_traitMenuMaxPoints;
