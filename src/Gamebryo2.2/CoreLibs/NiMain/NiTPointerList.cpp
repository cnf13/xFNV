#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

#include "BSMain/BSMultiBound.h"
#include "BSMenu/Tile.h"
#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"

void UIntPointerList_RemoveAll(NiTPointerList<uint32_t>* apList) {
	apList->RemoveAll();
}
