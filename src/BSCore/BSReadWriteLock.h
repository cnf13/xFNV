#pragma once

#include <cstdint>

// A spin lock many readers or one writer hold. uiLock counts the readers and carries the writer flag; uiWriterThread
// is the id of the thread that holds it for writing.
class BSReadWriteLock {
public:
    BSReadWriteLock();

    // Waits until the lock is held for reading.
    void LockForRead();
    // Waits until the lock is held for writing.
    void LockForWrite();
    // Holds the lock for reading if it is not held for writing.
    bool TryLockForRead();
    bool TryLockForWriteAndReserve();
    void UnlockRead();
    void UnlockWrite();
    // Whether the calling thread holds the lock for writing.
    bool IsWritingThread();
    bool TryLockForWrite();

    uint32_t uiLock;
    uint32_t uiWriterThread;
};
