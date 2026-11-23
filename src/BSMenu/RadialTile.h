#pragma once

#include "BSMenu/TileImage.h"

// An image tile with a radial rather than rectangular bounds test; no extra PC fields.
class RadialTile : public TileImage {
public:
    RadialTile();
    ~RadialTile() override;

    bool SpecialBoundsCheck(float fx, float fy) override;

    static void operator delete(void* apMemory);
};
