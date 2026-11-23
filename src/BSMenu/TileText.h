#pragma once

#include "BSMenu/Tile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// A tile that draws a line of text.
class TileText : public Tile {
public:
    TileText();
    ~TileText() override;

    void Init(Tile* apParent, const char* apName, Tile* apReplacedTile) override;
    NiNode* MakeNode() override;
    int32_t GetType() override;
    const char* GetTypeName() override;
    // Flags the text as changed when the string trait still matches or the justification trait differs, and
    // returns the tile then, otherwise null.
    Tile* PostParse(int32_t aiTrait, float afValue, const char* axString) override;

    static void operator delete(void* apMemory);

    float fTextWidth;
    // The text shape's own translation, as the font laid it out.
    NiPoint3 xOffset;
    // Set when the text was laid out as hypertext.
    bool bIsHyper;
};
