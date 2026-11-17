#pragma once

#include "BSCore/BSStringT.h"
#include "BSMenu/Tile.h"
#include "Reconstruction.hpp"

// A line of text the hacking terminal types out and blinks (IDA's name; the PDB's HackingMenu::TerminalIO has a
// member of this size at offset 0, an Animation, whose type the PDB does not name).
class HackingText {
public:
	// The default constructor is the exe's HackingText_ (0x726C90).
	HackingText();
	HackingText(const char* apString);
	~HackingText();

	// The PDB's AnimatingText (Fallout\Interface\AnimatingText.cpp) has these members at these offsets; src keeps the
	// IDA class name because gen code still names it.
	// Assigns the full text and resets the display buffer; returns *this (PDB operator=(apString), ret 4).
	HackingText& operator=(const char* apString);
	// Sets the tile value that shows the text and the one that toggles its visibility (PDB pStringTrait and
	// pVisTrait, both Tile::Value*).
	void SetTraits(Tile::Value* apStringTrait, Tile::Value* apVisTrait);
	// Sets the interval between displayed characters, at least 1.
	void SetDisplayInterval(uint32_t aiInterval);
	// Whether any of the aiType animation bits is set in ucFlags.
	bool GetAnimationMode(int32_t aiType);
	// Whether the text is displayed a character at a time (animation bit 1).
	bool GetDisplayMode();
	// Whether the text flashes (animation bit 2).
	bool GetFlashMode();
	// Frees the display buffer, allocates a zeroed one when there is text, and restarts the
	// display (thiscall, ret 0, 0x6FFDA0; private in the PDB).
	void ResetBuffer();
	HackingText* init();
	char Update();
	// Shows the whole text at once when it is being typed out and is not finished yet.
	void ForceFinish();
	// The tile of the text trait's or the visibility trait's owner (pParent of the Tile::Value at +4); the PDB's
	// AnimatingText::GetParent (thiscall, ret 0, 0x7001F0).
	Tile* GetParent();
	// The PDB's AnimatingText::GetFullText: the full text (strFullText's string).
	const char* GetFullText();
	HackingText* HackingText_();
	// Sets or clears the blink flag (2) of ucFlags.
	void SetShouldBlink(bool abBlink);
	// Sets or clears auiFlag in ucFlags.
	void SetFlagBit(uint32_t auiFlag, bool abSet);
	void SetVisibility(bool abOn);
	bool operator!=(const char* apString);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BSStringT<char> strText;
	// IDA name; not in the PDB (its pStringTrait, a Tile::Value*: the exe calls Tile::Value::SetString on it).
	Tile::Value* pTileText;
	Tile::Value* pTileValueVisibility;
	uint32_t uiDisplayRate;
	uint32_t uiFlashOnDuration;
	uint32_t uiFlashOffDuration;
	uint32_t uiNextDisplayTime;
	uint32_t uiNextBlinkTime;
	char* pCurrentlyDisplayedText;
	uint8_t ucFlags;
	bool bIsTextRemaining;
	uint8_t padding2A[2];
};

// HackingText::init's data: this file's .data run is g_iComputersDisplayRateForCurrentMenu and the two flash
// durations, between two copies of <new>'s _bad_alloc_Message.
inline int32_t g_iHackingFlashOnDuration = 750;

inline int32_t g_iHackingFlashOffDuration = 500;
