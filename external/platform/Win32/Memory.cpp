// Virtual memory on Windows: VirtualAlloc/VirtualFree with the exe's flags (read-write pages).
#include "platform/Memory.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace platform {

void* Reserve(size_t size, void* address) {
	return VirtualAlloc(address, size, MEM_RESERVE, PAGE_READWRITE);
}

bool Commit(void* address, size_t size) {
	return VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE) != nullptr;
}

bool Decommit(void* address, size_t size) {
	return VirtualFree(address, size, MEM_DECOMMIT) != 0;
}

bool Release(void* base) {
	return VirtualFree(base, 0, MEM_RELEASE) != 0;
}

size_t PageSize() {
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwPageSize;
}

size_t AllocationGranularity() {
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwAllocationGranularity;
}

MemoryInfo MemoryStatus() {
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	if (!GlobalMemoryStatusEx(&status)) {
		return {};
	}
	return {status.ullTotalPhys, status.ullAvailPhys, status.ullTotalPageFile, status.ullAvailPageFile,
		status.ullTotalVirtual, status.ullAvailVirtual};
}

}  // namespace platform
