#pragma once

#include "BSMenu/Tile.h"

// A tile that draws a plain rectangle.
class TileRect : public Tile {
public:
    TileRect();
    ~TileRect() override;

    // Shows the rectangle.
    void Init(Tile* apParent, const char* apName, Tile* apReplacedTile) override;
    NiNode* MakeNode() override;
    int32_t GetType() override;
    const char* GetTypeName() override;
    // A rectangle reacts to no trait.
    Tile* PostParse(int32_t aiTrait, float afValue, const char* axString) override;

    // Non-zero when the rectangle is a scissor (clip) rectangle.
    uint32_t bIsScissor;
};
