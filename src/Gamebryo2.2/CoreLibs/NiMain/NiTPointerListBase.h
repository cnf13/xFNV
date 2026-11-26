#pragma once

#include "BSCore/BSStringT.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"

class Actor;
class BSPortal;
class HackingText;
class NiAVObject;
class NiDX92DBufferData;
class NiGeometry;
class NiNode;
class TESObjectCELL;
class TESObjectREFR;
class Tile;

// TheAllocator hands out and takes back the nodes: the shared node pool by default, the heap for NiTList.
template <class T, class TheAllocator = NiTPointerAllocator<uint32_t>>
class NiTPointerListBase : public NiTListBase<T> {
public:
    NiTPointerListBase() {}
    ~NiTPointerListBase() {
        RemoveAll();
    }

    // First element, read without a null check.
    T& GetHead() const;

    // Last element, read without a null check.
    T& GetTail() const;

    T& Get(NiTListItem<T>* kPos) const;

    // Returns the element at pos and advances pos to the following node.
    T& GetNext(NiTListItem<T>*& kPos) const;

    // Returns the element at pos and steps pos back to the previous node.
    T& GetPrev(NiTListItem<T>*& kPos) const;

    // First node at or after kStart (the head when null) holding the element, or null.
    NiTListItem<T>* FindPos(const T& element, NiTListItem<T>* kStart = nullptr) const;

    void AddHead(const T& element);
    void AddTail(const T& element);
    NiTListItem<T>* InsertBefore(NiTListItem<T>* kPos, const T& element);
    NiTListItem<T>* InsertAfter(NiTListItem<T>* kPos, const T& element);
    T RemoveTail();
    T Remove(const T& element);
    // Removes the node at kPos, advances kPos to the following node and returns the removed element.
    T RemovePos(NiTListItem<T>*& kPos);
    void RemoveAll();
    // Takes a node from the allocator, which the list folds into its item count field.
    NiTListItem<T>* NewItem();
    // Clears the node's element and returns the node to the allocator.
    void DeleteItem(NiTListItem<T>* pkItem);

    // Unlinks the head node, frees it with DeleteItem and returns its element (inline in the engine).
    T RemoveHead();
};

using AnimatingText = HackingText;

#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.inl"
