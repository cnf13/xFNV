#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkAction.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"

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

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
bool au_re_dereference_70(const BSSoundHandle* apThis, [[maybe_unused]] void* apEdx, int32_t a2);

// A singly linked list whose first node is stored inline in its owner; an empty list is a head node with no item.
// T is the stored value, in practice always a pointer (BSSimpleList<TESFile*>), and a null item marks an empty slot.
// The exe folds every pointer instantiation of a member into one body.
template <class T> class BSSimpleList {
public:
	// Not named in the PDB or IDA; names chosen here. Range-for over the items, node by node from the head:
	// `for (T item : list)` is the exe's inlined `for (pNode = &list; pNode; pNode = pNode->m_pkNext)`, the item read
	// at the top of each pass. An empty list's head yields its null item, so a loop that skips null items keeps its
	// own test. StopAtNull also ends the walk at the first null item, as the exe's `pNode && pNode->m_item` loops do
	// (ItemsUntilNull).
	struct End {};

	template <class Node, bool StopAtNull> class Iterator {
	public:
		explicit Iterator(Node* apNode) : m_pNode(apNode) {}

		auto& operator*() const { return m_pNode->m_item; }

		Iterator& operator++() {
			m_pNode = m_pNode->m_pkNext;
			return *this;
		}

		bool operator!=(End) const {
			if constexpr (StopAtNull)
				return m_pNode && m_pNode->m_item;
			return m_pNode != nullptr;
		}

	private:
		Node* m_pNode;
	};

	template <class Node, bool StopAtNull> struct Range {
		Iterator<Node, StopAtNull> begin() const { return Iterator<Node, StopAtNull>(m_pHead); }
		End end() const { return {}; }

		Node* m_pHead;
	};

	// The exe's pointer instantiation is 0x96A2D0: the item, then the link, are cleared.
	BSSimpleList() : m_item(), m_pkNext(nullptr) {}
	explicit BSSimpleList(const T& item) : m_item(item), m_pkNext(nullptr) {}
	// The exe's destructor (0x46FFB0) is RemoveAll: the nodes after the head are freed.
	~BSSimpleList() { RemoveAll(); }

	T& GetItem() { return m_item; }
	const T& GetItem() const { return m_item; }
	BSSimpleList* GetNext() const { return m_pkNext; }
	void SetItem(const T& item);
	bool IsEmpty() const;
	// Sorts the items with apCompare (the exe's single body for the pointer instantiations, 0x83FD60).
	void Sort(int32_t (*apCompareFn)(T, T));
	// IDA name; not in the PDB. Sorts the items with the exe's random compare (0x76B2E0), shuffling the HackingMenu
	// word lists (thiscall, ret 0, 0x7687B0).
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
	// IDA name; not in the PDB. Makes the list a copy of apSource's items: RemoveAll, then AddTail of each source
	// item. Each instantiation src uses gets an explicit specialization forwarding to the exe's body.
	void CopyList(const BSSimpleList* apSource);
	// AddTail of an item passed by value.
	void AddTailByValue(T aItem) { AddTail(aItem); }
	// Inserts apNewItem before the first item apCompare ranks it above.
	void Insert(T apNewItem, int32_t (*apCompareFn)(T, T));

	// Not named in the PDB or IDA; name chosen here. The folded pointer-list RemoveAll (0x470470) under its own name,
	// for an instantiation whose RemoveAll is another exe copy: BSSimpleList<TESPackage *>'s (0x67E400) calls it.
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

	// Calls apTraverseFn on the items with aiIn. Each
	// instantiation src uses gets an explicit specialization forwarding to the exe's body.
	void Traverse(bool (*apTraverseFn)(T, uint32_t), uint32_t aiIn);
	Iterator<BSSimpleList, false> begin() { return Iterator<BSSimpleList, false>(this); }
	Iterator<const BSSimpleList, false> begin() const { return Iterator<const BSSimpleList, false>(this); }
	End end() const { return {}; }
	Range<BSSimpleList, true> ItemsUntilNull() { return {this}; }
	Range<const BSSimpleList, true> ItemsUntilNull() const { return {this}; }

	// The exe's deleting destructor (0x4702F0) frees the list through ourFree.
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	T m_item;
	BSSimpleList* m_pkNext;
};

// The sound handle list's own exe copies differ from the generic bodies in what an empty item is: a handle with sound
// ID 0 (BSSoundHandle(0)), where the default handle is the invalid one (ID -1). The constructor (0x76B930) and
// RemoveAll (0x76B7A0) store BSSoundHandle(0); AddHead (0x76B6A0) and AddTail (0x7A19A0) skip an item whose ID is 0
// (0x76B780) and test the node's item with operator!=(0) (0x76A3B0). Defined in BSSimpleList.cpp.
template <> BSSimpleList<BSSoundHandle>::BSSimpleList();
template <> void BSSimpleList<BSSoundHandle>::AddHead(const BSSoundHandle& item);
template <> void BSSimpleList<BSSoundHandle>::AddTail(const BSSoundHandle& item);
template <> void BSSimpleList<BSSoundHandle>::RemoveAll();

#include "BSCore/BSSimpleList.inl"

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
uint32_t* RemoveAll(void* apThis, [[maybe_unused]] void* apEdx);
