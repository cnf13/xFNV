// Platform.h on Windows: VirtualAlloc pages, the TEB's stack base, the image's PE headers, a vectored exception
// handler for the fault report, the C runtime's _aligned_malloc family.

#include "Platform.h"

#include <windows.h>

#include <cstdlib>
#include <malloc.h>

namespace xfnv::debugheap::platform {
    namespace {

        HANDLE gLog = INVALID_HANDLE_VALUE;
        char gcImagePath[MAX_PATH];

        // On an access violation: the faulting address's block and the blocks the registers point into. The exception
        // goes on to the debugger or the crash handler as before.
        LONG CALLBACK OnException(EXCEPTION_POINTERS* apInfo) {
            const EXCEPTION_RECORD* pException = apInfo->ExceptionRecord;
            if (pException->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) {
                return EXCEPTION_CONTINUE_SEARCH;
            }
            const CONTEXT* pContext = apInfo->ContextRecord;
#if defined(_WIN64)
            const uintptr_t uiIp = pContext->Rip;
            const uintptr_t uiSp = pContext->Rsp;
            const FaultRegister kRegisters[] = {{"rax", pContext->Rax}, {"rbx", pContext->Rbx}, {"rcx", pContext->Rcx},
                                                {"rdx", pContext->Rdx}, {"rsi", pContext->Rsi}, {"rdi", pContext->Rdi},
                                                {"rbp", pContext->Rbp}, {"r8", pContext->R8},   {"r9", pContext->R9},
                                                {"r10", pContext->R10}, {"r11", pContext->R11}, {"r12", pContext->R12},
                                                {"r13", pContext->R13}, {"r14", pContext->R14}, {"r15", pContext->R15}};
#else
            const uintptr_t uiIp = pContext->Eip;
            const uintptr_t uiSp = pContext->Esp;
            const FaultRegister kRegisters[] = {{"eax", pContext->Eax}, {"ebx", pContext->Ebx}, {"ecx", pContext->Ecx},
                                                {"edx", pContext->Edx}, {"esi", pContext->Esi}, {"edi", pContext->Edi},
                                                {"ebp", pContext->Ebp}};
#endif
            DescribeFault(
                pException->ExceptionInformation[1], pException->ExceptionInformation[0] != 0, uiIp, uiSp, StackTop(),
                kRegisters, sizeof(kRegisters) / sizeof(kRegisters[0])
            );
            return EXCEPTION_CONTINUE_SEARCH;
        }

        const IMAGE_NT_HEADERS* NtHeaders() {
            const uint8_t* pImage = reinterpret_cast<const uint8_t*>(GetModuleHandleA(nullptr));
            return reinterpret_cast<const IMAGE_NT_HEADERS*>(
                pImage + reinterpret_cast<const IMAGE_DOS_HEADER*>(pImage)->e_lfanew
            );
        }

    } // namespace

    size_t PageSize() {
        SYSTEM_INFO kInfo;
        GetSystemInfo(&kInfo);
        return kInfo.dwPageSize;
    }

    char* Reserve(size_t aSize) {
        return static_cast<char*>(VirtualAlloc(nullptr, aSize, MEM_RESERVE, PAGE_NOACCESS));
    }

    bool Commit(void* apPages, size_t aSize) {
        return VirtualAlloc(apPages, aSize, MEM_COMMIT, PAGE_READWRITE) != nullptr;
    }

    void Decommit(void* apPages, size_t aSize) {
        VirtualFree(apPages, aSize, MEM_DECOMMIT);
    }

    void* AllocateTable(size_t aSize) {
        return VirtualAlloc(nullptr, aSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    }

    uint32_t ThreadId() {
        return GetCurrentThreadId();
    }

    uintptr_t StackTop() {
        return reinterpret_cast<uintptr_t>(reinterpret_cast<const NT_TIB*>(NtCurrentTeb())->StackBase);
    }

    void CodeRange(const uint8_t*& arBegin, const uint8_t*& arEnd) {
        const IMAGE_NT_HEADERS* pNt = NtHeaders();
        arBegin = reinterpret_cast<const uint8_t*>(GetModuleHandleA(nullptr)) + pNt->OptionalHeader.BaseOfCode;
        arEnd = arBegin + pNt->OptionalHeader.SizeOfCode;
    }

    uintptr_t ImageBase() {
        return reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    }

    const char* ImagePath() {
        if (!gcImagePath[0]) {
            GetModuleFileNameA(nullptr, gcImagePath, MAX_PATH);
        }
        return gcImagePath;
    }

    void OpenLog() {
        gLog = CreateFileA(
            "xfnv_debugheap.log", GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr
        );
    }

    void WriteLog(const char* apText, size_t aSize) {
        DWORD uiWritten;
        if (gLog != INVALID_HANDLE_VALUE) {
            WriteFile(gLog, apText, static_cast<DWORD>(aSize), &uiWritten, nullptr);
        }
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), apText, static_cast<DWORD>(aSize), &uiWritten, nullptr);
    }

    size_t ForeignSize(void* apMem) {
        return _msize(apMem);
    }

    [[noreturn]] void Stop() {
        __debugbreak();
        abort();
    }

    void InstallFaultHook() {
        AddVectoredExceptionHandler(1, OnException);
    }

    void* AlignedAllocate(size_t aSize, size_t aAlign) {
        return _aligned_malloc(aSize, aAlign);
    }

    void AlignedFree(void* apMem) {
        _aligned_free(apMem);
    }

    void* AlignedReallocate(void* apMem, size_t aSize, size_t aAlign) {
        return _aligned_realloc(apMem, aSize, aAlign);
    }

    size_t AlignedSize(void* apMem, size_t aAlign) {
        return _aligned_msize(apMem, aAlign, 0);
    }

} // namespace xfnv::debugheap::platform
