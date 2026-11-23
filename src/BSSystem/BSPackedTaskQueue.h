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
