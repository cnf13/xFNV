// The guard-page heap of DebugHeap.h. Built into every program without the exe; only XFNV_GUARD_ALLOC's pass-through
// (SystemAlloc.cpp) calls it. The system's side (pages, threads, the log, the fault hook) is Platform.h's.

#include "DebugHeap.h"

#include "Platform.h"

#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>

namespace xfnv::debugheap {
    namespace {

        // The knobs. An allocation of kGuardMinSize bytes or more gets its own pages; so does every
        // kGuardSampleSmall-th smaller one (0: none). The region is the first of kRegionSizes that can be reserved;
        // once the allocator has gone round it and finds no run of free pages, it falls back to the small form (logged
        // once). A freed small block is filled with kFreedFill and held back from the C runtime until kQuarantineBytes
        // (or kQuarantineEntries blocks) of later frees have passed it; its fill is checked when it is let go.
        constexpr size_t kGuardMinSize = 512;
        constexpr uint32_t kGuardSampleSmall = 0;
        constexpr size_t kRegionSizes[] = {0xA0000000u, 0x80000000u, 0x60000000u, 0x40000000u, 0x20000000u};
        constexpr size_t kQuarantineBytes = 256u << 20;
        constexpr uint32_t kQuarantineEntries = 1u << 21;
        constexpr uint32_t kFrames = 10;
        constexpr uint32_t kSmallFrames = 6;
        constexpr uint32_t kStatsEvery = 0x40000;

        constexpr uint32_t kGuardMagic = 0x6A4D4B47u;
        constexpr uint32_t kSmallMagic = 0x6C6D5347u;
        constexpr uint8_t kSlackFill = 0xAB;
        constexpr uint8_t kFreedFill = 0xFD;
        constexpr uint32_t kNoRun = 0xFFFFFFFFu;

        // What sits right before a guarded block.
        struct GuardHeader {
            uint32_t uiMagic;
            uint32_t uiSize;
            uint32_t uiSeq;
            uint32_t uiThread;
            void* apFrames[kFrames];
        };

        // What sits right before a small block; a 4-byte canary follows the block.
        struct SmallHeader {
            void* pRaw;
            uint32_t uiSize;
            uint32_t uiSeq;
            uint32_t uiThread;
            void* apFrames[kSmallFrames];
            uint32_t uiCanary;
            uint32_t uiMagic;
        };

        // A freed small block held back, with who freed it.
        struct Quarantined {
            char* pUser;
            uint32_t uiThread;
            void* apFrames[kSmallFrames];
        };

        // Per run, kept outside the run so that a freed block can still be described.
        struct Record {
            char* pUser;
            uint32_t uiSize;
            uint32_t uiSeq;
            uint32_t uiAllocThread;
            uint32_t uiFreeThread;
            uint32_t uiPages; // data pages, the guard page not counted
            uint32_t bLive;
            void* apAllocFrames[kFrames];
            void* apFreeFrames[kFrames];
        };

        std::mutex gLock;
        std::once_flag gInitOnce;
        char* gBase;
        size_t gPages;
        uint8_t* gPageUsed;
        uint32_t* gRunOf;
        Record* gRecords;
        size_t gCursor;
        const uint8_t* gTextBegin;
        const uint8_t* gTextEnd;
        uintptr_t gImageBase;
        const char* gImageName = "";

        std::atomic<uint32_t> gSeq;
        std::atomic<uint32_t> gSmallCount;
        uint32_t gLive;
        uint32_t gLivePages;
        uint32_t gWraps;
        uint32_t gFallbacks;
        std::atomic<uint32_t> gForeign;
        std::atomic<uint32_t> gFaults;
        std::mutex gQuarantineLock;
        Quarantined* gQuarantine;
        uint32_t gQuarantineHead; // the oldest
        uint32_t gQuarantineCount;
        size_t gQuarantineSize;

        void Log(const char* apFormat, ...) {
            char acBuffer[1024];
            va_list kArgs;
            va_start(kArgs, apFormat);
            int iLen = vsnprintf(acBuffer, sizeof(acBuffer), apFormat, kArgs);
            va_end(kArgs);
            if (iLen < 0 || iLen >= static_cast<int>(sizeof(acBuffer))) {
                iLen = sizeof(acBuffer) - 1;
            }
            platform::WriteLog(acBuffer, static_cast<size_t>(iLen));
        }

        // Each frame (a return address) as NAME+0xOFFSET of its call, the return address - 1, in the program's image
        // (harness/tools/crashlog-symbolize.py names them), else as an address.
        void LogFrames(const char* apWhat, void* const* apFrames, uint32_t aCount) {
            char acLine[512];
            int iLen = snprintf(acLine, sizeof(acLine), "    %s:", apWhat);
            for (uint32_t i = 0; i < aCount && apFrames[i] && iLen < static_cast<int>(sizeof(acLine)); ++i) {
                const uint8_t* pFrame = static_cast<const uint8_t*>(apFrames[i]);
                iLen += pFrame >= gTextBegin && pFrame < gTextEnd
                    ? snprintf(
                          acLine + iLen,
                          sizeof(acLine) - iLen,
                          " %s+0x%llX",
                          gImageName,
                          static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(pFrame) - 1 - gImageBase)
                      )
                    : snprintf(acLine + iLen, sizeof(acLine) - iLen, " %p", apFrames[i]);
            }
            Log("%s\n", acLine);
        }

        // Whether v follows a call instruction: E8 rel32, or FF /2 in its 2-, 3-, 4-, 6- and 7-byte forms.
        bool FollowsCall(const uint8_t* v) {
            if (v - 7 < gTextBegin || v > gTextEnd) {
                return false;
            }
            if (v[-5] == 0xE8) {
                return true;
            }
            static const int kLengths[] = {2, 3, 4, 6, 7};
            for (int iLen : kLengths) {
                if (v[-iLen] == 0xFF && (v[-iLen + 1] & 0x38) == 0x10) {
                    return true;
                }
            }
            return false;
        }

        // The program is built without frame pointers, so the return addresses are found by scanning the stack: every
        // word pointing into the program's code right after a call, the nearest first. Some are stale; the first is in
        // this file.
        void CaptureFrames(void** apOut, uint32_t aCount) {
            uint32_t n = 0;
            // The scan starts at the first pointer-aligned word at or above n (n is 4-byte aligned; an unaligned start
            // would read the last word across the stack's top).
            const uintptr_t uiStart = (reinterpret_cast<uintptr_t>(&n) + sizeof(uintptr_t) - 1) &
                ~(sizeof(uintptr_t) - 1);
            const uintptr_t* pWord = reinterpret_cast<const uintptr_t*>(uiStart);
            const uintptr_t* pEnd = reinterpret_cast<const uintptr_t*>(platform::StackTop());
            if (pEnd > pWord + 2048) {
                pEnd = pWord + 2048;
            }
            // The caller's output array lies above this frame: its words are skipped, or the scan reads back what it
            // wrote.
            const uintptr_t* pOutBegin = reinterpret_cast<const uintptr_t*>(apOut);
            const uintptr_t* pOutEnd = pOutBegin + aCount;
            for (; pWord < pEnd && n < aCount; ++pWord) {
                if (pWord >= pOutBegin && pWord < pOutEnd) {
                    continue;
                }
                const uint8_t* v = reinterpret_cast<const uint8_t*>(*pWord);
                if (v >= gTextBegin && v < gTextEnd && FollowsCall(v)) {
                    apOut[n++] = const_cast<uint8_t*>(v);
                }
            }
            for (; n < aCount; ++n) {
                apOut[n] = nullptr;
            }
        }

        bool InRegion(const void* apMem) {
            const char* p = static_cast<const char*>(apMem);
            return gBase && p >= gBase && p < gBase + gPages * kPage;
        }

        // The record of the run holding apMem, or null.
        Record* RecordOf(const void* apMem) {
            if (!InRegion(apMem)) {
                return nullptr;
            }
            const uint32_t uiRun = gRunOf[(static_cast<const char*>(apMem) - gBase) / kPage];
            return uiRun == kNoRun ? nullptr : &gRecords[uiRun];
        }

        // The quarantined small block holding apMem, or null (a scan of the quarantine: for fault reports only).
        const Quarantined* QuarantinedOf(const void* apMem) {
            const char* p = static_cast<const char*>(apMem);
            if (!gQuarantine || p < reinterpret_cast<const char*>(0x10000)) {
                return nullptr;
            }
            for (uint32_t i = 0; i < gQuarantineCount; ++i) {
                const Quarantined& rEntry = gQuarantine[(gQuarantineHead + i) % kQuarantineEntries];
                const SmallHeader* pHeader = reinterpret_cast<const SmallHeader*>(rEntry.pUser) - 1;
                if (p >= reinterpret_cast<const char*>(pHeader) &&
                    p < rEntry.pUser + pHeader->uiSize + sizeof(uint32_t)) {
                    return &rEntry;
                }
            }
            return nullptr;
        }

        void Describe(const void* apMem) {
            const Record* pRecord = RecordOf(apMem);
            if (!pRecord) {
                if (const Quarantined* pEntry = InRegion(apMem) ? nullptr : QuarantinedOf(apMem)) {
                    const SmallHeader* pHeader = reinterpret_cast<const SmallHeader*>(pEntry->pUser) - 1;
                    Log("  %p: inside small block %p size %u (+%d), FREED (quarantined), seq %u, allocated by thread "
                        "%04x\n",
                        apMem,
                        static_cast<const void*>(pEntry->pUser),
                        pHeader->uiSize,
                        static_cast<int>(static_cast<const char*>(apMem) - pEntry->pUser),
                        pHeader->uiSeq,
                        pHeader->uiThread);
                    LogFrames("allocated at", pHeader->apFrames, kSmallFrames);
                    Log("    freed by thread %04x\n", pEntry->uiThread);
                    LogFrames("freed at", pEntry->apFrames, kSmallFrames);
                    return;
                }
                Log("  %p: %s\n",
                    apMem,
                    InRegion(apMem) ? "guarded region, never handed out" : "not a guarded or quarantined block");
                return;
            }
            const char* p = static_cast<const char*>(apMem);
            // The run's guard page starts at the first page boundary at or after the block's end.
            const char* pGuard = reinterpret_cast<const char*>(
                (reinterpret_cast<uintptr_t>(pRecord->pUser) + pRecord->uiSize + kPage - 1) & ~(kPage - 1)
            );
            const char* pWhere = p < pRecord->pUser    ? "before (header)"
                : p < pRecord->pUser + pRecord->uiSize ? "inside"
                : p < pGuard                           ? "past the end (slack)"
                                                       : "past the end (guard page)";
            Log("  %p: %s block %p size %u (+%d), %s, seq %u, allocated by thread %04x\n",
                apMem,
                pWhere,
                static_cast<const void*>(pRecord->pUser),
                pRecord->uiSize,
                static_cast<int>(p - pRecord->pUser),
                pRecord->bLive ? "live" : "FREED",
                pRecord->uiSeq,
                pRecord->uiAllocThread);
            LogFrames("allocated at", pRecord->apAllocFrames, kFrames);
            if (!pRecord->bLive) {
                Log("    freed by thread %04x\n", pRecord->uiFreeThread);
                LogFrames("freed at", pRecord->apFreeFrames, kFrames);
            }
        }

        void Initialize() {
            platform::OpenLog();
            platform::CodeRange(gTextBegin, gTextEnd);
            gImageBase = platform::ImageBase();
            gImageName = platform::ImagePath();
            for (const char* p = gImageName; *p; ++p) {
                if (*p == '/' || *p == '\\') {
                    gImageName = p + 1;
                }
            }
            if (platform::PageSize() == kPage) {
                for (size_t uiSize : kRegionSizes) {
                    gBase = platform::Reserve(uiSize);
                    if (gBase) {
                        gPages = uiSize / kPage;
                        break;
                    }
                }
            }
            if (gBase) {
                gPageUsed = static_cast<uint8_t*>(platform::AllocateTable(gPages));
                gRunOf = static_cast<uint32_t*>(platform::AllocateTable(gPages * sizeof(uint32_t)));
                gRecords = static_cast<Record*>(platform::AllocateTable(gPages * sizeof(Record)));
                if (!gPageUsed || !gRunOf || !gRecords) {
                    gBase = nullptr;
                }
                else {
                    memset(gRunOf, 0xFF, gPages * sizeof(uint32_t));
                }
            }
            gQuarantine = static_cast<Quarantined*>(platform::AllocateTable(kQuarantineEntries * sizeof(Quarantined)));
            platform::InstallFaultHook();
            Log("debugheap: image %s %s\n", gImageName, platform::ImagePath());
            Log("debugheap: region %p, %u MB; guarded from %u bytes, every %u-th smaller one\n",
                static_cast<void*>(gBase),
                static_cast<uint32_t>(gPages * kPage >> 20),
                static_cast<uint32_t>(kGuardMinSize),
                kGuardSampleSmall);
        }

        [[noreturn]] void Corrupt(const char* apWhat, const void* apMem) {
            Log("debugheap: %s, block %p, thread %04x\n", apWhat, apMem, platform::ThreadId());
            void* apStack[kFrames];
            CaptureFrames(apStack, kFrames);
            LogFrames("detected at", apStack, kFrames);
            Describe(apMem);
            platform::Stop();
        }

        void* AllocateSmall(size_t aSize, size_t aAlign) {
            if (aAlign < alignof(SmallHeader)) {
                aAlign = alignof(SmallHeader);
            }
            char* pRaw = static_cast<char*>(malloc(sizeof(SmallHeader) + aAlign + aSize + sizeof(uint32_t)));
            if (!pRaw) {
                return nullptr;
            }
            char* pUser = reinterpret_cast<char*>(
                (reinterpret_cast<uintptr_t>(pRaw) + sizeof(SmallHeader) + aAlign - 1) &
                ~static_cast<uintptr_t>(aAlign - 1)
            );
            SmallHeader* pHeader = reinterpret_cast<SmallHeader*>(pUser) - 1;
            pHeader->pRaw = pRaw;
            pHeader->uiSize = static_cast<uint32_t>(aSize);
            pHeader->uiSeq = ++gSmallCount;
            pHeader->uiThread = platform::ThreadId();
            CaptureFrames(pHeader->apFrames, kSmallFrames);
            pHeader->uiCanary = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pUser)) ^ kSmallMagic;
            pHeader->uiMagic = kSmallMagic;
            memcpy(pUser + aSize, &pHeader->uiCanary, sizeof(uint32_t));
            return pUser;
        }

        // Holds a freed small block (its fill written) back from the C runtime; lets the oldest go once the quarantine
        // is over its budget, after checking that nothing wrote to it since its free.
        void Quarantine(char* apUser, uint32_t aSize) {
            Quarantined kEntry;
            kEntry.pUser = apUser;
            kEntry.uiThread = platform::ThreadId();
            CaptureFrames(kEntry.apFrames, kSmallFrames);
            if (!gQuarantine) {
                free((reinterpret_cast<SmallHeader*>(apUser) - 1)->pRaw);
                return;
            }
            std::unique_lock kLock(gQuarantineLock);
            gQuarantine[(gQuarantineHead + gQuarantineCount) % kQuarantineEntries] = kEntry;
            ++gQuarantineCount;
            gQuarantineSize += aSize;
            while (gQuarantineSize > kQuarantineBytes || gQuarantineCount == kQuarantineEntries) {
                const Quarantined& rOldest = gQuarantine[gQuarantineHead];
                const SmallHeader* pHeader = reinterpret_cast<const SmallHeader*>(rOldest.pUser) - 1;
                for (uint32_t i = 0; i < pHeader->uiSize; ++i) {
                    if (static_cast<uint8_t>(rOldest.pUser[i]) != kFreedFill) {
                        kLock.unlock();
                        Corrupt("small block written after its free (use after free)", rOldest.pUser + i);
                    }
                }
                gQuarantineSize -= pHeader->uiSize;
                free(pHeader->pRaw);
                gQuarantineHead = (gQuarantineHead + 1) % kQuarantineEntries;
                --gQuarantineCount;
            }
        }

        // Under the lock: the first run of aPages free pages from the cursor on, going round the region once; kNoRun if
        // none.
        size_t FindRun(size_t aPages) {
            size_t uiStart = gCursor;
            size_t uiFound = 0;
            for (size_t uiScanned = 0; uiScanned < gPages + aPages; ++uiScanned) {
                size_t uiPage = uiStart + uiFound;
                if (uiPage >= gPages) {
                    uiStart = 0;
                    uiFound = 0;
                    ++gWraps;
                    continue;
                }
                if (gPageUsed[uiPage]) {
                    uiStart = uiPage + 1;
                    uiFound = 0;
                    continue;
                }
                if (++uiFound == aPages) {
                    return uiStart;
                }
            }
            return kNoRun;
        }

        void* AllocateGuarded(size_t aSize, size_t aAlign) {
            if (aAlign < alignof(GuardHeader)) {
                aAlign = alignof(GuardHeader);
            }
            const size_t uiUser = (aSize + aAlign - 1) & ~(aAlign - 1);
            const size_t uiDataPages = (uiUser + sizeof(GuardHeader) + kPage - 1) / kPage;
            std::unique_lock kLock(gLock);
            const size_t uiRun = FindRun(uiDataPages + 1);
            if (uiRun == kNoRun) {
                if (!gFallbacks++) {
                    Log("debugheap: the region is full (%u live blocks); smaller form from here on when full\n", gLive);
                }
                return nullptr;
            }
            char* pRun = gBase + uiRun * kPage;
            if (!platform::Commit(pRun, uiDataPages * kPage)) {
                ++gFallbacks;
                return nullptr;
            }
            memset(gPageUsed + uiRun, 1, uiDataPages + 1);
            for (size_t i = 0; i <= uiDataPages; ++i) {
                gRunOf[uiRun + i] = static_cast<uint32_t>(uiRun);
            }
            gCursor = uiRun + uiDataPages + 1;
            ++gLive;
            gLivePages += static_cast<uint32_t>(uiDataPages);
            const uint32_t uiLive = gLive;
            const uint32_t uiLivePages = gLivePages;
            kLock.unlock();

            char* pGuard = pRun + uiDataPages * kPage;
            char* pUser = pGuard - uiUser;
            memset(pUser + aSize, kSlackFill, uiUser - aSize);
            GuardHeader* pHeader = reinterpret_cast<GuardHeader*>(pUser) - 1;
            pHeader->uiMagic = kGuardMagic;
            pHeader->uiSize = static_cast<uint32_t>(aSize);
            pHeader->uiSeq = ++gSeq;
            pHeader->uiThread = platform::ThreadId();
            CaptureFrames(pHeader->apFrames, kFrames);
            Record* pRecord = &gRecords[uiRun];
            pRecord->pUser = pUser;
            pRecord->uiSize = pHeader->uiSize;
            pRecord->uiSeq = pHeader->uiSeq;
            pRecord->uiAllocThread = pHeader->uiThread;
            pRecord->uiFreeThread = 0;
            pRecord->uiPages = static_cast<uint32_t>(uiDataPages);
            memcpy(pRecord->apAllocFrames, pHeader->apFrames, sizeof(pRecord->apAllocFrames));
            memset(pRecord->apFreeFrames, 0, sizeof(pRecord->apFreeFrames));
            pRecord->bLive = 1;
            if (pHeader->uiSeq % kStatsEvery == 0) {
                Log("debugheap: %u guarded allocations, %u live in %u pages, %u small, %u wraps, %u fallbacks, %u "
                    "foreign\n",
                    pHeader->uiSeq,
                    uiLive,
                    uiLivePages,
                    gSmallCount.load(),
                    gWraps,
                    gFallbacks,
                    gForeign.load());
            }
            return pUser;
        }

        void FreeGuarded(void* apMem) {
            char* pUser = static_cast<char*>(apMem);
            const uint32_t uiRun = gRunOf[(pUser - gBase) / kPage];
            Record* pRecord = uiRun == kNoRun ? nullptr : &gRecords[uiRun];
            if (!pRecord || pRecord->pUser != pUser) {
                Corrupt("free of an address inside the guarded region that no block starts at", apMem);
            }
            if (!pRecord->bLive) {
                Corrupt("double free", apMem);
            }
            const GuardHeader* pHeader = reinterpret_cast<const GuardHeader*>(pUser) - 1;
            if (pHeader->uiMagic != kGuardMagic || pHeader->uiSize != pRecord->uiSize) {
                Corrupt("header overwritten (underrun)", apMem);
            }
            const char* pGuard = gBase + (uiRun + pRecord->uiPages) * kPage;
            for (const char* p = pUser + pRecord->uiSize; p < pGuard; ++p) {
                if (static_cast<uint8_t>(*p) != kSlackFill) {
                    Corrupt("bytes after the block overwritten (overrun into the alignment slack)", apMem);
                }
            }
            pRecord->uiFreeThread = platform::ThreadId();
            CaptureFrames(pRecord->apFreeFrames, kFrames);
            std::unique_lock kLock(gLock);
            if (!pRecord->bLive) {
                kLock.unlock();
                Corrupt("double free (two threads)", apMem);
            }
            pRecord->bLive = 0;
            platform::Decommit(gBase + uiRun * kPage, pRecord->uiPages * kPage);
            memset(gPageUsed + uiRun, 0, pRecord->uiPages + 1);
            --gLive;
            gLivePages -= pRecord->uiPages;
        }

    } // namespace

    // The exception or signal goes on to the debugger or the crash log as before.
    void DescribeFault(
        uintptr_t auiAddress,
        bool abWrite,
        uintptr_t auiIp,
        uintptr_t auiSp,
        uintptr_t auiStackTop,
        const FaultRegister* apRegisters,
        size_t auiCount
    ) {
        if (++gFaults > 20) {
            return;
        }
        Log("debugheap: access violation (%s) at %p, ip %p, thread %04x\n",
            abWrite ? "write" : "read",
            reinterpret_cast<const void*>(auiAddress),
            reinterpret_cast<const void*>(auiIp),
            platform::ThreadId());
        Describe(reinterpret_cast<const void*>(auiAddress));
        for (size_t i = 0; i < auiCount; ++i) {
            const void* p = reinterpret_cast<const void*>(apRegisters[i].uiValue);
            if (RecordOf(p) || QuarantinedOf(p)) {
                Log(" %s:", apRegisters[i].pName);
                Describe(p);
            }
        }
        // The words on the stack that point into guarded blocks: locals and arguments of the faulting frames.
        const uintptr_t* pWord = reinterpret_cast<const uintptr_t*>(auiSp);
        const uintptr_t* pTop = reinterpret_cast<const uintptr_t*>(auiStackTop);
        for (uint32_t i = 0, n = 0; i < 64 && n < 8 && pWord + i < pTop; ++i) {
            const void* p = reinterpret_cast<const void*>(pWord[i]);
            if (RecordOf(p) || QuarantinedOf(p)) {
                Log(" [sp+%x]:", static_cast<uint32_t>(i * sizeof(uintptr_t)));
                Describe(p);
                ++n;
            }
        }
    }

    void* Allocate(size_t aSize, size_t aAlign) {
        std::call_once(gInitOnce, Initialize);
        static std::atomic<uint32_t> uiSmallSeen;
        const bool bGuard = gBase &&
            aAlign <= kPage &&
            (aSize >= kGuardMinSize || (kGuardSampleSmall && ++uiSmallSeen % kGuardSampleSmall == 0));
        if (bGuard) {
            if (void* p = AllocateGuarded(aSize, aAlign)) {
                return p;
            }
        }
        return AllocateSmall(aSize, aAlign);
    }

    void Free(void* apMem) {
        if (!apMem) {
            return;
        }
        if (InRegion(apMem)) {
            FreeGuarded(apMem);
            return;
        }
        SmallHeader* pHeader = static_cast<SmallHeader*>(apMem) - 1;
        if (pHeader->uiMagic == kSmallMagic && pHeader->uiCanary == 0) {
            Corrupt("double free of a small block (still quarantined)", apMem);
        }
        if (pHeader->uiMagic != kSmallMagic ||
            pHeader->uiCanary != (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apMem)) ^ kSmallMagic)) {
            // Not this heap's: memory the program took from the C runtime directly and gives back through the game's
            // allocator, as the exe's Deallocate allows.
            if (++gForeign <= 20) {
                Log("debugheap: free of %p, not this heap's: given to the C runtime\n", apMem);
                void* apFrames[10];
                CaptureFrames(apFrames, 10);
                LogFrames("freed at", apFrames, 10);
            }
            free(apMem);
            return;
        }
        uint32_t uiTail;
        memcpy(&uiTail, static_cast<char*>(apMem) + pHeader->uiSize, sizeof(uiTail));
        if (uiTail != pHeader->uiCanary) {
            Corrupt("small block's tail canary overwritten (overrun)", apMem);
        }
        pHeader->uiCanary = 0;
        memset(apMem, kFreedFill, pHeader->uiSize);
        Quarantine(static_cast<char*>(apMem), pHeader->uiSize);
    }

    size_t Size(const void* apMem) {
        if (!apMem) {
            return 0;
        }
        if (const Record* pRecord = RecordOf(apMem)) {
            return pRecord->uiSize;
        }
        const SmallHeader* pHeader = static_cast<const SmallHeader*>(apMem) - 1;
        return pHeader->uiMagic == kSmallMagic ? pHeader->uiSize : platform::ForeignSize(const_cast<void*>(apMem));
    }

    void* Reallocate(void* apMem, size_t aSize, size_t aAlign) {
        void* pNew = Allocate(aSize, aAlign);
        if (apMem) {
            const size_t uiOld = Size(apMem);
            memcpy(pNew, apMem, uiOld < aSize ? uiOld : aSize);
            Free(apMem);
        }
        return pNew;
    }

} // namespace xfnv::debugheap
