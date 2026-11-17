#pragma once

#include <atomic>
#include <bit>
#include <memory>

#include "Reconstruction.hpp"
#include "SDK/Windows/windef.h"
#include "SDK/Windows/winnt.h"

// The Windows SDK v6.0A <winbase.h> declarations the exe's code uses (KERNEL32.dll), with the SDK's names and 32-bit
// layouts, so no Windows header is needed. Global, as Windows declares them.
//
// The value primitives (Interlocked, critical sections, TLS indices, time, Sleep) are standard-C++ bodies under the
// SDK's names, the same text on every target; the time bodies are in winbase.cpp, which keeps <chrono> out of every
// file. FARPROC keeps the SDK's type.

using FARPROC = int(WINAPI*)();

// Interlocked operations: each is the sequentially consistent atomic operation on the target's own width (on x86 the
// same locked instructions as Windows'). std::atomic_ref takes the object as non-volatile, so the volatile the SDK
// spells is cast away; every target is naturally aligned, which is atomic_ref's requirement for these sizes. The
// typed views (UInt32, Int32, SizeT, 32, Ptr) are src's names for the SDK function at the field's type.
namespace fnvd::sync {
template <class T>
inline std::atomic_ref<T> Atomic(volatile T* apTarget)
{
	return std::atomic_ref<T>(*const_cast<T*>(apTarget));
}
// Compare-and-exchange: the target's old value; the exchange happens when it equals aComparand.
template <class T>
inline T CompareExchange(volatile T* apTarget, T aExchange, T aComparand)
{
	Atomic(apTarget).compare_exchange_strong(aComparand, aExchange);
	return aComparand;
}
// A signed count's new value after adding aiDelta to aiOld, wrapping as the locked instruction does (no overflow).
inline int32_t Added(int32_t aiOld, int32_t aiDelta)
{
	return static_cast<int32_t>(static_cast<uint32_t>(aiOld) + static_cast<uint32_t>(aiDelta));
}
} // namespace fnvd::sync

// Each increment and decrement returns the new value. There is no `volatile long*` form: `long` is 64 bits on
// Linux, over the SDK's 32-bit LONG; a count goes through the view of its own type.
inline int32_t InterlockedIncrementUInt32(volatile uint32_t* lpAddend)
{
	return static_cast<int32_t>(fnvd::sync::Atomic(lpAddend).fetch_add(1) + 1);
}
inline int32_t InterlockedDecrementUInt32(volatile uint32_t* lpAddend)
{
	return static_cast<int32_t>(fnvd::sync::Atomic(lpAddend).fetch_sub(1) - 1);
}
// Signed counts (BSSemaphore::iCount, the PDB's volatile int).
inline int32_t InterlockedIncrementInt32(volatile int32_t* lpAddend)
{
	return fnvd::sync::Added(fnvd::sync::Atomic(lpAddend).fetch_add(1), 1);
}
inline int32_t InterlockedDecrementInt32(volatile int32_t* lpAddend)
{
	return fnvd::sync::Added(fnvd::sync::Atomic(lpAddend).fetch_sub(1), -1);
}
// NiString's size_t buffer header (its reference count slot).
inline size_t InterlockedDecrementSizeT(volatile size_t* lpAddend)
{
	return fnvd::sync::Atomic(lpAddend).fetch_sub(1) - 1;
}
// The 32-bit compare-and-exchange, typed for state and count fields.
inline int32_t InterlockedCompareExchange32(volatile int32_t* lpDestination, int32_t lExchange, int32_t lComparand)
{
	return fnvd::sync::CompareExchange(lpDestination, lExchange, lComparand);
}
// Unsigned lock words (BSReadWriteLock::uiLock, the PDB's unsigned int).
inline uint32_t InterlockedCompareExchangeUInt32(volatile uint32_t* lpDestination, uint32_t lExchange,
	uint32_t lComparand)
{
	return fnvd::sync::CompareExchange(lpDestination, lExchange, lComparand);
}
// Node pointers (the lock-free queues' head and tail words): Windows' InterlockedCompareExchangePointer.
inline void* InterlockedCompareExchangePtr(void* volatile* lpDestination, void* lExchange, void* lComparand)
{
	return fnvd::sync::CompareExchange(lpDestination, lExchange, lComparand);
}
// Exchanges: each returns the target's old value.
inline int32_t InterlockedExchange(volatile int32_t* Target, int32_t Value)
{
	return fnvd::sync::Atomic(Target).exchange(Value);
}
inline int32_t InterlockedExchangeAdd(volatile int32_t* Addend, int32_t Value)
{
	return fnvd::sync::Atomic(Addend).fetch_add(Value);
}

// Critical sections: the section's std::recursive_mutex (winnt.h) lives from the initialisation to the deletion.
// The spin count does not apply to it.
inline void InitializeCriticalSection(_RTL_CRITICAL_SECTION* lpCriticalSection)
{
	std::construct_at(&lpCriticalSection->kMutex);
}
// Always succeeds (BOOL TRUE).
inline int32_t InitializeCriticalSectionAndSpinCount(_RTL_CRITICAL_SECTION* lpCriticalSection,
	[[maybe_unused]] uint32_t dwSpinCount)
{
	std::construct_at(&lpCriticalSection->kMutex);
	return 1;
}
inline void DeleteCriticalSection(_RTL_CRITICAL_SECTION* lpCriticalSection)
{
	std::destroy_at(&lpCriticalSection->kMutex);
}
inline void EnterCriticalSection(_RTL_CRITICAL_SECTION* lpCriticalSection)
{
	lpCriticalSection->kMutex.lock();
}
// BOOL: nonzero when the section is now held by this thread.
inline int32_t TryEnterCriticalSection(_RTL_CRITICAL_SECTION* lpCriticalSection)
{
	return lpCriticalSection->kMutex.try_lock() ? 1 : 0;
}
inline void LeaveCriticalSection(_RTL_CRITICAL_SECTION* lpCriticalSection)
{
	lpCriticalSection->kMutex.unlock();
}

// Thread local storage indices: Windows' 1,088 slots, a thread_local value per slot and a bitmap of the allocated
// indices. As on Windows, a newly allocated index reads null in every thread: each index has a generation, bumped
// when it is freed, and a value stored under an older generation reads as null.
inline constexpr uint32_t TLS_OUT_OF_INDEXES = 0xFFFFFFFF;
namespace fnvd::tls {
inline constexpr uint32_t kSlotCount = 1088;
struct Slot {
	void* pValue;
	uint32_t uiGeneration;
};
inline thread_local Slot tSlots[kSlotCount];
inline std::atomic<uint32_t> aAllocated[kSlotCount / 32];
inline std::atomic<uint32_t> aGeneration[kSlotCount];
} // namespace fnvd::tls

// A new index (DWORD), TLS_OUT_OF_INDEXES when all are taken.
inline uint32_t TlsAlloc()
{
	for (uint32_t uiWord = 0; uiWord < fnvd::tls::kSlotCount / 32; ++uiWord) {
		std::atomic<uint32_t>& rBits = fnvd::tls::aAllocated[uiWord];
		uint32_t uiBits = rBits.load(std::memory_order_relaxed);
		while (uiBits != 0xFFFFFFFF) {
			uint32_t uiBit = static_cast<uint32_t>(std::countr_one(uiBits));
			if (rBits.compare_exchange_weak(uiBits, uiBits | (1u << uiBit), std::memory_order_acquire,
					std::memory_order_relaxed)) {
				return uiWord * 32 + uiBit;
			}
		}
	}
	return TLS_OUT_OF_INDEXES;
}
// BOOL: 0 for an index that is not allocated.
inline int32_t TlsFree(uint32_t dwTlsIndex)
{
	if (dwTlsIndex >= fnvd::tls::kSlotCount) {
		return 0;
	}
	uint32_t uiMask = 1u << (dwTlsIndex % 32);
	std::atomic<uint32_t>& rBits = fnvd::tls::aAllocated[dwTlsIndex / 32];
	if (!(rBits.load(std::memory_order_relaxed) & uiMask)) {
		return 0;
	}
	fnvd::tls::aGeneration[dwTlsIndex].fetch_add(1, std::memory_order_relaxed);
	rBits.fetch_and(~uiMask, std::memory_order_release);
	return 1;
}
inline void* TlsGetValue(uint32_t dwTlsIndex)
{
	if (dwTlsIndex >= fnvd::tls::kSlotCount) {
		return nullptr;
	}
	const fnvd::tls::Slot& rSlot = fnvd::tls::tSlots[dwTlsIndex];
	if (rSlot.uiGeneration != fnvd::tls::aGeneration[dwTlsIndex].load(std::memory_order_relaxed)) {
		return nullptr;
	}
	return rSlot.pValue;
}
// BOOL: 0 for an index out of range.
inline int32_t TlsSetValue(uint32_t dwTlsIndex, void* lpTlsValue)
{
	if (dwTlsIndex >= fnvd::tls::kSlotCount) {
		return 0;
	}
	fnvd::tls::tSlots[dwTlsIndex] = {lpTlsValue, fnvd::tls::aGeneration[dwTlsIndex].load(std::memory_order_relaxed)};
	return 1;
}

// Time (winbase.cpp). GetTickCount, the performance counter and timeGetTime (mmsystem.h) count on
// std::chrono::steady_clock: milliseconds, and 100-nanosecond units at a constant frequency of 10 MHz.
uint32_t GetTickCount();
// Always succeed (BOOL TRUE).
int32_t QueryPerformanceCounter(_LARGE_INTEGER* lpPerformanceCount);
int32_t QueryPerformanceFrequency(_LARGE_INTEGER* lpFrequency);
// The SDK's SYSTEMTIME (16 bytes).
struct _SYSTEMTIME {
	uint16_t wYear;
	uint16_t wMonth;
	uint16_t wDayOfWeek;
	uint16_t wDay;
	uint16_t wHour;
	uint16_t wMinute;
	uint16_t wSecond;
	uint16_t wMilliseconds;
};
// The SDK's TIME_ZONE_INFORMATION (0xAC bytes); the names are WCHAR strings.
struct _TIME_ZONE_INFORMATION {
	int32_t Bias;
	char16_t StandardName[32];
	_SYSTEMTIME StandardDate;
	int32_t StandardBias;
	char16_t DaylightName[32];
	_SYSTEMTIME DaylightDate;
	int32_t DaylightBias;
};
// std::chrono::system_clock, as UTC and as the local time of the system's time zone (platform::UtcOffset: Windows'
// own zone on Windows; TZ or /etc/localtime elsewhere).
void GetSystemTime(_SYSTEMTIME* lpSystemTime);
void GetLocalTime(_SYSTEMTIME* lpSystemTime);
// BOOL: 0 for a FILETIME past 0x7FFFFFFFFFFFFFFF, as Windows.
int32_t FileTimeToSystemTime(const _FILETIME* lpFileTime, _SYSTEMTIME* lpSystemTime);
// The local time of a UTC time. The game passes only a null zone (the system's, as GetLocalTime); a given
// TIME_ZONE_INFORMATION is not modelled and fails (BOOL FALSE), as does an invalid date. Where Windows' and the
// IANA database's zone rules disagree (years before a zone's current rules), the IANA rules apply.
int32_t SystemTimeToTzSpecificLocalTime(const _TIME_ZONE_INFORMATION* lpTimeZoneInformation,
	const _SYSTEMTIME* lpUniversalTime, _SYSTEMTIME* lpLocalTime);
// -1, 0 or 1 as the first time is earlier, equal or later (the 64-bit values compared unsigned).
inline int32_t CompareFileTime(const _FILETIME* lpFileTime1, const _FILETIME* lpFileTime2)
{
	uint64_t ullTime1 = (static_cast<uint64_t>(lpFileTime1->dwHighDateTime) << 32) | lpFileTime1->dwLowDateTime;
	uint64_t ullTime2 = (static_cast<uint64_t>(lpFileTime2->dwHighDateTime) << 32) | lpFileTime2->dwLowDateTime;
	return ullTime1 < ullTime2 ? -1 : ullTime1 > ullTime2 ? 1 : 0;
}
// std::this_thread::sleep_for; Sleep(0) gives up the rest of the time slice (std::this_thread::yield), as Windows'.
void Sleep(uint32_t dwMilliseconds);

// The calling thread's id as the system knows it (platform::CurrentThreadId: Windows' GetCurrentThreadId, Linux's
// gettid): the id BSSystemThread records for each thread it starts and the id the crash log reads for a thread, so
// the log finds a thread's BSThread by it on either system. std::thread::id has no such number.
uint32_t GetCurrentThreadId();
// The SDK's WIN32_FIND_DATAA (0x140 bytes); find handles (HANDLE) are void*, INVALID_HANDLE_VALUE is -1.
struct _WIN32_FIND_DATAA {
	uint32_t dwFileAttributes;
	_FILETIME ftCreationTime;
	_FILETIME ftLastAccessTime;
	_FILETIME ftLastWriteTime;
	uint32_t nFileSizeHigh;
	uint32_t nFileSizeLow;
	uint32_t dwReserved0;
	uint32_t dwReserved1;
	char cFileName[260];
	char cAlternateFileName[14];
};
