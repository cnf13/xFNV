#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSpinLock.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

class BipedAnim;
class NavMesh;
class NavMeshPtr;
class TESObjectREFR;

// Wrapper of the Win32 InterlockedExchange; stores aiValue in apTarget and returns the previous value.
int32_t au_re_InterlockedExchange(volatile int32_t* apTarget, int32_t aiValue);
// a1 is the collector's queued node array; a2 is &GarbageCollector::Actor3DGarbageA.
void MoveQueuedNiAVObjects(NiTObjectArray<NiPointer<NiAVObject>>* a1, NiTObjectArray<NiPointer<NiAVObject>>* a2);
// Appends the non-null navmeshes of apSource to apTarget (the garbage collector's queued navmeshes to its navmeshes).
void MoveQueuedNavMeshes(BSSimpleArray<NavMesh*, 1024>* apSource, BSSimpleArray<NavMesh*, 1024>* apTarget);
// Appends the non-null temp effects of apSource to apTarget (the garbage collector's queued temp effects to
// EffectGarbageA).
void MoveQueuedTempEffects(
    NiTObjectArray<NiPointer<BSTempEffect>>* apSource,
    NiTObjectArray<NiPointer<BSTempEffect>>* apTarget
);
void Deallocate(void* a1);
void* Allocate(uint32_t a1);

// Defers the deletion of objects the game threads may still be using. Only some of the members are declared.
class GarbageCollector {
public:
    using kQueuedNiAVObjects_t = NiTObjectArray<NiPointer<NiAVObject>>;
    using Actor3DGarbageA_t = NiTObjectArray<NiPointer<NiAVObject>>;
    using kQueuedNavMeshes_t = BSSimpleArray<NavMesh*, 1024>;
    using kNavMeshes_t = BSSimpleArray<NavMesh*, 1024>;

    // Scope guard of the collector's critical section, used by the Add functions: records auiFlag and whether any of
    // its bits was set already, takes CritSec and sets the bits, and returns this.
    GarbageCollector* EnterCS(uint32_t auiFlag, const char* apName);
    // Clears the guard's bits unless they were set before EnterCS, then releases CritSec.
    void LeaveCS();

    // Frees what the collector may free this frame.
    static void Update();
    // Queues a navmesh for deletion.
    static void Add(NavMeshPtr& arNavMesh);
    // Frees everything queued for deletion. When abTryEnter is set it only tries to enter the collector's critical
    // section.
    static void ClearAll(bool abTryEnter);
    // Queues a temp effect for deletion.
    static void Add(BSTempEffect* apEffect);
    // Queues apObject for deletion.
    static void Add(NiAVObject* apObject);
    // Queues apRef for deletion.
    static void Add(TESObjectREFR* apRef);
    // Queues apBiped for deletion.
    static void Add(BipedAnim* apBiped);

    // Takes CritSec; apName names the caller. The second argument is unused (callers pass true).
    static void EnterCrit(const char* apName, bool abUnused);
    // Takes CritSec only if no other thread holds it; true when taken.
    static bool TryEnterCrit();
    // Releases CritSec. The argument is unused.
    static void LeaveCrit(bool abUnused);
    // Drops apRef from the reference arrays, queued or not.
    static void Remove(TESObjectREFR* apRef);
    // Deletes the queued temp effects when the model map can be locked.
    static void ClearTempEffects();
    // Appends the non-null elements of apSource to apTarget.
    static void CopyArrays(NiTObjectArray<TESObjectREFR*>* apSource, NiTObjectArray<TESObjectREFR*>* apTarget);
    static void CopyArrays(NiTObjectArray<BipedAnim*>* apSource, NiTObjectArray<BipedAnim*>* apTarget);
    static void CanRunGarbageCollector(bool abFlagSet);

    // The guard's fields (EnterCS, LeaveCS). uiFlag holds the bits this guard sets in GarbageCollectorCS::uiFlags.
    uint32_t uiFlag;
    // Whether any of uiFlag's bits was set when the guard was taken; LeaveCS then leaves them.
    bool bWasSet;

    // EffectGarbageA and ObjectGarbageA are the garbage arrays for temp effects and references; the queued arrays exist
    // only on PC. EffectGarbageA holds smart pointers.
    static NiTObjectArray<NiPointer<BSTempEffect>> EffectGarbageA;
    static NiTObjectArray<TESObjectREFR*> ObjectGarbageA;
    static NiTObjectArray<TESObjectREFR*> kQueuedTESObjectREFRs;
    static BSSpinLock CritSec;
    static NiTObjectArray<Animation*> AnimGarbageA;
    static kQueuedNiAVObjects_t kQueuedNiAVObjects;
    static Actor3DGarbageA_t Actor3DGarbageA;
    static inline bool bRunning;
    static NiTObjectArray<BipedAnim*> kQueuedBipedAnims;
    static NiTObjectArray<NiPointer<BSTempEffect>> kBSTempEffectArray;
    static kQueuedNavMeshes_t kQueuedNavMeshes;
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
