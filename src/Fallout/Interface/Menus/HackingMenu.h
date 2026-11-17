#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "Fallout/Interface/Menus/HackingText.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class BGSTerminal;
class HackingMenu;
class InterfaceManager;
class TESObjectREFR;

// IDA name; not in the PDB. The characters the two words share at the same positions; the exe (0x768A10) only
// reads them.
// Inferred type: a1: char* from 0x768a40 (passed to BSstrlen 0); apString: char* from 0x768a33 (passed to BSstrlen 0)
int32_t get_num_matching_characters(const char* a1, const char* apString);
uint8_t Max(uint8_t a1, uint8_t a2);
int32_t RandomCompare(void* a1, void* a2);

class HackingMenu : public Menu {
public:
	// The game's stage, in eStage. Signed: DoGamepad tests `eStage < HMS_INGAME` with `jge`.
	enum Stage : int32_t {
		HMS_INTRO = 0,
		HMS_PREGAME = 1,
		HMS_INGAME = 2,
		HMS_SHUTDOWN = 3,
		HMS_FINISHED = 4,
	};

	// One line of the terminal's output: the animated text and the tiles that show it (the exe's IDA name is
	// HackingMenu::Entry; its fields are IDA's text/tile/templateTile).
	struct TerminalIO {
		TerminalIO();
		TerminalIO(const char* apString, Tile* apParentTile, const char* apTemplateName, int32_t aiY, uint8_t aucRate);
		~TerminalIO();

		static void operator delete(void* apMemory) { ourFree(apMemory); }

		HackingText Animation;
		Tile* pTextTile;
		Tile* pPromptTile;
	};

	// The PDB's WordList: a list of heap copies of words (Add is AppendStr 0x76B240, Flush DeleteAll 0x76B2A0,
	// Shuffle Randomize 0x7687B0).
	class WordList : public BSSimpleList<char*> {
	public:
		WordList();
		~WordList();
	};

	// One row of a byte grid: iSize bytes at ucData (PDB names; SetSize 0x768920 frees ucData, stores the size at +4
	// and allocates that many bytes with ourMalloc).
	class GridRow {
	public:
		// The exe builds each row of Grid's new[] with 0x96A2D0, which clears both fields.
		GridRow();
		~GridRow();

		// Frees ucData, stores aiSize and allocates that many bytes with ourMalloc.
		void SetSize(uint32_t aiSize);
		// Sizes the row to aCopy's size and copies aCopy's bytes.
		// No src code copies a GridRow.
		GridRow& operator=(const GridRow& aCopy);
		// Writes the column of each byte equal to aiVal to apCols and returns how many there
		// are, zero-extended into eax (thiscall, ret 8, 0x76AE30).
		uint32_t GetTally(uint32_t aiVal, uint8_t* apCols);
		// Counts each byte value below 32 and folds the counts with 0x76AF60 into the
		// result byte, zero-extended into eax (thiscall, ret 0, 0x76AE90).
		uint32_t GetMaxTally();

		static void operator delete(void* apMemory) { ourFree(apMemory); }
		static void operator delete[](void* apMemory) { ourFree(apMemory); }

		uint8_t* ucData;
		uint32_t iSize;
	};

	// A grid of iSize rows of iSize bytes (PDB names; the constructor 0x768810 stores its argument at +4, allocates
	// that many 8-byte GridRow elements with a count prefix into pData and sizes each row to it).
	class Grid {
	public:
		// An empty grid, for a local MakeSubGrid fills later: ~Grid reads pData, so it starts null.
		Grid() : pData(nullptr), iSize(0) {}
		Grid(uint32_t aiSize);
		Grid(const Grid& aCopy);
		~Grid();

		// Frees the rows, then allocates aCopy's row count of rows and copies each row.
		// No src code copies a Grid.
		Grid& operator=(const Grid& aCopy);
		// The PDB's Grid MakeSubGrid(apIndices, ucSize) const: a ucSize-square grid of the
		// cells at rows and columns apIndices[0..ucSize), copied into apResult (the exe's hidden result) through the
		// copy constructor 0x76B090; returns apResult (thiscall, ret 0xC, 0x76AF90).
		Grid* MakeSubGrid(Grid* apResult, uint8_t* apIndices, uint8_t ucSize) const;

		GridRow* pData;
		// Unsigned: the constructor's row loop compares against it with jae.
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
	// Inferred type: return: int32_t: signed uses from 0x767752
	int32_t GetSelectedTileIndex();
	// Empties the typed entry and clears the entry tiles' user1 and user4 values.
	void ClearEntry();
	Tile* FindTile(uint32_t aiIndex, uint32_t aiCol);
	bool IsWellFormedMenu(int32_t& ai);
	void ScrollLog(uint32_t aiTicks, const char** pNewEntries);
	// Picks the aiWordsWant candidate words and the password.
	void Prepare(uint32_t aiWordsWant);
	// Writes ausVal as "0x" and four hex digits to apResult (thiscall, ret 8, 0x768970).
	// The exe returns nothing its caller reads (eax holds the last digit's dword).
	void MakeHexString(uint16_t ausVal, char* apResult);
	void SetupWordTiles();
	// Recurses over aGrid's sub-grids (MakeSubGrid) to bound the guesses needed (thiscall,
	// ret 4, 0x769520; Prepare passes its local Grid).
	uint32_t GetGuaranteedPossible(const Grid& aGrid);
	void DoIntro();
	void DoInGame();
	void RemoveDud();
	// IDA name; not in the PDB. The exe's only caller (DoIdle) ignores eax.
	void HandleCursorBlink();
	void UpdateCursorPos();
	// Moves the cursor tile to the end of apIO's text tile.
	void UpdateScrollingCursor(TerminalIO* apIO);
	void AddAttemptsRemainingSquares();

	// Creates the menu for apTerminalRef.
	static Tile* Create(TESObjectREFR* apTerminalRef);
	static void Close();
	static int32_t GetClassStatic();
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void* GetGarbageString(void* apDest, const char* apSrc);

	// The PDB's IOLines: 39 lines in the Xbox build, 46 in the PC exe (the constructor zeroes 0xB8 bytes at
	// IOLines, and FindTile and Prepare scan 46).
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
	// The tile the cursor was over at the last selection change (the PDB's pLastTarget; Tile* in the PDB).
	Tile* pLastTarget;
	Stage eStage;
	// The tile the cursor is on; the exe gets it from FindTile and the IO lines' text tiles and only uses Tile's
	// traits and position.
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

inline constexpr fnvd::Vtable HackingMenu_vftable{};

// The game settings HackingMenu.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> fHackingMinSkillHard;
extern SettingT<INISettingCollection> fHackingMinSkillVeryEasy;
extern SettingT<INISettingCollection> fHackingMinSkillVeryHard;
extern SettingT<INISettingCollection> fHackingMinSkillEasy;
extern SettingT<INISettingCollection> fHackingMinSkillAverage;
inline int32_t time_11D969C;
inline char* pCloseBrackets = const_cast<char*>(")]}>");
inline char* Str = const_cast<char*>("([{<");
