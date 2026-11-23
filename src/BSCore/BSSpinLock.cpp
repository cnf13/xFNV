#include "BSCore/BSSpinLock.h"

#include "BSCore/BSThread.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/TESDataHandler.h"

BSSpinLock::BSSpinLock() : uiOwningThread(0), uiLockCount(0) {}

void BSSpinLock::Lock(const char* apName) {
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

bool BSSpinLock::TryLock() {
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

bool BSSpinLock::IsLocked() {
    return uiOwningThread != 0;
}
