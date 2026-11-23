#pragma once

#include <mutex>

// The Windows SDK v6.0A <winnt.h> declarations the exe's code uses, with the SDK's names and 32-bit layouts, so no
// Windows header is needed. Global, as Windows declares them.

// The SDK's import declaration: a function or object a DLL exports, called through the importing module's slot.
#ifdef _WIN32
#define DECLSPEC_IMPORT __declspec(dllimport)
#else
#define DECLSPEC_IMPORT
#endif

// The SDK's LONG: 32 bits. Windows spells it `long` (a type of its own there: overloads, template arguments and
// mangled names tell it from int); elsewhere `long` is 64 bits, so LONG is int32_t.
#ifdef _WIN32
typedef long LONG;
#else
typedef int32_t LONG;
#endif
// The SDK's HRESULT (winnt.h: typedef LONG HRESULT).
typedef LONG HRESULT;

// The SDK's HANDLE: an opaque pointer (8 bytes on x64).
typedef void* HANDLE;

// The SDK's LARGE_INTEGER (8 bytes): a 64-bit value and its two halves.
typedef union _LARGE_INTEGER {
    struct {
        uint32_t LowPart;
        int32_t HighPart;
    } u;
    int64_t QuadPart;
} LARGE_INTEGER;

// LAYOUT DEVIATION: the exe's CRITICAL_SECTION is Win32's structure, 0x18 bytes: void* DebugInfo, int32_t LockCount,
// int32_t RecursionCount, void* OwningThread, void* LockSemaphore, uint32_t SpinCount. Here it is a standard lock on
// every target (docs/DEVIATIONS.md): a std::recursive_mutex (MS STL: 0x30 bytes on x86, 0x50 on x64), whose lifetime
// is a Windows section's: InitializeCriticalSection(AndSpinCount) constructs it and DeleteCriticalSection destroys it
// (winbase.h). The game initialises and deletes its sections explicitly, some in memory C++ does not construct
// (allocated raw or constructed by hand), so the structure's own constructor and destructor leave the mutex alone: a
// section is usable only between those two calls, as on Windows. Not copyable. No game code reads the SDK's fields.
struct _RTL_CRITICAL_SECTION {
    constexpr _RTL_CRITICAL_SECTION() {}
    ~_RTL_CRITICAL_SECTION() {}
    _RTL_CRITICAL_SECTION(const _RTL_CRITICAL_SECTION&) = delete;
    _RTL_CRITICAL_SECTION& operator=(const _RTL_CRITICAL_SECTION&) = delete;

    union {
        std::recursive_mutex kMutex;
    };
};
