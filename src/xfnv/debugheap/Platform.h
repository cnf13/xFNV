#pragma once

// What the guard-page heap (DebugHeap.cpp) and the pass-through (SystemAlloc.cpp) need from the system, defined once
// per platform (PlatformWindows.cpp, PlatformPosix.cpp; CMake picks one): pages, the thread, the program's code, the
// log file, the C runtime's aligned heap, and the hook that reports an access violation.

#include <stddef.h>
#include <stdint.h>

namespace xfnv::debugheap {

    // The page size the heap is laid out in.
    constexpr size_t kPage = 0x1000;

    // One register the fault report looks at.
    struct FaultRegister {
        const char* pName;
        uintptr_t uiValue;
    };
    // Defined by DebugHeap.cpp, called by the platform's fault hook on an access violation: the faulting address's
    // block and the blocks the registers and the stack's first words (auiSp up to auiStackTop) point into.
    void DescribeFault(
        uintptr_t auiAddress,
        bool abWrite,
        uintptr_t auiIp,
        uintptr_t auiSp,
        uintptr_t auiStackTop,
        const FaultRegister* apRegisters,
        size_t auiCount
    );

    namespace platform {

        // The system's page size (the heap is used only when it is kPage).
        size_t PageSize();
        // Address space with no access, null on failure.
        char* Reserve(size_t aSize);
        // Reserved pages made readable and writable; false on failure.
        bool Commit(void* apPages, size_t aSize);
        // Committed pages given back to the system and made inaccessible again (still reserved).
        void Decommit(void* apPages, size_t aSize);
        // Readable, writable, zeroed memory for the heap's own tables; null on failure.
        void* AllocateTable(size_t aSize);

        // The calling thread's id, as the logs print it.
        uint32_t ThreadId();
        // The top (highest address) of the calling thread's stack.
        uintptr_t StackTop();
        // The program's own code, where return addresses are looked for.
        void CodeRange(const uint8_t*& arBegin, const uint8_t*& arEnd);
        // The program's image base and file path: frames are logged as NAME+0xOFFSET from the base, OFFSET in the
        // image's own terms (an RVA, an ELF virtual address).
        uintptr_t ImageBase();
        const char* ImagePath();

        // xfnv_debugheap.log in the current folder, and standard error.
        void OpenLog();
        void WriteLog(const char* apText, size_t aSize);

        // The size of a block of the C runtime's malloc.
        size_t ForeignSize(void* apMem);
        // After a corruption is logged: the debugger, then the crash log.
        [[noreturn]] void Stop();
        // Installs the hook that calls DescribeFault on an access violation, beside the crash log's own report.
        void InstallFaultHook();

        // The C runtime's aligned heap (_aligned_malloc and its family), for the pass-through without the guard-page
        // heap.
        void* AlignedAllocate(size_t aSize, size_t aAlign);
        void AlignedFree(void* apMem);
        void* AlignedReallocate(void* apMem, size_t aSize, size_t aAlign);
        size_t AlignedSize(void* apMem, size_t aAlign);

    } // namespace platform
} // namespace xfnv::debugheap
