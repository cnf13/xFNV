#pragma once

#include "Reconstruction.hpp"

class TESObjectCELL;
class TESWorldSpace;

// What an exterior cell load task carries: the cell's grid coordinates, its worldspace, the cell once it exists and
// whether it has loaded. IDA's names for the PDB's EXTERIOR_CELL_DATA (members iX, iY, pWorld, pCell, bDone;
// harness/tools/ida_type_aliases.tsv). ExteriorCellLoader::PreLoadCellData (0x5285C0) allocates 20 bytes for it.
struct CELL_LOAD_DATA {
	int32_t iX;
	int32_t iY;
	TESWorldSpace* pWorld;
	TESObjectCELL* pCell;
	bool bLoaded;
};
