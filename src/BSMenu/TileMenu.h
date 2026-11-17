#pragma once

#include "BSMenu/TileRect.h"
#include "Reconstruction.hpp"

class Menu;

// The root tile of a menu tree.
class TileMenu : public TileRect {
public:
	TileMenu(Tile* p, char* x);
	~TileMenu() override;

	void Init(Tile* apParent, const char* apName, Tile* apReplacedTile) override;
	// Builds this tile's Gamebryo node.
	NiNode* MakeNode() override;
	int32_t GetType() override;
	const char* GetTypeName() override;
	Tile* PostParse(int32_t aiTrait, float afValue, const char* axString) override;
	void ForceTextureRelease() override;

	// pMenu = m. Same function as TESTopicInfo::SetSpeaker (0x409480), registered there.
	void SetMenu(Menu* m);

	// The menu this tile is the root of.
	Menu* pMenu;
};
inline constexpr fnvd::Vtable TileMenu_vftable{};
