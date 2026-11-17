#pragma once

#include "SDK/Windows/winbase.h"

// The Windows SDK v6.0A <mmsystem.h> declarations the exe's code uses (WINMM.dll), with the SDK's names, so no Windows
// header is needed.

// Milliseconds on the same clock as GetTickCount (std::chrono::steady_clock), at its 1 ms resolution.
inline uint32_t timeGetTime() {
    return GetTickCount();
}
