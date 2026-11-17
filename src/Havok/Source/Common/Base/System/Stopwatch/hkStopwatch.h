#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// Havok's timer over the system clock's ticks. Only the fields and the inline members used by the visual debugger's
// frame timer are declared.
class hkStopwatch {
public:
	// Clears the counts and the timing state.
	void reset() {
		m_ticks_at_start = 0;
		m_ticks_total = 0;
		m_ticks_at_split = 0;
		m_split_total = 0;
		m_running_flag.m_bool = 0;
		m_num_timings = 0;
	}

	// Starts a run from now.
	void start() {
		m_running_flag.m_bool = 1;
		m_ticks_at_start = getTickCounter();
		m_ticks_at_split = m_ticks_at_start;
	}

	// Ends the run, adding it to the totals and counting the timing.
	void stop() {
		m_running_flag.m_bool = 0;
		const uint64_t uiNow = getTickCounter();
		m_ticks_total += uiNow - m_ticks_at_start;
		m_split_total += uiNow - m_ticks_at_split;
		++m_num_timings;
	}

	bool isRunning() const { return m_running_flag.m_bool != 0; }

	// The ticks timed so far, the run in progress included.
	uint64_t getElapsedTicks() const {
		uint64_t uiTicks = m_ticks_total;
		if (m_running_flag.m_bool) {
			uiTicks += getTickCounter() - m_ticks_at_start;
		}
		return uiTicks;
	}

	// getElapsedTicks over the clock's rate.
	float getElapsedSeconds() const { return divide64(getElapsedTicks(), getTicksPerSecond()); }
	int32_t getNumTimings() const { return m_num_timings; }
	const char* getName() const { return m_name; }

	// The system clock's current tick count.
	static uint64_t getTickCounter();
	// The system clock's ticks per second.
	static uint64_t getTicksPerSecond();
	// auiTicks over auiTicksPerSecond, rounded to a float.
	static float divide64(uint64_t a, uint64_t b);

	uint64_t m_ticks_at_start;
	uint64_t m_ticks_total;
	uint64_t m_ticks_at_split;
	uint64_t m_split_total;
	hkBool m_running_flag;
	int32_t m_num_timings;
	const char* m_name;
};
