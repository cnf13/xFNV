#pragma once

#include <cstddef>
#include <cstdint>

// Virtual memory: VirtualAlloc/VirtualFree's reserve, commit, decommit and release (POSIX: mmap PROT_NONE, mprotect,
// madvise), and GlobalMemoryStatusEx's figures.
namespace platform {

    // Reserves size bytes of address space (no access, no memory behind it); nullptr on failure. With an address, the
    // reservation is at exactly that address or fails (MemoryPool's fixed `i << 24` ranges).
    void* Reserve(size_t size, void* address = nullptr);

    // Makes the pages covering [address, address + size) of a reservation readable and writable. Pages committed for
    // the first time, or again after Decommit, read as zero; pages already committed keep their contents.
    bool Commit(void* address, size_t size);

    // Returns the pages covering [address, address + size) to the reserved state (no access, contents dropped).
    bool Decommit(void* address, size_t size);

    // Releases a whole reservation by the address Reserve returned.
    bool Release(void* base);

    // The page size (Commit's granularity) and the allocation granularity (Reserve's alignment: 64 KB on Windows).
    size_t PageSize();
    size_t AllocationGranularity();

    // GlobalMemoryStatusEx's figures in bytes. POSIX: physical from the system, page file = the memory a commit can
    // still get (available physical plus free swap), virtual = the process's address space.
    struct MemoryInfo {
        uint64_t totalPhysical;
        uint64_t availablePhysical;
        uint64_t totalPageFile;
        uint64_t availablePageFile;
        uint64_t totalVirtual;
        uint64_t availableVirtual;
    };
    MemoryInfo MemoryStatus();

} // namespace platform
