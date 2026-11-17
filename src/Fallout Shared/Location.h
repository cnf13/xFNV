#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector3.h"
#include "Reconstruction.hpp"

class TESObjectCELL;

// A point and the cell it is in.
struct PosAndCell {
	NiPoint3 pos;
	TESObjectCELL* cell;
};

// IDA name; not in the PDB. A point in a cell and the rotation it faces. IDA's decompiler text uses it only as the
// type of Script::StartConversation's (0x5C8740) two pointers to PackageLocation locals, which it passes to
// Actor::InitiateDialogue; it is declared, with IDA's layout, so that text can be translated.
struct Location {
	PosAndCell posAndCell;
	NiVector3 rotation;
};
