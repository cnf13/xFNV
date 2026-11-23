// The crash log's POSIX side (Linux, x86-64): on SIGSEGV, SIGBUS, SIGILL, SIGFPE or SIGABRT,
// FalloutNV.crash-<yyyymmdd-hhmmss>.log beside the program (local time), then the process dies of the signal as it
// would have. The faulting thread writes the log in its handler, on an alternate stack on the thread that installed
// it (the main thread: a stack overflow there still gets its log; another thread's overflow dies without one). It
// stops every other thread with a dump signal (tgkill): each one records its registers and frames in its own
// handler and waits there until the log is written, so its stack holds still while it is read.
//
// Frames come from _Unwind_Backtrace in the thread's handler: the program is built without frame pointers (-O2), so a
// frame-pointer walk from the context finds nothing; the unwinder follows the .eh_frame tables every object has on
// x86-64 and steps over the kernel's signal frame by the restorer's own unwind table (glibc's __restore_rt). The walk
// starts at the frame whose address is the context's instruction pointer. No symbols in-process: every code address
// is module+0xOFFSET (OFFSET is the address in the ELF's own terms), which harness/tools/crashlog-symbolize.py LOG
// resolves afterwards from the ELF's DWARF. Memory is read with process_vm_readv on the process itself, which fails
// with EFAULT on an unreadable page instead of faulting. The modules come from dl_iterate_phdr, taken before the
// other threads are stopped (it takes the loader's lock).
//
// It installs itself from a static initialiser. Self-test: XFNV_CRASHLOG_SELFTEST=<seconds> in the environment starts
// a thread that reads a null pointer that many seconds after start.

#include "crashlog/BuildInfo.h"
#include "xfnv/crashlog/CrashLog.hpp"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cxxabi.h>
#include <typeinfo>

#include <climits>
#include <csignal>
#include <fcntl.h>
#include <link.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <ucontext.h>
#include <unistd.h>
#include <unwind.h>

namespace xfnv::crashlog {
    namespace {

        constexpr int kCrashSignals[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT};
        constexpr int32_t kSignalCount = sizeof(kCrashSignals) / sizeof(kCrashSignals[0]);
        // How long a second crashing thread waits for the first one's log, and how long the log waits for a thread to
        // answer its dump signal.
        constexpr int32_t kWaitForLogMs = 180000;
        constexpr int32_t kWaitForThreadsMs = 1000;
        // The x86-64 page fault's error code: the access was a write, an instruction fetch.
        constexpr uintptr_t kErrorWrite = 0x2;
        constexpr uintptr_t kErrorFetch = 0x10;
        constexpr uintptr_t kTrapPageFault = 14;
        constexpr uintptr_t kTrapGeneralProtection = 13;

        // The signal the other threads are stopped with; one of the real-time signals, which nothing else in the
        // program uses.
        int dumpSignal() {
            return SIGRTMIN + 7;
        }

        struct Module {
            uintptr_t uiBegin;
            uintptr_t uiEnd;
            // The load bias: an address minus it is the address in the ELF's own terms
            uintptr_t uiBias;
            char acName[64];
            char acPath[256];
        };
        Module gModules[256];
        int32_t giModules = 0;

        struct Segment {
            uintptr_t uiBegin;
            uintptr_t uiEnd;
        };
        Segment gCode[1024];
        int32_t giCode = 0;

        struct Thread {
            pid_t iId;
            // 0: asked; 1: answered (registers and frames recorded)
            std::atomic<int32_t> iState;
            gregset_t kRegisters;
            uintptr_t auiFrames[kMaxFrames];
            uintptr_t auiFrameCfas[kMaxFrames];
            int32_t iFrames;
        };
        Thread gThreads[512];
        int32_t giThreads = 0;

        struct sigaction gPrevious[kSignalCount];
        std::atomic<int32_t> gCrashing{0};
        std::atomic<int32_t> gDone{0};
        pid_t giProcess = 0;
        pid_t giFaultThread = 0;

        char gcExe[PATH_MAX];
        // The program's directory, with its separator
        char gcDirectory[PATH_MAX];
        int giFile = -1;
        Out gOut;
        DateTime gLinked = {};
        // Local time minus UTC at start (the C library's time zone functions are not async-signal-safe)
        long glUtcOffset = 0;
        alignas(16) unsigned char gacAltStack[128 * 1024];

        pid_t threadId() {
            return static_cast<pid_t>(syscall(SYS_gettid));
        }

        void sleepMs(int32_t aiMs) {
            timespec kTime = {aiMs / 1000, (aiMs % 1000) * 1000000L};
            nanosleep(&kTime, nullptr);
        }

        // Copies a string into a fixed buffer, always terminated.
        template <size_t N>
        void copyText(char (&acOut)[N], const char* apText) {
            size_t i = 0;
            for (; i + 1 < N && apText[i]; ++i) {
                acOut[i] = apText[i];
            }
            acOut[i] = '\0';
        }

        const char* baseName(const char* apPath) {
            const char* pName = apPath;
            for (const char* p = apPath; *p; ++p) {
                if (*p == '/') {
                    pName = p + 1;
                }
            }
            return pName;
        }

        // The civil date of a count of seconds since 1970 (H. Hinnant's days-from-civil, inverted).
        DateTime dateTime(int64_t aiSeconds) {
            const int64_t iDays = (aiSeconds >= 0 ? aiSeconds : aiSeconds - 86399) / 86400;
            const int64_t iSecond = aiSeconds - iDays * 86400;
            const int64_t iShifted = iDays + 719468;
            const int64_t iEra = (iShifted >= 0 ? iShifted : iShifted - 146096) / 146097;
            const int64_t iDayOfEra = iShifted - iEra * 146097;
            const int64_t iYearOfEra = (iDayOfEra - iDayOfEra / 1460 + iDayOfEra / 36524 - iDayOfEra / 146096) / 365;
            const int64_t iDayOfYear = iDayOfEra - (365 * iYearOfEra + iYearOfEra / 4 - iYearOfEra / 100);
            const int64_t iMonthIndex = (5 * iDayOfYear + 2) / 153;
            const int64_t iDay = iDayOfYear - (153 * iMonthIndex + 2) / 5 + 1;
            const int64_t iMonth = iMonthIndex < 10 ? iMonthIndex + 3 : iMonthIndex - 9;
            const int64_t iYear = iYearOfEra + iEra * 400 + (iMonth <= 2 ? 1 : 0);
            return {
                static_cast<uint16_t>(iYear),
                static_cast<uint16_t>(iMonth),
                static_cast<uint16_t>(iDay),
                static_cast<uint16_t>(iSecond / 3600),
                static_cast<uint16_t>(iSecond / 60 % 60),
                static_cast<uint16_t>(iSecond % 60)
            };
        }

        const Module* moduleOf(uintptr_t auiAddress) {
            for (int32_t i = 0; i < giModules; ++i) {
                if (auiAddress >= gModules[i].uiBegin && auiAddress < gModules[i].uiEnd) {
                    return &gModules[i];
                }
            }
            return nullptr;
        }

        // The loaded modules and their executable segments. The program headers are read through readMemory: a module's
        // header page can be unreadable while the loader still lists it (libnvidia-tls.so seen so), and a fault here
        // would end the process with an empty log.
        int onModule(dl_phdr_info* apInfo, size_t, void*) {
            if (giModules == 256) {
                return 1;
            }
            Module& rModule = gModules[giModules];
            rModule.uiBegin = UINTPTR_MAX;
            rModule.uiEnd = 0;
            rModule.uiBias = apInfo->dlpi_addr;
            for (int32_t i = 0; i < apInfo->dlpi_phnum; ++i) {
                ElfW(Phdr) kHeader;
                if (!readMemory(&kHeader, reinterpret_cast<uintptr_t>(&apInfo->dlpi_phdr[i]), sizeof(kHeader))) {
                    break;
                }
                if (kHeader.p_type != PT_LOAD) {
                    continue;
                }
                const uintptr_t uiBegin = apInfo->dlpi_addr + kHeader.p_vaddr;
                const uintptr_t uiEnd = uiBegin + kHeader.p_memsz;
                rModule.uiBegin = uiBegin < rModule.uiBegin ? uiBegin : rModule.uiBegin;
                rModule.uiEnd = uiEnd > rModule.uiEnd ? uiEnd : rModule.uiEnd;
                if ((kHeader.p_flags & PF_X) && giCode < 1024) {
                    gCode[giCode++] = {uiBegin, uiEnd};
                }
            }
            if (rModule.uiBegin >= rModule.uiEnd) {
                return 0;
            }
            // the program itself has no name here
            const char* pPath = apInfo->dlpi_name && apInfo->dlpi_name[0] ? apInfo->dlpi_name : gcExe;
            copyText(rModule.acPath, pPath);
            copyText(rModule.acName, baseName(pPath));
            ++giModules;
            return 0;
        }

        void snapshotModules() {
            giModules = 0;
            giCode = 0;
            dl_iterate_phdr(&onModule, nullptr);
        }

        // The end of the mapping holding auiAddress (/proc/self/maps, read with plain system calls): the top of a
        // thread's stack for its stack pointer. auiAddress itself when no mapping holds it.
        uintptr_t mappingEnd(uintptr_t auiAddress) {
            const int iFile = open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
            if (iFile < 0) {
                return auiAddress;
            }
            static char acBuffer[65536];
            size_t uiHave = 0;
            uintptr_t uiResult = auiAddress;
            bool bDone = false;
            while (!bDone) {
                const ssize_t iRead = read(iFile, acBuffer + uiHave, sizeof(acBuffer) - uiHave);
                if (iRead <= 0) {
                    break;
                }
                uiHave += static_cast<size_t>(iRead);
                size_t uiLine = 0;
                for (size_t i = 0; i < uiHave && !bDone; ++i) {
                    if (acBuffer[i] != '\n') {
                        continue;
                    }
                    // "begin-end perms ..."
                    uintptr_t auiRange[2] = {0, 0};
                    const char* p = acBuffer + uiLine;
                    for (uintptr_t& rValue : auiRange) {
                        for (;; ++p) {
                            const char c = *p;
                            const int32_t iDigit = c >= '0' && c <= '9' ? c - '0'
                                : c >= 'a' && c <= 'f'                  ? c - 'a' + 10
                                                                        : -1;
                            if (iDigit < 0) {
                                break;
                            }
                            rValue = rValue * 16 + static_cast<uintptr_t>(iDigit);
                        }
                        ++p;
                    }
                    if (auiAddress >= auiRange[0] && auiAddress < auiRange[1]) {
                        uiResult = auiRange[1];
                        bDone = true;
                    }
                    uiLine = i + 1;
                }
                std::memmove(acBuffer, acBuffer + uiLine, uiHave - uiLine);
                uiHave -= uiLine;
            }
            close(iFile);
            return uiResult;
        }

        // The thread's name (/proc/self/task/<id>/comm) into acName; empty when unknown.
        void threadName(pid_t aiId, char (&acName)[32]) {
            char acPath[64] = "/proc/self/task/";
            char* pEnd = acPath + std::strlen(acPath);
            char acDigits[12];
            int32_t iDigits = 0;
            for (uint32_t uiId = static_cast<uint32_t>(aiId); uiId || !iDigits; uiId /= 10) {
                acDigits[iDigits++] = static_cast<char>('0' + uiId % 10);
            }
            while (iDigits) {
                *pEnd++ = acDigits[--iDigits];
            }
            std::memcpy(pEnd, "/comm", 6);
            acName[0] = '\0';
            const int iFile = open(acPath, O_RDONLY | O_CLOEXEC);
            if (iFile < 0) {
                return;
            }
            const ssize_t iRead = read(iFile, acName, sizeof(acName) - 1);
            close(iFile);
            size_t uiLength = iRead > 0 ? static_cast<size_t>(iRead) : 0;
            while (uiLength && (acName[uiLength - 1] == '\n' || acName[uiLength - 1] == '\0')) {
                --uiLength;
            }
            acName[uiLength] = '\0';
        }

        struct Walk {
            uintptr_t uiPc;
            Thread* pThread;
            bool bStarted;
        };

        _Unwind_Reason_Code onFrame(_Unwind_Context* apContext, void* apWalk) {
            Walk& rWalk = *static_cast<Walk*>(apWalk);
            int iBeforeInstruction = 0;
            const uintptr_t uiIp = _Unwind_GetIPInfo(apContext, &iBeforeInstruction);
            // the handler's own frames and the signal frame come first: the walk starts at the interrupted instruction
            if (!rWalk.bStarted) {
                if (uiIp != rWalk.uiPc) {
                    return _URC_NO_REASON;
                }
                rWalk.bStarted = true;
            }
            // the outermost frame's return address is 0
            if (!uiIp) {
                return _URC_END_OF_STACK;
            }
            Thread& rThread = *rWalk.pThread;
            rThread.auiFrames[rThread.iFrames] = uiIp;
            rThread.auiFrameCfas[rThread.iFrames] = _Unwind_GetCFA(apContext);
            return ++rThread.iFrames == kMaxFrames ? _URC_END_OF_STACK : _URC_NO_REASON;
        }

        // The thread's registers and its frames from the interrupted instruction on, from inside its signal handler.
        void record(Thread& arThread, const ucontext_t& arContext) {
            std::memcpy(arThread.kRegisters, arContext.uc_mcontext.gregs, sizeof(gregset_t));
            arThread.iFrames = 0;
            Walk kWalk = {static_cast<uintptr_t>(arContext.uc_mcontext.gregs[REG_RIP]), &arThread, false};
            _Unwind_Backtrace(&onFrame, &kWalk);
        }

        void frames(Out& arOut, const Thread& arThread) {
            if (!arThread.iFrames) {
                arOut
                    .str(
                        " Frames: the unwinder did not reach the interrupted instruction (the stack scan below has the "
                        "return addresses)"
                    )
                    .nl();
                return;
            }
            arOut.str(" Frames (_Unwind_Backtrace; from #1 on, the module offset is the call's: return address - 1):")
                .nl();
            for (int32_t i = 0; i < arThread.iFrames; ++i) {
                const uintptr_t uiAddress = arThread.auiFrames[i];
                arOut.str("  #").dec(i).chr(' ').hex(uiAddress).str(" (cfa ").hex(arThread.auiFrameCfas[i]).str(") ");
                codeAddress(arOut, i ? uiAddress - 1 : uiAddress);
                arOut.nl();
            }
        }

        void thread(Out& arOut, const Thread& arThread, bool abFault) {
            char acName[32];
            threadName(arThread.iId, acName);
            arOut.nl().str(abFault ? "Faulting thread " : "Thread ").dec(arThread.iId);
            if (arThread.iId == giProcess) {
                arOut.str(" \"main\"");
            }
            else if (acName[0]) {
                arOut.str(" \"").str(acName).chr('"');
            }
            if (arThread.iState.load(std::memory_order_acquire) != 1) {
                arOut.str(" (did not answer the dump signal: it blocks it, or is stopped)").nl();
                return;
            }
            const greg_t* pRegisters = arThread.kRegisters;
            const uintptr_t uiSp = static_cast<uintptr_t>(pRegisters[REG_RSP]);
            const uintptr_t uiHigh = mappingEnd(uiSp);
            threadClass(arOut, uiSp, uiHigh, static_cast<uint32_t>(arThread.iId));
            arOut.nl();
            auto value = [&](int aiIndex) { return static_cast<uintptr_t>(pRegisters[aiIndex]); };
            const Register akRegisters[] = {
                {"RAX", value(REG_RAX)},
                {"RBX", value(REG_RBX)},
                {"RCX", value(REG_RCX)},
                {"RDX", value(REG_RDX)},
                {"RSI", value(REG_RSI)},
                {"RDI", value(REG_RDI)},
                {"RBP", value(REG_RBP)},
                {"RSP", value(REG_RSP)},
                {"R8", value(REG_R8)},
                {"R9", value(REG_R9)},
                {"R10", value(REG_R10)},
                {"R11", value(REG_R11)},
                {"R12", value(REG_R12)},
                {"R13", value(REG_R13)},
                {"R14", value(REG_R14)},
                {"R15", value(REG_R15)},
                {"RIP", value(REG_RIP)}
            };
            registers(arOut, akRegisters, sizeof(akRegisters) / sizeof(akRegisters[0]), uiSp, uiHigh);
            // cs, gs, fs and ss in one word
            const uintptr_t uiSegments = value(REG_CSGSFS);
            arOut.str("  EFLAGS ")
                .hex(value(REG_EFL))
                .str(" CS ")
                .hex(uiSegments & 0xFFFF, 4)
                .str(" FS ")
                .hex(uiSegments >> 32 & 0xFFFF, 4)
                .str(" GS ")
                .hex(uiSegments >> 16 & 0xFFFF, 4)
                .str(" TRAPNO ")
                .hex(value(REG_TRAPNO), 2)
                .str(" ERR ")
                .hex(value(REG_ERR), 4)
                .nl();
            frames(arOut, arThread);
            scan(arOut, uiSp, uiHigh, abFault ? kFaultScanWords : kThreadScanWords);
        }

        const char* signalName(int aiSignal) {
            switch (aiSignal) {
                case SIGSEGV: return "SIGSEGV";
                case SIGBUS: return "SIGBUS";
                case SIGILL: return "SIGILL";
                case SIGFPE: return "SIGFPE";
                case SIGABRT: return "SIGABRT";
                default: return "signal";
            }
        }

        // What si_code says, for the signals a fault raises.
        const char* signalCause(int aiSignal, int aiCode) {
            if (aiCode <= 0) {
                return "sent by kill, tgkill or raise (abort())";
            }
            switch (aiSignal) {
                case SIGSEGV:
                    return aiCode == SEGV_MAPERR ? "access violation: address not mapped"
                        : aiCode == SEGV_ACCERR  ? "access violation: no access to the page"
                                                 : "access violation";
                case SIGBUS:
                    return aiCode == BUS_ADRALN ? "misaligned address" : "bus error (a file mapping past its end?)";
                case SIGILL: return aiCode == ILL_PRVOPC ? "privileged instruction" : "illegal instruction";
                case SIGFPE:
                    return aiCode == FPE_INTDIV ? "integer divide by zero"
                        : aiCode == FPE_INTOVF  ? "integer overflow"
                        : aiCode == FPE_FLTDIV  ? "float divide by zero"
                        : aiCode == FPE_FLTINV  ? "float invalid operation"
                                                : "arithmetic exception";
                default: return "";
            }
        }

        void
        header(Out& arOut, int aiSignal, const siginfo_t& arInfo, const ucontext_t& arContext, const DateTime& arNow) {
            const greg_t* pRegisters = arContext.uc_mcontext.gregs;
            const uintptr_t uiPc = static_cast<uintptr_t>(pRegisters[REG_RIP]);
            arOut.str("Fallout: New Vegas (fnv_decomp native program) crash log").nl();
            // the link time is the program file's own modification time
            arOut.str("Build: commit " XFNV_CRASHLOG_COMMIT ", linked ");
            printTime(arOut, gLinked);
            arOut.str(" UTC").nl();
            arOut.str("Image: ").str(gcExe).str(" at ").hex(kImage.uiBegin).str("..").hex(kImage.uiEnd).nl();
            arOut.str("Time: ");
            printTime(arOut, arNow);
            arOut.str(" local").nl();
            arOut.str("Signal: ")
                .dec(aiSignal)
                .chr(' ')
                .str(signalName(aiSignal))
                .str(", ")
                .str(signalCause(aiSignal, arInfo.si_code))
                .str(" at ")
                .hex(uiPc)
                .chr(' ');
            codeAddress(arOut, uiPc);
            arOut.nl();
            if ((aiSignal == SIGSEGV || aiSignal == SIGBUS) && arInfo.si_code > 0) {
                const uintptr_t uiTrap = static_cast<uintptr_t>(pRegisters[REG_TRAPNO]);
                const uintptr_t uiError = static_cast<uintptr_t>(pRegisters[REG_ERR]);
                const uintptr_t uiAddress = reinterpret_cast<uintptr_t>(arInfo.si_addr);
                if (uiTrap == kTrapGeneralProtection) {
                    arOut.str("  general protection fault: the address is not known (a non-canonical pointer?)").nl();
                }
                else {
                    arOut.str("  ")
                        .str(
                            uiTrap != kTrapPageFault    ? "accessing"
                                : uiError & kErrorFetch ? "executing"
                                : uiError & kErrorWrite ? "writing"
                                                        : "reading"
                        )
                        .chr(' ')
                        .hex(uiAddress);
                    annotateData(arOut, uiAddress);
                    arOut.nl();
                }
            }
            // std::terminate's abort(): the exception it was called for
            if (aiSignal == SIGABRT) {
                if (const std::type_info* pThrown = abi::__cxa_current_exception_type()) {
                    arOut.str("  thrown ").str(pThrown->name()).nl();
                }
            }
            arOut
                .str(
                    "Symbols: none in-process; harness/tools/crashlog-symbolize.py LOG names the module+0xOFFSET "
                    "addresses"
                )
                .nl();
        }

        // Stops every other thread in its dump handler; they answer in gThreads.
        void stopThreads(pid_t aiSelf) {
            struct Entry {
                uint64_t uiInode;
                int64_t iOffset;
                uint16_t uiLength;
                uint8_t uiType;
                char acName[1];
            };
            giThreads = 0;
            gThreads[giThreads].iId = aiSelf;
            gThreads[giThreads++].iState.store(1, std::memory_order_relaxed);
            const int iDirectory = open("/proc/self/task", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
            if (iDirectory < 0) {
                return;
            }
            alignas(8) static char acEntries[16384];
            for (;;) {
                const long lRead = syscall(SYS_getdents64, iDirectory, acEntries, sizeof(acEntries));
                if (lRead <= 0) {
                    break;
                }
                for (long lAt = 0; lAt < lRead;) {
                    const Entry* pEntry = reinterpret_cast<const Entry*>(acEntries + lAt);
                    lAt += pEntry->uiLength;
                    pid_t iId = 0;
                    const char* p = pEntry->acName;
                    for (; *p >= '0' && *p <= '9'; ++p) {
                        iId = iId * 10 + (*p - '0');
                    }
                    if (*p ||
                        !iId ||
                        iId == aiSelf ||
                        giThreads == static_cast<int32_t>(sizeof(gThreads) / sizeof(gThreads[0]))) {
                        continue;
                    }
                    Thread& rThread = gThreads[giThreads++];
                    rThread.iId = iId;
                    rThread.iState.store(0, std::memory_order_release);
                    syscall(SYS_tgkill, giProcess, iId, dumpSignal());
                }
            }
            close(iDirectory);
            for (int32_t iWaited = 0; iWaited < kWaitForThreadsMs; ++iWaited) {
                bool bAll = true;
                for (int32_t i = 0; i < giThreads; ++i) {
                    bAll = bAll && gThreads[i].iState.load(std::memory_order_acquire) == 1;
                }
                if (bAll) {
                    return;
                }
                sleepMs(1);
            }
        }

        void writeCrash(int aiSignal, const siginfo_t& arInfo, const ucontext_t& arContext) {
            timespec kNow;
            clock_gettime(CLOCK_REALTIME, &kNow);
            const DateTime kLocal = dateTime(static_cast<int64_t>(kNow.tv_sec) + glUtcOffset);
            // FalloutNV.crash-yyyymmdd-hhmmss.log
            char acPath[PATH_MAX + 64];
            std::memcpy(acPath, gcDirectory, std::strlen(gcDirectory) + 1);
            char* pEnd = acPath + std::strlen(acPath);
            std::memcpy(pEnd, "FalloutNV.crash-", 16);
            pEnd += 16;
            const uint16_t auiParts[] = {
                static_cast<uint16_t>(kLocal.uiYear / 100),
                static_cast<uint16_t>(kLocal.uiYear % 100),
                kLocal.uiMonth,
                kLocal.uiDay,
                0xFFFF,
                kLocal.uiHour,
                kLocal.uiMinute,
                kLocal.uiSecond
            };
            for (uint16_t uiPart : auiParts) {
                if (uiPart == 0xFFFF) {
                    *pEnd++ = '-';
                }
                else {
                    *pEnd++ = static_cast<char>('0' + uiPart / 10);
                    *pEnd++ = static_cast<char>('0' + uiPart % 10);
                }
            }
            std::memcpy(pEnd, ".log", 5);
            giFile = open(acPath, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
            if (giFile < 0) {
                return;
            }
            const char acNotice[] = "crash log: ";
            [[maybe_unused]] ssize_t iWritten = write(STDERR_FILENO, acNotice, sizeof(acNotice) - 1);
            iWritten = write(STDERR_FILENO, acPath, std::strlen(acPath));
            iWritten = write(STDERR_FILENO, "\n", 1);

            snapshotModules();
            const pid_t iSelf = threadId();
            stopThreads(iSelf);
            record(gThreads[0], arContext);

            header(gOut, aiSignal, arInfo, arContext, kLocal);
            gOut.flush();
            thread(gOut, gThreads[0], true);
            gOut.flush();
            for (int32_t i = 1; i < giThreads; ++i) {
                thread(gOut, gThreads[i], false);
                gOut.flush();
            }
            gOut.nl().str("Modules:").nl();
            for (int32_t i = 0; i < giModules; ++i) {
                gOut.str("  ")
                    .hex(gModules[i].uiBegin)
                    .str("..")
                    .hex(gModules[i].uiEnd)
                    .chr(' ')
                    .str(gModules[i].acName)
                    .chr(' ')
                    .str(gModules[i].acPath)
                    .nl();
            }
            gOut.str("End of log").nl();
            gOut.flush();
            close(giFile);
            giFile = -1;
        }

        // The signal's default action, once this handler returns: a fault repeats at its instruction; a sent signal is
        // raised again (blocked until then).
        void dieOf(int aiSignal, const siginfo_t& arInfo) {
            struct sigaction kDefault = {};
            kDefault.sa_handler = SIG_DFL;
            sigaction(aiSignal, &kDefault, nullptr);
            if (arInfo.si_code <= 0) {
                raise(aiSignal);
            }
        }

        void onCrash(int aiSignal, siginfo_t* apInfo, void* apContext) {
            const pid_t iSelf = threadId();
            int32_t iIdle = 0;
            if (gCrashing.compare_exchange_strong(iIdle, 1)) {
                giFaultThread = iSelf;
                writeCrash(aiSignal, *apInfo, *static_cast<const ucontext_t*>(apContext));
                gDone.store(1, std::memory_order_release);
            }
            else if (giFaultThread != iSelf) {
                // a second faulting thread waits too, so the process outlives the first one's log
                for (int32_t iWaited = 0; iWaited < kWaitForLogMs && !gDone.load(std::memory_order_acquire);
                     ++iWaited) {
                    sleepMs(1);
                }
            }
            // a handler installed before this one (the debug heap's fault report) goes on as it would have
            for (int32_t i = 0; i < kSignalCount; ++i) {
                const struct sigaction& rPrevious = gPrevious[i];
                if (kCrashSignals[i] != aiSignal) {
                    continue;
                }
                if (rPrevious.sa_flags & SA_SIGINFO) {
                    rPrevious.sa_sigaction(aiSignal, apInfo, apContext);
                }
                else if (rPrevious.sa_handler != SIG_DFL && rPrevious.sa_handler != SIG_IGN) {
                    rPrevious.sa_handler(aiSignal);
                }
            }
            dieOf(aiSignal, *apInfo);
        }

        void onDump(int, siginfo_t*, void* apContext) {
            if (!gCrashing.load(std::memory_order_acquire)) {
                return;
            }
            const pid_t iSelf = threadId();
            for (int32_t i = 1; i < giThreads; ++i) {
                Thread& rThread = gThreads[i];
                if (rThread.iId == iSelf) {
                    record(rThread, *static_cast<const ucontext_t*>(apContext));
                    rThread.iState.store(1, std::memory_order_release);
                    break;
                }
            }
            for (int32_t iWaited = 0; iWaited < kWaitForLogMs && !gDone.load(std::memory_order_acquire); ++iWaited) {
                sleepMs(1);
            }
        }

        void* selfTest(void* apSeconds) {
            pthread_setname_np(pthread_self(), "CrashLogTest");
            sleep(static_cast<unsigned>(reinterpret_cast<uintptr_t>(apSeconds)));
            volatile int32_t* pNull = nullptr;
            return reinterpret_cast<void*>(static_cast<intptr_t>(*pNull));
        }

        void install() {
            giProcess = getpid();
            const ssize_t iLength = readlink("/proc/self/exe", gcExe, sizeof(gcExe) - 1);
            gcExe[iLength > 0 ? iLength : 0] = '\0';
            copyText(gcDirectory, gcExe);
            for (char* p = gcDirectory + std::strlen(gcDirectory); p > gcDirectory && p[-1] != '/';) {
                *--p = '\0';
            }
            struct stat kStat;
            if (stat(gcExe, &kStat) == 0) {
                gLinked = dateTime(static_cast<int64_t>(kStat.st_mtime));
            }
            const time_t iNow = time(nullptr);
            tm kLocal;
            if (localtime_r(&iNow, &kLocal)) {
                glUtcOffset = kLocal.tm_gmtoff;
            }
            // the program's own image: the first module dl_iterate_phdr reports
            snapshotModules();
            if (giModules) {
                kImage.uiBegin = gModules[0].uiBegin;
                kImage.uiEnd = gModules[0].uiEnd;
            }

            stack_t kStack = {};
            kStack.ss_sp = gacAltStack;
            kStack.ss_size = sizeof(gacAltStack);
            sigaltstack(&kStack, nullptr);
            struct sigaction kAction = {};
            kAction.sa_sigaction = &onCrash;
            kAction.sa_flags = SA_SIGINFO | SA_ONSTACK;
            sigemptyset(&kAction.sa_mask);
            for (int32_t i = 0; i < kSignalCount; ++i) {
                sigaction(kCrashSignals[i], &kAction, &gPrevious[i]);
            }
            struct sigaction kDump = {};
            kDump.sa_sigaction = &onDump;
            kDump.sa_flags = SA_SIGINFO | SA_RESTART;
            sigemptyset(&kDump.sa_mask);
            sigaction(dumpSignal(), &kDump, nullptr);

            if (const char* pSeconds = getenv("XFNV_CRASHLOG_SELFTEST")) {
                uintptr_t uiSeconds = 0;
                for (const char* p = pSeconds; *p >= '0' && *p <= '9'; ++p) {
                    uiSeconds = uiSeconds * 10 + static_cast<uintptr_t>(*p - '0');
                }
                pthread_t kThread;
                if (pthread_create(&kThread, nullptr, &selfTest, reinterpret_cast<void*>(uiSeconds)) == 0) {
                    pthread_detach(kThread);
                }
            }
        }

        struct Installer {
            Installer() {
                install();
            }
        } gInstaller;

    } // namespace

    bool isCode(uintptr_t auiAddress) {
        for (int32_t i = 0; i < giCode; ++i) {
            if (auiAddress >= gCode[i].uiBegin && auiAddress < gCode[i].uiEnd) {
                return true;
            }
        }
        return false;
    }

    // "module+0xOFFSET", OFFSET in the ELF's own terms (its virtual address): what llvm-symbolizer --obj=ELF takes.
    void codeAddress(Out& arOut, uintptr_t auiAddress) {
        if (const Module* pModule = moduleOf(auiAddress)) {
            arOut.str(pModule->acName).str("+0x").hex(auiAddress - pModule->uiBias, 1);
            return;
        }
        arOut.str("<no module>");
    }

    bool readMemory(void* apDest, uintptr_t apSource, size_t auiSize) {
        if (!auiSize) {
            return true;
        }
        if (apSource < 0x10000 || apSource + auiSize < apSource) {
            return false;
        }
        iovec kLocal = {apDest, auiSize};
        iovec kRemote = {reinterpret_cast<void*>(apSource), auiSize};
        return process_vm_readv(giProcess, &kLocal, 1, &kRemote, 1, 0) == static_cast<ssize_t>(auiSize);
    }

    void writeOut(const char* apText, uint32_t auiSize) {
        while (giFile >= 0 && auiSize) {
            const ssize_t iWritten = write(giFile, apText, auiSize);
            if (iWritten <= 0) {
                return;
            }
            apText += iWritten;
            auiSize -= static_cast<uint32_t>(iWritten);
        }
    }

} // namespace xfnv::crashlog
