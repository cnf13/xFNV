#pragma once

#include "BSCore/MemoryManager.h"

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
	// Links pkNode in as the first node.
	void AddNodeHead(BSListItem<T>* pkNode);
	// Links pkNode in as the last node.
	void AddNodeTail(BSListItem<T>* pkNode);
	// True when m_uiCount == 0.
	bool IsEmpty() const;

	// Lists are freed through ourFree; the semaphore and IO request instances through MemoryManager::Deallocate.
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BSListItem<T>* m_pkHead;
	BSListItem<T>* m_pkTail;
	uint32_t m_uiCount;
};

// A list of pointers the list does not own.
template <class T> class BSPointerListBase : public BSListBase<T> {
public:
	BSPointerListBase() {}

	// Allocates an uninitialised node with ourMalloc.
	BSListItem<T>* NewItem() override;
	// Clears the node's element and frees the node.
	void DeleteItem(BSListItem<T>* pkItem) override;
	// The nodes are freed through DeleteItem.
	~BSPointerListBase() override { this->RemoveAll(); }

	// Removes the first node from the list and returns its element.
	T RemoveHead();
	// Adds an element at the head of the list.
	void AddHead(const T& element);
	// Removes the node at position arpPos, advances arpPos to the next node and returns the element. One body per
	// instantiation, as AddHead.
	T RemovePos(void*& kPos);
	// The element at position kPos.
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

// The destructor only chains to the base's.
template <class T> BSList<T>::~BSList() {}

#include "BSCore/BSList.inl"
