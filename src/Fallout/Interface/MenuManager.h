#pragma once

#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

class Menu;
class Tile;
class TileMenu;

// Builds, reloads and deletes the menus by class id.
class MenuManager {
public:
	// The constructor.
	MenuManager();
	~MenuManager();

	// The menu drawn on top of the others, or null.
	Menu* GetFrontmostMenu();
	// Whether the menu of class aiClass is open (thiscall, ecx unused, ret 4, al,
	// 0x720F90).
	bool IsMenuOpen(int32_t aiClass);
	// Allocates and constructs the menu of class aiMenuClass; null for an unknown class.
	Menu* CreateMenuByClass(int32_t aiMenuClass);
	Tile* ReloadMenu(int32_t aiMenuClass);
	bool DeleteMenu(int32_t aiMenuClass);

	// Creates the menu manager.
	static void Init();
	// The manager singleton; created on demand when abCreate is set.
	static MenuManager* Instance(bool abCreate);
	static void Release();
	static void operator delete(void* apMemory);
	static void Free();

	static inline MenuManager* pInstance;
};

// The common interface of the menus' list boxes. Every slot of the exe's vtable (0x1070418) is _purecall.
class ListBoxBase {
public:
	ListBoxBase();

	// Highlights the item of the given tile; true when the tile belongs to this list box.
	virtual bool Highlight(Tile* apTile) = 0;
	// The highlighted item, or null.
	virtual void* GetHighlightedItem() = 0;
	// The tile the highlight moves to for a gamepad button, or null.
	virtual Tile* GetHighlightTarget(int32_t aiButton) = 0;
	// True when the list box belongs to the menu.
	virtual bool BelongsTo(Menu* apMenu) = 0;
	virtual void ScrollToHighlight() = 0;
	// The tile of the item at aiIndex (the aiIndex-th node when abLiteral); the exe's slot pops 8 (0x7A30A0).
	virtual Tile* GetTileAt(uint32_t aiIndex, bool abLiteral) = 0;

	// Adds apListBox to InterfaceManager's open list boxes.
	static void Append(ListBoxBase* apListBox);
	// Removes apListBox from InterfaceManager's open list boxes.
	static void Remove(ListBoxBase* apListBox);
};
inline constexpr fnvd::Vtable ListBoxBase_vftable{};
