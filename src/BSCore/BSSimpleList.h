#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkAction.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <new>
#include <stdint.h>

class ActiveEffect;
class BGSPerk;
class BSPortal;
class Crime;
class ExtraDataList;
struct FileData;
class MagicItem;
class NiAVObject;
class ObjectstoAcquire;
class TESBoundObject;
class TESFile;
class TESObjectREFR;
class TESPackage;
class TESWorldSpace;
class Tile;

bool au_re_dereference_70(const BSSoundHandle* apThis, [[maybe_unused]] void* apEdx, int32_t aiSoundID);

// A singly linked list whose first node is stored inline in its owner; an empty list is a head node with no item. T is
// the stored value, in practice always a pointer (BSSimpleList<TESFile*>), and a null item marks an empty slot.
template <class T>
class BSSimpleList {
public:
    // Range-for over the items, node by node from the head: `for (T item : list)` walks `for (pNode = &list; pNode;
    // pNode = pNode->m_pkNext)`, the item read at the top of each pass. An empty list's head yields its null item, so a
    // loop that skips null items keeps its own test. StopAtNull also ends the walk at the first null item, like `pNode
    // && pNode->m_item` loops (ItemsUntilNull).
    struct End {};

    template <class Node, bool StopAtNull>
    class Iterator {
    public:
        explicit Iterator(Node* apNode) : m_pNode(apNode) {}

        auto& operator*() const {
            return m_pNode->m_item;
        }

        Iterator& operator++() {
            m_pNode = m_pNode->m_pkNext;
            return *this;
        }

        bool operator!=(End) const {
            if constexpr (StopAtNull) {
                return m_pNode && m_pNode->m_item;
            }
            return m_pNode != nullptr;
        }

    private:
        Node* m_pNode;
    };

    template <class Node, bool StopAtNull>
    struct Range {
        Iterator<Node, StopAtNull> begin() const {
            return Iterator<Node, StopAtNull>(m_pHead);
        }
        End end() const {
            return {};
        }

        Node* m_pHead;
    };

    // The item, then the link, are cleared.
    BSSimpleList() : m_item(), m_pkNext(nullptr) {}
    explicit BSSimpleList(const T& item) : m_item(item), m_pkNext(nullptr) {}
    // The destructor is RemoveAll: the nodes after the head are freed.
    ~BSSimpleList() {
        RemoveAll();
    }

    T& GetItem() {
        return m_item;
    }
    const T& GetItem() const {
        return m_item;
    }
    BSSimpleList* GetNext() const {
        return m_pkNext;
    }
    void SetItem(const T& item);
    bool IsEmpty() const;
    // Sorts the items with apCompareFn.
    void Sort(int32_t (*apCompareFn)(T, T));
    // Sorts the items with a random compare, shuffling the HackingMenu word lists.
    void Randomize();
    // Shell-sorts the item array Sort copies out.
    void ShellSort(T* apArray, int32_t (*apCompareFn)(T, T), int32_t aiCount);
    // The bubble sort Sort runs when the item array could not be allocated.
    void BubbleSort(int32_t (*apCompareFn)(T, T));

    bool IsInList(const T& item) const;
    int32_t ItemsInList() const;
    // Puts the item first: an empty head takes it, otherwise the head's item moves to a new second node.
    void AddHead(const T& item);
    // Puts the item last: an empty last node takes it, otherwise a new node is linked after it.
    void AddTail(const T& item);
    // Unlinks and frees the first node holding the item; the inline head node takes its successor's item.
    void Remove(const T& item);
    // Frees every node after the head and clears the head's item.
    void RemoveAll();
    // Prepends a heap copy of apSrc (BSstrlen and BSstrcpy through ourMalloc, then
    // AddHead), as the HackingMenu word lists do.
    void AppendStr(char* apSrc);
    // Frees every item, the inline head's included, and drops the nodes;
    // returns this.
    BSSimpleList* DeleteAll();
    // DeleteAll, then the list's own destructor.
    void FreeStrings();

    // Moves the next node into the inline head node.
    void RemoveHead();
    // Makes the list a copy of apSource's items: RemoveAll, then AddTail of each source item.
    void CopyList(const BSSimpleList* apSource);
    // AddTail of an item passed by value.
    void AddTailByValue(T aItem) {
        AddTail(aItem);
    }
    // Inserts apNewItem before the first item apCompare ranks it above.
    void Insert(T apNewItem, int32_t (*apCompareFn)(T, T));

    // The pointer-list RemoveAll under its own name, for an instantiation that has a different RemoveAll:
    // BSSimpleList<TESPackage *> calls it.
    void RemoveAllPointerList() {
        // The generic RemoveAll's loop: BSSimpleList<TESPackage *>'s RemoveAll calls this one, so calling RemoveAll
        // here would recurse.
        while (m_pkNext) {
            BSSimpleList* pNext = m_pkNext->m_pkNext;
            m_pkNext->m_pkNext = nullptr;
            BSSimpleList* pNode = m_pkNext;
            pNode->RemoveAllPointerList();
            ourFree(pNode);
            m_pkNext = pNext;
        }
        m_item = T();
    }

    // Calls apTraverseFn on the items with aiIn.
    void Traverse(bool (*apTraverseFn)(T, uint32_t), uint32_t aiIn);
    // Every node's item, for range-for; an empty list yields its head's null item. ItemsUntilNull stops at the first
    // null item.
    Iterator<BSSimpleList, false> begin() {
        return Iterator<BSSimpleList, false>(this);
    }
    Iterator<const BSSimpleList, false> begin() const {
        return Iterator<const BSSimpleList, false>(this);
    }
    End end() const {
        return {};
    }
    Range<BSSimpleList, true> ItemsUntilNull() {
        return {this};
    }
    Range<const BSSimpleList, true> ItemsUntilNull() const {
        return {this};
    }

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    T m_item;
    BSSimpleList* m_pkNext;
};

// These specializations differ from the generic bodies in what an empty item is: a handle with sound ID 0
// (BSSoundHandle(0)), where the default handle is the invalid one (ID -1). The constructor and RemoveAll store
// BSSoundHandle(0); AddHead and AddTail skip an item whose ID is 0 and test the node's item with operator!=(0). Defined
// in BSSimpleList.cpp.
template <>
BSSimpleList<BSSoundHandle>::BSSimpleList();
template <>
void BSSimpleList<BSSoundHandle>::AddHead(const BSSoundHandle& item);
template <>
void BSSimpleList<BSSoundHandle>::AddTail(const BSSoundHandle& item);
template <>
void BSSimpleList<BSSoundHandle>::RemoveAll();

#include "BSCore/BSSimpleList.inl"

uint32_t* RemoveAll(void* apThis, [[maybe_unused]] void* apEdx);
