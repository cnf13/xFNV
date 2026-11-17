#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Tile;

// Packs the glow branches of the menus' tiles into one area so that they do not overlap.
class BGSMenuPacker {
public:
	// One tile's branch: where it was packed to, its size, and where it was before (DumpInfo, 0x700350, prints them
	// as these pairs). The PDB's pretrans_x and pretrans_y after pTile are not in IDA's PC type, whose 0x1C size
	// no exe function confirms.
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
	// The exe never touches the next five fields (the constructor does not set them); IDA has them as a 17-byte gap.
	// Their names and types are the PDB's (the sizes floats); no exe access confirms or contradicts them.
	float w;
	float h;
	float max_w;
	float max_h;
	bool dump_info_flag;
	bool bPackFailed;
};

// The packer's array of packed menus and the primitive array that wraps it. The registration macros take the class
// as one token.
using BGSMenuPackerPackedMenuArray =
	NiTArray<BGSMenuPacker::PackedMenu*, NiTMallocInterface<BGSMenuPacker::PackedMenu*>>;
using BGSMenuPackerPackedMenuPrimitiveArray = NiTPrimitiveArray<BGSMenuPacker::PackedMenu*>;

// The two arrays' vtable instances, as the exe writes them in their constructors.
inline constexpr fnvd::Vtable NiTArray_BGSMenuPacker_PackedMenu_vftable{};
inline constexpr fnvd::Vtable NiTPrimitiveArray_BGSMenuPacker_PackedMenu_vftable{};

// The game settings BGSMenuPacker.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> fMenuPackerBufferSize;
