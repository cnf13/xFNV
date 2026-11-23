#pragma once

#include <cstdint>

class TESObjectCELL;
class TESWorldSpace;

// What an exterior cell load task carries: the cell's grid coordinates, its worldspace, the cell once it exists and
// whether it has loaded.
struct CELL_LOAD_DATA {
    int32_t iX;
    int32_t iY;
    TESWorldSpace* pWorld;
    TESObjectCELL* pCell;
    bool bLoaded;
};
