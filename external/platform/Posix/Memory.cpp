// Virtual memory on POSIX: a reservation is an inaccessible anonymous mapping without swap reserved for it; commit
// makes pages read-write, decommit maps fresh inaccessible pages over them (their contents gone, their memory back).
#include "platform/Memory.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <mutex>
#include <new>

#include <sys/mman.h>
#include <unistd.h>

namespace platform {

namespace {

// Windows' reservation alignment: kept, so that code relying on 64 KB-aligned regions sees the same.
constexpr size_t kGranularity = 0x10000;

// The C heap, for the layer's own bookkeeping: the game's operator new is its MemoryManager, whose construction
// reserves memory through this file, so the bookkeeping must not allocate there.
template <class T>
struct SystemAllocator {
	using value_type = T;
	SystemAllocator() = default;
	template <class U>
	SystemAllocator(const SystemAllocator<U>&) {}
	T* allocate(size_t count) {
		void* memory = std::malloc(count * sizeof(T));
		if (!memory) {
			throw std::bad_alloc();
		}
		return static_cast<T*>(memory);
	}
	void deallocate(T* memory, size_t) { std::free(memory); }
	template <class U>
	bool operator==(const SystemAllocator<U>&) const {
		return true;
	}
};

using ReservationMap = std::map<uintptr_t, size_t, std::less<uintptr_t>,
	SystemAllocator<std::pair<const uintptr_t, size_t>>>;

// Each reservation's size by its base: munmap needs it, VirtualFree(MEM_RELEASE) does not take it. Made on first use
// and never destroyed: the game's static constructors reserve memory (MemoryManager_Singleton) before this file's
// would run, and static destructors may release it after.
std::mutex gLock;
ReservationMap& Reservations() {
	alignas(ReservationMap) static unsigned char storage[sizeof(ReservationMap)];
	static ReservationMap* reservations = new (storage) ReservationMap;
	return *reservations;
}

constexpr int kReserveFlags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE;

// The pages covering [address, address + size).
bool PageRange(void* address, size_t size, char*& start, size_t& length) {
	const uintptr_t page = PageSize();
	const uintptr_t begin = reinterpret_cast<uintptr_t>(address) & ~(page - 1);
	const uintptr_t end = (reinterpret_cast<uintptr_t>(address) + size + page - 1) & ~(page - 1);
	start = reinterpret_cast<char*>(begin);
	length = end - begin;
	return address && size;
}

// Whether the pages [start, start + length) lie inside one reservation. Windows fails a commit or decommit anywhere
// else; here a decommit (a fixed mapping) would replace whatever else is mapped there, a library's pages too. A
// refused call is reported on stderr with its caller, the game's bug to find.
bool Reserved(const char* start, size_t length, const char* call, const void* caller) {
	const uintptr_t begin = reinterpret_cast<uintptr_t>(start);
	{
		std::lock_guard lock(gLock);
		const ReservationMap& reservations = Reservations();
		auto it = reservations.upper_bound(begin);
		if (it != reservations.begin()) {
			--it;
			if (begin + length >= begin && begin + length <= it->first + it->second) {
				return true;
			}
		}
	}
	std::fprintf(stderr, "platform: %s of %p+0x%zx outside every reservation (caller %p)\n", call,
		static_cast<const void*>(start), length, caller);
	return false;
}

}  // namespace

void* Reserve(size_t size, void* address) {
	if (!size) {
		return nullptr;
	}
	const size_t length = (size + PageSize() - 1) & ~(PageSize() - 1);
	char* base;
	if (address) {
		// At exactly that address or not at all (VirtualAlloc's rule); the hint is only a hint to mmap.
		if (reinterpret_cast<uintptr_t>(address) % kGranularity) {
			return nullptr;
		}
		void* mapped = mmap(address, length, PROT_NONE, kReserveFlags, -1, 0);
		if (mapped == MAP_FAILED) {
			return nullptr;
		}
		if (mapped != address) {
			munmap(mapped, length);
			return nullptr;
		}
		base = static_cast<char*>(mapped);
	} else {
		// Over-reserved by the granularity, then trimmed to an aligned start.
		void* mapped = mmap(nullptr, length + kGranularity, PROT_NONE, kReserveFlags, -1, 0);
		if (mapped == MAP_FAILED) {
			return nullptr;
		}
		char* raw = static_cast<char*>(mapped);
		base = reinterpret_cast<char*>((reinterpret_cast<uintptr_t>(raw) + kGranularity - 1) & ~(kGranularity - 1));
		if (base > raw) {
			munmap(raw, static_cast<size_t>(base - raw));
		}
		const size_t tail = static_cast<size_t>(raw + length + kGranularity - (base + length));
		if (tail) {
			munmap(base + length, tail);
		}
	}
	std::lock_guard lock(gLock);
	Reservations()[reinterpret_cast<uintptr_t>(base)] = length;
	return base;
}

bool Commit(void* address, size_t size) {
	char* start;
	size_t length;
	return PageRange(address, size, start, length)
		&& Reserved(start, length, "Commit", __builtin_return_address(0))
		&& mprotect(start, length, PROT_READ | PROT_WRITE) == 0;
}

bool Decommit(void* address, size_t size) {
	char* start;
	size_t length;
	return PageRange(address, size, start, length)
		&& Reserved(start, length, "Decommit", __builtin_return_address(0))
		&& mmap(start, length, PROT_NONE, kReserveFlags | MAP_FIXED, -1, 0) != MAP_FAILED;
}

bool Release(void* base) {
	size_t length;
	{
		std::lock_guard lock(gLock);
		ReservationMap& reservations = Reservations();
		const auto it = reservations.find(reinterpret_cast<uintptr_t>(base));
		if (it == reservations.end()) {
			return false;
		}
		length = it->second;
		reservations.erase(it);
	}
	return munmap(base, length) == 0;
}

size_t PageSize() {
	static const size_t size = static_cast<size_t>(sysconf(_SC_PAGESIZE));
	return size;
}

size_t AllocationGranularity() {
	return kGranularity;
}

namespace {

// A `Name:  N kB` line of /proc/meminfo, in bytes; 0 when absent.
uint64_t MemInfo(const char* text, const char* name) {
	const char* line = std::strstr(text, name);
	unsigned long long kilobytes = 0;
	if (line && std::sscanf(line + std::strlen(name), " %llu", &kilobytes) == 1) {
		return kilobytes * 1024;
	}
	return 0;
}

}  // namespace

MemoryInfo MemoryStatus() {
	MemoryInfo info{};
	const uint64_t page = PageSize();
	info.totalPhysical = static_cast<uint64_t>(sysconf(_SC_PHYS_PAGES)) * page;
	char text[8192] = {};
	if (std::FILE* file = std::fopen("/proc/meminfo", "r")) {
		const size_t count = std::fread(text, 1, sizeof(text) - 1, file);
		text[count] = '\0';
		std::fclose(file);
	}
	info.availablePhysical = MemInfo(text, "MemAvailable:");
	if (!info.availablePhysical) {
		info.availablePhysical = static_cast<uint64_t>(sysconf(_SC_AVPHYS_PAGES)) * page;
	}
	info.totalPageFile = info.totalPhysical + MemInfo(text, "SwapTotal:");
	info.availablePageFile = info.availablePhysical + MemInfo(text, "SwapFree:");
	// The user half of the address space: 47 bits on x86-64, the whole 4 GB for a 32-bit process.
	info.totalVirtual = sizeof(void*) == 8 ? uint64_t(1) << 47 : uint64_t(1) << 32;
	uint64_t used = 0;
	if (std::FILE* file = std::fopen("/proc/self/statm", "r")) {
		unsigned long long pages = 0;
		if (std::fscanf(file, "%llu", &pages) == 1) {
			used = pages * page;
		}
		std::fclose(file);
	}
	info.availableVirtual = info.totalVirtual > used ? info.totalVirtual - used : 0;
	return info;
}

}  // namespace platform
