#pragma once

#include "Reconstruction.hpp"

// Lets the main loop sleep a little when a frame has time to spare: RequestSleep asks, RunSleep sleeps (at most 20
// milliseconds) and clears the request. All static.
class SleepManager {
public:
	static void RunSleep();

	// The milliseconds the next RunSleep sleeps; 0 for none.
	static inline uint32_t iNextSleepTime;
};
