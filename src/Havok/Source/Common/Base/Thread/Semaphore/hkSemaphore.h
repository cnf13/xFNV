#pragma once

#include <cstdint>

// The standard-C++ semaphore hkSemaphore keeps in its handle's slot
// (hkWin32Semaphore.cpp).
struct hkSemaphoreHandle;

// A counting semaphore. Not polymorphic.
class hkSemaphore {
public:
	hkSemaphore(int32_t a1, int32_t a2);
	~hkSemaphore();

	// Waits for the semaphore without a timeout.
	void acquire();
	// Releases the semaphore lReleaseCount times. ReleaseSemaphore's BOOL is not returned because no caller reads it,
	// nor do the callers of the static form that passes it on.
	void release(int32_t lReleaseCount);

	// Releases semaphore count times (the static form hkJobQueue calls).
	static void release(hkSemaphore* semaphore, int32_t count);
	// Waits for semaphore (the static form hkJobQueue calls).
	static void acquire(hkSemaphore* semaphore);

	hkSemaphoreHandle* m_semaphore;
};
