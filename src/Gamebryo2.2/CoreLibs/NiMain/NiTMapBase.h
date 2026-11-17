#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSHavok/bhkPoseArray.h"
#include "BSHavok/bhkRagdollShareData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderLibrary.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"

#include <stdint.h>

struct _TREEITEM;
class Actor;
class BGSLoadGameSubBuffer;
class BGSReconstructCellBlocksMap;
class BGSReconstructCellsMap;
class BGSReconstructCellSubBlocksMap;
class BGSReconstructFormsInFileMap;
class BGSReconstructFormsMap;
class bhkRigidBody;
class BSMultiBoundNode;
class BSTreeModel;
class ChangeData;
class CombatTargetData;
class CombatThreat;
class hkpRigidBody;
class IDebugTextPage;
class IDLE_ANIM_ROOT;
class NavMesh;
class NavMeshInfo;
struct NavMeshPOVData;
class NiNode;
class NiRenderedCubeMap;
class NiShader;
class NiVBChip;
class ObstacleData;
class ObstacleTaskData;
class PathingLOSGrid;
struct POVVertexData;
struct ReconstructFormData;
class ReferenceObstacleArray;
class ReservedCoverLocation;
class ShadowSceneLight;
class TESActorBase;
class TESFile;
class TESForm;
class TESGlobal;
class TESGrassAreaParam;
class TESObjectCELL;
class TESObjectREFR;
class TESObjectTREE;
class TESWaterForm;
class TEX_USER_DATA;
class WadingWaterData;
class WaterZone;

template <class T, uint32_t uiGrowStep>
class BSSimpleArray;
template <class TKEY, class TVAL>
class NiTMap;

char* ConstructWorldLocation(void* apThis, [[maybe_unused]] void* apEdx);
void* ConstructMapItem(void* apThis, [[maybe_unused]] void* apEdx);

// One entry of a hash map bucket's singly linked chain.
template <class TKEY, class TVAL>
class NiTMapItem : public NiMemObject {
public:
    // Constructs the value only; the map sets the key and the link.
    NiTMapItem() {}

    NiTMapItem* m_pkNext;
    TKEY m_key;
    TVAL m_val;
};

// Chained hash map: m_uiHashSize buckets of items from Allocator. The default hash is the key's integer value modulo
// the bucket count; derived maps override the hash, the key test and the item handling. A position is an item
// pointer (null past the end).
template <class Allocator, class TKEY, class TVAL>
class NiTMapBase : public NiMemObject {
public:
    // The item count rides in the allocator's tail, so an empty allocator adds no size.
    struct AntiBloatAllocator : public Allocator {
        uint32_t m_uiCount;
    };

    // auiHashSize cleared buckets, no items (Gamebryo's default 37 buckets).
    NiTMapBase(uint32_t auiHashSize = 37);
    // Derived maps empty the table in their own destructor (DeleteItem is theirs); this one frees the table.
    virtual ~NiTMapBase();

    virtual uint32_t KeyToHashIndex(TKEY key) const;
    virtual bool IsKeysEqual(TKEY key1, TKEY key2) const;
    virtual void SetValue(NiTMapItem<TKEY, TVAL>* pkItem, TKEY key, TVAL val);
    virtual void ClearValue(NiTMapItem<TKEY, TVAL>*) {}
    virtual NiTMapItem<TKEY, TVAL>* NewItem() = 0;
    virtual void DeleteItem(NiTMapItem<TKEY, TVAL>* pkItem) = 0;

    uint32_t GetCount() const;
    bool IsEmpty() const;

    // Replaces the value of an existing key, else adds the key at the head of its bucket.
    void SetAt(TKEY key, TVAL val);

    bool RemoveAt(TKEY key);
    void RemoveAll();
    void RemoveAll_0() {
        RemoveAll();
    }
    void RemoveAll_1() {
        RemoveAll();
    }
    void SetAt_B57EA0(TKEY key, TVAL val) {
        SetAt(key, val);
    }
    bool GetAt_A53C30(TKEY key, TVAL& val) const {
        return GetAt(key, val);
    }

    // Copies the key's value to val; false when the key is absent.
    bool GetAt(TKEY key, TVAL& val) const;

    NiTMapItem<TKEY, TVAL>* GetFirstPos() const;

    // Returns the item at pos and advances pos along its bucket, then to the next non-empty bucket.
    void GetNext(NiTMapItem<TKEY, TVAL>*& pos, TKEY& key, TVAL& val) const;

    // Sets the bucket count, empties the count and allocates the cleared buckets (definitions in NiTMapBase.cpp).
    NiTMapBase* Init(uint32_t auiHashSize);

    // The deleting destructors free the map through ourFree.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    uint32_t m_uiHashSize;
    NiTMapItem<TKEY, TVAL>** m_ppkHashTable;
    AntiBloatAllocator m_kAllocator;
};

template <class Allocator, class TKEY, class TVAL>
NiTMapBase<Allocator, TKEY, TVAL>::NiTMapBase(uint32_t auiHashSize) : m_uiHashSize(auiHashSize) {
    m_kAllocator.m_uiCount = 0;
    m_ppkHashTable = static_cast<NiTMapItem<TKEY, TVAL>**>(_NiMalloc(sizeof(NiTMapItem<TKEY, TVAL>*) * m_uiHashSize));
    std::memset(m_ppkHashTable, 0, sizeof(NiTMapItem<TKEY, TVAL>*) * m_uiHashSize);
}

// Empties the table (the derived map has already, with its own DeleteItem) and frees the buckets.
template <class Allocator, class TKEY, class TVAL>
NiTMapBase<Allocator, TKEY, TVAL>::~NiTMapBase() {
    RemoveAll();
    _NiFree(m_ppkHashTable);
}

// The constructor's work on a map already in place: the bucket count, an empty count and auiHashSize cleared buckets.
// The old buckets are deliberately not freed, as the game does.
template <class Allocator, class TKEY, class TVAL>
NiTMapBase<Allocator, TKEY, TVAL>* NiTMapBase<Allocator, TKEY, TVAL>::Init(uint32_t auiHashSize) {
    m_uiHashSize = auiHashSize;
    m_kAllocator.m_uiCount = 0;
    m_ppkHashTable = static_cast<NiTMapItem<TKEY, TVAL>**>(_NiMalloc(sizeof(NiTMapItem<TKEY, TVAL>*) * m_uiHashSize));
    std::memset(m_ppkHashTable, 0, sizeof(NiTMapItem<TKEY, TVAL>*) * m_uiHashSize);
    return this;
}

// The maps Fallout Shared/TESObjectCELL.cpp instantiates. Defined in Fallout Shared/TESObjectCELL.cpp.
using NiTMapBaseDFALLREFRNode = NiTMapBase<DFALL<NiTMapItem<TESObjectREFR*, NiNode*>>, TESObjectREFR*, NiNode*>;
using NiTMapBaseDFALLFormREFR = NiTMapBase<DFALL<NiTMapItem<TESForm*, TESObjectREFR*>>, TESForm*, TESObjectREFR*>;
using NiTMapBaseDFALLREFRBound = NiTMapBase<
    DFALL<NiTMapItem<TESObjectREFR*, NiPointer<BSMultiBoundNode>>>,
    TESObjectREFR*,
    NiPointer<BSMultiBoundNode>>;
using NiTMapBaseDFALLREFRREFR =
    NiTMapBase<DFALL<NiTMapItem<TESObjectREFR*, TESObjectREFR*>>, TESObjectREFR*, TESObjectREFR*>;

// The pathing line-of-sight grid map's base. Its constructor is defined in Fallout
// Shared/Pathfinding/PathingLOSGridMap.cpp.
using NiTMapBaseDFALLLOSGrid = NiTMapBase<DFALL<NiTMapItem<uint32_t, PathingLOSGrid*>>, uint32_t, PathingLOSGrid*>;

using NiTMapBasePtrREFRREFR = NiTMapBase<NiTPointerAllocator<size_t>, TESObjectREFR*, TESObjectREFR*>;
using NiTMapBasePtrWaterFormBool = NiTMapBase<NiTPointerAllocator<size_t>, TESWaterForm*, bool>;
using NiTMapBasePtrREFRWadingWater = NiTMapBase<NiTPointerAllocator<size_t>, TESObjectREFR*, WadingWaterData*>;

// The land's grass map (TESObjectLAND::LoadedLandData::pmGrassMap): its vtable, constructor and destructor are defined
// in Fallout Shared/TESLand.cpp.
using NiTMapBasePtrUIntGrassAreaParam = NiTMapBase<NiTPointerAllocator<size_t>, uint32_t, TESGrassAreaParam**>;

using NiTMapBasePtrUIntRefList = NiTMapBase<NiTPointerAllocator<size_t>, uint32_t, BSSimpleList<TESObjectREFR*>*>;
using NiTMapBasePtrIntCell = NiTMapBase<NiTPointerAllocator<size_t>, int32_t, TESObjectCELL*>;

// The map from a node to an unsigned int that Fallout Shared/BSMain/NiBSBoneLODController.cpp builds in place; its
// vtable and constructor for this instantiation are defined in that file.
using NiTMapBasePtrNodeUInt = NiTMapBase<NiTPointerAllocator<size_t>, NiNode*, uint32_t>;

using NiTMapBaseREFRBool = NiTMapBase<DFALL<NiTMapItem<TESObjectREFR*, bool>>, TESObjectREFR*, bool>;
using NiTMapBaseWaterZoneInt = NiTMapBase<DFALL<NiTMapItem<WaterZone*, int32_t>>, WaterZone*, int32_t>;

// The maps Fallout Shared/Pathfinding/NavMeshObstacleManager.cpp instantiates. Defined in Fallout
// Shared/Pathfinding/NavMeshObstacleManager.cpp.
using NiTMapBaseUIntRefObstacleArray = NiTMapBase<
    DFALL<NiTMapItem<uint32_t, NiPointer<ReferenceObstacleArray>>>,
    uint32_t,
    NiPointer<ReferenceObstacleArray>>;
using NiTMapBaseRigidBodyObstacle =
    NiTMapBase<DFALL<NiTMapItem<bhkRigidBody*, NiPointer<ObstacleData>>>, bhkRigidBody*, NiPointer<ObstacleData>>;
using NiTMapBaseUIntObstacleTask =
    NiTMapBase<DFALL<NiTMapItem<uint32_t, ObstacleTaskData*>>, uint32_t, ObstacleTaskData*>;

using NiTMapBaseTESObjectTREEModel = NiTMapBase<NiTPointerAllocator<size_t>, TESObjectTREE*, NiPointer<BSTreeModel>*>;

using NiTMapBaseHkpRigidBodyBool = NiTMapBase<DFALL<NiTMapItem<hkpRigidBody*, bool>>, hkpRigidBody*, bool>;
// The pointer-allocator maps Fallout Shared/TESObjectREFR.cpp builds in place; their vtables, constructors and
// destructors are defined in that file.
using NiTMapBasePtrREFRBool = NiTMapBase<NiTPointerAllocator<size_t>, TESObjectREFR*, bool>;
using NiTMapBasePtrUIntBool = NiTMapBase<NiTPointerAllocator<size_t>, uint32_t, bool>;

// The int-to-bool pointer map of Fallout Shared/TESObjectDOOR.cpp; its vtable and constructor are defined in that file.
using NiTMapBasePtrIntBool = NiTMapBase<NiTPointerAllocator<size_t>, int32_t, bool>;
// The loaded reference map's base (LoadedReferenceMap); its vtable, constructor and destructor for this instantiation
// are defined in Fallout Shared/BGSLoadedReferenceCollection.cpp.
using NiTMapBaseDFALLUInt32REFR = NiTMapBase<DFALL<NiTMapItem<uint32_t, TESObjectREFR*>>, uint32_t, TESObjectREFR*>;

// The navmesh info maps (NavMeshInfoMap's info map and its two cell key maps). Defined in Fallout
// Shared/Pathfinding/NavMeshInfoMap.cpp.
using NiTMapBasePtrUIntNavMeshInfo = NiTMapBase<NiTPointerAllocator<size_t>, uint32_t, NavMeshInfo*>;
using NiTMapBaseUIntWorldSpaceCellKeyMap = NiTMapBase<
    DFALL<NiTMapItem<uint32_t, NiTMap<uintptr_t, BSSimpleArray<NavMeshInfo*, 1024>*>*>>,
    uint32_t,
    NiTMap<uintptr_t, BSSimpleArray<NavMeshInfo*, 1024>*>*>;
using NiTMapBaseUIntCellKeyInfoMap = NiTMapBase<
    DFALL<NiTMapItem<uintptr_t, BSSimpleArray<NavMeshInfo*, 1024>*>>,
    uintptr_t,
    BSSimpleArray<NavMeshInfo*, 1024>*>;

// The navmesh maps: the triangle-to-obstacle map and the POV maps of Fallout Shared/Pathfinding/NavMesh.cpp and
// NavMesh_Util.cpp.
using NiTMapBaseUShortObstacleData =
    NiTMapBase<DFALL<NiTMapItem<uint16_t, NiPointer<ObstacleData>>>, uint16_t, NiPointer<ObstacleData>>;
using NiTMapBaseUShortNavMeshPOVData =
    NiTMapBase<DFALL<NiTMapItem<uint16_t, NavMeshPOVData>>, uint16_t, NavMeshPOVData>;
using NiTMapBaseFastVertHandlePOVVertexData =
    NiTMapBase<DFALL<NiTMapItem<FastNavMeshVertHandle, POVVertexData*>>, FastNavMeshVertHandle, POVVertexData*>;

// The save loader's reconstruction maps, built in Fallout/Misc/SaveLoad/BGSReconstructFormsInAllFilesMap.cpp.
using NiTMapBaseReconstructFormData =
    NiTMapBase<DFALL<NiTMapItem<uint32_t, ReconstructFormData>>, uint32_t, ReconstructFormData>;
using NiTMapBaseBGSReconstructFormsMap =
    NiTMapBase<DFALL<NiTMapItem<uint32_t, BGSReconstructFormsMap*>>, uint32_t, BGSReconstructFormsMap*>;
using NiTMapBaseBGSReconstructCellsMap =
    NiTMapBase<DFALL<NiTMapItem<uint32_t, BGSReconstructCellsMap*>>, uint32_t, BGSReconstructCellsMap*>;
using NiTMapBaseBGSReconstructCellSubBlocksMap =
    NiTMapBase<DFALL<NiTMapItem<uint32_t, BGSReconstructCellSubBlocksMap*>>, uint32_t, BGSReconstructCellSubBlocksMap*>;
using NiTMapBaseBGSReconstructCellBlocksMap =
    NiTMapBase<DFALL<NiTMapItem<uint32_t, BGSReconstructCellBlocksMap*>>, uint32_t, BGSReconstructCellBlocksMap*>;
using NiTMapBaseBGSReconstructFormsInFileMap =
    NiTMapBase<DFALL<NiTMapItem<TESFile*, BGSReconstructFormsInFileMap*>>, TESFile*, BGSReconstructFormsInFileMap*>;

// The maps Fallout Shared/Havok/BGSAcousticSpaceListener.cpp (the acoustic-space removal queue) and Fallout
// Shared/Havok/LoadedAreaBound.cpp (the loaded area's rigid bodies) build in place: their vtables and the constructor
// and destructor of each instantiation are defined in those files.
using NiTMapBaseREFRInt = NiTMapBase<DFALL<NiTMapItem<TESObjectREFR*, int32_t>>, TESObjectREFR*, int32_t>;
using NiTMapBaseRigidBodyUShort = NiTMapBase<DFALL<NiTMapItem<bhkRigidBody*, uint16_t>>, bhkRigidBody*, uint16_t>;

// The queued load sub-buffer maps' base (BGSSaveLoadQueuedSubBufferMap's three maps of form to sub-buffer): the vtable,
// constructor and destructor are defined in Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.cpp.
using NiTMapBaseDFALLTESFormBGSLoadGameSubBuffer =
    NiTMapBase<DFALL<NiTMapItem<TESForm*, BGSLoadGameSubBuffer>>, TESForm*, BGSLoadGameSubBuffer>;

// The player's random-door map (a form ID to a byte) and the cover map's reserved-location map (a cover location ID to
// a ReservedCoverLocation*): their vtables and destructors are defined in Fallout/AI/PlayerCharacter.cpp and
// Fallout/AI/Combat/CombatCoverMap.cpp.
using NiTMapBaseUIntUInt8 = NiTMapBase<DFALL<NiTMapItem<uint32_t, uint8_t>>, uint32_t, uint8_t>;
using NiTMapBaseUIntReservedCoverLocation =
    NiTMapBase<DFALL<NiTMapItem<uint32_t, ReservedCoverLocation*>>, uint32_t, ReservedCoverLocation*>;

// The object maps Fallout Shared/BipedAnim.cpp builds in place; their vtables, constructors and destructors are defined
// in that file.
using NiTMapBasePtrNiObjectNiObject = NiTMapBase<NiTPointerAllocator<uint32_t>, NiObject*, NiObject*>;
using NiTMapBasePtrNiObjectBool = NiTMapBase<NiTPointerAllocator<uint32_t>, NiObject*, bool>;

// The maps Fallout Shared/TESFile.cpp (the file's temp-ID maps, both allocators) and Fallout
// Shared/BGSDestructibleObjectForm.cpp (the loaded references' stage index) build in place; their vtables and
// per-instantiation constructor and destructor are defined in those files.
using NiTMapBasePtrUIntTESFile = NiTMapBase<NiTPointerAllocator<size_t>, uint32_t, TESFile*>;
using NiTMapBaseUIntTESFile = NiTMapBase<DFALL<NiTMapItem<uint32_t, TESFile*>>, uint32_t, TESFile*>;
using NiTMapBaseREFRUInt = NiTMapBase<DFALL<NiTMapItem<TESObjectREFR*, uint32_t>>, TESObjectREFR*, uint32_t>;

// The debug text page map of Fallout\Debug\DebugTextManager.cpp, the sound map of Fallout\Audio\FalloutAudio.cpp and
// the impact map of Fallout\Audio\ImpactMixer.cpp: their vtables and constructors are defined in those files.
using NiTMapBaseDFALLUIntDebugTextPagePtr =
    NiTMapBase<DFALL<NiTMapItem<uint32_t, IDebugTextPage*>>, uint32_t, IDebugTextPage*>;
using NiTMapBasePtrUIntSoundHandle = NiTMapBase<NiTPointerAllocator<size_t>, uint32_t, BSSoundHandle*>;
using NiTMapBasePtrIntUInt = NiTMapBase<NiTPointerAllocator<size_t>, int32_t, uint32_t>;

// Each instantiation has its own Init, which writes the instantiation's vtable, sets the bucket count, empties the
// count and allocates the cleared buckets (defined in NiTMapBase.cpp).
using NiTMapBaseTextureUserData = NiTMapBase<DFALL<NiTMapItem<NiTexture*, TEX_USER_DATA*>>, NiTexture*, TEX_USER_DATA*>;
using NiTMapBaseAVObjectTreeItem = NiTMapBase<DFALL<NiTMapItem<NiAVObject*, _TREEITEM*>>, NiAVObject*, _TREEITEM*>;
using NiTMapBaseStringIdleAnimRoot =
    NiTMapBase<DFALL<NiTMapItem<const char*, IDLE_ANIM_ROOT*>>, const char*, IDLE_ANIM_ROOT*>;
using NiTMapBaseUIntUInt = NiTMapBase<DFALL<NiTMapItem<uint32_t, uint32_t>>, uint32_t, uint32_t>;
using NiTMapBasePtrUIntChangeData = NiTMapBase<NiTPointerAllocator<uint32_t>, uint32_t, ChangeData*>;
using NiTMapBasePtrUIntVoid = NiTMapBase<NiTPointerAllocator<uint32_t>, uint32_t, void*>;
// The combat manager's map from an actor to a 64-bit value.
using NiTMapBaseActorInt64 = NiTMapBase<DFALL<NiTMapItem<Actor*, int64_t>>, Actor*, int64_t>;

// The maps Fallout Shared/BGSSceneInfo.cpp instantiates (a cell, a texture and an actor base to a bool, an int to an
// int, a shadow scene light to an int) and the maps other files build in place (a form ID to a byte, a cover location
// ID to a ReservedCoverLocation*, an actor to a 64-bit value, an actor to a CombatTargetData*, a form ID to a
// CombatThreat*): their vtables and the constructor, destructor and Init bodies, which write them. Defined in Fallout
// Shared/BGSSceneInfo.cpp and NiTMapBase.cpp.
using NiTMapBaseDFALLTESObjectCELLBool = NiTMapBase<DFALL<NiTMapItem<TESObjectCELL*, bool>>, TESObjectCELL*, bool>;
using NiTMapBaseDFALLNiTextureBool = NiTMapBase<DFALL<NiTMapItem<NiTexture*, bool>>, NiTexture*, bool>;
using NiTMapBaseDFALLTESActorBaseBool = NiTMapBase<DFALL<NiTMapItem<TESActorBase*, bool>>, TESActorBase*, bool>;
using NiTMapBaseDFALLIntInt = NiTMapBase<DFALL<NiTMapItem<int32_t, int32_t>>, int32_t, int32_t>;
using NiTMapBaseDFALLShadowSceneLightInt =
    NiTMapBase<DFALL<NiTMapItem<ShadowSceneLight*, int32_t>>, ShadowSceneLight*, int32_t>;
using NiTMapBaseDFALLActorCombatTargetData =
    NiTMapBase<DFALL<NiTMapItem<Actor*, CombatTargetData*>>, Actor*, CombatTargetData*>;
using NiTMapBaseDFALLUIntCombatThreat = NiTMapBase<DFALL<NiTMapItem<uint32_t, CombatThreat*>>, uint32_t, CombatThreat*>;

// The maps from a 4-byte key to a NiPointer; their SetAt is defined in NiTMapBase.cpp.
using NiTMapBaseUIntNiRefObjectPtr =
    NiTMapBase<DFALL<NiTMapItem<uint32_t, NiPointer<NiRefObject>>>, uint32_t, NiPointer<NiRefObject>>;

#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.inl"
