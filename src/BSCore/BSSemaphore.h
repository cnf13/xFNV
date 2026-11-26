#pragma once

#include "BSCore/BSSystemObjects.h"

// A counting semaphore over a system semaphore.
class BSSemaphore {
public:
    BSSemaphore();
    BSSemaphore(int32_t aiCount, int32_t aiMaxCount);
    BSSemaphore(int32_t aiCount);
    ~BSSemaphore();

    // Releases one count of the handle, then increments iCount and returns the new count.
    int32_t Signal();
    // Waits on the handle without a timeout, then decrements iCount and returns the new count.
    int32_t Wait();
    // Defined in BSSemaphore.cpp.
    int32_t QCount() const;

    static void operator delete(void* apMemory);
    static void operator delete[](void* apMemory);

    int32_t iCount;
    BSSystemSemaphore* hSemaphore;
    int32_t iMaxCount;
};

inline BSSemaphore::BSSemaphore(int32_t aiCount)
  : iCount(aiCount), hSemaphore(new BSSystemSemaphore(aiCount, aiCount + 1)), iMaxCount(aiCount + 1) {}
