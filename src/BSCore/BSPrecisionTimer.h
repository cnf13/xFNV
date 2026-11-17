#pragma once

#include "Reconstruction.hpp"

// A deadline on the performance counter: the start time and the length in counter ticks.
class BSPrecisionTimer {
public:
	// Constructs a cleared timer in apMemory; returns apMemory.
	BSPrecisionTimer();

	// Starts the timer now with a length of afTargetTime milliseconds.
	void Reset(float afTargetTime);
	// Whether the length has passed since the start.
	bool Check();

	static int64_t GetTimer();
	static void Initialize();

	int64_t iStartTime;
	int64_t iTargetTime;

	static inline float fFrequencyMS = 1.0f;
};
