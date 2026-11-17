#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

class ItemChange;
class Tile;

// The HUD's hotkey wheel: the wheel tile, its eight hotkey tiles and the highlighted hotkey.
class HotKeysWheel {
public:
	// Releases the player's hotkeyed items: each item's model file, then the item.
	~HotKeysWheel();

	// thiscall, ret 4, 0x701610: builds the hotkey tiles under apWheel.
	void Initialize(Tile* apWheel);
	bool GetVisible();
	// thiscall, ret 4, 0x701760: shows or hides the wheel (refreshing the hotkey list first
	// when shown).
	void SetVisible(bool abVisible);
	// thiscall, ret 0, 0x7017B0: refills the hotkey tiles from the player's hotkeyed items.
	void UpdateHotkeyList();
	// thiscall, ret 4, 0x701BD0: highlights hotkey aiNewIndex (-1: the one the gamepad
	// points at) and returns the highlighted hotkey, negative for none.
	int32_t MoveHighlight(int32_t aiNewIndex);
	// thiscall, ret 0, 0x701E80: equips or unequips the highlighted hotkey's item.
	void UseHotkeyItem();
	// thiscall, ret 4, 0x7019E0: assigns apHotkeyItem to the highlighted hotkey.
	void SetHotkeyItem(ItemChange* apHotkeyItem);
	// The number of hotkeys, ciNumHotkeys. Both exe callers (0x77DA60, 0x781BA0) call this shared out-of-line
	// constant getter. Same function as CombatProcedureActivateObject::GetType (0x6E4BA0), registered there.
	int32_t QNumHotkeys();
	// Inferred type: a3: int32_t: signed uses from 0x701e22
	void HighlightHotkey(int32_t a3);

	static bool IsDPadPressed();

	Tile* pWheel;
	Tile* pHotkeyTilesA[8];
	bool bSelectingHotkey;
	int32_t iLastHighlightedHotkey;
	int32_t iUT_HighlightedHotkey;
	int32_t iUT_HighlightedText;

	static constexpr int32_t ciNumHotkeys = 8;
};

// Not named in the PDB or IDA; name chosen here. Twelve zero bytes the exe never references, between ikeyPressStart
// and Interface::bIsDroppingInInventory.
using data_11D88F8_t = int32_t[3];
inline data_11D88F8_t data_11D88F8{};
