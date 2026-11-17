#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

// The DirectShow BaseClasses' wide-string compare for builds without the Unicode API (wxutil.cpp; not in the PDB,
// name from the BaseClasses source). Strings are WCHAR (2-byte) arrays. Returns the difference of the first
// differing characters, or 0 when the strings are equal (stdcall).
int32_t WINAPI lstrcmpWInternal(const uint16_t* lpString1, const uint16_t* lpString2);

// The DirectShow BaseClasses' AMGetWideString (wxutil.cpp; not in the PDB, name from the BaseClasses source; stdcall,
// ret 8, 0x9FD0A0): a CoTaskMemAlloc'd copy of psz at *ppszReturn.
HRESULT WINAPI AMGetWideString(const uint16_t* psz, uint16_t** ppszReturn);

uint16_t* WINAPI lstrcpynWInternal(uint16_t* a1, int16_t* a2, int32_t a3);

// The length of a wide string, in characters (the DirectShow base classes' lstrlenW).
int32_t WINAPI strlen_w(const uint16_t* a1);
