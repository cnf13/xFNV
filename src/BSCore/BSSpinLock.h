#pragma once

#include "BSCore/BSCoreMath.h"
#include "BSCore/BSCoreUtils.h"
#include "BSCore/BSThread.h"
#include "BSMain/MessageHandler.h"

#include <cstdint>

// The engine's spin lock: the owning thread's ID and a recursion count, padded to 0x20 bytes.
class BSSpinLock {
public:
    BSSpinLock();

    // Takes the lock, spinning then sleeping; apName names the caller.
    void Lock(const char* apName);
    // Releases one level of the lock; the last level clears the owning thread.
    void Unlock();
    // Takes the lock only if no other thread holds it; true when taken.
    bool TryLock();
    // Whether a thread holds the lock.
    bool IsLocked();

    uint32_t uiOwningThread;
    uint32_t uiLockCount;
    uint8_t padding08[0x18];
};

inline BSSpinLock::BSSpinLock() : uiOwningThread(0), uiLockCount(0) {}

inline void BSSpinLock::Lock(const char* apName) {
    uint32_t uicurrentThread = BSGetCurrentThreadID();
    if (uiOwningThread == uicurrentThread) {
        ++uiLockCount;
    }
    else {
        uint32_t ispincount = 0;
        while (BSAtomicCompareExchange(&uiOwningThread, 0, uicurrentThread)) {
            if (++ispincount <= 0x2710) {
                BSSleep(0);
            }
            else {
                BSSleep(1u);
            }
        }
        InitializationFlow();
        uiLockCount = 1;
    }
}

// Releases one level of the lock and clears the owner when the count reaches 0. The owner is cleared with a plain
// store, not an interlocked compare-exchange.
inline void BSSpinLock::Unlock() {
    InitializationFlow();
    if (--uiLockCount == 0) {
        uiOwningThread = 0;
    }
}

inline bool BSSpinLock::TryLock() {
    bool bLocked;
    uint32_t uiThreadID = BSGetCurrentThreadID();
    if (uiOwningThread == uiThreadID) {
        ++uiLockCount;
        return true;
    }
    else {
        uint32_t uiPrevOwner = BSAtomicCompareExchange(&uiOwningThread, 0, uiThreadID);
        bLocked = uiPrevOwner == 0;
        if (!uiPrevOwner) {
            InitializationFlow();
            uiLockCount = 1;
        }
    }
    return bLocked;
}

inline bool BSSpinLock::IsLocked() {
    return uiOwningThread != 0;
}
