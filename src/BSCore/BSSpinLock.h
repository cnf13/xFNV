#pragma once

#include "Reconstruction.hpp"

// The engine's spin lock: the owning thread's ID and a recursion count, padded to 0x20 bytes.
class BSSpinLock {
public:
	BSSpinLock();

	// Takes the lock, spinning then sleeping; apName names the caller. thiscall, ret 4.
	void Lock(const char* apName);
	// Releases one level of the lock; the last level clears the owning thread. thiscall, ret 0.
	void Unlock();
	// Takes the lock only if no other thread holds it; true when taken. thiscall, ret 0
	// (0x78D200).
	bool TryLock();
	// Whether a thread holds the lock; callers call it out of line.
	bool IsLocked();

	uint32_t uiOwningThread;
	uint32_t uiLockCount;
	uint8_t padding08[0x18];
};
