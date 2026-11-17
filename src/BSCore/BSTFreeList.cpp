#include "BSCore/BSTFreeList.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/ActorPathingMessage.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout/AI/AITaskManager.h"
#include "Fallout/AI/Movement/ActorMover.h"

#include <new>

template <class T> void BSTFreeListElem<T>::ConstructElem(const T& aP0) {
	new (RawElem) T(aP0);
}

template <class T> void BSTFreeListElem<T>::DestructElem() {
	Elem().~T();
}

// Links the auiNumElem nodes from apElem on, in order, ending with a null link.
template <class T> void BSTFreeList<T>::Init(uint32_t auiNumElem, BSTFreeListElem<T>* apElem) {
	pFree = apElem;
	while (BSAtomicCompareExchange(&uiLock, 0, 1)) {
	}
	InitializationFlow();
	BSTFreeListElem<T>* pElem = apElem;
	for (uint32_t i = 0; i < auiNumElem - 1; ++i) {
		pElem->pNext = pElem + 1;
		++pElem;
	}
	pElem->pNext = nullptr;
	InitializationFlow();
	BSAtomicCompareExchange(&uiLock, 1, 0);
}

template <class T> bool BSTFreeList<T>::Allocate(BSTFreeListElem<T>*& apResult, const T& aP0) {
	if (!DoAllocate(apResult)) {
		return false;
	}
	apResult->ConstructElem(aP0);
	return true;
}

// The MobileObjectMessage instance calls no DestructElem: its element's destructor is trivial.
template <class T> void BSTFreeList<T>::Deallocate(BSTFreeListElem<T>*& apElem) {
	apElem->DestructElem();
	DoDeallocate(&apElem);
}

template <class T> void BSTFreeList<T>::DoDeallocate(BSTFreeListElem<T>** apElem) {
	while (BSAtomicCompareExchange(&uiLock, 0, 1)) {
	}
	InitializationFlow();
	(*apElem)->pNext = pFree;
	pFree = *apElem;
	*apElem = nullptr;
	InitializationFlow();
	BSAtomicCompareExchange(&uiLock, 1, 0);
}

template <class T> bool BSTFreeList<T>::DoAllocate(BSTFreeListElem<T>*& apResult) {
	bool bAllocated;
	while (BSAtomicCompareExchange(&uiLock, 0, 1)) {
	}
	InitializationFlow();
	if (pFree) {
		apResult = pFree;
		pFree = apResult->pNext;
		apResult->pNext = nullptr;
		bAllocated = true;
	} else {
		bAllocated = false;
		apResult = nullptr;
	}
	InitializationFlow();
	BSAtomicCompareExchange(&uiLock, 1, 0);
	return bAllocated;
}

// The three instances; the members' generic bodies are defined only here.
template struct BSTFreeListElem<PathingMessage>;
template struct BSTFreeListElem<ActorPathingMessage>;
template struct BSTFreeListElem<MobileObjectMessage>;
template class BSTFreeList<PathingMessage>;
template class BSTFreeList<ActorPathingMessage>;
template class BSTFreeList<MobileObjectMessage>;
