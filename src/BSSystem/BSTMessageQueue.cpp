#include "BSSystem/BSTMessageQueue.h"

#include "BSCore/BSSemaphore.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/ScrapHeap.h"
#include "BSSystem/BSPackedTask.h"
#include "Fallout Shared/Pathfinding/ActorPathingMessage.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout/AI/AITaskManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/string.h"

#include <new>

// Spins until the lock is free, then pushes under it.
bool BSTCommonMessageQueue::Push(const void* aMessage) {
	bool bResult;
	while (true) {
		if (!BSAtomicCompareExchange(&uiLock, 0u, 1u)) {
			InitializationFlow();
			bResult = DoTryPush(aMessage);
			InitializationFlow();
			BSAtomicCompareExchange(&uiLock, 1u, 0u);
			break;
		}
	}
	return bResult;
}

bool BSTCommonMessageQueue::TryPush(const void* aMessage) {
	bool bResult;
	if (BSAtomicCompareExchange(&uiLock, 0u, 1u)) {
		bResult = false;
	} else {
		InitializationFlow();
		bResult = DoTryPush(aMessage);
		InitializationFlow();
		BSAtomicCompareExchange(&uiLock, 1u, 0u);
	}
	return bResult;
}

// Spins until the lock is free, then pops under it.
bool BSTCommonMessageQueue::Pop(void* aMessage) {
	bool bResult;
	while (true) {
		if (!BSAtomicCompareExchange(&uiLock, 0u, 1u)) {
			InitializationFlow();
			bResult = DoTryPop(aMessage);
			InitializationFlow();
			BSAtomicCompareExchange(&uiLock, 1u, 0u);
			break;
		}
	}
	return bResult;
}

bool BSTCommonMessageQueue::TryPop(void* aMessage) {
	bool bResult;
	if (BSAtomicCompareExchange(&uiLock, 0u, 1u)) {
		bResult = false;
	} else {
		InitializationFlow();
		bResult = DoTryPop(aMessage);
		InitializationFlow();
		BSAtomicCompareExchange(&uiLock, 1u, 0u);
	}
	return bResult;
}

bool BSTCommonScrapHeapMessageQueue::DoTryPush(const void* aMessage) {
	BSScrapHeapQueueElem<BSPackedTask>* pElem = static_cast<BSScrapHeapQueueElem<BSPackedTask>*>(pScrapHeap
			->Allocate(sizeof(BSScrapHeapQueueElem<BSPackedTask>), ScrapHeap::ALIGNMENT));
	BSScrapHeapQueueElem<BSPackedTask>* apMemory =
		static_cast<BSScrapHeapQueueElem<BSPackedTask>*>(::operator new(sizeof(BSScrapHeapQueueElem<BSPackedTask>),
			pElem));
	if (apMemory) {
		::new (apMemory) BSScrapHeapQueueElem<BSPackedTask>;
	}
	rt::memcpy(&pElem->Elem, aMessage, sizeof(pElem->Elem));
	if (!pHead) {
		ppTail = &pHead;
	}
	*ppTail = pElem;
	pElem->pNext = 0;
	ppTail = &pElem->pNext;
	return 1;
}

bool BSTCommonScrapHeapMessageQueue::DoTryPop(void* aMessage) {
	BSScrapHeapQueueElem<BSPackedTask>* pElem = pHead;
	if (!pElem) {
		return false;
	}
	rt::memcpy(aMessage, &pElem->Elem, sizeof(pElem->Elem));
	pHead = pElem->pNext;
	pElem->~BSScrapHeapQueueElem<BSPackedTask>();
	pScrapHeap->Deallocate(pElem);
	return true;
}

bool BSTCommonScrapHeapMessageQueue::IsEmpty() const {
	return pHead == nullptr;
}

void Clear(BSTCommonScrapHeapMessageQueue* apThis, [[maybe_unused]] void* apEdx) {
	BSPackedTask v2;
	while (apThis->TryPop(&v2)) {
		;
	}
}
