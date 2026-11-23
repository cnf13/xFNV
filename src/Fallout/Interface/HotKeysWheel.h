#pragma once

#include <stdint.h>

class ItemChange;
class Tile;

// The HUD's hotkey wheel: the wheel tile, its eight hotkey tiles and the highlighted hotkey.
class HotKeysWheel {
public:
    // Releases the player's hotkeyed items: each item's model file, then the item.
    ~HotKeysWheel();

    // Builds the hotkey tiles under apWheel.
    void Initialize(Tile* apWheel);
    bool GetVisible();
    // Shows or hides the wheel, refreshing the hotkey list first when shown.
    void SetVisible(bool abVisible);
    // Refills the hotkey tiles from the player's hotkeyed items.
    void UpdateHotkeyList();
    // Highlights hotkey aiNewIndex (-1: the one the gamepad points at) and returns the highlighted hotkey, negative for
    // none.
    int32_t MoveHighlight(int32_t aiNewIndex);
    // Equips or unequips the highlighted hotkey's item.
    void UseHotkeyItem();
    // Assigns apHotkeyItem to the highlighted hotkey.
    void SetHotkeyItem(ItemChange* apHotkeyItem);
    // The number of hotkeys, ciNumHotkeys.
    int32_t QNumHotkeys();
    void HighlightHotkey(int32_t aiIndex);

    static bool IsDPadPressed();

    Tile* pWheel;
    Tile* pHotkeyTilesA[8];
    bool bSelectingHotkey;
    int32_t iLastHighlightedHotkey;
    int32_t iUT_HighlightedHotkey;
    int32_t iUT_HighlightedText;

    static constexpr int32_t ciNumHotkeys = 8;
};
