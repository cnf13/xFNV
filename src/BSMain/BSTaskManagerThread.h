#pragma once

#include "BSMain/BSTaskManager.h"
#include "Fallout Shared/BSTaskThread.h"

// A BSTaskManager's worker thread (a BSTaskThread). It serves BSTaskManager<int64_t>, the only instantiation.
class BSTaskManagerThread : public BSTaskThread {
public:
    BSTaskManagerThread(BSTaskManager<int64_t>* apOwner, uint32_t aiProcessor);
    ~BSTaskManagerThread() override;

    // Runs the manager's queued tasks until it exits (IOManager.cpp).
    void ThreadUpdate() override;

    // The manager that owns the thread; the constructor stores it.
    BSTaskManager<int64_t>* pOwner;
};
