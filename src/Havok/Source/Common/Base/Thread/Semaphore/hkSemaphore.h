#pragma once

#include "Reconstruction.hpp"

// The standard-C++ semaphore hkSemaphore keeps in its handle's slot
// (hkWin32Semaphore.cpp).
struct hkSemaphoreHandle;

// A counting semaphore (the exe's Win32 HANDLE of one). Not polymorphic.
class hkSemaphore {
public:
	hkSemaphore(int32_t a1, int32_t a2);
	~hkSemaphore();

	// Waits for the semaphore without a timeout (thiscall, ret 0, 0xD04380). The eax it leaves (WaitForSingleObject's
	// result) is read by no caller.
	void acquire();
	// Releases the semaphore lReleaseCount times (thiscall, ret 4, 0xD04390). The eax it leaves (ReleaseSemaphore's
	// BOOL) is read by no caller, nor by the callers of the static form that passes it on.
	void release(int32_t lReleaseCount);

	// Releases semaphore count times (the static form hkJobQueue calls).
	static void release(hkSemaphore* semaphore, int32_t count);
	// Waits for semaphore (the static form hkJobQueue calls; a jump to acquire, 0xD043B0). Its one caller (0xD05840)
	// reads no eax.
	static void acquire(hkSemaphore* semaphore);

	// The exe's semaphore HANDLE.
	hkSemaphoreHandle* m_semaphore;
};
