#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Actor;
class BGSLoadGameSubBuffer;
class BGSReconstructCellBlocksMap;
class BGSReconstructCellSubBlocksMap;
class BGSReconstructFormsInFileMap;
class BSMultiBoundNode;
template <class T, uint32_t uiGrowStep> class BSSimpleArray;
class IDLE_ANIM_ROOT;
class NavMesh;
class NavMeshInfo;
class NiNode;
class ReferenceObstacleArray;
class TESActorBase;
class TESFile;
class TESForm;
class TESGlobal;
class TESObjectCELL;
class TESObjectREFR;
class bhkLiquidAction;
class bhkRigidBody;
class hkpRigidBody;

void* AllocateVertexDistMapItem();
void* au_re_FormHeap_Allocate_27();

// Hash map whose items are separate heap blocks.
template <class TKEY, class TVAL> class NiTMap : public NiTMapBase<DFALL<NiTMapItem<TKEY, TVAL>>, TKEY, TVAL> {
public:
	// uiHashSize cleared buckets, no items (Gamebryo's default 37 buckets).
	NiTMap(uint32_t uiHashSize = 37) : NiTMapBase<DFALL<NiTMapItem<TKEY, TVAL>>, TKEY, TVAL>(uiHashSize) {}
	~NiTMap() override;

	NiTMapItem<TKEY, TVAL>* NewItem() override;
	void DeleteItem(NiTMapItem<TKEY, TVAL>* pkItem) override;

	// The RemoveAt the exe calls for the PDB's NiTMap<long, long> (TallGrassShaderProperty::CachedGrass's
	// pLowTerrainChunkMap): the shared pointer-allocator copy 0xE91FD0 (thiscall, ret 4), not the RTTI class
	// NiTMap<int,int>'s 0xA1E240 that src's RemoveAt calls. Only per-instance definitions exist.
	bool RemoveAt_00E91FD0(TKEY key);
	// Sets up the bucket table, then stores the map's vtable; returns this. Only
	// per-instantiation definitions exist.
	NiTMap* Init(uint32_t auiHashSize);
};

// Empties the map while its own DeleteItem is in place; NiTMapBase's destructor frees the buckets.
template <class TKEY, class TVAL> NiTMap<TKEY, TVAL>::~NiTMap() {
	this->RemoveAll();
}

// The item comes from the node allocator, which the exe calls out of line per instantiation.
template <class TKEY, class TVAL> NiTMapItem<TKEY, TVAL>* NiTMap<TKEY, TVAL>::NewItem() {
	return static_cast<NiTMapItem<TKEY, TVAL>*>(this->m_kAllocator.Allocate());
}

// Clears the value, so that a smart pointer drops its reference, and frees the item (Gamebryo's NiTMap::DeleteItem;
// the exe's copies assign each TVAL's value built from 0: 0x629EE0, 0x691A30, 0x844520, 0x913910 and the others).
template <class TKEY, class TVAL> void NiTMap<TKEY, TVAL>::DeleteItem(NiTMapItem<TKEY, TVAL>* pkItem) {
	pkItem->m_val = 0;
	this->m_kAllocator.Deallocate(pkItem);
}

// Sets up the bucket table; returns this (the exe's copies, 0x4A95D0, 0x96A280 and the others, then store the
// map's vtable, which the native constructor does).
template <class TKEY, class TVAL> NiTMap<TKEY, TVAL>* NiTMap<TKEY, TVAL>::Init(uint32_t auiHashSize) {
	NiTMapBase<DFALL<NiTMapItem<TKEY, TVAL>>, TKEY, TVAL>::Init(auiHashSize);
	return this;
}

// The pointer-allocator RemoveAt the exe calls for the PDB's NiTMap<long, long> (0xE91FD0): the same steps as
// NiTMapBase::RemoveAt.
template <class TKEY, class TVAL> bool NiTMap<TKEY, TVAL>::RemoveAt_00E91FD0(TKEY key) {
	return this->RemoveAt(key);
}

using NiTMapREFRBound = NiTMap<TESObjectREFR*, NiPointer<BSMultiBoundNode>>;

// The pathing line-of-sight map's base (PathingLOSMap.cpp): the exe's out-of-line destructor body, which writes the
// map's vtable. Defined in Fallout Shared/Pathfinding/PathingLOSMap.cpp.
using NiTMapBaseDFALLUInt32UInt32 = NiTMapBase<DFALL<NiTMapItem<uint32_t, uint32_t>>, uint32_t, uint32_t>;

// The navmesh info map's two cell key maps (NavMeshInfoMap's maps from a world space form ID and from a cell key to
// the navmeshes' infos): their vtables and the constructor the exe emits per instantiation. Defined in
// Fallout Shared/Pathfinding/NavMeshInfoMap.cpp.
using NiTMapUIntWorldSpaceCellKeyMap = NiTMap<uint32_t, NiTMap<uintptr_t, BSSimpleArray<NavMeshInfo*, 1024>*>*>;
using NiTMapUIntCellKeyInfoMap = NiTMap<uintptr_t, BSSimpleArray<NavMeshInfo*, 1024>*>;

// FixNormals' map from a source vertex to its VertexDist: the vtable and the exe's out-of-line constructor and
// destructor (0x4B9950, 0x4B9CD0); defined in Fallout Shared/GlobalFunc.cpp.
using NiTMapVertexDist = NiTMap<uint32_t, VertexDist>;

// The string-to-setting map (the game settings collection's map, vtable 0x1012278): the base's destructor and
// constructor and the map's own destructor are the exe's out-of-line bodies 0x405370, 0x405630 and 0x405310;
// defined in Fallout Shared/Magic/EffectItem.cpp.
using NiTMapBaseStringSetting = NiTMapBase<DFALL<NiTMapItem<const char*, Setting*>>, const char*, Setting*>;

using NiTMapStringSetting = NiTMap<const char*, Setting*>;

using NiTMapStringIdleRoot = NiTMap<const char*, IDLE_ANIM_ROOT*>;
// The map from a target object to the list of liquid actions following it (bhkLiquidAction::LiquidListMap): its
// destructor is out of line at 0xCBC740, and it stores these two vtables. Defined in bhkLiquidAction.cpp.
using NiTMapBaseAVObjectLiquidActionList = NiTMapBase<DFALL<NiTMapItem<NiAVObject*, BSSimpleList<bhkLiquidAction*>*>>,
	NiAVObject*, BSSimpleList<bhkLiquidAction*>*>;
using NiTMapAVObjectLiquidActionList = NiTMap<NiAVObject*, BSSimpleList<bhkLiquidAction*>*>;

// The map of Fallout Shared/Pathfinding/NavMeshObstacleManager.cpp: the exe's out-of-line constructor and
// destructor, which write this vtable. Defined there.
using NiTMapUIntRefObstacleArray = NiTMap<uint32_t, NiPointer<ReferenceObstacleArray>>;

// The save loader's reconstruction maps, built in Fallout/Misc/SaveLoad/BGSReconstructFormsInAllFilesMap.cpp: the
// exe's out-of-line constructor and destructor bodies, which write these vtables.
using NiTMapBGSReconstructCellSubBlocksMap = NiTMap<uint32_t, BGSReconstructCellSubBlocksMap*>;
using NiTMapBGSReconstructCellBlocksMap = NiTMap<uint32_t, BGSReconstructCellBlocksMap*>;
using NiTMapBGSReconstructFormsInFileMap = NiTMap<TESFile*, BGSReconstructFormsInFileMap*>;

// The queued load sub-buffer maps (BGSSaveLoadQueuedSubBufferMap's three maps of form to sub-buffer): the vtable and
// the exe's out-of-line destructor, which writes it. Defined in Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.cpp.
using NiTMapTESFormBGSLoadGameSubBuffer = NiTMap<TESForm*, BGSLoadGameSubBuffer>;

inline constexpr fnvd::Vtable NiTMap_TESObject_bool_vftable{};
inline constexpr fnvd::Vtable NiTMap_char_Setting_vftable{};
inline constexpr fnvd::Vtable NiTMap_TESObjectREFR_bool_vftable{};
inline constexpr fnvd::Vtable NiTMap_WaterZone_int_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_int_TESFile_vftable{};
inline constexpr fnvd::Vtable NiTMap_TESObjectREFR_unsigned_int_vftable{};
inline constexpr fnvd::Vtable NiTMap_char_TESForm_vftable{};
inline constexpr fnvd::Vtable NiTMap_NiTexture_TEX_USER_DATA_vftable{};
inline constexpr fnvd::Vtable NiTMap_TESObjectCELL_bool_vftable{};
inline constexpr fnvd::Vtable NiTMap_NiTexture_bool_vftable{};
inline constexpr fnvd::Vtable NiTMap_TESActorBase_bool_vftable{};
inline constexpr fnvd::Vtable NiTMap_int_int_vftable{};
inline constexpr fnvd::Vtable NiTMap_ShadowSceneLight_int_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_int_TESObjectREFR_vftable{};
inline constexpr fnvd::Vtable NiTMap_TESObjectREFR_NiNode_vftable{};
inline constexpr fnvd::Vtable NiTMap_TESForm_TESObjectREFR_vftable{};
inline constexpr fnvd::Vtable NiTMap_TESObjectREFR_TESObjectREFR_vftable{};
inline constexpr fnvd::Vtable NiTMap_NiAVObject_TESObjectCELL_QUEUED_ATTACH_vftable{};
inline constexpr fnvd::Vtable NiTMap_char_IDLE_ANIM_ROOT_vftable{};
inline constexpr fnvd::Vtable NiTMap_TESObjectREFR_int_vftable{};
inline constexpr fnvd::Vtable NiTMap_bhkRigidBody_unsigned_short_vftable{};
inline constexpr fnvd::Vtable NiTMap_hkpRigidBody_bool_vftable{};
inline constexpr fnvd::Vtable NiTMap_char_NiPointer_BSFaceGenModelMap_Entry_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_short_NiPointer_ObstacleData_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_short_NavMeshPOVData_vftable{};
inline constexpr fnvd::Vtable NiTMap_FastNavMeshVertHandle_POVVertexData_vftable{};
inline constexpr fnvd::Vtable NiTMap_NavMesh_NiPointer_NiNode_vftable{};
inline constexpr fnvd::Vtable NiTMap_TESObjectCELL_NiPointer_NiNode_vftable{};
inline constexpr fnvd::Vtable NiTMap_bhkRigidBody_NiPointer_ObstacleData_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_int_ObstacleTaskData_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_int_PathingLOSGrid_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_int_unsigned_int_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_int_IDebugTextPage_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_int_ReconstructFormData_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_int_BGSReconstructFormsMap_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_int_BGSReconstructCellsMap_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_int_Actor_vftable{};
inline constexpr fnvd::Vtable NiTMap_long_long_int_vftable{};
inline constexpr fnvd::Vtable NiTMap_TESGlobal_bool_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_int_unsigned_char_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_int_ReservedCoverLocation_vftable{};
inline constexpr fnvd::Vtable NiTMap_Actor_long_long_vftable{};
inline constexpr fnvd::Vtable NiTMap_Actor_CombatTargetData_vftable{};
inline constexpr fnvd::Vtable NiTMap_NiAVObject_TREEITEM_vftable{};
inline constexpr fnvd::Vtable NiTMap_unsigned_int_CombatThreat_vftable{};
inline constexpr fnvd::Vtable NiTMap_char_NiPointer_bhkPoseArray_vftable{};
inline constexpr fnvd::Vtable NiTMap_char_NiPointer_bhkRagdollShareData_vftable{};
inline constexpr fnvd::Vtable NiTMap_NiAVObject_short_vftable{};
inline constexpr fnvd::Vtable NiTMap_char_char_vftable{};
inline constexpr fnvd::Vtable NiTMap_TESObjectREFR_NiPointer_BSMultiBoundNode_vftable{};
inline constexpr fnvd::Vtable NiTMap_UInt_WorldSpaceCellKeyMap_vftable{};
inline constexpr fnvd::Vtable NiTMap_UInt_CellKeyInfoMap_vftable{};
inline constexpr fnvd::Vtable NiTMap_uint_VertexDist_vftable{};
inline constexpr fnvd::Vtable NiTMapBaseStringSetting_vftable{};
inline constexpr fnvd::Vtable NiTMapBaseAVObjectLiquidActionList_vftable{};
inline constexpr fnvd::Vtable NiTMapAVObjectLiquidActionList_vftable{};
inline constexpr fnvd::Vtable NiTMap_UInt_NiPointer_ReferenceObstacleArray_vftable{};
inline constexpr fnvd::Vtable NiTMap_BGSReconstructCellSubBlocksMap_vftable{};
inline constexpr fnvd::Vtable NiTMap_BGSReconstructCellBlocksMap_vftable{};
inline constexpr fnvd::Vtable NiTMap_BGSReconstructFormsInFileMap_vftable{};
inline constexpr fnvd::Vtable NiTMap_TESForm_BGSLoadGameSubBuffer_vftable{};
inline constexpr fnvd::Vtable NiTMap_LowProcess_LP_LOCK_DATA_vftable{};
inline constexpr fnvd::Vtable NiTMap__D3DFORMAT_NiPixelFormat_vftable{};
