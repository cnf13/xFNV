#pragma once

#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"

// One free-list node: the element's storage, then the link to the next free node.
template <class T> struct BSTFreeListElem {
	BSTFreeListElem() : pNext(nullptr) {}

	// The element in RawElem, the untyped storage the engine constructs T in. The exe's out-of-line copy is the
	// shared identity function 0x6815C0 (a trivial accessor, registered as NiNode::IsNode).
	T& Elem() { return *reinterpret_cast<T*>(RawElem); }
	// Copy-constructs arElem in RawElem: the PDB's ConstructElem<T>, a member template the
	// engine instantiates only with T (thiscall, ret 4).
	void ConstructElem(const T& aP0);
	// Destroys the element in RawElem.
	void DestructElem();

	char RawElem[sizeof(T)];
	BSTFreeListElem<T>* pNext;
};

// A spin-locked free list of preallocated elements.
template <class T> class BSTFreeList {
public:
	BSTFreeList() : uiLock(0), pFree(nullptr) {}
	virtual ~BSTFreeList();

	// Links auiNumElem nodes from apElem on as the free list, under the lock.
	void Init(uint32_t auiNumElem, BSTFreeListElem<T>* apElem);
	// Takes a node off the free list and copy-constructs arElem in it; false, with no node,
	// when the list is empty. The PDB's Allocate<T>, a member template the engine instantiates only with T (thiscall,
	// ret 8).
	bool Allocate(BSTFreeListElem<T>*& apResult, const T& aP0);
	// Destroys the node's element and gives the node back.
	void Deallocate(BSTFreeListElem<T>*& apElem);

protected:
	// Pushes the node back onto the free list under the lock and clears the caller's pointer. thiscall, ret 4.
	// Defined per instance where the exe has it; natively the generic body in BSTFreeList.cpp.
	void DoDeallocate(BSTFreeListElem<T>** apElem);
	// Takes the first free node under the lock into arpElem; false, with a null arpElem, when none is left.
	bool DoAllocate(BSTFreeListElem<T>*& apResult);

public:
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	// 0 when free, 1 while held; taken and released through BSAtomicCompareExchange.
	volatile int32_t uiLock;
	BSTFreeListElem<T>* pFree;
};

inline constexpr fnvd::Vtable BSTFreeList_PathingMessage_vftable{};
inline constexpr fnvd::Vtable BSTFreeList_ActorPathingMessage_vftable{};
inline constexpr fnvd::Vtable BSTFreeList_MobileObjectMessage_vftable{};

template <class T> BSTFreeList<T>::~BSTFreeList() {}
