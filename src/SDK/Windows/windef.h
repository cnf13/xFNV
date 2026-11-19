#pragma once

#include "Reconstruction.hpp"
#include "SDK/Windows/winnt.h"

// The Windows SDK v6.0A <windef.h> declarations the exe's code uses, with the SDK's names and 32-bit layouts, so no
// Windows header is needed. Global, as Windows declares them.

// The calling convention of the Windows API and of the callbacks it takes: stdcall on 32-bit Windows; nothing
// elsewhere (x64 has one convention, and the Linux stand-ins are plain functions). The program spells a convention
// only with these names, at the boundary with code outside it.
#if defined(_WIN32) && defined(_M_IX86)
#define WINAPI __stdcall
#define CALLBACK __stdcall
#else
#define WINAPI
#define CALLBACK
#endif
#define APIENTRY WINAPI

// HWND's type, without windows.h (the same as windows.h declares it)
struct HWND__ {
    int32_t unused;
};

// HINSTANCE's type, without windows.h (the same as windows.h declares it)
struct HINSTANCE__ {
    int32_t unused;
};

// HHOOK's type, without windows.h (the same as windows.h declares it)
struct HHOOK__ {
    int32_t unused;
};

// HMONITOR's and HDC's types, without windows.h (the same as windows.h declares them): pointers, 8 bytes on x64.
struct HMONITOR__ {
    int32_t unused;
};
struct HDC__ {
    int32_t unused;
};

typedef struct tagRECT {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} RECT;

typedef struct tagPOINT {
    int32_t x;
    int32_t y;
} POINT;

// The SDK's FILETIME (8 bytes): a 64-bit count of 100-nanosecond intervals as two dwords.
struct _FILETIME {
    uint32_t dwLowDateTime;
    uint32_t dwHighDateTime;
};
