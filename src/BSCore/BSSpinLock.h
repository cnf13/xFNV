#pragma once

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
