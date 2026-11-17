#pragma once

#include "BSMain/BSMultiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class HackingText;
class TESObjectREFR;
class Tile;

// Pointer list with the engine's pooled node allocator; no state beyond the base.
template <class T> class NiTPointerList : public NiTPointerListBase<T> {
public:
	// An empty list (NiTListBase's constructor); NiTPointerListBase's destructor empties it.
	NiTPointerList() {}

	// The deleting destructor frees the list through ourFree.
	static void operator delete(void* apMemory) { ourFree(apMemory); }
};

// Not named in the PDB or IDA; name chosen here. The pool-allocated NiTPointerList<unsigned int>'s RemoveAll
// (HackingMenu::xUsedBonuses): the exe calls the pointer lists' shared copy (thiscall, ret 0, 0x4ED900), where
// NiTList<unsigned int>, the same src base type, has its own (0xB58530).
void UIntPointerList_RemoveAll(NiTPointerList<uint32_t>* apList);

using AnimatingText = HackingText;  // the PDB's name for src's HackingText
