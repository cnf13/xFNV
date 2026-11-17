#pragma once

#include "BSCore/MemoryManager.h"
#include "BSMenu/Menu.h"
#include "Reconstruction.hpp"

class BGSMessage;

class TutorialMenu : public Menu {
public:
	TutorialMenu();
	~TutorialMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	// The PC inherits DoIdle, unlike the Xbox PDB.
	int32_t GetClass() override;
	bool DoGamepad(int32_t, float) override;
	bool DoGamepadUpEvent(int32_t, float) override;

	void NextPage();
	void PrevPage();
	void SetPage(uint32_t a1);
	// Shows apMessage's name and description.
	void SetTitleAndText(BGSMessage* a1);
	// Sets trait 0xFAF of pTiles[2] to iCurrentPage > 0 and of pTiles[1] to
	// iCurrentPage + 1 < iNumPages (thiscall, ret 0, 0x7E9010; IDA's _sub_7E9010).
	void UpdatePrevNext();

	// Hides the open tutorial menu, fades it out and goes back in the start menu.
	static void Close();
	// Opens the tutorial menu on apMessage.
	static bool Create(BGSMessage* apMessage, bool abHelpManual);
	// The menu id, 0x423 (eTutorialMenu; cdecl, 0x7CC6D0).
	static int32_t GetClassStatic();
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	Tile* pTiles[8];
	uint32_t iCurrentPage;
	uint32_t iNumPages;
	BSSimpleList<TESForm*>* pHelpManual;

	static inline TutorialMenu* pMe;
	static SettingT<INIPrefSettingCollection> bHelpEnabled;
};
inline constexpr fnvd::Vtable TutorialMenu_vftable{};
