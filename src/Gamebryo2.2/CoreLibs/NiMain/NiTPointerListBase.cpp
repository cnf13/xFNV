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
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

template <>
void NiTPointerListBase<BSStringT<char>, DFALL<NiTListItem<BSStringT<char>>>>::DeleteItem(
    NiTListItem<BSStringT<char>>* pkItem
) {
    pkItem->m_element = static_cast<const char*>(nullptr);
    delete pkItem;
}
