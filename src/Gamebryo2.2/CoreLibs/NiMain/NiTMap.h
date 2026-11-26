#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

#include <stdint.h>

class Actor;
class BGSLoadGameSubBuffer;
class BGSReconstructCellBlocksMap;
class BGSReconstructCellSubBlocksMap;
class BGSReconstructFormsInFileMap;
class bhkLiquidAction;
class bhkRigidBody;
class BSMultiBoundNode;
class hkpRigidBody;
class IDLE_ANIM_ROOT;
class NavMesh;
class NavMeshInfo;
class NiNode;
class ReferenceObstacleArray;
class Setting;
class TESActorBase;
class TESFile;
class TESForm;
class TESGlobal;
class TESObjectCELL;
class TESObjectREFR;
struct VertexDist;

template <class T, uint32_t uiGrowStep>
class BSSimpleArray;

void* AllocateVertexDistMapItem();
void* au_re_FormHeap_Allocate_27();

// Hash map whose items are separate heap blocks.
template <class TKEY, class TVAL>
class NiTMap : public NiTMapBase<DFALL<NiTMapItem<TKEY, TVAL>>, TKEY, TVAL> {
public:
    // uiHashSize cleared buckets, no items (Gamebryo's default 37 buckets).
    NiTMap(uint32_t uiHashSize = 37) : NiTMapBase<DFALL<NiTMapItem<TKEY, TVAL>>, TKEY, TVAL>(uiHashSize) {}
    ~NiTMap() override;

    NiTMapItem<TKEY, TVAL>* NewItem() override;
    void DeleteItem(NiTMapItem<TKEY, TVAL>* pkItem) override;

    // Sets up the bucket table, then stores the map's vtable; returns this. Only
    // per-instantiation definitions exist.
    NiTMap* Init(uint32_t auiHashSize);
};

// Empties the map while its own DeleteItem is in place; NiTMapBase's destructor frees the buckets.
template <class TKEY, class TVAL>
NiTMap<TKEY, TVAL>::~NiTMap() {
    this->RemoveAll();
}

// The item comes from the node allocator.
template <class TKEY, class TVAL>
NiTMapItem<TKEY, TVAL>* NiTMap<TKEY, TVAL>::NewItem() {
    return static_cast<NiTMapItem<TKEY, TVAL>*>(this->m_kAllocator.Allocate());
}

// Clears the value, so that a smart pointer drops its reference, and frees the item.
template <class TKEY, class TVAL>
void NiTMap<TKEY, TVAL>::DeleteItem(NiTMapItem<TKEY, TVAL>* pkItem) {
    pkItem->m_val = 0;
    this->m_kAllocator.Deallocate(pkItem);
}

// Sets up the bucket table; returns this.
template <class TKEY, class TVAL>
NiTMap<TKEY, TVAL>* NiTMap<TKEY, TVAL>::Init(uint32_t auiHashSize) {
    NiTMapBase<DFALL<NiTMapItem<TKEY, TVAL>>, TKEY, TVAL>::Init(auiHashSize);
    return this;
}

using NiTMapREFRBound = NiTMap<TESObjectREFR*, NiPointer<BSMultiBoundNode>>;

// The pathing line-of-sight map's base. Defined in Fallout Shared/Pathfinding/PathingLOSMap.cpp.
using NiTMapBaseDFALLUInt32UInt32 = NiTMapBase<DFALL<NiTMapItem<uint32_t, uint32_t>>, uint32_t, uint32_t>;

// The navmesh info map's two cell key maps (NavMeshInfoMap's maps from a world space form ID and from a cell key to the
// navmeshes' infos). Defined in Fallout Shared/Pathfinding/NavMeshInfoMap.cpp.
using NiTMapUIntWorldSpaceCellKeyMap = NiTMap<uint32_t, NiTMap<uintptr_t, BSSimpleArray<NavMeshInfo*, 1024>*>*>;
using NiTMapUIntCellKeyInfoMap = NiTMap<uintptr_t, BSSimpleArray<NavMeshInfo*, 1024>*>;

// FixNormals' map from a source vertex to its VertexDist.
using NiTMapVertexDist = NiTMap<uint32_t, VertexDist>;

// The string-to-setting map (the game settings collection's map); the base's destructor and constructor and the map's
// own destructor are defined in Fallout Shared/Magic/EffectItem.cpp.
using NiTMapBaseStringSetting = NiTMapBase<DFALL<NiTMapItem<const char*, Setting*>>, const char*, Setting*>;

using NiTMapStringSetting = NiTMap<const char*, Setting*>;

using NiTMapStringIdleRoot = NiTMap<const char*, IDLE_ANIM_ROOT*>;
// The map from a target object to the list of liquid actions following it (bhkLiquidAction::LiquidListMap). Defined in
// bhkLiquidAction.cpp.
using NiTMapBaseAVObjectLiquidActionList = NiTMapBase<
    DFALL<NiTMapItem<NiAVObject*, BSSimpleList<bhkLiquidAction*>*>>,
    NiAVObject*,
    BSSimpleList<bhkLiquidAction*>*>;
using NiTMapAVObjectLiquidActionList = NiTMap<NiAVObject*, BSSimpleList<bhkLiquidAction*>*>;

// The map of Fallout Shared/Pathfinding/NavMeshObstacleManager.cpp; its constructor and destructor are defined there.
using NiTMapUIntRefObstacleArray = NiTMap<uint32_t, NiPointer<ReferenceObstacleArray>>;

// The save loader's reconstruction maps, built in Fallout/Misc/SaveLoad/BGSReconstructFormsInAllFilesMap.cpp.
using NiTMapBGSReconstructCellSubBlocksMap = NiTMap<uint32_t, BGSReconstructCellSubBlocksMap*>;
using NiTMapBGSReconstructCellBlocksMap = NiTMap<uint32_t, BGSReconstructCellBlocksMap*>;
using NiTMapBGSReconstructFormsInFileMap = NiTMap<TESFile*, BGSReconstructFormsInFileMap*>;

// The queued load sub-buffer maps (BGSSaveLoadQueuedSubBufferMap's three maps of form to sub-buffer). Defined in
// Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.cpp.
using NiTMapTESFormBGSLoadGameSubBuffer = NiTMap<TESForm*, BGSLoadGameSubBuffer>;
