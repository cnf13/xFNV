#pragma once

#include "SDK/MSVC/stdlib.h"
#include "SDK/Windows/winbase.h"

#include <cstdint>

// The calling thread's id (GetCurrentThreadId).
inline uint32_t BSGetCurrentThreadID() {
    return GetCurrentThreadId();
}

// The system tick count in milliseconds.
inline uint32_t BSGetTickCount() {
    return GetTickCount();
}

// Allocates a thread-local storage index through the kernel32 import; returns the index, or
// 0xFFFFFFFF when none is free.
inline uint32_t BSTlsAlloc() {
    return TlsAlloc();
}

// Frees a TLS index through the TlsFree import.
inline void BSTlsFree(uint32_t aiTlsIndex) {
    TlsFree(aiTlsIndex);
}

// Sets the calling thread's value at a TLS index through the TlsSetValue import; returns its result.
inline int32_t SetTlsValue(uint32_t aiTlsIndex, void* apValue) {
    return TlsSetValue(aiTlsIndex, apValue);
}

// The calling thread's value at a TLS index, through the TlsGetValue import.
inline void* GetTlsValue(uint32_t dwTlsIndex) {
    return TlsGetValue(dwTlsIndex);
}

// Splits apPath into device, directory, file name and extension, each into a buffer of the given size (a null buffer is
// skipped).
inline int32_t BSsplitpath(
    const char* apPath,
    char* apDevice,
    int32_t aiDeviceLen,
    char* apDir,
    int32_t aiDirLen,
    char* apFile,
    int32_t aiFileLen,
    char* apExt,
    int32_t aiExtLen
) {
    return _splitpath_s(apPath, apDevice, aiDeviceLen, apDir, aiDirLen, apFile, aiFileLen, apExt, aiExtLen);
}
