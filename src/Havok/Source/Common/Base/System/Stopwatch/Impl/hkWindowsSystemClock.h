#pragma once

#include "Havok/Source/Common/Base/System/Stopwatch/hkSystemClock.h"
#include "Reconstruction.hpp"
#include "SDK/Windows/winbase.h"

#include <x86intrin.h>

// The SDK's getTicksPerSecond keeps it as its function-local static freq.
inline LARGE_INTEGER freq_0{};

// The Windows tick counter (PDB source hkSystemClock.cpp; the SDK's header is Stopwatch/Impl). No data of its own.
// The exe's vtable 0x10D7C58 shares the destructor 0xDE10B0 with hkBaseObject and overrides both of hkSystemClock's
// slots: getTickCounter 0xDD8E00 (rdtsc) and getTicksPerSecond 0xDD8E20 (calibrated once against the performance
// counter).
class hkWindowsSystemClock : public hkSystemClock {
public:
	~hkWindowsSystemClock() override;

	// The time stamp counter (rdtsc).
	uint64_t getTickCounter() override { return __rdtsc(); }

	// Calibrated once: the performance counter's frequency scaled by the smallest ratio of time stamp ticks to
	// performance counter ticks over ten timed busy loops.
	uint64_t getTicksPerSecond() override {
		if (freq_0.QuadPart == 0) {
			LARGE_INTEGER qticks;
			LARGE_INTEGER qticks2;
			double minFactor = 1e6f;
			for (int32_t iter = 0; iter < 10; iter++) {
				uint64_t ticks = getTickCounter();
				QueryPerformanceCounter(&qticks);
				volatile int32_t x = 1;
				for (int32_t j = 0; j < 5000; j++) {
					x += x * x;
				}
				uint64_t ticks2 = getTickCounter();
				QueryPerformanceCounter(&qticks2);
				QueryPerformanceFrequency(&freq_0);
				uint64_t diff = ticks2 - ticks;
				uint64_t qdiff = static_cast<uint64_t>(qticks2.QuadPart) - static_cast<uint64_t>(qticks.QuadPart);
				double factor = double(diff) / double(qdiff);
				if (factor < minFactor) {
					minFactor = factor;
				}
			}
			freq_0.QuadPart =
				static_cast<int64_t>(static_cast<uint64_t>(minFactor * double(static_cast<uint64_t>(freq_0.QuadPart))));
		}
		return static_cast<uint64_t>(freq_0.QuadPart);
	}
};

inline constexpr fnvd::Vtable hkWindowsSystemClock_vftable{};
