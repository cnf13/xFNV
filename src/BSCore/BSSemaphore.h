#pragma once

#include "BSCore/BSSystemObjects.h"

// A counting semaphore over a system semaphore.
class BSSemaphore {
public:
    BSSemaphore();
    BSSemaphore(int32_t a1, int32_t a2);
    BSSemaphore(int32_t aiCount);
    ~BSSemaphore();

    // Releases one count of the handle, then increments iCount and returns the new count.
    int32_t Signal();
    // Waits on the handle without a timeout, then decrements iCount and returns the new count.
    int32_t Wait();
    // Defined in BSSemaphore.cpp.
    int32_t QCount() const;
    void Create();
    // Does what the default constructor does with (0, 20), and returns this.
    BSSemaphore* Create_00BA2D40();

    static void operator delete(void* apMemory);
    static void operator delete[](void* apMemory);

    int32_t iCount;
    BSSystemSemaphore* hSemaphore;
    int32_t iMaxCount;
};
