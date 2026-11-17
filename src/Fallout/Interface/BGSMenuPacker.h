#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

#include <stdint.h>

class Tile;

// Packs the glow branches of the menus' tiles into one area so that they do not overlap.
class BGSMenuPacker {
public:
    // One tile's branch: where it was packed to, its size, and where it was before (DumpInfo prints them as these
    // pairs).
    struct PackedMenu {
        float x0;
        float y0;
        float w;
        float h;
        float x_original;
        float y_original;
        Tile* pTile;
    };

    BGSMenuPacker();
    ~BGSMenuPacker();

    // Prints each packed menu's tile names, packed and original position, size and model position.
    void DumpInfo();

    NiTPrimitiveArray<PackedMenu*> PackedMenus;
    // The next five fields are never read or written, and the constructor does not set them.
    float w;
    float h;
    float max_w;
    float max_h;
    bool dump_info_flag;
    bool bPackFailed;
};

// The packer's array of packed menus and the primitive array that wraps it.
using BGSMenuPackerPackedMenuArray =
    NiTArray<BGSMenuPacker::PackedMenu*, NiTMallocInterface<BGSMenuPacker::PackedMenu*>>;
using BGSMenuPackerPackedMenuPrimitiveArray = NiTPrimitiveArray<BGSMenuPacker::PackedMenu*>;

extern SettingT<INISettingCollection> fMenuPackerBufferSize;
