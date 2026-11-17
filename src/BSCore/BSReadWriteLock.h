#pragma once

#include "Reconstruction.hpp"

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
	// IDA name TryLock (thiscall, 0xAA6220).
	bool TryLockForWriteAndReserve();
	// IDA types the result as the LONG the interlocked decrement returns (thiscall, 0xAA62C0).
	void UnlockRead();
	void UnlockWrite();
	// IDA name IsOnWriterThread; the PDB's IsWritingThread. Whether the calling thread holds the
	// lock for writing (thiscall, 0xAA6330).
	bool IsWritingThread();
	bool TryLockForWrite();

	uint32_t uiLock;
	uint32_t uiWriterThread;
};
