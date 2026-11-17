#pragma once

#include "BSCore/BSSystemObjects.h"
#include "Reconstruction.hpp"

// A counting semaphore over a system semaphore (the exe's Windows handle), with its current and maximum count.
class NiSemaphore {
public:
	NiSemaphore(int32_t aiCount, int32_t aiMaxCount);
	~NiSemaphore();

	// Releases the semaphore once.
	int32_t Signal();
	// Waits for the semaphore for aiTime milliseconds.
	int32_t Wait(uint32_t aiTime);

	int32_t m_iCount;
	int32_t m_iMaxCount;
	BSSystemSemaphore* m_hSemaphore;
};
