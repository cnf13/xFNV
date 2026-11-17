#pragma once

#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "BSCore/BSSimpleArray.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class BGSSaveLoadFileEntry;

void CloseCreditsMenu();
void ContinuePlayingAfterCreditsCallback();

// The credits roll.
class CreditsMenu : public Menu {
public:
	CreditsMenu();
	~CreditsMenu() override;

	int32_t GetClass() override { return 0x417; }
	void AttachTileByID(int32_t aiID, Tile* apTile) override;
	void DoIdle() override;
	// Buttons -1 to 4 are ignored; any other closes the credits while they are the top menu and no message box is up.
	bool DoGamepad(int32_t aiButton, float afValue) override;

	// Same function as BookMenu::IsWellFormedMenu (0x73AF60): both test the tiles at +0x28 and +0x2C.
	bool IsWellFormedMenu();
	void InitCreditsRoot();
	void SkipLine();
	// The exe leaves eax as it falls out of the loop; its one caller (InitCreditsRoot) ignores it.
	void SkipKey();
	bool GetString(BSStringT<char>* a1);
	// Builds the node that shows the credits image Textures\Interface\Credits\<filename> at
	// the current position.
	NiNode* CreateImage(BSStringT<char>* apImageFilename, int32_t aiWidth, int32_t aiHeight, float afAlpha);

	// Builds and opens the credits menu.
	static bool Create();
	// The credits menu's class id.
	static int32_t GetClassStatic();
	// Ends the credits: clears the play-credits global and offers the post-game save (cdecl,
	// 0x75F750). IDA name endGame_MSGBox; matched to the PDB's CreditsMenu::Close by shape.
	static void Close();
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	// The layout is the PDB's; the exe's constructor (0x75F070) writes each field at the PDB's offset and the
	// factory allocates 0x80 bytes.
	Tile* pClickRect;
	Tile* pBackgroundTile;
	NiPointer<NiNode> spScrollRoot;
	uint32_t iFileOffset;
	uint32_t iFileSize;
	char* pFileData;
	FONTS eFont;
	int32_t iJustification;
	int32_t iX;
	int32_t iY;
	NiColorA Color;
	BSSimpleArray<NiFixedString, 1024> BackgroundTextureFilenames;
	uint32_t uiCreditsTotalHeight;
	float fBackgroundSwapInterval;
	float fBackgroundSwapTimer;
	uint32_t uiBackgroundIndexCounter;
};
inline constexpr fnvd::Vtable CreditsMenu_vftable{};

// The game settings CreditsMenu.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> fCreditsScrollSpeed;
extern SettingT<GameSettingCollection> gs_sContinuePlayingAfterCredits;
// The save Close offers to load after the credits: Close stores the entry
// BGSSaveLoadManager returns and ContinuePlayingAfterCreditsCallback loads it.
inline BGSSaveLoadFileEntry* hooverDamSave;
inline char bPlayingCreditsMenuMovie;
