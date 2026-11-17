#pragma once

#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "BSMenu/Tile.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Menu;

// A menu of the interface: a tile menu plus the bookkeeping the interface keeps for it.
class Menu {
public:
	// The header of DATA\FINAL_MASTER_XML.DAT, then uiCount entries
	// of a FinalMasterXMLEntry followed by its uiSize bytes of XML.
	struct FinalMasterXMLHeader {
		uint32_t uiVersion;
		uint32_t uiCount;
		uint32_t uiSize;
	};

	struct FinalMasterXMLEntry {
		char acFileName[0x80];
		uint32_t uiSize;
	};

	// The tile menu of every menu, indexed by class id - 1001. The exe constructs it as a NiTPrimitiveArray<Tile*>
	// (vtable 0x1095684, at 0xFB3E72).
	using xMenuList_t = NiTPrimitiveArray<Tile*>;
	// The visibility flag of each menu class, 1001 (MessageMenu) to 1084: the exe indexes it by class id from
	// 0x11F308F, 1001 entries below its first.
	using MenusVisibleArray = bool[84];

	Menu();
	virtual ~Menu();

	// Remembers a tile the menu's XML created under its id (id, tile).
	virtual void AttachTileByID(int32_t, Tile*) {}
	// Mouse and focus events on one of the menu's tiles (id, target; drags: id, drag target, target).
	virtual void DoDownClick(int32_t, Tile*) {}
	virtual void DoClick(int32_t, Tile*) {}
	virtual void DoEnter(int32_t, Tile*) {}
	virtual void DoLeave(int32_t, Tile*) {}
	virtual void DoDragEnter(int32_t, Tile*, Tile*) {}
	virtual void DoDragLeave(int32_t, Tile*, Tile*) {}
	virtual void DoStillDown(int32_t, Tile*) {}
	virtual void DoStillDragOver(int32_t, Tile*) {}
	virtual void DoWheelMove(int32_t, Tile*) {}
	// Runs once per frame while the menu is up.
	virtual void DoIdle() {}
	// Handles a key press; true when the menu consumed it.
	virtual bool DoKey(int32_t) { return false; }
	// The class id of this menu (an Interface::enumMenus value).
	virtual int32_t GetClass() { return 0; }
	// Handles a gamepad button press or release (button, value); true when the menu consumed it.
	virtual bool DoGamepad(int32_t, float) { return false; }
	virtual bool DoGamepadUpEvent(int32_t, float) { return false; }
	// Called when one of the menu's XML traits changes.
	virtual void OnXMLDataChange(int32_t) {}
	// Called when a 360 controller is connected or disconnected (true when one is present).
	virtual void DoControllerSwitch(bool) {}

	// Fades the menu in through the root menu's kTileValue_MenuFade trait.
	void StartFadeIn();
	// Hides the menu at once unless it is hidden: clears its visibility and alpha, leaves the enter stack when it is
	// modal, and stops its fade timer.
	void InstantFadeOut();
	// Stops a fade out: shows the menu again at full alpha and stops its fade timer.
	void CancelFadeOut();
	// Hides the menu and fades it back in.
	void PrepForVisibility(bool abStayHidden);
	// Adopts the tile menu this menu renders through.
	void SetMenuTile(TileMenu* apMenu, bool abDontAddToStack);
	// Adds a template to the list of this menu's templates.
	void AddTemplate(Tile::TileTemplate* apTemplate);
	// Builds the menu template named axName under apParent and applies its traits; the first tile created, or
	// null. apLast, when given, becomes pLastTile.
	Tile* RenderTemplate(Tile* apParent, const char* axName, Tile* apLast);
	// The tile menu this menu renders through.
	TileMenu* GetMenuTile();
	// The menu's fade state (thiscall, ret 0, the one-field getter 0x59BB30 the exe shares).
	Interface::FADE_STATE GetFadeState();
	// Sets the menu's fade state (thiscall, ret 4, the one-field setter 0x7037C0 the exe shares).
	void SetFadeState(Interface::FADE_STATE afade);
	// Marks the menu as rendered by the rendered-menu pass.
	void SetRenderedMenu();
	void StartFadeOut();
	void CancelFadeIn();

	// The XML text of a menu file, read and compressed on first request (cached in XMLHash when abAdd is set).
	static XMLStorage* GetXML(const char* apFilename, bool abAdd);
	// The file name part of a path: everything after the last backslash.
	static const char* StripFileName(const char* apFilename);
	// The menu with the given class id, or null.
	static Tile* GetMenuByClass(int32_t aiClass);
	// The deepest menu level in the whole menu tree plus 2; puts the cursor above it.
	static float GetMaxDepth();
	// Drops the cached XML and parsed tree of a menu file. When the parsed tree owned its templates, apMenu takes
	// them over (bDeleteTemplates).
	static bool RemoveFromXMLArchive(const char* apFilename, Menu* apMenu);
	// Caches apBuild as the parsed tree of apFilename.
	static void SetParsedXML(const char* apFilename, Tile::BuildStorage* apBuild);
	// The XML text of apFilename (or of apFileData when given) with tabs and line breaks, comments, repeated
	// spaces and spaces around tag brackets dropped; a line break goes after every closing tag once a line passes
	// 100000 characters. Null when the file cannot be read.
	static XMLStorage* CompressAndStoreXML(const char* apFilename, const char* apFileData);
	// Parses every menu file packed in DATA\FINAL_MASTER_XML.DAT (version 100) and caches its tree; true when the
	// packed data was read whole.
	static bool LoadFinalMasterXML();
	// Registers the tile menu of a menu class in xMenuList.
	static void SetMenuByClass(int32_t aiClass, Tile* apTile);
	// True when the menu's tiles scale by the resolution ratio: its tile menu's id is &pipboymenu; (0x70) or its
	// class is &RaceSexMenu; (0x40C), &HackingMenu; (0x41F) or &ComputersMenu; (0x421). False without a tile menu.
	static bool ShouldScaleByResolutionRatio(Menu* apMenu);
	static void operator delete(void* apMemory);
	// Creates the interned "Tileptr" fixed string a tile's extra data is named with.
	static void InitSDM();
	static void KillSDM();
	static void FreeXMLArchive();
	static Tile::BuildStorage* GetParsedXML(const char* apFilename);
	static int32_t LoadMasterXMLFile();
	static bool CreateFinalMasterXMLFile();

	// The tile menu this menu renders through.
	TileMenu* pMenu;
	// The templates this menu was built from.
	BSSimpleList<Tile::TileTemplate*> xTemplates;
	// The last tile that was looked up in the menu's XML.
	Tile* pLastTile;
	// The class id of this menu, from the engine's GetClass virtual.
	int32_t iIsModal;
	// The pixel thickness of this menu's frame.
	int32_t iMenuThickness;
	// True while this menu is rendered by the rendered-menu pass.
	bool bRenderedMenu;
	// True when SetMenuTile must delete the templates with the menu.
	bool bDeleteTemplates;
	// The class id of this menu.
	int32_t iClass;
	// The fade state of this menu: one FADE_STATE bit, which IsMenuIDVisible tests against its mask.
	Interface::FADE_STATE xFadeState;

	static xMenuList_t xMenuList;
	// The extra data name under which a tile's model stores its tile.
	static inline NiFixedString* pTileptrStr;
	static inline MenusVisibleArray pMenusVisible{};
	// The XML text of each menu file and the parsed trees, keyed by file name.
	static NiTStringPointerMap<XMLStorage*> XMLHash;
	static NiTStringPointerMap<Tile::BuildStorage*> XMLParsedHash;
};
inline constexpr fnvd::Vtable Menu_vftable{};

inline int32_t g_traitMenuRank;
