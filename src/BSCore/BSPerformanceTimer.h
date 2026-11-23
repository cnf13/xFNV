#pragma once

// A named profiling timer. The PC build compiles its members out: the constructor stores nothing, and owners lay timers
// out one byte apart.
class BSPerformanceTimer {
public:
    BSPerformanceTimer() = default;
    BSPerformanceTimer(const char* apSystem, const char* apName, bool abDisplayPerFrame, bool abGlobalReset);

    // Compiled out like the rest: both are empty.
    void Start();
    void Stop();
};
