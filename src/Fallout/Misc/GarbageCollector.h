#pragma once

#include "BSCore/BSSpinLock.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Reconstruction.hpp"

class BipedAnim;
class NavMesh;
class NavMeshPtr;
class TESObjectREFR;

// IDA name; not in the PDB. The exe's out-of-line wrapper of the Win32 InterlockedExchange
// (cdecl, 0x868770); stores aiValue in apTarget and returns the previous value.
int32_t au_re_InterlockedExchange(volatile int32_t* apTarget, int32_t aiValue);
// a1: the collector's queued node array (0x11DE848), a NiTObjectArray<NiPointer<NiAVObject>> by the Add (0x4AFC50)
// the exe calls on it. Inferred type: a2: NiTObjectArray<NiPointer<NiAVObject>>* from 0x868af2
// (&GarbageCollector::Actor3DGarbageA)
void MoveQueuedNiAVObjects(NiTObjectArray<NiPointer<NiAVObject>>* a1, NiTObjectArray<NiPointer<NiAVObject>>* a2);
// Not named in the PDB or IDA; name chosen here. Appends the non-null navmeshes of apSource to apTarget (the
// garbage collector's queued navmeshes to its navmeshes; cdecl).
void MoveQueuedNavMeshes(BSSimpleArray<NavMesh*, 1024>* apSource, BSSimpleArray<NavMesh*, 1024>* apTarget);
// Not named in the PDB or IDA; name chosen here. Appends the non-null temp effects of apSource to apTarget (the
// garbage collector's queued temp effects to EffectGarbageA; cdecl).
void MoveQueuedTempEffects(NiTObjectArray<NiPointer<BSTempEffect>>* apSource,
	NiTObjectArray<NiPointer<BSTempEffect>>* apTarget);
void Deallocate(void* a1);
// Inferred type: a1: uint32_t: unsigned uses from 0x86984e
void* Allocate(uint32_t a1);

// Defers the deletion of objects the game threads may still be using. Only the members src calls are declared.
class GarbageCollector {
public:
	// IDA name; not in the PDB. IDA's NiTMallocInterface array; the exe calls the NiTNewInterface array's Add
	// (0x4AFC50) and RemoveAll (0x4DFFA0) on it, as on Actor3DGarbageA.
	using kQueuedNiAVObjects_t = NiTObjectArray<NiPointer<NiAVObject>>;
	// The PDB's NiTObjectArray<NiPointer<NiAVObject>>; the exe calls the NiTNewInterface array's 0x869260 on it.
	using Actor3DGarbageA_t = NiTObjectArray<NiPointer<NiAVObject>>;
	// The exe constructs it as a BSSimpleArray<NavMesh*, 1024> (0x869F00).
	using kQueuedNavMeshes_t = BSSimpleArray<NavMesh*, 1024>;
	// The exe constructs it as a BSSimpleArray<NavMesh*, 1024> (0x869F00): the PDB's NavmeshGarbageA is this one or
	// kQueuedNavMeshes.
	using kNavMeshes_t = BSSimpleArray<NavMesh*, 1024>;

	// IDA name; not in the PDB. The PC build's scope guard of the collector's critical section, an 8-byte local of
	// the Add functions (the exe constructs it at [ebp-0x14] and destroys it in their EH frame): records auiFlag,
	// whether any of its bits was set already, takes CritSec and sets the bits (thiscall, ret 8, returns this).
	GarbageCollector* EnterCS(uint32_t auiFlag, const char* apName);
	// Clears the guard's bits unless they were set before EnterCS, then releases CritSec.
	void LeaveCS();

	// Frees what the collector may free this frame.
	static void Update();
	// Queues a navmesh for deletion.
	static void Add(NavMeshPtr& arNavMesh);
	// Frees everything queued for deletion (cdecl, 0x868D70). The PC build takes a bool the
	// PDB's ClearAll lacks: when set it only tries to enter the collector's critical section; the name is ours.
	static void ClearAll(bool abTryEnter);
	// Queues a temp effect for deletion.
	static void Add(BSTempEffect* apEffect);
	// Queues apObject for deletion.
	static void Add(NiAVObject* apObject);
	// Queues apRef for deletion.
	static void Add(TESObjectREFR* apRef);
	// Queues apBiped for deletion.
	static void Add(BipedAnim* apBiped);

	// Takes CritSec; apName names the caller. The PC build's second argument is unused (callers pass true).
	// Inferred type: apName: char* from 0x867f56 (passed to BSSpinLock::Lock 0)
	static void EnterCrit(const char* apName, bool abUnused);
	// Takes CritSec only if no other thread holds it; true when taken. IDA name; not in the PDB.
	static bool TryEnterCrit();
	// Releases CritSec. The PC build's argument is unused. IDA name; not in the PDB.
	static void LeaveCrit(bool abUnused);
	// Drops apRef from the reference arrays, queued or not.
	static void Remove(TESObjectREFR* apRef);
	// Deletes the queued temp effects when the model map can be locked.
	static void ClearTempEffects();
	// Appends the non-null elements of apSource to apTarget. IDA name; not in the PDB.
	static void CopyArrays(NiTObjectArray<TESObjectREFR*>* apSource, NiTObjectArray<TESObjectREFR*>* apTarget);
	// Same function as GarbageCollector::CopyArrays (0x8694C0), registered there: the exe passes the biped arrays.
	static void CopyArrays(NiTObjectArray<BipedAnim*>* apSource, NiTObjectArray<BipedAnim*>* apTarget);
	static void CanRunGarbageCollector(bool abFlagSet);

	// The guard's fields (EnterCS, LeaveCS); names chosen here.
	// The bits this guard sets in GarbageCollectorCS::uiFlags.
	uint32_t uiFlag;
	// Whether any of uiFlag's bits was set when the guard was taken; LeaveCS then leaves them.
	bool bWasSet;

	// The PC build's garbage arrays that the reconstructed bodies use. The exe folds the pointer-element
	// instantiations' members (see NiTArray.h). EffectGarbageA and ObjectGarbageA are the PDB's (Xbox) names for the
	// temp effect and reference arrays; the queued arrays are PC-only.
	// EffectGarbageA holds smart pointers: the exe calls NiTArray<NiPointer<BSTempEffect>>'s Add (0x8693F0) on it.
	static NiTObjectArray<NiPointer<BSTempEffect>> EffectGarbageA;
	// The exe constructs it with NiTObjectArray<TESObjectREFR*>'s constructor (0x8699C0), the PDB's type.
	static NiTObjectArray<TESObjectREFR*> ObjectGarbageA;
	// IDA name; not in the PDB. The exe constructs it with NiTObjectArray<TESObjectREFR*>'s constructor (0x8699C0).
	static NiTObjectArray<TESObjectREFR*> kQueuedTESObjectREFRs;
	static BSSpinLock CritSec;
	// The exe's only NiTObjectArray<Animation*> (constructor 0x869C60, called from its initializer 0xFA1F30 only):
	// the PDB's AnimGarbageA.
	static NiTObjectArray<Animation*> AnimGarbageA;
	static kQueuedNiAVObjects_t kQueuedNiAVObjects;
	static Actor3DGarbageA_t Actor3DGarbageA;
	static inline bool bRunning;
	// The exe constructs it with NiTObjectArray<BipedAnim*>'s constructor (0x869B10).
	static NiTObjectArray<BipedAnim*> kQueuedBipedAnims;
	// The exe constructs it with NiTObjectArray<NiPointer<BSTempEffect>>'s constructor (0x869DB0).
	static NiTObjectArray<NiPointer<BSTempEffect>> kBSTempEffectArray;
	static kQueuedNavMeshes_t kQueuedNavMeshes;
	// The exe constructs it with NiTObjectArray<BipedAnim*>'s constructor (0x869B10): the PDB's BipedGarbageA is this
	// one or kQueuedBipedAnims.
	static NiTObjectArray<BipedAnim*> kBipedAnims;
	static kNavMeshes_t kNavMeshes;
};

// The flags of the garbage collector's critical section.
class GarbageCollectorCS {
public:
	// Whether any bit of auiFlag is set in uiFlags.
	static bool GetFlag(uint32_t auiFlag);

	static inline uint32_t uiFlags;
};
