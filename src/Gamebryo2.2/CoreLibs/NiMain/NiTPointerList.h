#pragma once

#include "BSMain/BSMultiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"

#include <stdint.h>

class HackingText;
class TESObjectREFR;
class Tile;

// Pointer list with the engine's pooled node allocator; no state beyond the base.
template <class T>
class NiTPointerList : public NiTPointerListBase<T> {
public:
    // An empty list (NiTListBase's constructor); NiTPointerListBase's destructor empties it.
    NiTPointerList() {}

    // The deleting destructor frees the list through ourFree.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
};

using AnimatingText = HackingText;
