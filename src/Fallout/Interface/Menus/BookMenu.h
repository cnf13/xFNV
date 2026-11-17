#pragma once

#include "BSMenu/Menu.h"
#include "Reconstruction.hpp"

class Actor;
class TESObjectBOOK;
class TESObjectREFR;

class BookMenu : public Menu {
public:
	BookMenu();
	~BookMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoDownClick(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	// The PC inherits DoEnter and DoLeave, unlike the Xbox PDB.
	void DoStillDown(int32_t, Tile*) override;
	void DoWheelMove(int32_t, Tile*) override;
	// The PC inherits DoIdle, unlike the Xbox PDB.
	bool DoKey(int32_t) override;
	int32_t GetClass() override;

	bool IsWellFormedMenu();
	void Close();
	// Inferred type: a1: TESObjectBOOK* from 0x73aecf (parameter 0 of BookMenu::Create)
	void SetupTextAndButtons(TESObjectBOOK* a1, Actor* a2);
	void UseWorkbook();

	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static Tile* Create(TESObjectBOOK* apBook, TESObjectREFR* apRef);

	Tile* pScrollBar;
	Tile* pScrollMarker;
	// The PC pActivator field is not typed in src.
	TESObjectREFR* unk30;
	// The book being read; the PDB names it pScroll (and calls the TESObjectREFR* above pBook).
	TESObjectBOOK* pBook;
	float fScrollOffset;
};
inline constexpr fnvd::Vtable BookMenu_vftable{};
