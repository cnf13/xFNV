#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>

// The Microsoft-only functions of VC9's <stdio.h> the game calls, under their own names, and the file functions that
// take a game path (rt). The standard functions are std:: (<cstdio>); the game's stream is the C library's FILE.
//
// The Windows CRT declares and defines every one of the Microsoft-only functions itself (in its <stdio.h>), with the
// contract written here: a second definition would be a second definition of the CRT's own C function. So on Windows
// the CRT's are used, and elsewhere they are declared here and stdio.cpp defines them to VC9's buffer rules (the
// conversions themselves are the C library's, which differ from VC9's in details: glibc prints two exponent digits
// where VC9 prints three).
#if !defined(_WIN32)

// _snprintf, 0xECA38D (cdecl; IDA's name snprintf): as _vsnprintf.
int _snprintf(char* apcBuffer, size_t astCount, const char* apcFormat, ...);

// _vsnprintf, 0xECDDF1 (IDA: __vsnprintf): writes at most astCount characters. A text shorter than astCount is
// terminated and its length returned; one of exactly astCount characters is not terminated, its length returned; a
// longer one leaves its first astCount characters, unterminated, and -1 is returned. A null apBuffer (with
// astCount 0) returns the length.
int _vsnprintf(char* apBuffer, size_t astCount, const char* apFormat, va_list apArgs);

// vsprintf_s, 0xEC6BB5 (IDA's name vsnprintf; the code is VC9's vsprintf_s, _vsnprintf_helper over _output_s_l): a
// text that does not fit in astCount characters with its terminator empties the buffer and is an ERANGE invalid
// parameter; -1 then, else the length.
int vsprintf_s(char* apBuffer, size_t astCount, const char* apFormat, va_list apArgs);

// fread_s, 0xECB312 (cdecl): reads elements of elementSize bytes, at most bufferSize bytes in all; returns the
// elements read. A null buffer or stream is EINVAL, a request larger than bufferSize ERANGE (the buffer zeroed,
// unless bufferSize is unlimited); both return 0.
size_t fread_s(void* buffer, size_t bufferSize, size_t elementSize, size_t count, FILE* stream);

#endif

// rt: runtime support the game relies on. The file functions that take a game path open its native path
// (platform::NativePath: the game's paths are Windows paths, relative to the game's folder). Defined in stdio.cpp.
namespace rt {

    // fopen, 0xEC9A47: the C library's fopen of the game path's native path.
    FILE* fopen(const char* apFilename, const char* apMode);

    // fopen_s, 0xEC9A5E: opens apFilename into *appFile (null on failure); 0 or the errno. VC9 opens with _SH_SECURE
    // (no other writer while it writes); the standard fopen shares the file.
    int32_t fopen_s(FILE** appFile, const char* apFilename, const char* apMode);

    // fgets of a stream opened in text mode (no `b` in its mode), as the Windows C runtime reads one: a CR LF line end
    // reads as LF, and Ctrl-Z ends the file. glibc's streams have no text mode and would keep the CR; on Windows this
    // is the C library's fgets.
    char* fgets(char* apBuffer, int32_t aiCount, FILE* apStream);

    // rename, 0xEC862C: 0 on success. The files are the game paths' native paths.
    int32_t rename(const char* apOldName, const char* apNewName);

} // namespace rt
