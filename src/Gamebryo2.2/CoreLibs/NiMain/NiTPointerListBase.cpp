#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"

#include "BSAudio/AudioLoadTask.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/BSOcclusionPlane.h"
#include "BSMenu/DebugText.h"
#include "BSMenu/Tile.h"
#include "BSShader/ShadowSceneLight.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainChunk.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/AI/BaseProcess.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX92DBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

// The string list's node: a heap block, constructed (DFALL<NiTListItem<BSStringT<char>>>::Allocate, 0x71E120).
template <> NiTListItem<BSStringT<char>>* NiTPointerListBase<BSStringT<char>>::NewItem() {
	void* pMemory = ourMalloc(sizeof(NiTListItem<BSStringT<char>>));
	return pMemory ? new (pMemory) NiTListItem<BSStringT<char>> : nullptr;
}

// Empties the string, then frees the node through its deleting destructor, which destroys the string and gives the
// block back to the heap with ourFree, the pair of NewItem's ourMalloc.
template <> void NiTPointerListBase<BSStringT<char>>::DeleteItem(NiTListItem<BSStringT<char>>* pkItem) {
	pkItem->m_element = static_cast<const char*>(nullptr);
	pkItem->~NiTListItem();
	ourFree(pkItem);
}

// Same function as NiTPointerListBase<unsigned int>::AddHead (0x76B660), registered there. The exe's pooled list
// copy (NewItem 0x49FA80), where src's NiTPointerListBase<NiAVObject *>::AddHead is the heap list's (0x4700D0): src's
// lists have no allocator parameter to tell them apart. Natively there is one list: the template's AddHead.
void NiAVObjectList_AddHead(NiTPointerListBase<NiAVObject*>* apList, NiAVObject* const& arElement) {
	apList->AddHead(arElement);
}
