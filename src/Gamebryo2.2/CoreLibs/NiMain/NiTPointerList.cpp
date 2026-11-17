#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

#include "BSMain/BSMultiBound.h"
#include "BSMenu/Tile.h"
#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"

// Same function as NiTPointerListBase<BSOcclusionPlane *>::RemoveAll (0x4ED900), registered there. The exe's pooled
// list copy, where src's NiTPointerListBase<unsigned int>::RemoveAll is the other instance's (0xB58530): src's lists
// have no allocator parameter to tell them apart. Natively there is one list: the template's RemoveAll.
void UIntPointerList_RemoveAll(NiTPointerList<uint32_t>* apList) {
	apList->RemoveAll();
}
