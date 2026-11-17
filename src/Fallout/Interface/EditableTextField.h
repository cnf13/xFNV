#pragma once

#include "BSCore/BSStringT.h"
#include "Reconstruction.hpp"

// A text field the player types into: the entered text, the text shown (with the cursor) and the cursor state.
class EditableTextField {
public:
	EditableTextField();

	// Whether the field is taking keyboard input.
	bool EditingText();
	void SetMaxSize(int32_t iSize);
	// Applies a typed character or an editing key (the 0x80000000-based codes) to the text.
	void HandleKey(int32_t iKey);
	// True when apString fits iMaxPixelSize in the field's font (thiscall, ret 4, 0x717230;
	// the caller tests al).
	bool StringIsOk(char* sString);
	void SetText(const char* sDefaultText);
	void SetEditing(bool bEditingText);
	// Blinks the cursor every 500 ms (thiscall, ret 0; its one caller ignores eax).
	void BlinkCursor();
	// The text shown: sText with the cursor character at iCurPos while editing, or " " when empty.
	// Inferred type: return: char* from 0x7e6714 (passed to Tile::Set 1)
	char* GetDisplayText();
	// Same function as hkPadSpu<char *>::operator char * (0x458B50), registered there: sText's string.
	const char* GetEnteredText();

	BSStringT<char> sText;
	BSStringT<char> sDisplayText;
	uint32_t iCurPos;
	uint32_t iMaxPixelSize;
	uint32_t iFontIdx;
	uint32_t uLastBlinkTime;
	bool bCursorOn;
	bool bEditing;
	bool bClearOnKey;
	uint8_t padding23;
};
