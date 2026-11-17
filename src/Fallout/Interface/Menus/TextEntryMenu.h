#pragma once

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMenu/Menu.h"
#include "Fallout/Interface/EditableTextField.h"

// The menu that asks the player for a line of text (TextEditMenu.xml; the start menu opens one for the redeem code).
class TextEditMenu : public Menu {
public:
	TextEditMenu();
	~TextEditMenu() override;

	void AttachTileByID(int32_t aiID, Tile* apTile) override;
	void DoClick(int32_t aiID, Tile* apTile) override;
	void DoIdle() override;
	bool DoKey(int32_t aiKey) override;
	// The menu's class id, 1051.
	int32_t GetClass() override;

	// True once text has been accepted: the static entered-text string, which the constructor clears and
	// the accept button fills, holds a buffer. The body ignores this.
	bool IsDone();
	void UpdateText();

	// The static entered-text string's buffer, read through the shared NiPointer
	// operator->.
	static const char* GetText();
	// Clears the static entered-text string through BSStringT<char>::operator=(nullptr) and returns it;
	// PlayerNameEntryMenu's constructor and DoIdle call it once the name is taken.
	static BSStringT<char>* ResetText();
	static int32_t GetMenuID();
	static void Close();
	static char Create(const char* apTitle, const char* apDefaultText, bool (*apFunc)(const char*));
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	// AttachTileByID and DoClick index these three pointers as one array by tile id (0 to 2): [0] the text (a
	// TileText), [1] the OK button (a TileImage), [2] the title (a TileText).
	Tile* pTiles[3];
	// The text field the menu edits: the destructor runs EditableTextField::Destruct on it.
	EditableTextField EditField;
	// Create stores its callback argument here; UpdateText calls it with EditField.GetEnteredText() and tests the
	// result.
	bool (*menuCallback)(const char*);

	static inline TextEditMenu* pInstance;
	// The entered text: the constructor clears it, the OK button fills it.
	static BSStringT<char> sEnteredText;
};
