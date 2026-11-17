#pragma once

#include "Reconstruction.hpp"

// A named profiling timer. The PC build compiles its members out: the constructor
// stores nothing, and owners lay timers out one byte apart (NavMeshObstacleManager's two at +0x1A0 and +0x1A1).
class BSPerformanceTimer {
public:
	// The hybrid's implicit default constructor (no zero-parameter Construct): owners such as PathManager and
	// NavMeshObstacleManager build their timers without a call.
	BSPerformanceTimer() = default;
	BSPerformanceTimer(const char* apSystem, const char* apName, bool abDisplayPerFrame, bool abGlobalReset);

	// Compiled out like the rest: both are the engine's shared empty thiscall function (0x483710).
	void Start();
	void Stop();
};
