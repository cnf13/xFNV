#pragma once

#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A node of a doubly linked list.
template <class T> class BSListItem {
public:
	BSListItem<T>* m_pkNext;
	BSListItem<T>* m_pkPrev;
	T m_element;
};

// A doubly linked list of nodes the list owns.
template <class T> class BSListBase {
public:
	// An empty list. The semaphore and IO request instances' constructors store only the
	// vtable; the native constructor empties every instance.
	BSListBase() : m_pkHead(nullptr), m_pkTail(nullptr), m_uiCount(0) {}
	// The class has a vtable on PC: the destructor, NewItem and DeleteItem.
	virtual ~BSListBase() = default;

protected:
	virtual BSListItem<T>* NewItem() = 0;
	virtual void DeleteItem(BSListItem<T>* apItem) = 0;

public:
	// Frees every node through DeleteItem and empties the list.
	void RemoveAll();
	// The position before apPos; null for a null apPos.
	void* GetPrevPos(void* kPos);
	// Links apItem in as the first node (thiscall, ret 4; protected in the engine).
	void AddNodeHead(BSListItem<T>* pkNode);
	// Links apItem in as the last node (thiscall, ret 4; protected in the engine).
	void AddNodeTail(BSListItem<T>* pkNode);
	// Whether the list has no nodes (m_uiCount == 0; thiscall, ret 0).
	bool IsEmpty() const;

	// The exe frees lists through ourFree; the semaphore and IO request instances through MemoryManager::Deallocate.
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BSListItem<T>* m_pkHead;
	BSListItem<T>* m_pkTail;
	uint32_t m_uiCount;
};

// A list of pointers the list does not own.
template <class T> class BSPointerListBase : public BSListBase<T> {
public:
	BSPointerListBase() {}

	// Allocates an uninitialised node (ourMalloc, 0xC bytes). The exe folds every pointer instantiation, and the BSMap
	// NewItem slots, into one body (0x6F99E0).
	BSListItem<T>* NewItem() override;
	// Clears the node's element and frees the node. The exe folds every pointer instantiation, and the BSMap
	// DeleteItem slots, into one body (0x48B500).
	void DeleteItem(BSListItem<T>* pkItem) override;
	// The nodes are freed through DeleteItem.
	~BSPointerListBase() override { this->RemoveAll(); }

	// Removes the first node from the list and returns its element.
	T RemoveHead();
	// Adds an element at the head of the list. The exe has one body per instantiation; src declares the ones it calls.
	void AddHead(const T& element);
	// Removes the node at position arpPos, advances arpPos to the next node and returns the element. One body per
	// instantiation, as AddHead.
	T RemovePos(void*& kPos);
	// The element at position apPos. The exe's out-of-line copy (0x6317A0, IDA's NiTPointerListBase<void *>::Get)
	// is a trivial accessor: it returns apPos plus the element's offset.
	T& Get(void* kPos) { return static_cast<BSListItem<T>*>(kPos)->m_element; }
	// The element at position arpPos; arpPos moves to the previous node. One body per instantiation, as AddHead.
	T& GetPrev(void*& kPos);
	// Adds an element at the tail of the list: a NewItem node, through AddNodeTail.
	void AddTail(const T& element);
	// Removes the last node from the list and returns its element.
	T RemoveTail();
};

// The list the engine instantiates over pointers.
template <class T> class BSList : public BSPointerListBase<T> {
public:
	BSList() {}
	~BSList() override;
};

inline constexpr fnvd::Vtable BSListBase_TriggerEntry_TriggerEvent_vftable{};
inline constexpr fnvd::Vtable BSListBase_BSTempEffectDebris_vftable{};
inline constexpr fnvd::Vtable BSListBase_BSSemaphore_vftable{};
inline constexpr fnvd::Vtable BSListBase_OEI_IO_IO_Request_vftable{};
inline constexpr fnvd::Vtable BSPointerListBase_TriggerEntry_TriggerEvent_vftable{};
inline constexpr fnvd::Vtable BSPointerListBase_BSTempEffectDebris_vftable{};
inline constexpr fnvd::Vtable BSPointerListBase_BSSemaphore_vftable{};
inline constexpr fnvd::Vtable BSPointerListBase_OEI_IO_IO_Request_vftable{};
inline constexpr fnvd::Vtable BSList_TriggerEntry_TriggerEvent_vftable{};
inline constexpr fnvd::Vtable BSList_BSTempEffectDebris_vftable{};
inline constexpr fnvd::Vtable BSList_BSSemaphore_vftable{};
inline constexpr fnvd::Vtable BSList_OEI_IO_IO_Request_vftable{};

// The exe's destructors only chain to the base's.
template <class T> BSList<T>::~BSList() {}

#include "BSCore/BSList.inl"
