#pragma once

#include "BSCore/MemoryManager.h"
#include "BSMenu/Menu.h"

class TraitSelectMenu : public Menu {
public:
    // The menu's tile ids.
    enum TILE_INDEX : int32_t {
        TI_TRAIT_HEAD_01 = 0,
        TI_TRAIT_HEAD_02 = 1,
        TI_TRAIT_HEAD_03 = 2,
        TI_TRAIT_HEAD_04 = 3,
        TI_TRAIT_BODY_01 = 4,
        TI_TRAIT_FINEMOTOR_01 = 5,
        TI_TRAIT_FINEMOTOR_02 = 6,
        TI_TRAIT_FINEMOTOR_03 = 7,
        TI_TRAIT_FINEMOTOR_04 = 8,
        TI_TRAIT_GROSSMOTOR_01 = 9,
        TI_TRAIT_GROSSMOTOR_02 = 10,
        TI_TRAIT_GROSSMOTOR_03 = 11,
        TI_TRAIT_GROSSMOTOR_04 = 12,
        TI_SCROLL_BAR = 13,
        TI_BUTTON_DONE = 14,
        TI_MAIN_RECT = 15,
        TI_PAPER_GFX = 16,
        TI_CHECKMARK = 17,
        NUM_TILES = 18,
        // No tile: m_CurrentlyCheckedTile and m_CurrentlySelectedTile are tested against -1.
        TI_NO_TILE = -1,
    };

    TraitSelectMenu();
    ~TraitSelectMenu() override;

    void AttachTileByID(int32_t, Tile*) override;
    void DoClick(int32_t, Tile*) override;
    void DoEnter(int32_t, Tile*) override;
    void DoWheelMove(int32_t, Tile*) override;
    void DoIdle() override;
    int32_t GetClass() override;
    bool DoGamepad(int32_t, float) override;

    // Checks box aiID (-1: none) and unchecks the previous one.
    void SetBoxAsChecked(int32_t aiID, bool bForceRefresh);
    // Checks box aiID, or clears it when it is the checked one.
    void ToggleCheckBox(int32_t aiID);
    // Scrolls the trait list by afDeltaY.
    void ScrollForm(float afDeltaY);
    // Unchecks every box.
    void ClearCheckBox();
    bool IsWellFormedMenu();
    void SetButtonAsHighlighted(int32_t aiID, char abForceRefresh);
    void Navigate(char abUp);

    // Creates the menu's tile.
    static Tile* Create();
    // Fades the open menu out.
    static void Close();
    // The menu's class id.
    static int32_t GetClassStatic();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    // The trait buttons.
    Tile* pTiles[18];
    // How far the list is scrolled (0 to 1), and where it is scrolling to; the two differ while it animates.
    float mfCurrentYScroll;
    float m_fDesiredYScroll;
    // The button the wheel or a gamepad moves through, and the one whose checkmark is shown; -1 for none.
    TILE_INDEX m_CurrentlySelectedTile;
    TILE_INDEX m_CurrentlyCheckedTile;
    // Tick counts: when the checkmark animation started, and the last update.
    uint32_t m_nCheckmarkAnimStartTime;
    uint32_t m_nLastMenuUpdateTime;

    static inline TraitSelectMenu* pMe;
};
