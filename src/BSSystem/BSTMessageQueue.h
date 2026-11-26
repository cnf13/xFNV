#pragma once

#include "BSCore/BSSemaphore.h"
#include "BSCore/BSTFreeList.h"
#include "BSCore/NewOverloads.h"
#include "BSSystem/BSPackedTask.h"
#include "Fallout Shared/Pathfinding/ActorPathingMessage.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class NiAVObject;
class ScrapHeap;

// The base of the message queues the engine's threads pass messages through. The message parameter is untyped: the
// queues' message types (BSPackedTask, ActorPathingMessage, PathingMessage, MobileObjectMessage, NiPointer<NiAVObject>)
// share no type, and every function takes the message by reference; the push functions take it const. The virtual
// functions are, in order: the destructor, Push, TryPush, Pop, TryPop.
class BSTMessageQueue {
public:
    BSTMessageQueue() {}
    virtual ~BSTMessageQueue() {}

    virtual bool Push(const void* apMessage) = 0;
    virtual bool TryPush(const void* apMessage) = 0;
    virtual bool Pop(void* apMessage) = 0;
    virtual bool TryPop(void* apMessage) = 0;

    // Every queue is freed through ourFree.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
};

// A message queue guarded by a lock. Push to TryPop take the lock and call DoTryPush or DoTryPop, which the derived
// queues supply.
class BSTCommonMessageQueue : public BSTMessageQueue {
public:
    BSTCommonMessageQueue() : uiLock(0) {}

    // Waits for the lock, then pushes the message.
    bool Push(const void* aMessage) override;
    // Pushes the message unless the lock is taken.
    bool TryPush(const void* aMessage) override;
    // Waits for the lock, then pops into the message.
    bool Pop(void* aMessage) override;
    // Pops into the message unless the lock is taken.
    bool TryPop(void* aMessage) override;
    virtual bool DoTryPush([[maybe_unused]] const void* aMessage) {
        return false;
    }
    virtual bool DoTryPop([[maybe_unused]] void* aMessage) {
        return false;
    }
    ~BSTCommonMessageQueue() override {}

    uint32_t uiLock;
};

// A common message queue kept as a linked list of the message type T's free-list nodes, taken from the free list.
// Derives from the untyped BSTCommonMessageQueue. DoTryPush and DoTryPop are per message type.
template <class T>
class BSTCommonLLMessageQueue : public BSTCommonMessageQueue {
public:
    explicit BSTCommonLLMessageQueue(BSTFreeList<T>* apFreeList)
      : pFreeList(apFreeList), pHead(nullptr), ppTail(&pHead) {}

    // Appends the message (a T) in a node from the free list.
    bool DoTryPush(const void* aMessage) override;
    // Takes the head node's message into the message (a T).
    bool DoTryPop(void* aMessage) override;

    // Drains the queue: each queued message is popped into a default message.
    ~BSTCommonLLMessageQueue() override {
        T kMessage;
        while (BSTCommonMessageQueue::TryPop(&kMessage)) {}
    }

    BSTFreeList<T>* pFreeList;
    // The first queued node.
    BSTFreeListElem<T>* pHead;
    // The link the next pushed node is stored in: pHead, or the last node's pNext.
    BSTFreeListElem<T>** ppTail;
};
// Copies the message (a T) into a node from the free list and links the node last; false when there is no free list
// or no free node.
template <class T>
bool BSTCommonLLMessageQueue<T>::DoTryPush(const void* aMessage) {
    BSTFreeListElem<T>* pElem;
    if (!pFreeList || !pFreeList->Allocate(pElem, *static_cast<const T*>(aMessage))) {
        return false;
    }
    if (!pHead) {
        ppTail = &pHead;
    }
    *ppTail = pElem;
    pElem->pNext = nullptr;
    ppTail = &pElem->pNext;
    return true;
}

// Assigns the head node's message to the message (a T), unlinks the node and gives it back to the free list; false
// when the queue is empty.
template <class T>
bool BSTCommonLLMessageQueue<T>::DoTryPop(void* aMessage) {
    BSTFreeListElem<T>* pElem = pHead;
    if (!pElem) {
        return false;
    }
    *static_cast<T*>(aMessage) = pElem->Elem();
    pHead = pElem->pNext;
    pFreeList->Deallocate(pElem);
    return true;
}

// A common message queue over a ring of N messages kept inside the object. DoTryPush and DoTryPop have generic bodies
// after the class.
template <class T, uint32_t uiSize>
class BSTCommonStaticMessageQueue : public BSTCommonMessageQueue {
public:
    // The ring's slots are left uninitialised: DoTryPush constructs each message in place.
    BSTCommonStaticMessageQueue() : uiCurrentDepth(0), uiCurrentPush(0), uiCurrentPop(0) {}

    bool DoTryPush(const void* aMessage) override;
    bool DoTryPop(void* aMessage) override;
    ~BSTCommonStaticMessageQueue() override {}

    // Raw slots: the constructor and destructor touch none; DoTryPush constructs a message in place and DoTryPop
    // destroys it.
    union {
        T MessageBuffer[uiSize];
    };

    uint32_t uiCurrentDepth;
    uint32_t uiCurrentPush;
    uint32_t uiCurrentPop;
};

// Copy-constructs the message (a T) in the next free slot of the ring; false when the ring is full.
template <class T, uint32_t uiSize>
bool BSTCommonStaticMessageQueue<T, uiSize>::DoTryPush(const void* aMessage) {
    if (uiCurrentDepth >= uiSize) {
        return false;
    }
    new (&MessageBuffer[uiCurrentPush]) T(*static_cast<const T*>(aMessage));
    ++uiCurrentPush;
    uiCurrentPush %= uiSize;
    ++uiCurrentDepth;
    return true;
}

// Assigns the oldest message to the message (a T) and destroys it in its slot; false when the ring is empty.
template <class T, uint32_t uiSize>
bool BSTCommonStaticMessageQueue<T, uiSize>::DoTryPop(void* aMessage) {
    if (uiCurrentDepth == 0) {
        return false;
    }
    T* pMessage = &MessageBuffer[uiCurrentPop];
    *static_cast<T*>(aMessage) = *pMessage;
    pMessage->~T();
    ++uiCurrentPop;
    uiCurrentPop %= uiSize;
    --uiCurrentDepth;
    return true;
}

// The pathing manager's paths to draw and to remove.
using AVObjectStaticMessageQueue = BSTCommonStaticMessageQueue<NiPointer<NiAVObject>, 256>;

// A queue item of BSTCommonScrapHeapMessageQueue: the next item and the message, both allocated from a scrap heap.
template <class T>
struct BSScrapHeapQueueElem {
    BSScrapHeapQueueElem() : pNext(nullptr), Elem() {}

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BSScrapHeapQueueElem* pNext;
    T Elem;
};

// A common message queue kept as a linked list whose items come from a scrap heap.
class BSTCommonScrapHeapMessageQueue : public BSTCommonMessageQueue {
public:
    // The semaphore's (0, 100) arguments are the ones used by its one owner, BSPackedTaskQueue's constructor.
    explicit BSTCommonScrapHeapMessageQueue(ScrapHeap* apScrapHeap)
      : pScrapHeap(apScrapHeap), pHead(nullptr), ppTail(&pHead), semaphore(0, 100) {}

    // Appends the message (a BSPackedTask) in an item from the scrap heap.
    bool DoTryPush(const void* aMessage) override;
    // Takes the head item's message into the message.
    bool DoTryPop(void* aMessage) override;

    // Drains the queue: each queued message is popped into a scratch message.
    ~BSTCommonScrapHeapMessageQueue() override {
        BSPackedTask kMessage;
        while (BSTCommonMessageQueue::TryPop(&kMessage)) {}
    }

    // Whether no item is queued (pHead is null).
    // Defined in BSTMessageQueue.cpp.
    bool IsEmpty() const;

    ScrapHeap* pScrapHeap;
    BSScrapHeapQueueElem<BSPackedTask>* pHead;
    BSScrapHeapQueueElem<BSPackedTask>** ppTail;
    BSSemaphore semaphore;
};

void Clear(BSTCommonScrapHeapMessageQueue* apThis, [[maybe_unused]] void* apEdx);
