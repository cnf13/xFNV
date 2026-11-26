#pragma once

#include "BSSystem/BSTMessageQueue.h"

using UnpackFunc = void (*)(BSPackedTask* apTask);

// A queue of packed tasks (BSPackedTask) with the function that unpacks and runs them.
class BSPackedTaskQueue {
public:
    // Defined in Main.cpp.
    BSPackedTaskQueue(ScrapHeap* apScrapHeap, UnpackFunc apUnpackFunc);

    void ThreadBeginInput();
    void ThreadEndInput();
    bool IsEmpty();
    bool Process();
    bool AddTask(BSPackedTask* apMessage);

    // A BSTCommonScrapHeapMessageQueue of BSPackedTask; the instance is declared in BSTMessageQueue.h.
    BSTCommonScrapHeapMessageQueue Queue;
    UnpackFunc pUnpackFunc;
    bool bProcessing;
};

inline bool BSPackedTaskQueue::AddTask(BSPackedTask* apMessage) {
    return static_cast<BSTMessageQueue*>(&Queue)->Push(apMessage);
}

// The queue's semaphore (0, 100) is built by BSTCommonScrapHeapMessageQueue's own constructor.
inline BSPackedTaskQueue::BSPackedTaskQueue(ScrapHeap* apScrapHeap, UnpackFunc apUnpackFunc)
  : Queue(apScrapHeap), pUnpackFunc(apUnpackFunc), bProcessing(false) {}

inline void BSPackedTaskQueue::ThreadBeginInput() {
    Queue.semaphore.Signal();
}

inline void BSPackedTaskQueue::ThreadEndInput() {
    Queue.semaphore.Wait();
}

inline bool BSPackedTaskQueue::IsEmpty() {
    return Queue.IsEmpty();
}

// Runs the queued tasks until none is left and the semaphore holds no count. The loop leaves only when TryPop failed,
// so the result is always false.
inline bool BSPackedTaskQueue::Process() {
    bool bResult;
    BSPackedTask task;
    bProcessing = true;
    bResult = true;
    while (bResult || Queue.semaphore.iCount > 0) {
        bResult = static_cast<BSTMessageQueue*>(&Queue)->TryPop(&task);
        if (bResult) {
            pUnpackFunc(&task);
        }
    }
    bProcessing = false;
    return bResult;
}
