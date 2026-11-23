#pragma once

#include "BSMenu/TileRect.h"

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

    // pMenu = m.
    void SetMenu(Menu* m);

    // The menu this tile is the root of.
    Menu* pMenu;
};
