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
    bool v4;
    uint32_t Exchange = BSGetCurrentThreadID();
    if (uiOwningThread == Exchange) {
        ++uiLockCount;
        return 1;
    }
    else {
        uint32_t v1 = BSAtomicCompareExchange(&uiOwningThread, 0, Exchange);
        v4 = v1 == 0;
        if (!v1) {
            InitializationFlow();
            uiLockCount = 1;
        }
    }
    return v4;
}

bool BSSpinLock::IsLocked() {
    return uiOwningThread != 0;
}
