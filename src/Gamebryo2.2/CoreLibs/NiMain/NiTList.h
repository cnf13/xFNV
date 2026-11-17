#pragma once

#include "BSCore/BSStringT.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"

// List whose nodes are separate heap blocks (the engine's NiTPointerListBase<DFALL<NiTListItem<T>>, T>); no state
// beyond the base.
template <class T> class NiTList : public NiTPointerListBase<T> {
public:
	// An empty list (NiTListBase's constructor); NiTPointerListBase's destructor empties it.
	NiTList() {}
};
