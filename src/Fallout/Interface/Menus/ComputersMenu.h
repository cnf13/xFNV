#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "Fallout/Interface/ListBox.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

#include <stdint.h>

class BGSNote;
class BGSTerminal;
class ComputersMenu;
class HackingText;
struct TERMINAL_MENU_ITEM;
class TESObjectREFR;

using AnimatingText = HackingText;

class ComputersMenu : public Menu {
public:
    // The menu's phase (eMode).
    enum Mode : int32_t {
        CMM_INTRO = 0,
        CMM_WAIT = 1,
        CMM_NORMAL = 2,
    };

    // The list of the terminal text lines that type themselves out on screen. The one instance is created on first
    // use.
    class AnimatingTextManager {
    public:
        AnimatingTextManager();
        ~AnimatingTextManager();

        // Types out the queued text lines whose display time has come.
        void Update();
        // The number of text lines queued.
        uint32_t GetCount();
        void FlushList();
        void AddItem(const char* apString, Tile::Value* apStringTrait, Tile::Value* apVisTrait, int32_t aiWhere);
        // Walks xAnimatingText and finishes each line; its callers (DoClick, ShowNote, ...) pass pATManager.
        void ForceFinish();

        // Deletes the one instance, if any, and clears it.
        static void DeleteInstance();
        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }
        static ComputersMenu::AnimatingTextManager* GetInstance();

        NiTPointerList<AnimatingText*> xAnimatingText;

        static inline AnimatingTextManager* pInstance;
    };

    ComputersMenu();
    ~ComputersMenu() override;

    void AttachTileByID(int32_t, Tile*) override;
    void DoClick(int32_t, Tile*) override;
    void DoIdle() override;
    int32_t GetClass() override;
    // The PC inherits DoGamepad.
    bool DoGamepadUpEvent(int32_t, float) override;

    // Goes one level back in the terminal.
    void DisplayPreviousTerminal();
    // Blinks the cursor when its blink time has come.
    void FlashCursor();
    // Ends the intro and shows the terminal's main screen.
    void FinishIntro();
    void DisplayTerminal(char acAddToStack);
    bool IsWellFormedMenu(int32_t& ai);
    void DisplayIntro();
    // Plays the holotape note's sound.
    void PlayNoteSound(BGSNote* apNote);
    // Types out the next screenful of the note's text.
    void DisplayNoteText(BGSNote* apNote);
    void ShowImage(BGSNote* apNote);

    // Opens the terminal menu for apTerminalRef.
    static Tile* Create(TESObjectREFR* apTerminalRef);
    static int32_t GetClassStatic();
    // The traversal callback DoIdle runs over the menu items: enables each item's tile.
    static void PostDisplay(Tile* apTile, TERMINAL_MENU_ITEM* apMenuItem);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void Close();
    static void Redisplay(Tile* apTile, TERMINAL_MENU_ITEM* apMenuItem);

    Tile* pTiles[16];
    ListBox<TERMINAL_MENU_ITEM*> lbMenuItems;
    // The terminals the player went down through: ForceTerminalBack pops the head and shows the terminal now at the
    // head.
    BSSimpleList<BGSTerminal*> TerminalStack;
    uint32_t uiResultDisplayTimeout;
    Tile::Value* pCursorVis;
    BGSTerminal* pTerminal;
    TESObjectREFR* pTerminalRef;
    Mode eMode;
    BSStringT<char> strNoteText;
    const char* pNoteText;
    uint8_t bTutorial;
    bool bDisplayFlag;
    BSSoundHandle Sounds[3];
    // The queue of the hacking text lines.
    AnimatingTextManager* pATManager;
    // The note-display callbacks, one per note type. An entry is called as (this, apNote), so they are spelled as
    // function pointers here. The constructor fills the first three with PlayHolotape, ShowNote and ShowImage and
    // clears the last; DoClick indexes it by BGSNote::GetNoteType.
    void (*NoteActivationFn[4])(ComputersMenu* apThis, BGSNote* apNote);

    static inline ComputersMenu* pMe;
    static inline NiColorA SaveColor{0.0f, 0.0f, 0.0f, 0.0f};
};

// The tick count (plus 2000) ComputersMenu::DoIdle sets when it leaves mode 0 and compares
// against in mode 1.
inline int32_t g_computersSkipMainMenuTime;
inline int32_t g_iComputersDisplayRateForCurrentMenu = 25;
inline char g_bCurrentMenuEntryHasFlags2;
inline int32_t g_computersNextCursorBlinkTime;
inline int32_t g_computersNextMainMenuTextDisplayTime;
