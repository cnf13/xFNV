#pragma once

#include "BSMenu/Menu.h"

class QuantityMenu : public Menu {
public:
	// The callback the menu's owner passed to Create, called with the chosen quantity.
	using ResultCallback = void (*)(int32_t aiQuantity);

	QuantityMenu();
	~QuantityMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	void DoIdle() override;
	int32_t GetClass() override;
	bool DoGamepad(int32_t, float) override;
	bool DoGamepadUpEvent(int32_t, float) override;

	bool IsWellFormedMenu();

	// Hides the open quantity menu and fades it out, at once while a rendered menu shows.
	static void Close();
	// Opens the quantity menu for at most aiMaxAmount, starting at aiStartAmount; afpCallback receives the chosen
	// quantity.
	static TileMenu* Create(int32_t aiMaxAmount, ResultCallback afpCallback, int32_t aiStartAmount);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	// The menu's class ID. It is called with no object and reads none, so it is static.
	static int32_t GetClass_007ABDA0();

	Tile* pTiles[6];
	float fPrevQuantity;

	static inline QuantityMenu* pMe;
	static inline ResultCallback fpResultCallback{};
};
