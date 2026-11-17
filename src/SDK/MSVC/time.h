#pragma once

#include "Reconstruction.hpp"
#include <chrono>
#include <cstdint>
#include <ctime>

// rt: runtime support the game relies on. Here VC9's <time.h> clock, whose unit and clock the C libraries do not
// share. The rest of <time.h> is std:: (<ctime>).
namespace rt {

// When the program started, for clock.
inline const std::chrono::steady_clock::time_point kClockStart = std::chrono::steady_clock::now();

// Not reconstructed yet: clock, 0xEC7D50 (cdecl, ret 0): VC9's clock is the wall time since the process started, in
// milliseconds (CLOCKS_PER_SEC 1000; clock_t is a long), which the callers subtract and read as milliseconds. The C
// library's is not that everywhere (glibc's counts the processor time of all threads, in microseconds), so it is
// measured here.
inline int32_t clock()
{
	return static_cast<int32_t>(
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - kClockStart).count());
}

} // namespace rt
