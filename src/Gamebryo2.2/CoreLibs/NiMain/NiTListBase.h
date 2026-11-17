#pragma once

#include "BSCore/BSStringT.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Actor;
class BSPortal;
class NiGeometry;
class NiTriShape;
class Tile;

// One node of a doubly linked NiTList; a NiMemObject, as the engine's NiTListBase.h declares it.
template <class T> struct NiTListItem : public NiMemObject {
	// Constructs the element only; the links are set by the list.
	NiTListItem() {}

	// The deleting destructor frees the node through ourFree.
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	NiTListItem* m_pkNext;
	NiTListItem* m_pkPrev;
	T m_element;
};

// Doubly linked list base. The PDB spells this NiTListBase<Allocator, T>; the allocator, whose only state is the
// item count at +8, is folded into this class.
template <class T> class NiTListBase : public NiMemObject {
public:
	NiTListBase() : m_pkHead(nullptr), m_pkTail(nullptr), m_uiCount(0) {}

	// Removes every item. Defined only for the instances the exe has their own copy of (specialized below and next
	// to their users); a pointer list empties itself with NiTPointerListBase::RemoveAll.
	void RemoveAll();
	uint32_t GetSize() const;
	bool IsEmpty() const;

	NiTListItem<T>* GetHeadPos() const;
	NiTListItem<T>* GetTailPos() const;

	// Position after pos, or null when pos is null or last.
	NiTListItem<T>* GetNextPos(NiTListItem<T>* kPos) const;

	// Position before pos, or null when pos is null or first.
	NiTListItem<T>* GetPrevPos(NiTListItem<T>* kPos) const;

	// Unlinks the node at kObjectToMove and links it again before (after) kDestination.
	void MoveToBefore(NiTListItem<T>* kObjectToMove, NiTListItem<T>* kDestination);
	void MoveToAfter(NiTListItem<T>* kObjectToMove, NiTListItem<T>* kDestination);

	// Link an already allocated node and bump the count.
	void AddNodeHead(NiTListItem<T>* pkNode);
	void AddNodeTail(NiTListItem<T>* pkNode);
	NiTListItem<T>* AddNodeAfter(NiTListItem<T>* kPos, NiTListItem<T>* pkNode);
	NiTListItem<T>* AddNodeBefore(NiTListItem<T>* kPos, NiTListItem<T>* pkNode);

	NiTListItem<T>* m_pkHead;
	NiTListItem<T>* m_pkTail;
	uint32_t m_uiCount;
};

#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.inl"
