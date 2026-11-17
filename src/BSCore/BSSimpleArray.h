#pragma once

#include "Reconstruction.hpp"
#include "Fallout/AI/Combat/GroupMemberData.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "BSCore/BSStringT.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVLink.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVNode.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout/AI/Combat/CombatSearchDoor.h"
#include "Fallout Shared/Pathfinding/FastNavMeshEdgeLocationProxy.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/NavmeshTriFan.h"
#include "Fallout Shared/Pathfinding/RemoveIndex.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/AiFormulas.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"

#include <stdint.h>

class Actor;
class Animation;
class BGSLoadFormBuffer;
class BSPortal;
class NavMesh;
struct NavMeshSearchLink;
class PathingCoverLocation;
class QueuedFile;
class TESBoundObject;
class TESIdleForm;
class TESObjectCELL;
class TESObjectREFR;
class TESObjectWEAP;
class Tile;
class hkpCollidable;

#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Fallout Shared/BGSSceneInfo.h"

// Destroys auiCount 0x54-byte cover locations
// from apItems on: the PathingCoverLocation array's _DestructItems.
// Inferred type: apItems: PathingCoverLocation* from 0x6dbe01 (BSSimpleArray<PathingCoverLocation, 1024>::pBuffer (this
// of BSSimpleArray<PathingCoverLocation, 1024>::Clear)), its members used at 0x6dbee0; auiCount: uint32_t: unsigned
// uses from 0x6dbf01
void _DestroyItems(PathingCoverLocation* apItems, uint32_t auiCount);
// IDA name. The PDB's BSSimpleArray<unsigned short,1024>::DefaultEqualFunction<unsigned int>, a member template src's
// BSSimpleArray does not declare: the exe compares the zero-extended word at a1 with the dword at a2.
bool apFunc(const uint16_t* a1, const uint32_t* a2);
// Inferred type: a3: uint32_t: unsigned uses from 0x5efe04
void MoveItems_0(uint16_t* a1, uint16_t* a2, uint32_t a3);
// IDA name; not in the PDB, which has the function as BSSimpleArray<unsigned short,1024>::DefaultEqualFunction<unsigned
// short> (const references). The exe passes its address to BSSimpleArray<uint16_t, 1024>::Find as the EqualFunction
// (0x69DEE0, 0x6987D0, 0x69A940, 0x6BCC60, 0x6BE6F0): the parameters are that type's const pointers; it only reads.
bool return_equals(const uint16_t* a1, const uint16_t* a2);
// Not named in the PDB or IDA; name chosen here. BSSimpleArray<BSStringT<char>>'s
// _DestructItems (stdcall, ret 8): 0x7A86B0 on each of the auiCount strings from apItems on.
// Inferred type: apItems: BSStringT<char>* from 0x7a1c51 (BSSimpleArray<BSStringT<char>, 1024>::pBuffer (this of
// BSSimpleArray<BSStringT<char>, 1024>::Clear)), its members used at 0x7a63b0; auiCount: uint32_t: unsigned uses from
// 0x7a63d1
void DestroyStringRange(BSStringT<char>* apItems, uint32_t auiCount);

// Root that gives BSSimpleArray its vtable pointer; the vtable slots are the destructor, _Allocate, _Deallocate and
// _Reallocate.
class BSSimpleArrayRoot {
public:
	// Sort's, SortedInsert's and SortedFind's compare (the CRT's qsort and bsearch form): <0, 0 or >0 as *apOne
	// sorts before, with or after *apTwo. It does not depend on BSSimpleArray's arguments, so it lives here: a
	// non-dependent member typedef of the template keeps the template's own name (`BSSimpleArray::CompareFunction`),
	// which code outside the class cannot spell; every BSSimpleArray<T, N>::CompareFunction still names it.
	using CompareFunction = int32_t (*)(const void* apOne, const void* apTwo);

	virtual ~BSSimpleArrayRoot();
};

// Growable array. The second parameter is the step size above which the buffer grows and shrinks linearly instead of
// by doubling and halving.
template <class T, uint32_t uiGrowStep = 1024> class BSSimpleArray : public BSSimpleArrayRoot {
public:
	// Find's compare: whether *apItem equals *apValue.
	using EqualFunction = bool (*)(const T* apItem, const T* apValue);

	// An empty array: the exe's _InitialSetup(0, 0).
	BSSimpleArray() : pBuffer(nullptr), iSize(0), iReservedSize(0) {}

	// Reserves aiReserveSize items (at least aiInitialSize) and constructs the first aiInitialSize: the exe's
	// _InitialSetup(aiReserveSize, aiInitialSize), written out here.
	BSSimpleArray(uint32_t aiReserveSize, uint32_t aiInitialSize) : pBuffer(nullptr), iSize(0), iReservedSize(0) {
		if (aiInitialSize > aiReserveSize) {
			aiReserveSize = aiInitialSize;
		}
		if (aiReserveSize) {
			pBuffer = _Allocate(aiReserveSize);
			iReservedSize = aiReserveSize;
		}
		if (aiInitialSize) {
			_ConstructItems(pBuffer, aiInitialSize);
			iSize = aiInitialSize;
		}
	}

	explicit BSSimpleArray(uint32_t aiInitialSize) : BSSimpleArray(aiInitialSize, aiInitialSize) {}
	~BSSimpleArray() override;

	// vtable slot 1: a buffer of auiCount items.
	virtual T* _Allocate(uint32_t aiSizeInItem);

	// vtable slot 2: releases apItems.
	virtual void _Deallocate(T* apBuffer);

	// vtable slot 3: resizes apItems to auiCount items.
	virtual T* _Reallocate(T* apBuffer, uint32_t aiNewSizeInItem);

	// True when no spare capacity is left.
	bool _ShouldGrow() const;

	bool IsEmpty() const;

	// True when the array is small enough compared to its capacity that the buffer should shrink.
	bool _ShouldShrink() const;

	T& GetAt(int32_t aiIndex);
	// The PDB's const overload, for an array reached through a const object.
	const T& GetAt(int32_t aiIndex) const;
	T& operator[](uint32_t auiIndex);
	// The PDB's const overload, for an array reached through a const object.
	const T& operator[](uint32_t auiIndex) const;

	// Capacity to grow to when the buffer is full.
	uint32_t _GetNewSizeForGrow() const;

	// Capacity to shrink to when the array is mostly empty.
	uint32_t _GetNewSizeForShrink() const;

	// Releases the buffer through the (engine virtual) _Deallocate.
	void _DeallocateBuffer();

	// Destroys auiCount items from apItems on (T's destructor; nothing for plain items).
	void _DestructItems(T* apFirstAddr, uint32_t aiCount);

	// Constructs auiCount items in place from apItems on (T's default constructor; plain items stay unset).
	void _ConstructItems(T* apFirstAddr, uint32_t aiCount);

	// Copies auiCount items element by element in the direction that is safe for overlapping ranges.
	void _MoveItems(T* apDestAddr, T* apSrcAddr, uint32_t aiCount);

	// Moves the first auiCopyCount items into a buffer of auiNewSize items. Leaves iReservedSize to the caller unless
	// there was no buffer yet.
	void _ReallocateBuffer(uint32_t aiNewSize, uint32_t aiCopyCount);

	// Empties the array; abFreeMemory also releases the buffer.
	void Clear(bool abFreeMemory);

	// Grows or shrinks the array to auiNewSize items; abFreeMemory lets a shrink down to a quarter release memory.
	void SetSize(uint32_t aiNewSize, bool abFreeMemory);
	// The operations below exist only as the per-instantiation forwarders declared after the class.
	// Index of the first item from aiStartIndex on for which apFunc(item, aValue) holds, else -1; items are skipped
	// while there is no buffer.
	int32_t Find(const T& aValue, uint32_t aiStartIndex, EqualFunction apFunc) const;
	// Removes the item at auiIndex, keeping the order; abShrink lets a mostly empty buffer shrink.
	void Remove(uint32_t aiIndex, bool abShrink);
	// Removes auiCount items from auiIndex on, moving the last items into the gap.
	void RemoveFast(uint32_t aiIndex, uint32_t aiHowMany);
	// Appends one item, left unset, growing the buffer when needed; returns its index.
	uint32_t AddUninitialized();
	// Appends aItem, growing the buffer when needed; returns its index.
	uint32_t Add(const T& aValue);
	// The item count, iSize; the exe reads it through the shared one-field getter 0x44DDC0.
	uint32_t GetSize() const;
	// Whether an item equal to aValue is in the array (Find from 0 != -1); only per-instantiation definitions exist.
	bool IsInArray(const T& aValue) const;
	// IDA name; not in the PDB. Whether aArray has the same size and the same items in the same order. The exe's body
	// calls the non-const operator[] (0x6A7AD0) on both arrays, so neither the method nor aArray is const.
	bool CompareArrays(BSSimpleArray& aArray);
	// Grows the buffer to hold at least aiReserveSize items; only per-instantiation forwarders exist.
	void SetReservedSize(uint32_t aiReserveSize);
	// The first and the last item; only per-instantiation forwarders exist.
	T& GetFirst();
	const T& GetFirst() const;
	T& GetLast();
	const T& GetLast() const;
	// The PDB's CopyTo<1024>, a member template, here for this array's own instance only. Resizes apDest to this
	// array's size and copies every item into it; only per-instantiation forwarders exist.
	void CopyTo(BSSimpleArray* apDest) const;
	// Stores aValue at aiIndex, through GetAt; only per-instantiation definitions exist.
	void SetAt(uint32_t aiIndex, const T& aValue);
	// Reverses the items' order in place; only per-instantiation forwarders exist.
	void Reverse();
	// The PDB's QuickSort. Sorts the items with the caller's compare function through the CRT's qsort; only
	// per-instantiation definitions exist.
	void Sort(CompareFunction apfnCompare);
	void* GetTail();
	// Empties the members, then allocates aiReserveSize items (at least aiInitialSize) and constructs the first
	// aiInitialSize; only per-instantiation definitions exist.
	void _InitialSetup(uint32_t aiReserveSize, uint32_t aiInitialSize);
	// Inserts aValue at aiIndex, moving the later items up (Add when aiIndex is the end); only per-instantiation
	// forwarders exist.
	void Insert(uint32_t aiIndex, const T& aValue);
	// Removes aiHowMany items from aiIndex on, keeping the order (Clear when aiHowMany is at least iSize); abShrink
	// lets a mostly empty buffer shrink. Only per-instantiation forwarders exist.
	void RemoveRange(uint32_t aiIndex, uint32_t aiHowMany, bool abShrink);
	// Destroys the last item and drops it; only per-instantiation forwarders exist.
	void RemoveLast();
	// Inserts aValue where a binary search with apFunc places it in the sorted array; only per-instantiation
	// forwarders exist.
	void SortedInsert(const T& aValue, CompareFunction apFunc);
	// Index of an item apFunc finds equal to aValue in the sorted array (the CRT's bsearch), else -1; only
	// per-instantiation forwarders exist.
	uint32_t SortedFind(const T& aValue, CompareFunction apFunc);

	static void operator delete(void* apMemory) { ourFree(apMemory); }
	// Arrays of arrays (NavMeshGrid's cells) come from ourMalloc's heap in the exe, and their zero-count case is
	// freed with ourFree there.
	static void* operator new[](size_t astSize) { return ourMalloc(static_cast<uint32_t>(astSize)); }
	static void operator delete[](void* apMemory) { ourFree(apMemory); }
	// Find's default compare: whether aOne equals aTwo (T's operator==); only per-instantiation forwarders exist.
	static bool DefaultEqualFunction(const T& aOne, const T& aTwo);

	// DefaultEqualFunction in the form Find takes (EqualFunction: the items by address): whether *apItem equals
	// *apValue. The native side of the exe's shared compare copies a line passes to Find at this array's
	// EqualFunction type (0x9A3830 for the 4-byte items, registered under another function's name).
	static bool DefaultEqualFunctionByAddress(const T* apItem, const T* apValue) { return *apItem == *apValue; }

	T* pBuffer;
	uint32_t iSize;
	uint32_t iReservedSize;
};

inline constexpr fnvd::Vtable BSSimpleArray_NavMeshPtr_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_TESBoundObject_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_BSSimpleArray_FastNavMeshVertHandle_1024_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_DismemberedLimb_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_CombatTimeStamp_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_unsigned_int_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NiNode_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NiPoint3_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_BSSimpleArray_NiPoint3_1024_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_SCENE_INFO_DATA_STRUCT_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_TESObjectREFR_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshInfo_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_TESNPC_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_char_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_short_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_BGSHeadPart_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_INFO_LINK_ELEMENT_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_bhkRigidBody_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_hkpCollidable_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_FollowerDist_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_DecalVertex_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_unsigned_short_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshVertex_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshTriangle_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_EdgeExtraInfo_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshTriangleDoorPortal_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshClosedDoorInfo_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NiPointer_ObstacleUndoData_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshStaticAvoidNode_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshEdgeHandle_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_bool_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_FastNavMeshEdgeHandle_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshPOV_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshPOVSearchNode_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_RemoveIndex_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_FastNavMeshVertHandle_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NiColorA_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_unsigned_char_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshSearchNode_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshSearchLink_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_PathingLocation_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_PathSmootherPOV_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_PathSmootherPOVNode_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_PathSmootherPOVLink_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_PathSmootherPOVSearch_PathEdgeVert_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMesh_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_BoundsTriangleInfo_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_PortalEdgeSwap_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshObstacleVertex_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshObstacleEdge_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NiPointer_ObstacleData_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_ObstacleTaskData_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_TESObjectCELL_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_ObstacleTaskNavMeshOperation_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshEdgeLocation_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_FastNavMeshTriHandle_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_PathingCoverLocation_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_PathingNode_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavmeshTriFan_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_CrossedTriangle_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshTriHandle_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_PathingLOSGridCell_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_PathingAvoidNode_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_VirtualPathingNode_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_RayCastPathRequest_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_RayCastPath_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_PathSmoother_PathEdge_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_PathSmoother_UniqueVert_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_PathSmoother_BoundsEdge_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_PathSmoother_AvoidNode_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_ParentSpaceNode_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_TeleportLink_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_BGSDistantTreeBlock_TreeGroup_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_BGSDistantTreeBlock_TreeType_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_Tile_Value_8_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_Tile_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_SPECIALBookMenu_SpecialBookTextItem_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_SPECIALBookMenu_SpecialBookButton_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_LoveTesterMenu_LoveTesterLightBulbGlows_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_LoveTesterMenu_LoveTesterButton_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_TESRecipeCategory_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NiAVObject_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_TESCaravanCard_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_CaravanAIPackage_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NiFixedString_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_EffectItemData_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_TESLoadScreen_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_BSStringT_char_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_StartMenu_Option_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_StartMenu_UserOption_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_StartMenu_VideoRes_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NiPointer_NiAVObject_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_ReconstructFormData_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_OffsetData_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_BGSLoadFormBuffer_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_ActorHavokSyncData_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NiPointer_NiNode_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_Actor_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_TESIdleForm_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_ItemChange_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_TESAmmo_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_Animation_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_EffectTaskData_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_CombatProcedure_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_CoverReservation_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_CombatTarget_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_CombatMember_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_CombatSearchLocation_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_CombatSearchDoor_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_CombatGroupCluster_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_GroupMemberData_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_CombatPlanNode_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_CombatPlanLink_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_TESObjectWEAP_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_UnreachableCoverLocation_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_UnreachableLocation_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_WeaponData_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_BGSBodyPart_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_SandBoxChoiceListItem_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_SpectatorThreatInfo_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_DebugText_sPrintData_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_ButtonIcon_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_BSHash_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_ShadowSceneLight_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NiPointer_QueuedFile_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NiFrustumPlanes_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_bhkRagdollController_BoneTransf_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_bhkUtilFunctions_AVLevel_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_bhkRagdollPenetrationUtil_ConstrainedSystem_1024_vftable{};

// The vtable of BSSimpleArray<NiPointer<ParticleShaderProperty>,1024>.
inline constexpr fnvd::Vtable BSSimpleArray_NiPointer_ParticleShaderProperty_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshPOVSearchLink_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_AStarNode_NavMeshPOVSearchNode_NavMeshPOVSearchLink_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_LoveTesterTextItem_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_int_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_TeleportNodePtr_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_MessageMenu_MessageMenuEntry_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_MessageMenu_PerkActivation_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_FastNavMeshEdgeLocationProxy_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_BSSimpleArray_NavMeshPOV_1024_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_bhkRagdollController_bhkSkeletonBone_1024_vftable{};

// The vtable of BSSimpleArray<float, 1024>. Not named in the PDB or IDA; name chosen here.
inline constexpr fnvd::Vtable BSSimpleArray_float_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshInfoPtr_1024_vftable{};
inline constexpr fnvd::Vtable BSSimpleArray_NavMeshStaticAvoidNodePtr_1024_vftable{};

// BSSimpleArray<char, 1024>'s table (IDA names its slot 0 BSSimpleArray<char, 1024>::Destroy, 0x6E1250);
// BSSimpleArray_char_1024_vftable above is BSSimpleArray<const char*, 1024>'s, so the address is added to the name.
inline constexpr fnvd::Vtable BSSimpleArray_char_1024_vftable_106CF0C{};

// BSSimpleArray<PathingNode, 1024>'s table (its deleting destructor is 0x6E9500);
// BSSimpleArray_PathingNode_1024_vftable above is BSSimpleArray<PathingNode*, 1024>'s, so the address is added to
// the name.
inline constexpr fnvd::Vtable BSSimpleArray_PathingNode_1024_vftable_106D4E8{};
inline constexpr fnvd::Vtable BSSimpleArray_charPtr_1024_vftable{};

// Not named in the PDB or IDA; name chosen here. BSSimpleArray<BSMultiBoundNode*, 1024>'s table (its deleting
// destructor is 0xB9E920); ShadowSceneLight's BSSimpleArray<BSPortal*, 1024> uses the same one.
inline constexpr fnvd::Vtable BSSimpleArray_BSMultiBoundNodePtr_1024_vftable{};

// Not named in the PDB or IDA; name chosen here. BSSimpleArray<NiPoint2, 1024>'s table (its deleting destructor is
// 0xBA11E0).
inline constexpr fnvd::Vtable BSSimpleArray_NiPoint2_1024_vftable{};

// The exe's BSSimpleArray has no base: each instance's destructor is the whole destructor, so the root's does nothing.
inline BSSimpleArrayRoot::~BSSimpleArrayRoot() {}

// Destroys the items and releases the buffer.
template <class T, uint32_t uiGrowStep> BSSimpleArray<T, uiGrowStep>::~BSSimpleArray() {
	Clear(true);
}

#include "BSCore/BSSimpleArray.inl"
