#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "Fallout/Interface/Menus/HackingText.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class BGSTerminal;
class HackingMenu;
class InterfaceManager;
class TESObjectREFR;

// Returns the number of characters the two words share at the same positions; it only reads them.
int32_t get_num_matching_characters(const char* a1, const char* apString);
uint8_t Max(uint8_t a1, uint8_t a2);
int32_t RandomCompare(void* a1, void* a2);

class HackingMenu : public Menu {
public:
    // The game's stage, in eStage. Signed: DoGamepad tests `eStage < HMS_INGAME` as a signed compare.
    enum Stage : int32_t {
        HMS_INTRO = 0,
        HMS_PREGAME = 1,
        HMS_INGAME = 2,
        HMS_SHUTDOWN = 3,
        HMS_FINISHED = 4,
    };

    // One line of the terminal's output: the animated text and the tiles that show it.
    struct TerminalIO {
        TerminalIO();
        TerminalIO(const char* apString, Tile* apParentTile, const char* apTemplateName, int32_t aiY, uint8_t aucRate);
        ~TerminalIO();

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        HackingText Animation;
        Tile* pTextTile;
        Tile* pPromptTile;
    };

    // A list of heap copies of words.
    class WordList : public BSSimpleList<char*> {
    public:
        WordList();
        ~WordList();
    };

    // One row of a byte grid: iSize bytes at ucData. SetSize frees ucData, stores the size and allocates that many
    // bytes with ourMalloc.
    class GridRow {
    public:
        // Each row of Grid's new[] is built with this constructor, which clears both fields.
        GridRow();
        ~GridRow();

        // Frees ucData, stores aiSize and allocates that many bytes with ourMalloc.
        void SetSize(uint32_t aiSize);
        // Sizes the row to aCopy's size and copies aCopy's bytes.
        GridRow& operator=(const GridRow& aCopy);
        // Writes the column of each byte equal to aiVal to apCols and returns how many there are.
        uint32_t GetTally(uint32_t aiVal, uint8_t* apCols);
        // Counts each byte value below 32 and folds the counts into the result byte.
        uint32_t GetMaxTally();

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }
        static void operator delete[](void* apMemory) {
            ourFree(apMemory);
        }

        uint8_t* ucData;
        uint32_t iSize;
    };

    // A grid of iSize rows of iSize bytes. The constructor allocates iSize GridRow elements into pData and sizes each
    // row to iSize.
    class Grid {
    public:
        // An empty grid, for a local MakeSubGrid fills later: ~Grid reads pData, so it starts null.
        Grid() : pData(nullptr), iSize(0) {}
        Grid(uint32_t aiSize);
        Grid(const Grid& aCopy);
        ~Grid();

        // Frees the rows, then allocates aCopy's row count of rows and copies each row. Nothing copies a Grid.
        Grid& operator=(const Grid& aCopy);
        // Builds a ucSize-square grid of the cells at rows and columns apIndices[0..ucSize), copied into apResult
        // through the copy constructor; returns apResult.
        Grid* MakeSubGrid(Grid* apResult, uint8_t* apIndices, uint8_t ucSize) const;

        GridRow* pData;
        // Unsigned: the constructor's row loop compares against it as an unsigned value.
        uint32_t iSize;
    };

    HackingMenu();
    ~HackingMenu() override;

    void AttachTileByID(int32_t, Tile*) override;
    void DoClick(int32_t, Tile*) override;
    void DoLeave(int32_t, Tile*) override;
    void DoIdle() override;
    int32_t GetClass() override;
    bool DoGamepad(int32_t, float) override;
    bool DoGamepadUpEvent(int32_t, float) override;

    void TransitionToComputersMenu();
    void AccessTerminal();
    void DoShutdown();
    int32_t GetSelectedTileIndex();
    // Empties the typed entry and clears the entry tiles' user1 and user4 values.
    void ClearEntry();
    Tile* FindTile(uint32_t aiIndex, uint32_t aiCol);
    bool IsWellFormedMenu(int32_t& ai);
    void ScrollLog(uint32_t aiTicks, const char** pNewEntries);
    // Picks the aiWordsWant candidate words and the password.
    void Prepare(uint32_t aiWordsWant);
    // Writes ausVal as "0x" and four hex digits to apResult.
    void MakeHexString(uint16_t ausVal, char* apResult);
    void SetupWordTiles();
    // Recurses over aGrid's sub-grids (MakeSubGrid) to bound the guesses needed; Prepare passes its local Grid.
    uint32_t GetGuaranteedPossible(const Grid& aGrid);
    void DoIntro();
    void DoInGame();
    void RemoveDud();
    void HandleCursorBlink();
    void UpdateCursorPos();
    // Moves the cursor tile to the end of apIO's text tile.
    void UpdateScrollingCursor(TerminalIO* apIO);
    void AddAttemptsRemainingSquares();

    // Creates the menu for apTerminalRef.
    static Tile* Create(TESObjectREFR* apTerminalRef);
    static void Close();
    static int32_t GetClassStatic();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void* GetGarbageString(void* apDest, const char* apSrc);

    // IOLines holds 46 lines; the constructor zeroes all of them, and FindTile and Prepare scan 46.
    TerminalIO* IOLines[46];
    HackingText xEntryText;
    Tile* pTiles[8];
    BSSoundHandle Sounds[5];
    // The log's and the guesses' line tiles.
    NiTPointerList<Tile*> xLogTiles;
    NiTPointerList<Tile*> xGuessTiles;
    // The candidate words.
    BSSimpleList<char*> wordList;
    NiTPointerList<uint32_t> xUsedBonuses;
    // The terminal form of pTerminalRef (Create calls BGSTerminal's GetHackMiniGameValue and
    // GetMinSkillRequiredToHack on it).
    BGSTerminal* pTerminal;
    TESObjectREFR* pTerminalRef;
    char* pPassword;
    uint32_t iNumGuesses;
    uint32_t iTotGuesses;
    uint32_t iWordsHave;
    uint32_t iWordLength;
    uint32_t* iBufferPos;
    uint32_t iYStart;
    uint32_t iYInterval;
    uint32_t iLastOffset;
    // The tile the cursor was over at the last selection change.
    Tile* pLastTarget;
    Stage eStage;
    // The tile the cursor is on; it comes from FindTile and the IO lines' text tiles, and only Tile's traits and
    // position are used.
    Tile* pOverTile;
    uint32_t iCursorX;
    uint32_t dwAccessTime;
    Tile::Value* pCursorVis;
    bool bSuppressEntry;
    bool bUsedReset;
    bool bTutorial;
    uint8_t padding1DB[1];

    static inline HackingMenu* pMe;
    static inline int32_t iNextAllowedHackTime;
    static inline InterfaceManager* pim;
    static BSSimpleList<BSSoundHandle> listSoundsToPlay;
    // The system colour 3 the menu saves on creation and restores on destruction.
    static inline NiColorA SaveColor{0.0f, 0.0f, 0.0f, 0.0f};
};

extern SettingT<INISettingCollection> fHackingMinSkillHard;
extern SettingT<INISettingCollection> fHackingMinSkillVeryEasy;
extern SettingT<INISettingCollection> fHackingMinSkillVeryHard;
extern SettingT<INISettingCollection> fHackingMinSkillEasy;
extern SettingT<INISettingCollection> fHackingMinSkillAverage;
inline int32_t time_11D969C;
inline char* pCloseBrackets = const_cast<char*>(")]}>");
inline char* Str = const_cast<char*>("([{<");
