#include "SDK/Windows/winbase.h"
#include "platform/System.h"

#include <chrono>
#include <thread>

// The time and Sleep bodies of winbase.h, on std::chrono and std::this_thread.

namespace {

using namespace std::chrono;

// FILETIME's and the performance counter's unit: 100 nanoseconds.
using Ticks = duration<int64_t, std::ratio<1, 10000000>>;
// FILETIME's epoch, 1601-01-01 00:00 UTC.
constexpr sys_days kFileTimeEpoch{year{1601} / January / 1};

// A time of day on a calendar day as the SDK's SYSTEMTIME, to the millisecond.
void ToSystemTime(sys_time<Ticks> akTime, _SYSTEMTIME* apResult)
{
	sys_days kDay = floor<days>(akTime);
	year_month_day kDate{kDay};
	hh_mm_ss<milliseconds> kClock{floor<milliseconds>(akTime - kDay)};
	apResult->wYear = static_cast<uint16_t>(static_cast<int32_t>(kDate.year()));
	apResult->wMonth = static_cast<uint16_t>(static_cast<uint32_t>(kDate.month()));
	apResult->wDayOfWeek = static_cast<uint16_t>(weekday{kDay}.c_encoding());
	apResult->wDay = static_cast<uint16_t>(static_cast<uint32_t>(kDate.day()));
	apResult->wHour = static_cast<uint16_t>(kClock.hours().count());
	apResult->wMinute = static_cast<uint16_t>(kClock.minutes().count());
	apResult->wSecond = static_cast<uint16_t>(kClock.seconds().count());
	apResult->wMilliseconds = static_cast<uint16_t>(kClock.subseconds().count());
}

sys_time<Ticks> SystemNow()
{
	return floor<Ticks>(system_clock::now());
}

// The local time of a UTC time, in the system's time zone, as a SYSTEMTIME.
void ToLocalTime(sys_time<Ticks> akTime, _SYSTEMTIME* apResult)
{
	ToSystemTime(akTime + seconds{platform::UtcOffset(floor<seconds>(akTime).time_since_epoch().count())}, apResult);
}

} // namespace

uint32_t GetTickCount()
{
	return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

int32_t QueryPerformanceCounter(_LARGE_INTEGER* lpPerformanceCount)
{
	lpPerformanceCount->QuadPart = duration_cast<Ticks>(steady_clock::now().time_since_epoch()).count();
	return 1;
}

int32_t QueryPerformanceFrequency(_LARGE_INTEGER* lpFrequency)
{
	lpFrequency->QuadPart = Ticks::period::den;
	return 1;
}

void GetSystemTime(_SYSTEMTIME* lpSystemTime)
{
	ToSystemTime(SystemNow(), lpSystemTime);
}

void GetLocalTime(_SYSTEMTIME* lpSystemTime)
{
	ToLocalTime(SystemNow(), lpSystemTime);
}

int32_t FileTimeToSystemTime(const _FILETIME* lpFileTime, _SYSTEMTIME* lpSystemTime)
{
	uint64_t ullTicks = (static_cast<uint64_t>(lpFileTime->dwHighDateTime) << 32) | lpFileTime->dwLowDateTime;
	if (ullTicks > 0x7FFFFFFFFFFFFFFF) {
		return 0;
	}
	ToSystemTime(kFileTimeEpoch + Ticks{static_cast<int64_t>(ullTicks)}, lpSystemTime);
	return 1;
}

int32_t SystemTimeToTzSpecificLocalTime(const _TIME_ZONE_INFORMATION* lpTimeZoneInformation,
	const _SYSTEMTIME* lpUniversalTime, _SYSTEMTIME* lpLocalTime)
{
	year_month_day kDate{year{lpUniversalTime->wYear}, month{lpUniversalTime->wMonth}, day{lpUniversalTime->wDay}};
	if (lpTimeZoneInformation || !kDate.ok() || lpUniversalTime->wYear < 1601 || lpUniversalTime->wHour > 23
		|| lpUniversalTime->wMinute > 59 || lpUniversalTime->wSecond > 59 || lpUniversalTime->wMilliseconds > 999) {
		return 0;
	}
	sys_time<Ticks> kTime = sys_days{kDate} + hours{lpUniversalTime->wHour} + minutes{lpUniversalTime->wMinute}
		+ seconds{lpUniversalTime->wSecond} + milliseconds{lpUniversalTime->wMilliseconds};
	ToLocalTime(kTime, lpLocalTime);
	return 1;
}

void Sleep(uint32_t dwMilliseconds)
{
	if (dwMilliseconds == 0) {
		std::this_thread::yield();
	} else {
		std::this_thread::sleep_for(milliseconds{dwMilliseconds});
	}
}

uint32_t GetCurrentThreadId()
{
	return platform::CurrentThreadId();
}
