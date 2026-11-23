#include "BSCore/BSReadWriteLock.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

BSReadWriteLock::BSReadWriteLock() : uiLock(0), uiWriterThread(0) {}

// Takes the lock for reading: a writer holding it on this thread just counts up; otherwise the reader count (the
// low 28 bits) is raised with a compare-exchange until one sticks, spinning while a writer has the flag set.
void BSReadWriteLock::LockForRead() {
    if (uiWriterThread == GetCurrentThreadId()) {
        ++uiLock;
    }
    else {
        for (uint32_t uiComperand = uiLock & 0xFFFFFFF;
             InterlockedCompareExchangeUInt32(&uiLock, uiComperand + 1, uiComperand) != uiComperand;
             uiComperand = uiLock & 0xFFFFFFF) {
            Sleep(0);
        }
    }
}

// Takes the lock for writing: the writer flag is raised with a compare-exchange while the reader count is unchanged,
// then the lock is turned into the single-writer value 0x80000001 once no reader is left.
void BSReadWriteLock::LockForWrite() {
    uint32_t uicurrentThread = GetCurrentThreadId();
    if (uiWriterThread == uicurrentThread) {
        ++uiLock;
    }
    else {
        uint32_t uimarkedLock = uiLock & 0xFFFFFFF;
        for (uint32_t uiunmarkedLock = uiLock | 0x80000000u;
             InterlockedCompareExchangeUInt32(&uiLock, uiunmarkedLock, uimarkedLock) != uimarkedLock;
             uiunmarkedLock = uiLock | 0x80000000u) {
            Sleep(0);
            uimarkedLock = uiLock & 0xFFFFFFF;
        }
        while (InterlockedCompareExchangeUInt32(&uiLock, 0x80000001u, 0x80000000u) != 0x80000000u) {
            Sleep(0);
        }
        uiWriterThread = uicurrentThread;
    }
}

bool BSReadWriteLock::TryLockForRead() {
    if (uiWriterThread == GetCurrentThreadId()) {
        ++uiLock;
    }
    else {
        for (uint32_t uiComperand = uiLock & 0xFFFFFFF;
             InterlockedCompareExchangeUInt32(&uiLock, uiComperand + 1, uiComperand) != uiComperand;
             uiComperand = uiLock & 0xFFFFFFF) {
            if ((uiLock & 0x80000000u) != 0) {
                return false;
            }
            Sleep(0);
        }
    }
    return true;
}

// Takes the write lock if no writer holds it: sets the writer flag over the current readers, then turns the lone flag
// into the flag plus one write nesting level (0x80000001). Nests when the calling thread already writes.
bool BSReadWriteLock::TryLockForWriteAndReserve() {
    bool bLocked = false;
    uint32_t uiCurrentThread = GetCurrentThreadId();
    if (uiWriterThread == uiCurrentThread) {
        ++uiLock;
        bLocked = true;
    }
    else {
        uint32_t uiLockCopy = uiLock;
        if (!uiWriterThread &&
            InterlockedCompareExchangeUInt32(&uiLock, uiLock | 0x80000000u, uiLockCopy) == uiLockCopy) {
            if (InterlockedCompareExchangeUInt32(&uiLock, 0x80000001u, 0x80000000u) == 0x80000000u) {
                bLocked = true;
                uiWriterThread = uiCurrentThread;
            }
            else {
                bLocked = false;
            }
        }
    }
    return bLocked;
}

void BSReadWriteLock::UnlockRead() {
    InterlockedDecrementUInt32(&uiLock);
}

// Releases the write lock: once the last nesting level is gone (the lock is back to the writer flag alone), the flag
// is cleared with a compare-exchange and the writer thread id dropped.
void BSReadWriteLock::UnlockWrite() {
    if (--uiLock == 0x80000000u) {
        InterlockedCompareExchangeUInt32(&uiLock, 0, uiLock);
        uiWriterThread = 0;
    }
}

bool BSReadWriteLock::IsWritingThread() {
    return GetCurrentThreadId() == uiWriterThread;
}

bool BSReadWriteLock::TryLockForWrite() {
    bool blocked;
    uint32_t uicurrentThread = GetCurrentThreadId();
    if (uiWriterThread == uicurrentThread) {
        ++uiLock;
        return true;
    }
    else {
        uint32_t uiOldLock = InterlockedCompareExchangeUInt32(&uiLock, 0x80000001u, 0);
        blocked = uiOldLock == 0;
        if (!uiOldLock) {
            uiWriterThread = uicurrentThread;
        }
    }
    return blocked;
}
