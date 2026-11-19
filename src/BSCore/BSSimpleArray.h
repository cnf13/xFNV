#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/FastNavMeshEdgeLocationProxy.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/NavmeshTriFan.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVLink.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVNode.h"
#include "Fallout Shared/Pathfinding/RemoveIndex.h"
#include "Fallout/AI/Combat/CombatSearchDoor.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/GroupMemberData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

#include <stdint.h>

class Actor;
class Animation;
class BGSLoadFormBuffer;
class BSPortal;
class hkpCollidable;
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

// Destroys auiCount cover locations starting at apItems, for the PathingCoverLocation array.
void _DestroyItems(PathingCoverLocation* apItems, uint32_t auiCount);
// Compares the zero-extended value at a1 with the 32-bit value at a2.
bool apFunc(const uint16_t* a1, const uint32_t* a2);
void MoveItems_0(uint16_t* a1, uint16_t* a2, uint32_t a3);
// Used as the EqualFunction passed to BSSimpleArray<uint16_t, 1024>::Find: the parameters are that type's const
// pointers, and it only reads them.
bool return_equals(const uint16_t* a1, const uint16_t* a2);
// BSSimpleArray<BSStringT<char>>'s _DestructItems: destroys each of the auiCount strings from apItems on.
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
template <class T, uint32_t uiGrowStep = 1024>
class BSSimpleArray : public BSSimpleArrayRoot {
public:
    // Find's compare: whether *apItem equals *apValue.
    using EqualFunction = bool (*)(const T* apItem, const T* apValue);

    BSSimpleArray() : pBuffer(nullptr), iSize(0), iReservedSize(0) {}

    // Reserves aiReserveSize items (at least aiInitialSize) and constructs the first aiInitialSize.
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

    // Allocates a buffer of aiSizeInItem items.
    virtual T* _Allocate(uint32_t aiSizeInItem);

    // Releases apBuffer.
    virtual void _Deallocate(T* apBuffer);

    // Resizes apBuffer to aiNewSizeInItem items.
    virtual T* _Reallocate(T* apBuffer, uint32_t aiNewSizeInItem);

    // True when no spare capacity is left.
    bool _ShouldGrow() const;

    bool IsEmpty() const;

    // True when the array is small enough compared to its capacity that the buffer should shrink.
    bool _ShouldShrink() const;

    T& GetAt(int32_t aiIndex);
    const T& GetAt(int32_t aiIndex) const;
    T& operator[](uint32_t auiIndex);
    const T& operator[](uint32_t auiIndex) const;

    // The items below GetSize(), for range-for. Not in the game's class.
    T* begin() {
        return pBuffer;
    }
    T* end() {
        return pBuffer + iSize;
    }
    const T* begin() const {
        return pBuffer;
    }
    const T* end() const {
        return pBuffer + iSize;
    }

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
    // The item count, iSize.
    uint32_t GetSize() const;
    // Whether an item equal to aValue is in the array (Find from 0 != -1); only per-instantiation definitions exist.
    bool IsInArray(const T& aValue) const;
    // Whether aArray has the same size and the same items in the same order. The body calls the non-const operator[] on
    // both arrays, so neither the method nor aArray is const.
    bool CompareArrays(BSSimpleArray& aArray);
    // Grows the buffer to hold at least aiReserveSize items.
    void SetReservedSize(uint32_t aiReserveSize);
    T& GetFirst();
    const T& GetFirst() const;
    T& GetLast();
    const T& GetLast() const;
    // Declared for this array's own instance only rather than as a member template. Resizes apDest to this array's size
    // and copies every item into it.
    void CopyTo(BSSimpleArray* apDest) const;
    // Stores aValue at aiIndex, through GetAt; only per-instantiation definitions exist.
    void SetAt(uint32_t aiIndex, const T& aValue);
    // Reverses the items' order in place.
    void Reverse();
    // Sorts the items with the caller's compare function through the CRT's qsort; only per-instantiation definitions
    // exist.
    void Sort(CompareFunction apfnCompare);
    void* GetTail();
    // Empties the members, then allocates aiReserveSize items (at least aiInitialSize) and constructs the first
    // aiInitialSize; only per-instantiation definitions exist.
    void _InitialSetup(uint32_t aiReserveSize, uint32_t aiInitialSize);
    // Inserts aValue at aiIndex, moving the later items up (Add when aiIndex is the end).
    void Insert(uint32_t aiIndex, const T& aValue);
    // Removes aiHowMany items from aiIndex on, keeping the order (Clear when aiHowMany is at least iSize); abShrink
    // lets a mostly empty buffer shrink.
    void RemoveRange(uint32_t aiIndex, uint32_t aiHowMany, bool abShrink);
    // Destroys the last item and drops it.
    void RemoveLast();
    // Inserts aValue where a binary search with apFunc places it in the sorted array.
    void SortedInsert(const T& aValue, CompareFunction apFunc);
    // Index of an item apFunc finds equal to aValue in the sorted array (the CRT's bsearch), else -1.
    uint32_t SortedFind(const T& aValue, CompareFunction apFunc);

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    // Arrays of arrays (NavMeshGrid's cells) come from ourMalloc's heap, and their zero-count case is freed with
    // ourFree.
    static void* operator new[](size_t astSize) {
        return ourMalloc(static_cast<uint32_t>(astSize));
    }
    static void operator delete[](void* apMemory) {
        ourFree(apMemory);
    }
    // Find's default compare: whether aOne equals aTwo (T's operator==).
    static bool DefaultEqualFunction(const T& aOne, const T& aTwo);

    // DefaultEqualFunction in the form Find takes (EqualFunction: the items by address): whether *apItem equals
    // *apValue.
    static bool DefaultEqualFunctionByAddress(const T* apItem, const T* apValue) {
        return *apItem == *apValue;
    }

    T* pBuffer;
    uint32_t iSize;
    uint32_t iReservedSize;
};

// BSSimpleArray has no base in the game: each instance's destructor is the whole destructor, so the root's does
// nothing.
inline BSSimpleArrayRoot::~BSSimpleArrayRoot() {}

// Destroys the items and releases the buffer.
template <class T, uint32_t uiGrowStep>
BSSimpleArray<T, uiGrowStep>::~BSSimpleArray() {
    Clear(true);
}

#include "BSCore/BSSimpleArray.inl"
