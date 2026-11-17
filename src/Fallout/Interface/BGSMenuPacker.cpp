#include "Fallout/Interface/BGSMenuPacker.h"

#include "BSMain/Setting.h"
#include "BSMenu/Tile.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
SettingT<INISettingCollection> fMenuPackerBufferSize("fMenuPackerBufferSize:InterfaceFX", 20.0f);
static SettingT<INISettingCollection> fMenuPackerOverflowZone("fMenuPackerBufferOverflowZone:InterfaceFX", 15.0f);

BGSMenuPacker::BGSMenuPacker() : PackedMenus(0, 1) {
	bPackFailed = 0;
}

BGSMenuPacker::~BGSMenuPacker() {}

void BGSMenuPacker::DumpInfo() {
	Interface::PrintLine("==== Begin PACK AREA Output ====");
	// The exe compares the index with the array's size signed (jge).
	for (int32_t i = 0; i < static_cast<int32_t>(PackedMenus.GetSize()); ++i) {
		PackedMenu* pMenu = PackedMenus[i];
		// The exe reads the parent tile's name before the tile's own.
		const char* pParentName = pMenu->pTile->GetParent()->GetName();
		Interface::PrintLine("Final packing for glow branch %s of tile %s: ", pMenu->pTile->GetName(), pParentName);
		Interface::PrintLine("  Pos: ( %f, %f )    [Original Pos: ( %f, %f )]", pMenu->x0, pMenu->y0, pMenu->x_original,
			pMenu->y_original);
		// The exe reads the model's z before its x.
		float fNodeZ = pMenu->pTile->GetModel()->GetTranslate().z;
		float fNodeX = pMenu->pTile->GetModel()->GetTranslate().x;
		Interface::PrintLine("  Dim: ( %f, %f )    [Node Pos: ( %f, %f )]", pMenu->w, pMenu->h, fNodeX, fNodeZ);
	}
	Interface::PrintLine("==== End PACK AREA Output ====");
}
