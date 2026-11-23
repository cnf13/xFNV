#pragma once

#include "BSCore/BSStringT.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"

// List whose nodes are separate heap blocks from DFALL instead of the shared node pool; no state beyond the base.
template <class T>
class NiTList : public NiTPointerListBase<T, DFALL<NiTListItem<T>>> {
public:
    // An empty list (NiTListBase's constructor); NiTPointerListBase's destructor empties it.
    NiTList() {}
};

// The string list frees its nodes through the node's deleting destructor, which destroys the string. Defined in
// NiTPointerListBase.cpp.
template <>
void NiTPointerListBase<BSStringT<char>, DFALL<NiTListItem<BSStringT<char>>>>::DeleteItem(
    NiTListItem<BSStringT<char>>* pkItem
);
