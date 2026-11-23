// The crash log's Windows side: on an unhandled exception, FalloutNV.crash-<yyyymmdd-hhmmss>.log (and .dmp) beside
// the exe, then the process dies as it would have. The faulting thread only hands its exception to a thread started
// at load time and waits for it (so a stack overflow leaves the work enough stack); that thread loads the symbols,
// suspends the other threads, and writes the log. No heap of ours: static buffers, Win32 calls; dbghelp allocates
// from the process heap, a risk taken for in-process symbols (the faulting thread gives up waiting after 3 minutes).
//
// It installs itself from a static initialiser and re-asserts its filter every second, should anything replace it
// (the replacement is then chained to). Self-test: XFNV_CRASHLOG_SELFTEST=<seconds> in the environment starts a
// thread that reads a null pointer that many seconds after start.

#include "crashlog/BuildInfo.h"
#include "xfnv/crashlog/CrashLog.hpp"

#include <csignal>
#include <cstdlib>
#include <intrin.h>

#include <windows.h>

#include <dbghelp.h>
#include <tlhelp32.h>

namespace xfnv::crashlog {
    namespace {

        // Ours, raised so that a pure virtual call or abort() reaches the filter like any other fatal exception.
        constexpr DWORD kPureCallCode = 0xE0F00001;
        constexpr DWORD kAbortCode = 0xE0F00002;
        // The debugger's thread-naming exception (SetThreadName), seen first-chance to learn the names.
        constexpr DWORD kThreadNameCode = 0x406D1388;
        constexpr DWORD kCppExceptionCode = 0xE06D7363;
        constexpr DWORD kWaitForLog = 180000;

        // The context's program counter, stack and frame pointers, the walk's machine and the log's register list.
        constexpr DWORD kMachine = IMAGE_FILE_MACHINE_AMD64;
        uintptr_t pcOf(const CONTEXT& arContext) {
            return arContext.Rip;
        }
        uintptr_t spOf(const CONTEXT& arContext) {
            return arContext.Rsp;
        }
        uintptr_t fpOf(const CONTEXT& arContext) {
            return arContext.Rbp;
        }
#define XFNV_CRASHLOG_REGISTERS(c)                                                                                     \
    {"RAX", (c).Rax}, {"RBX", (c).Rbx}, {"RCX", (c).Rcx}, {"RDX", (c).Rdx}, {"RSI", (c).Rsi}, {"RDI", (c).Rdi},        \
        {"RBP", (c).Rbp}, {"RSP", (c).Rsp}, {"R8", (c).R8}, {"R9", (c).R9}, {"R10", (c).R10}, {"R11", (c).R11},        \
        {"R12", (c).R12}, {"R13", (c).R13}, {"R14", (c).R14}, {"R15", (c).R15}, {                                      \
        "RIP", (c).Rip                                                                                                 \
    }

        using SymSetOptionsFunction = DWORD(WINAPI*)(DWORD);
        using SymInitializeFunction = BOOL(WINAPI*)(HANDLE, PCSTR, BOOL);
        using SymLoadModuleExFunction =
            DWORD64(WINAPI*)(HANDLE, HANDLE, PCSTR, PCSTR, DWORD64, DWORD, PMODLOAD_DATA, DWORD);
        using SymGetModuleInfo64Function = BOOL(WINAPI*)(HANDLE, DWORD64, PIMAGEHLP_MODULE64);
        using SymFromAddrFunction = BOOL(WINAPI*)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
        using SymGetLineFromAddr64Function = BOOL(WINAPI*)(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
        using StackWalk64Function = BOOL(WINAPI*)(
            DWORD,
            HANDLE,
            HANDLE,
            LPSTACKFRAME64,
            PVOID,
            PREAD_PROCESS_MEMORY_ROUTINE64,
            PFUNCTION_TABLE_ACCESS_ROUTINE64,
            PGET_MODULE_BASE_ROUTINE64,
            PTRANSLATE_ADDRESS_ROUTINE64
        );
        using MiniDumpWriteDumpFunction = BOOL(WINAPI*)(
            HANDLE,
            DWORD,
            HANDLE,
            MINIDUMP_TYPE,
            PMINIDUMP_EXCEPTION_INFORMATION,
            PMINIDUMP_USER_STREAM_INFORMATION,
            PMINIDUMP_CALLBACK_INFORMATION
        );

        struct DbgHelp {
            SymSetOptionsFunction SymSetOptions;
            SymInitializeFunction SymInitialize;
            SymLoadModuleExFunction SymLoadModuleEx;
            SymGetModuleInfo64Function SymGetModuleInfo64;
            SymFromAddrFunction SymFromAddr;
            SymGetLineFromAddr64Function SymGetLineFromAddr64;
            StackWalk64Function StackWalk64;
            PFUNCTION_TABLE_ACCESS_ROUTINE64 SymFunctionTableAccess64;
            PGET_MODULE_BASE_ROUTINE64 SymGetModuleBase64;
            MiniDumpWriteDumpFunction MiniDumpWriteDump;
        } gDbgHelp;
        bool gbSymbols = false;
        // dbghelp holds the process (SymInitialize): its walk can run. The walk needs no PDB (the images' unwind
        // data).
        bool gbWalk = false;

        struct ThreadName {
            DWORD uiId;
            char acName[32];
        };
        ThreadName gNames[64];
        volatile LONG glNames = 0;

        struct Module {
            uintptr_t uiBase;
            uintptr_t uiEnd;
            char acName[64];
        };
        Module gModules[256];
        int32_t giModules = 0;

        struct Thread {
            DWORD uiId;
            HANDLE hThread;
            bool bSuspended;
            CONTEXT kContext;
        };
        Thread gThreads[256];
        int32_t giThreads = 0;

        HANDLE ghRequest = nullptr;
        HANDLE ghDone = nullptr;
        DWORD guiWorker = 0;
        DWORD guiMainThread = 0;
        volatile LONG glCrashing = 0;
        EXCEPTION_POINTERS* gpException = nullptr;
        DWORD guiFaultThread = 0;
        LPTOP_LEVEL_EXCEPTION_FILTER gpPrevious = nullptr;

        // The exe's directory, with its separator
        wchar_t gwcDirectory[MAX_PATH];
        char gcDirectory[MAX_PATH];
        char gcExe[MAX_PATH];
        HANDLE ghFile = INVALID_HANDLE_VALUE;
        Out gOut;

        // The last region VirtualQuery found readable, so most reads skip the query.
        uintptr_t guiReadableBegin = 0;
        uintptr_t guiReadableEnd = 0;

        constexpr DWORD kReadable = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ |
            PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
        constexpr DWORD kExecutable = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
            PAGE_EXECUTE_WRITECOPY;

        bool copyGuarded(void* apDest, uintptr_t apSource, size_t auiSize) {
            __try {
                volatile const unsigned char* pFrom = reinterpret_cast<const unsigned char*>(apSource);
                unsigned char* pTo = static_cast<unsigned char*>(apDest);
                for (size_t i = 0; i < auiSize; ++i) {
                    pTo[i] = pFrom[i];
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
            return true;
        }

        const Module* moduleOf(uintptr_t auiAddress) {
            for (int32_t i = 0; i < giModules; ++i) {
                if (auiAddress >= gModules[i].uiBase && auiAddress < gModules[i].uiEnd) {
                    return &gModules[i];
                }
            }
            return nullptr;
        }

        // Prints the source path from its src/ on, else whole.
        void sourcePath(Out& arOut, const char* apPath) {
            const char* pShort = apPath;
            for (const char* p = apPath; *p; ++p) {
                if ((p[0] == '/' || p[0] == '\\') &&
                    p[1] == 's' &&
                    p[2] == 'r' &&
                    p[3] == 'c' &&
                    (p[4] == '/' || p[4] == '\\')) {
                    pShort = p + 1;
                }
            }
            arOut.str(pShort);
        }

        bool symbolGuarded(
            uintptr_t auiAddress,
            SYMBOL_INFO* apSymbol,
            DWORD64& arDisplacement,
            IMAGEHLP_LINE64* apLine,
            DWORD& arLineDisplacement,
            bool& arLine
        ) {
            __try {
                const HANDLE hProcess = GetCurrentProcess();
                if (!gDbgHelp.SymFromAddr(hProcess, auiAddress, &arDisplacement, apSymbol)) {
                    return false;
                }
                arLine = gDbgHelp.SymGetLineFromAddr64(hProcess, auiAddress, &arLineDisplacement, apLine) != FALSE;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
            return true;
        }

        const char* threadName(DWORD auiId) {
            if (auiId == guiMainThread) {
                return "main";
            }
            if (auiId == guiWorker) {
                return "CrashLog";
            }
            const LONG lCount = glNames < 64 ? glNames : 64;
            for (LONG i = lCount - 1; i >= 0; --i) {
                if (gNames[i].uiId == auiId) {
                    return gNames[i].acName;
                }
            }
            return nullptr;
        }

        void rememberName(DWORD auiId, uintptr_t apName) {
            const LONG lSlot = InterlockedIncrement(&glNames) - 1;
            if (lSlot >= 64) {
                return;
            }
            ThreadName& rName = gNames[lSlot];
            uint32_t uiLength = 0;
            while (uiLength < sizeof(rName.acName) - 1 &&
                   readMemory(&rName.acName[uiLength], apName + uiLength, 1) &&
                   rName.acName[uiLength]) {
                ++uiLength;
            }
            rName.acName[uiLength] = '\0';
            rName.uiId = auiId;
        }

        void contextRegisters(Out& arOut, const CONTEXT& arContext, uintptr_t auiStackLow, uintptr_t auiStackHigh) {
            const Register akRegisters[] = {XFNV_CRASHLOG_REGISTERS(arContext)};
            registers(arOut, akRegisters, sizeof(akRegisters) / sizeof(akRegisters[0]), auiStackLow, auiStackHigh);
            arOut.str("  EFLAGS ")
                .hex(arContext.EFlags)
                .str(" CS ")
                .hex(arContext.SegCs, 4)
                .str(" SS ")
                .hex(arContext.SegSs, 4)
                .str(" DS ")
                .hex(arContext.SegDs, 4)
                .str(" FS ")
                .hex(arContext.SegFs, 4)
                .nl();
        }

        // dbghelp holds only the exe's module (loadSymbols): a frame in a system DLL (ntdll, kernelbase) takes its
        // unwind data and module base from the loader's tables instead, so the walk goes on through it.
        PVOID CALLBACK functionTable(HANDLE ahProcess, DWORD64 auiAddress) {
            if (PVOID pEntry = gDbgHelp.SymFunctionTableAccess64(ahProcess, auiAddress)) {
                return pEntry;
            }
            DWORD64 uiBase = 0;
            return RtlLookupFunctionEntry(auiAddress, &uiBase, nullptr);
        }

        DWORD64 CALLBACK moduleBase(HANDLE ahProcess, DWORD64 auiAddress) {
            if (const DWORD64 uiBase = gDbgHelp.SymGetModuleBase64(ahProcess, auiAddress)) {
                return uiBase;
            }
            PVOID pBase = nullptr;
            return reinterpret_cast<DWORD64>(RtlPcToFileHeader(reinterpret_cast<PVOID>(auiAddress), &pBase));
        }

        bool walkGuarded(HANDLE ahThread, STACKFRAME64* apFrame, CONTEXT* apContext) {
            __try {
                return gDbgHelp.StackWalk64(
                           kMachine,
                           GetCurrentProcess(),
                           ahThread,
                           apFrame,
                           apContext,
                           nullptr,
                           functionTable,
                           moduleBase,
                           nullptr
                       ) != FALSE;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
        }

        // The frames: dbghelp's walk (the images' unwind data, PDB or not), else the frame-pointer chain. A walk
        // longer than kMaxFrames (a deep recursion) goes on to count the frames and prints the outermost kOuterFrames,
        // where it began.
        constexpr int32_t kOuterFrames = 24;
        constexpr int32_t kWalkLimit = 1000000;
        void frames(Out& arOut, HANDLE ahThread, const CONTEXT& arContext, uintptr_t auiStackHigh) {
            if (gbWalk && gDbgHelp.StackWalk64) {
                arOut.str(" Frames (dbghelp StackWalk64):").nl();
                static CONTEXT kContext;
                kContext = arContext;
                STACKFRAME64 kFrame = {};
                kFrame.AddrPC.Offset = pcOf(arContext);
                kFrame.AddrPC.Mode = AddrModeFlat;
                kFrame.AddrFrame.Offset = fpOf(arContext);
                kFrame.AddrFrame.Mode = AddrModeFlat;
                kFrame.AddrStack.Offset = spOf(arContext);
                kFrame.AddrStack.Mode = AddrModeFlat;
                static DWORD64 auiOuter[kOuterFrames][2];
                int32_t i = 0;
                DWORD64 uiLastStack = 0;
                for (; i < kWalkLimit && walkGuarded(ahThread, &kFrame, &kContext) && kFrame.AddrPC.Offset; ++i) {
                    if (i < kMaxFrames) {
                        arOut.str("  #")
                            .dec(i)
                            .str(" ")
                            .hex(kFrame.AddrPC.Offset)
                            .str(" (stack ")
                            .hex(kFrame.AddrStack.Offset)
                            .str(") ");
                        codeAddress(arOut, static_cast<uintptr_t>(kFrame.AddrPC.Offset));
                        arOut.nl();
                    }
                    auiOuter[i % kOuterFrames][0] = kFrame.AddrPC.Offset;
                    auiOuter[i % kOuterFrames][1] = kFrame.AddrStack.Offset;
                    // a walk that stops climbing the stack has lost its way
                    if (i && kFrame.AddrStack.Offset <= uiLastStack) {
                        ++i;
                        break;
                    }
                    uiLastStack = kFrame.AddrStack.Offset;
                }
                if (i > kMaxFrames) {
                    const int32_t iFirst = i - kOuterFrames > kMaxFrames ? i - kOuterFrames : kMaxFrames;
                    arOut.str("  ... ").dec(i).str(" frames in all; the outermost:").nl();
                    for (int32_t j = iFirst; j < i; ++j) {
                        arOut.str("  #")
                            .dec(j)
                            .str(" ")
                            .hex(auiOuter[j % kOuterFrames][0])
                            .str(" (stack ")
                            .hex(auiOuter[j % kOuterFrames][1])
                            .str(") ");
                        codeAddress(arOut, static_cast<uintptr_t>(auiOuter[j % kOuterFrames][0]));
                        arOut.nl();
                    }
                }
                return;
            }
            arOut.str(" Frames (frame-pointer chain):").nl();
            arOut.str("  #0 ").hex(pcOf(arContext)).chr(' ');
            codeAddress(arOut, pcOf(arContext));
            arOut.nl();
            uintptr_t uiFrame = fpOf(arContext);
            for (int32_t i = 1; i < kMaxFrames && uiFrame >= spOf(arContext) && uiFrame + 2 * kWord <= auiStackHigh;
                 ++i) {
                uintptr_t auiFrame[2];
                if (!readMemory(auiFrame, uiFrame, 2 * kWord) || !isCode(auiFrame[1])) {
                    break;
                }
                arOut.str("  #").dec(i).chr(' ').hex(auiFrame[1]).str(" (frame ").hex(uiFrame).str(") ");
                codeAddress(arOut, auiFrame[1]);
                arOut.nl();
                if (auiFrame[0] <= uiFrame) {
                    break;
                }
                uiFrame = auiFrame[0];
            }
        }

        // The committed stack above the stack pointer ends where its region does (the thread's stack base).
        uintptr_t stackTop(uintptr_t auiSp) {
            MEMORY_BASIC_INFORMATION kInfo;
            if (VirtualQuery(reinterpret_cast<void*>(auiSp), &kInfo, sizeof(kInfo)) != sizeof(kInfo) ||
                kInfo.State != MEM_COMMIT) {
                return auiSp;
            }
            return reinterpret_cast<uintptr_t>(kInfo.BaseAddress) + kInfo.RegionSize;
        }

        void thread(Out& arOut, const Thread& arThread, bool abFault) {
            const char* pName = threadName(arThread.uiId);
            arOut.nl().str(abFault ? "Faulting thread " : "Thread ").dec(arThread.uiId);
            if (pName) {
                arOut.str(" \"").str(pName).chr('"');
            }
            const CONTEXT& rContext = arThread.kContext;
            const uintptr_t uiHigh = stackTop(spOf(rContext));
            threadClass(arOut, spOf(rContext), uiHigh, arThread.uiId);
            if (!abFault && !arThread.bSuspended) {
                arOut.str(" (not suspended: its state may be moving)");
            }
            arOut.nl();
            contextRegisters(arOut, rContext, spOf(rContext), uiHigh);
            frames(arOut, arThread.hThread, rContext, uiHigh);
            scan(arOut, spOf(rContext), uiHigh, abFault ? kFaultScanWords : kThreadScanWords);
        }

        const char* exceptionName(DWORD auiCode) {
            switch (auiCode) {
                case EXCEPTION_ACCESS_VIOLATION: return "access violation";
                case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal instruction";
                case EXCEPTION_PRIV_INSTRUCTION: return "privileged instruction";
                case EXCEPTION_INT_DIVIDE_BY_ZERO: return "integer divide by zero";
                case EXCEPTION_INT_OVERFLOW: return "integer overflow";
                case EXCEPTION_STACK_OVERFLOW: return "stack overflow";
                case EXCEPTION_IN_PAGE_ERROR: return "in-page error";
                case EXCEPTION_BREAKPOINT: return "breakpoint";
                case EXCEPTION_DATATYPE_MISALIGNMENT: return "misalignment";
                case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "float divide by zero";
                case EXCEPTION_FLT_INVALID_OPERATION: return "float invalid operation";
                case EXCEPTION_FLT_OVERFLOW: return "float overflow";
                case EXCEPTION_FLT_STACK_CHECK: return "float stack check";
                case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "array bounds exceeded";
                case 0xC0000374: return "heap corruption";
                case 0xC0000409: return "stack buffer overrun (__fastfail)";
                case kCppExceptionCode: return "C++ exception";
                case kPureCallCode: return "pure virtual call";
                case kAbortCode: return "abort()";
                default: return "exception";
            }
        }

        DateTime dateTime(const SYSTEMTIME& arTime) {
            return {arTime.wYear, arTime.wMonth, arTime.wDay, arTime.wHour, arTime.wMinute, arTime.wSecond};
        }

        void header(Out& arOut, const SYSTEMTIME& arNow) {
            const EXCEPTION_RECORD& rRecord = *gpException->ExceptionRecord;
            arOut.str("Fallout: New Vegas (fnv_decomp native program) crash log").nl();
            // the link time is the image's own time stamp
            const auto* pDos = reinterpret_cast<const IMAGE_DOS_HEADER*>(kImage.uiBegin);
            const auto* pNt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(kImage.uiBegin + pDos->e_lfanew);
            ULARGE_INTEGER kLinked;
            kLinked.QuadPart = (static_cast<uint64_t>(pNt->FileHeader.TimeDateStamp) + 11644473600ull) * 10000000ull;
            FILETIME kLinkedFile = {kLinked.LowPart, kLinked.HighPart};
            SYSTEMTIME kLinkedTime;
            FileTimeToSystemTime(&kLinkedFile, &kLinkedTime);
            arOut.str("Build: commit " XFNV_CRASHLOG_COMMIT ", linked ");
            printTime(arOut, dateTime(kLinkedTime));
            arOut.str(" UTC").nl();
            arOut.str("Image: ").str(gcExe).str(" at ").hex(kImage.uiBegin).str("..").hex(kImage.uiEnd).nl();
            arOut.str("Time: ");
            printTime(arOut, dateTime(arNow));
            arOut.str(" local").nl();
            arOut.str("Exception: ")
                .hex(rRecord.ExceptionCode)
                .chr(' ')
                .str(exceptionName(rRecord.ExceptionCode))
                .str(" at ")
                .hex(reinterpret_cast<uintptr_t>(rRecord.ExceptionAddress))
                .chr(' ');
            codeAddress(arOut, reinterpret_cast<uintptr_t>(rRecord.ExceptionAddress));
            arOut.nl();
            if ((rRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
                 rRecord.ExceptionCode == EXCEPTION_IN_PAGE_ERROR) &&
                rRecord.NumberParameters >= 2) {
                const ULONG_PTR uiKind = rRecord.ExceptionInformation[0];
                arOut.str("  ")
                    .str(
                        uiKind == 0       ? "reading"
                            : uiKind == 1 ? "writing"
                                          : "executing"
                    )
                    .chr(' ')
                    .hex(rRecord.ExceptionInformation[1]);
                annotateData(arOut, rRecord.ExceptionInformation[1]);
                arOut.nl();
            }
            if (rRecord.ExceptionCode == kCppExceptionCode && rRecord.NumberParameters >= 3) {
                // ThrowInfo: its catchable type array at +12, whose first type's descriptor name is at +8
                uint32_t uiArray = 0;
                uint32_t uiFirst = 0;
                uint32_t uiDescriptor = 0;
                if (readMemory(&uiArray, rRecord.ExceptionInformation[2] + 12, 4) &&
                    readMemory(&uiFirst, uiArray + 4, 4) &&
                    readMemory(&uiDescriptor, uiFirst + 4, 4)) {
                    arOut.str("  thrown ").guardedStr(uiDescriptor + 8, 120).nl();
                }
            }
            arOut.str("Symbols: ");
            IMAGEHLP_MODULE64 kModule = {};
            kModule.SizeOfStruct = sizeof(kModule);
            if (gbSymbols && gDbgHelp.SymGetModuleInfo64(GetCurrentProcess(), kImage.uiBegin, &kModule)) {
                arOut.str("dbghelp, type ").dec(kModule.SymType).str(", ").str(kModule.LoadedPdbName).nl();
            }
            else {
                arOut
                    .str(
                        "none in-process; harness/tools/crashlog-symbolize.py LOG [PDB] names the image+0xRVA addresses"
                    )
                    .nl();
            }
        }

        void loadSymbols() {
            if (!gDbgHelp.SymInitialize || !gDbgHelp.SymLoadModuleEx) {
                return;
            }
            __try {
                gDbgHelp.SymSetOptions(
                    SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_NO_PROMPTS
                );
                const HANDLE hProcess = GetCurrentProcess();
                gbWalk = gDbgHelp.SymInitialize(hProcess, gcDirectory, FALSE) != FALSE;
                if (gbWalk &&
                    gDbgHelp.SymLoadModuleEx(
                        hProcess,
                        nullptr,
                        gcExe,
                        nullptr,
                        kImage.uiBegin,
                        static_cast<DWORD>(kImage.uiEnd - kImage.uiBegin),
                        nullptr,
                        0
                    )) {
                    IMAGEHLP_MODULE64 kModule = {};
                    kModule.SizeOfStruct = sizeof(kModule);
                    gbSymbols = gDbgHelp.SymGetModuleInfo64(hProcess, kImage.uiBegin, &kModule) &&
                        (kModule.SymType == SymPdb || kModule.SymType == SymCv);
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                gbSymbols = false;
            }
        }

        // The process's threads and modules; every thread but this one and the faulting one suspended. Should the
        // snapshot with modules fail, the threads are taken alone; the faulting thread is listed even when no snapshot
        // is had.
        void snapshot() {
            const DWORD uiProcess = GetCurrentProcessId();
            HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD | TH32CS_SNAPMODULE, uiProcess);
            if (hSnapshot == INVALID_HANDLE_VALUE) {
                hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            }
            bool bFault = false;
            if (hSnapshot != INVALID_HANDLE_VALUE) {
                MODULEENTRY32 kModule;
                kModule.dwSize = sizeof(kModule);
                for (BOOL bMore = Module32First(hSnapshot, &kModule); bMore && giModules < 256;
                     bMore = Module32Next(hSnapshot, &kModule)) {
                    Module& rModule = gModules[giModules++];
                    rModule.uiBase = reinterpret_cast<uintptr_t>(kModule.modBaseAddr);
                    rModule.uiEnd = rModule.uiBase + kModule.modBaseSize;
                    lstrcpynA(rModule.acName, kModule.szModule, sizeof(rModule.acName));
                }
                THREADENTRY32 kThread;
                kThread.dwSize = sizeof(kThread);
                for (BOOL bMore = Thread32First(hSnapshot, &kThread); bMore && giThreads < 256;
                     bMore = Thread32Next(hSnapshot, &kThread)) {
                    if (kThread.th32OwnerProcessID != uiProcess || kThread.th32ThreadID == guiWorker) {
                        continue;
                    }
                    Thread& rThread = gThreads[giThreads];
                    rThread.uiId = kThread.th32ThreadID;
                    rThread.hThread = OpenThread(
                        THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
                        FALSE,
                        rThread.uiId
                    );
                    if (!rThread.hThread) {
                        continue;
                    }
                    rThread.bSuspended = rThread.uiId != guiFaultThread &&
                        SuspendThread(rThread.hThread) != static_cast<DWORD>(-1);
                    if (rThread.uiId == guiFaultThread) {
                        rThread.kContext = *gpException->ContextRecord;
                        bFault = true;
                    }
                    else {
                        rThread.kContext.ContextFlags = CONTEXT_FULL;
                        if (!GetThreadContext(rThread.hThread, &rThread.kContext)) {
                            if (rThread.bSuspended) {
                                ResumeThread(rThread.hThread);
                            }
                            CloseHandle(rThread.hThread);
                            continue;
                        }
                    }
                    ++giThreads;
                }
                CloseHandle(hSnapshot);
            }
            if (!bFault && giThreads < 256) {
                Thread& rThread = gThreads[giThreads++];
                rThread.uiId = guiFaultThread;
                rThread.hThread = OpenThread(THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT, FALSE, guiFaultThread);
                rThread.bSuspended = false;
                rThread.kContext = *gpException->ContextRecord;
            }
        }

        // Writes the log (and the dump) of the exception gpException of thread guiFaultThread.
        void writeCrash() {
            SYSTEMTIME kNow;
            GetLocalTime(&kNow);
            // FalloutNV.crash-yyyymmdd-hhmmss.log
            wchar_t wcPath[MAX_PATH + 64];
            lstrcpynW(wcPath, gwcDirectory, MAX_PATH);
            wchar_t* pEnd = wcPath + lstrlenW(wcPath);
            lstrcpyW(pEnd, L"FalloutNV.crash-");
            pEnd += lstrlenW(pEnd);
            const WORD awParts[] = {
                static_cast<WORD>(kNow.wYear / 100),
                static_cast<WORD>(kNow.wYear % 100),
                kNow.wMonth,
                kNow.wDay,
                0xFFFF,
                kNow.wHour,
                kNow.wMinute,
                kNow.wSecond
            };
            for (WORD wPart : awParts) {
                if (wPart == 0xFFFF) {
                    *pEnd++ = L'-';
                }
                else {
                    *pEnd++ = static_cast<wchar_t>(L'0' + wPart / 10);
                    *pEnd++ = static_cast<wchar_t>(L'0' + wPart % 10);
                }
            }
            lstrcpyW(pEnd, L".log");
            ghFile = CreateFileW(
                wcPath,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                nullptr,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
            );
            if (ghFile == INVALID_HANDLE_VALUE) {
                return;
            }
            loadSymbols();
            snapshot();

            header(gOut, kNow);
            gOut.flush();
            for (int32_t i = 0; i < giThreads; ++i) {
                if (gThreads[i].uiId == guiFaultThread) {
                    thread(gOut, gThreads[i], true);
                }
            }
            gOut.flush();
            for (int32_t i = 0; i < giThreads; ++i) {
                if (gThreads[i].uiId != guiFaultThread) {
                    thread(gOut, gThreads[i], false);
                    gOut.flush();
                }
            }
            gOut.nl().str("Modules:").nl();
            for (int32_t i = 0; i < giModules; ++i) {
                gOut.str("  ")
                    .hex(gModules[i].uiBase)
                    .str("..")
                    .hex(gModules[i].uiEnd)
                    .chr(' ')
                    .str(gModules[i].acName)
                    .nl();
            }
            gOut.flush();

            if (gDbgHelp.MiniDumpWriteDump) {
                lstrcpyW(pEnd, L".dmp");
                const HANDLE hDump = CreateFileW(
                    wcPath,
                    GENERIC_WRITE,
                    0,
                    nullptr,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    nullptr
                );
                MINIDUMP_EXCEPTION_INFORMATION kInfo = {guiFaultThread, gpException, FALSE};
                const bool bDump = hDump != INVALID_HANDLE_VALUE &&
                    gDbgHelp.MiniDumpWriteDump(
                        GetCurrentProcess(),
                        GetCurrentProcessId(),
                        hDump,
                        static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithDataSegs),
                        &kInfo,
                        nullptr,
                        nullptr
                    );
                if (hDump != INVALID_HANDLE_VALUE) {
                    CloseHandle(hDump);
                }
                gOut.nl().str(bDump ? "Minidump: written beside this log (.dmp)" : "Minidump: failed").nl();
            }
            gOut.str("End of log").nl();
            gOut.flush();
            CloseHandle(ghFile);
            ghFile = INVALID_HANDLE_VALUE;
            for (int32_t i = 0; i < giThreads; ++i) {
                if (gThreads[i].bSuspended) {
                    ResumeThread(gThreads[i].hThread);
                }
            }
        }

        LONG WINAPI onUnhandled(EXCEPTION_POINTERS* apException) {
            const DWORD uiThread = GetCurrentThreadId();
            if (uiThread != guiWorker) {
                if (InterlockedCompareExchange(&glCrashing, 1, 0) == 0) {
                    gpException = apException;
                    guiFaultThread = uiThread;
                    SetEvent(ghRequest);
                }
                // a second faulting thread waits too, so the process outlives the first one's log
                WaitForSingleObject(ghDone, kWaitForLog);
            }
            return gpPrevious ? gpPrevious(apException) : EXCEPTION_CONTINUE_SEARCH;
        }

        LONG CALLBACK onFirstChance(EXCEPTION_POINTERS* apException) {
            const EXCEPTION_RECORD& rRecord = *apException->ExceptionRecord;
            if (rRecord.ExceptionCode == kThreadNameCode &&
                rRecord.NumberParameters >= 3 &&
                rRecord.ExceptionInformation[0] == 0x1000) {
                const ULONG_PTR uiThread = rRecord.ExceptionInformation[2];
                rememberName(
                    uiThread == static_cast<ULONG_PTR>(-1) ? GetCurrentThreadId() : static_cast<DWORD>(uiThread),
                    rRecord.ExceptionInformation[1]
                );
            }
            return EXCEPTION_CONTINUE_SEARCH;
        }

        DWORD WINAPI worker(void*) {
            while (WaitForSingleObject(ghRequest, 1000) != WAIT_OBJECT_0) {
                const LPTOP_LEVEL_EXCEPTION_FILTER pFilter = SetUnhandledExceptionFilter(&onUnhandled);
                if (pFilter != &onUnhandled) {
                    gpPrevious = pFilter;
                }
            }
            writeCrash();
            SetEvent(ghDone);
            return 0;
        }

        DWORD WINAPI selfTest(void* apSeconds) {
            Sleep(static_cast<DWORD>(reinterpret_cast<uintptr_t>(apSeconds)) * 1000);
            rememberName(GetCurrentThreadId(), reinterpret_cast<uintptr_t>("CrashLogSelfTest"));
            volatile int32_t* pNull = nullptr;
            return static_cast<DWORD>(*pNull);
        }

        void __cdecl onPureCall() {
            RaiseException(kPureCallCode, EXCEPTION_NONCONTINUABLE, 0, nullptr);
        }

        void __cdecl onAbort(int) {
            RaiseException(kAbortCode, EXCEPTION_NONCONTINUABLE, 0, nullptr);
        }

        template <typename Function>
        void dbgHelp(HMODULE ahModule, const char* apName, Function& arFunction) {
            arFunction = reinterpret_cast<Function>(reinterpret_cast<void*>(GetProcAddress(ahModule, apName)));
        }

        extern "C" void __cdecl __report_gsfailure(ULONG_PTR);

        // The x64 CRT's failed stack-cookie check (__security_check_cookie jumps to __report_gsfailure). The CRT's
        // version fast-fails (int 0x29), which reaches no exception filter, so the process died without a log.
        // install() makes the CRT's entry jump here: the failure goes to the filter as exception c0000409 at the caller
        // (the epilogue of the function whose cookie was overwritten), then the process ends as the CRT would end it.
        void __cdecl onGsFailure(ULONG_PTR auiCookie) {
            static CONTEXT kContext;
            static EXCEPTION_RECORD kRecord;
            RtlCaptureContext(&kContext);
            kRecord.ExceptionCode = STATUS_STACK_BUFFER_OVERRUN;
            kRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            kRecord.ExceptionAddress = _ReturnAddress();
            kRecord.NumberParameters = 1;
            kRecord.ExceptionInformation[0] = auiCookie;
            EXCEPTION_POINTERS kPointers = {&kRecord, &kContext};
            onUnhandled(&kPointers);
            TerminateProcess(GetCurrentProcess(), STATUS_STACK_BUFFER_OVERRUN);
        }

        // Overwrites the CRT's __report_gsfailure entry with `mov rax, onGsFailure; jmp rax` (the CRT's object cannot
        // be replaced at link time: libcmt's gs_report defines it beside symbols the CRT needs).
        void hookGsFailure() {
            uint8_t* pEntry = reinterpret_cast<uint8_t*>(&__report_gsfailure);
            uint8_t aucJump[12] = {0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xE0};
            const uintptr_t uiTarget = reinterpret_cast<uintptr_t>(&onGsFailure);
            memcpy(aucJump + 2, &uiTarget, sizeof(uiTarget));
            DWORD uiOld = 0;
            if (VirtualProtect(pEntry, sizeof(aucJump), PAGE_EXECUTE_READWRITE, &uiOld)) {
                memcpy(pEntry, aucJump, sizeof(aucJump));
                VirtualProtect(pEntry, sizeof(aucJump), uiOld, &uiOld);
                FlushInstructionCache(GetCurrentProcess(), pEntry, sizeof(aucJump));
            }
        }

        // The main thread's stack kept for the handlers of its stack overflow.
        constexpr ULONG kStackGuarantee = 256 * 1024;
        constexpr uintptr_t kStackPage = 4096;

        // Wine commits a thread's whole stack and keeps its one guard page at the bottom, so the guarantee alone
        // changes nothing there: the overflow is raised at the last page, with no room left to dispatch it, and Wine
        // ends the thread
        // ("virtual_setup_exception stack overflow") with no log. A guard page at the top of the guaranteed space makes
        // Wine raise the overflow there (a guard page touched inside the guaranteed space is an overflow), with the
        // guaranteed bytes still free below it. On Windows that page is not committed yet, and the guarantee works by
        // itself.
        void guardStack() {
            MEMORY_BASIC_INFORMATION kInfo;
            if (VirtualQuery(&kInfo, &kInfo, sizeof(kInfo)) != sizeof(kInfo)) {
                return;
            }
            const uintptr_t uiPage = reinterpret_cast<uintptr_t>(kInfo.AllocationBase) + kStackGuarantee;
            if (uiPage + 64 * 1024 >= reinterpret_cast<uintptr_t>(&kInfo) ||
                VirtualQuery(reinterpret_cast<void*>(uiPage), &kInfo, sizeof(kInfo)) != sizeof(kInfo) ||
                kInfo.State != MEM_COMMIT ||
                kInfo.Protect != PAGE_READWRITE) {
                return;
            }
            DWORD uiOld = 0;
            VirtualProtect(reinterpret_cast<void*>(uiPage), kStackPage, PAGE_READWRITE | PAGE_GUARD, &uiOld);
        }

        void install() {
            guiMainThread = GetCurrentThreadId();
            const HMODULE hImage = GetModuleHandleW(nullptr);
            const auto* pDos = reinterpret_cast<const IMAGE_DOS_HEADER*>(hImage);
            const auto* pNt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(
                reinterpret_cast<uintptr_t>(hImage) + pDos->e_lfanew
            );
            kImage.uiBegin = reinterpret_cast<uintptr_t>(hImage);
            kImage.uiEnd = kImage.uiBegin + pNt->OptionalHeader.SizeOfImage;

            GetModuleFileNameA(nullptr, gcExe, MAX_PATH);
            GetModuleFileNameW(nullptr, gwcDirectory, MAX_PATH);
            lstrcpynA(gcDirectory, gcExe, MAX_PATH);
            for (wchar_t* p = gwcDirectory + lstrlenW(gwcDirectory);
                 p > gwcDirectory && p[-1] != L'\\' && p[-1] != L'/';) {
                *--p = L'\0';
            }
            for (char* p = gcDirectory + lstrlenA(gcDirectory); p > gcDirectory && p[-1] != '\\' && p[-1] != '/';) {
                *--p = '\0';
            }

            // loaded now: LoadLibrary in a crashed process could wait on a loader lock its owner never releases
            if (const HMODULE hDbgHelp = LoadLibraryW(L"dbghelp.dll")) {
                dbgHelp(hDbgHelp, "SymSetOptions", gDbgHelp.SymSetOptions);
                dbgHelp(hDbgHelp, "SymInitialize", gDbgHelp.SymInitialize);
                dbgHelp(hDbgHelp, "SymLoadModuleEx", gDbgHelp.SymLoadModuleEx);
                dbgHelp(hDbgHelp, "SymGetModuleInfo64", gDbgHelp.SymGetModuleInfo64);
                dbgHelp(hDbgHelp, "SymFromAddr", gDbgHelp.SymFromAddr);
                dbgHelp(hDbgHelp, "SymGetLineFromAddr64", gDbgHelp.SymGetLineFromAddr64);
                dbgHelp(hDbgHelp, "StackWalk64", gDbgHelp.StackWalk64);
                dbgHelp(hDbgHelp, "SymFunctionTableAccess64", gDbgHelp.SymFunctionTableAccess64);
                dbgHelp(hDbgHelp, "SymGetModuleBase64", gDbgHelp.SymGetModuleBase64);
                dbgHelp(hDbgHelp, "MiniDumpWriteDump", gDbgHelp.MiniDumpWriteDump);
            }

            ghRequest = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            ghDone = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            if (!ghRequest || !ghDone || !CreateThread(nullptr, 64 * 1024, &worker, nullptr, 0, &guiWorker)) {
                return;
            }
            gpPrevious = SetUnhandledExceptionFilter(&onUnhandled);
            AddVectoredExceptionHandler(0, &onFirstChance);
            // A stack overflow leaves the faulting thread no room to dispatch the exception: keep this much of the
            // installing thread's stack, the main thread's, for the handlers (the capture itself runs on the worker's
            // stack).
            ULONG uiStackGuarantee = kStackGuarantee;
            SetThreadStackGuarantee(&uiStackGuarantee);
            guardStack();
            _set_purecall_handler(&onPureCall);
            signal(SIGABRT, &onAbort);
            hookGsFailure();

            char acSeconds[16];
            if (GetEnvironmentVariableA("XFNV_CRASHLOG_SELFTEST", acSeconds, sizeof(acSeconds))) {
                uintptr_t uiSeconds = 0;
                for (const char* p = acSeconds; *p >= '0' && *p <= '9'; ++p) {
                    uiSeconds = uiSeconds * 10 + (*p - '0');
                }
                CreateThread(nullptr, 0, &selfTest, reinterpret_cast<void*>(uiSeconds), 0, nullptr);
            }
        }

        struct Installer {
            Installer() {
                install();
            }
        } gInstaller;

    } // namespace

    // Whether auiAddress lies in an executable section of a loaded image.
    bool isCode(uintptr_t auiAddress) {
        MEMORY_BASIC_INFORMATION kInfo;
        return auiAddress >= 0x10000 &&
            VirtualQuery(reinterpret_cast<void*>(auiAddress), &kInfo, sizeof(kInfo)) == sizeof(kInfo) &&
            kInfo.State == MEM_COMMIT &&
            kInfo.Type == MEM_IMAGE &&
            (kInfo.Protect & kExecutable) &&
            !(kInfo.Protect & PAGE_GUARD);
    }

    // "Class::Function+0x1A (src/.../File.cpp:123)" for the exe with symbols, else "module+0xRVA".
    void codeAddress(Out& arOut, uintptr_t auiAddress) {
        const Module* pModule = moduleOf(auiAddress);
        if (gbSymbols && auiAddress >= kImage.uiBegin && auiAddress < kImage.uiEnd) {
            alignas(8) static char acSymbol[sizeof(SYMBOL_INFO) + 512];
            SYMBOL_INFO* pSymbol = reinterpret_cast<SYMBOL_INFO*>(acSymbol);
            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = 511;
            static IMAGEHLP_LINE64 kLine;
            kLine.SizeOfStruct = sizeof(kLine);
            DWORD64 uiDisplacement = 0;
            DWORD uiLineDisplacement = 0;
            bool bLine = false;
            if (symbolGuarded(auiAddress, pSymbol, uiDisplacement, &kLine, uiLineDisplacement, bLine)) {
                pSymbol->Name[pSymbol->MaxNameLen] = '\0';
                arOut.str(pSymbol->Name).str("+0x").hex(uiDisplacement, 1);
                if (bLine && kLine.FileName) {
                    arOut.str(" (");
                    sourcePath(arOut, kLine.FileName);
                    arOut.chr(':').dec(kLine.LineNumber).chr(')');
                }
                return;
            }
        }
        if (pModule) {
            arOut.str(pModule->acName).str("+0x").hex(auiAddress - pModule->uiBase, 1);
            return;
        }
        MEMORY_BASIC_INFORMATION kInfo;
        VirtualQuery(reinterpret_cast<void*>(auiAddress), &kInfo, sizeof(kInfo));
        arOut.str("<image ")
            .hex(reinterpret_cast<uintptr_t>(kInfo.AllocationBase))
            .str(">+0x")
            .hex(auiAddress - reinterpret_cast<uintptr_t>(kInfo.AllocationBase), 1);
    }

    bool readMemory(void* apDest, uintptr_t apSource, size_t auiSize) {
        if (!auiSize) {
            return true;
        }
        if (apSource < 0x10000 || apSource + auiSize < apSource) {
            return false;
        }
        if (apSource < guiReadableBegin || apSource + auiSize > guiReadableEnd) {
            for (uintptr_t uiPage = apSource; uiPage < apSource + auiSize;) {
                MEMORY_BASIC_INFORMATION kInfo;
                if (VirtualQuery(reinterpret_cast<void*>(uiPage), &kInfo, sizeof(kInfo)) != sizeof(kInfo) ||
                    kInfo.State != MEM_COMMIT ||
                    (kInfo.Protect & (PAGE_GUARD | PAGE_NOACCESS)) ||
                    !(kInfo.Protect & kReadable)) {
                    return false;
                }
                guiReadableBegin = reinterpret_cast<uintptr_t>(kInfo.BaseAddress);
                guiReadableEnd = guiReadableBegin + kInfo.RegionSize;
                uiPage = guiReadableEnd;
            }
        }
        return copyGuarded(apDest, apSource, auiSize);
    }

    void writeOut(const char* apText, uint32_t auiSize) {
        DWORD uiWritten = 0;
        if (ghFile != INVALID_HANDLE_VALUE) {
            WriteFile(ghFile, apText, auiSize, &uiWritten, nullptr);
        }
    }

} // namespace xfnv::crashlog
