#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector3.h"

class TESObjectCELL;

// A point and the cell it is in.
struct PosAndCell {
	NiPoint3 pos;
	TESObjectCELL* cell;
};

// A point in a cell and the rotation it faces.
struct Location {
	PosAndCell posAndCell;
	NiVector3 rotation;
};
