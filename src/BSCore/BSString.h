#pragma once

#include "Reconstruction.hpp"

#include <cstddef>

// The C-variadic function itself (cdecl, 0x406D00; its body is in Fallout Shared/Magic/EffectItemList.cpp, the PDB's
// file). A call without format arguments resolves to it.
int32_t BSsprintf(char* apString1, int32_t aiSize, const char* apFormat, ...);
// Case-sensitive compare.
int32_t BSstrcmp(const char* apString1, const char* apString2);
// Case-insensitive compare: the CRT _stricmp.
int32_t BSstricmp(const char* apString1, const char* apString2);
// Bounded copy of apString2 into an aiSize-byte buffer: the CRT strcpy_s; returns its errno.
int32_t BSstrcpy(char* apString1, int32_t aiSize, const char* apString2);
// Bounded append of apSource to the string in an auiSize-byte buffer: the CRT strcat_s; returns its errno. Not
// reconstructed yet.
int32_t BSstrcat(char* apString1, int32_t aiSize, const char* apString2);
// String length: the CRT strlen.
uint32_t BSstrlen(const char* apString);
// Copies at most aiLen characters of apString2 into apString1: the CRT strncpy_s with a buffer of aiLen + 1;
// returns its errno.
int32_t BSstrncpy(char* apString1, const char* apString2, int32_t aiLen);
// Case-insensitive compare of at most auiCount characters.
int32_t BSstrnicmp(const char* apString1, const char* apString2, uint32_t aiCount);
// Unbounded copy of apStrSource into apStrDest; returns apStrDest.
char* ourStrcpy(char* apStrDest, const char* apStrSource);
// Bounded move of aiCount bytes from apSrc to apDest (a buffer of aiDestSize bytes): the CRT memmove_s (0xEC7CE3);
// returns its errno (cdecl, 0x8661E0; IDA: _memmove_s_).
int32_t BSmemmove(void* apDest, int32_t aiDestSize, void* apSrc, int32_t aiCount);

// vsnprintf into a buffer of aiSize bytes; returns vsnprintf's count. The template forwards calls with format
// arguments to the exe (cdecl, 0x406D00): the arguments after the format get the `...` promotions.
template <class... A> int32_t BSsprintf(char* apString1, int32_t aiSize, const char* apFormat, A... vList) {
	// The C-variadic overload by pointer: a plain call would pick this template again.
	int32_t (*pVariadic)(char*, int32_t, const char*, ...) = BSsprintf;
	return pVariadic(apString1, aiSize, apFormat, vList...);
}
