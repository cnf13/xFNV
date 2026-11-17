#include "Havok/Source/Common/Base/System/Stopwatch/hkStopwatch.h"

#include "Havok/Source/Common/Base/System/Stopwatch/hkSystemClock.h"

// The exe jumps to the clock's virtual; the 64-bit count comes back in edx:eax.
uint64_t hkStopwatch::getTickCounter() {
	return hkSystemClock::s_instance->getTickCounter();
}

// The exe jumps to the clock's virtual; the 64-bit rate comes back in edx:eax.
uint64_t hkStopwatch::getTicksPerSecond() {
	return hkSystemClock::s_instance->getTicksPerSecond();
}

float hkStopwatch::divide64(uint64_t a, uint64_t b) {
	return static_cast<double>(static_cast<int64_t>(a)) / static_cast<double>(static_cast<int64_t>(b));
}
