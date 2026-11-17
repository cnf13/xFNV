#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Reconstruction.hpp"

#include <stdarg.h>
#include <stdint.h>

// The string helpers the Havok code uses instead of the C run time. hkString has no instance data: every member is
// static.
class hkString {
public:
	// Whether a search or replace stops at the first match or goes through the whole string (4 bytes: 0xCB1440
	// compares the dword).
	enum ReplaceType : int32_t {
		REPLACE_ONE = 0,
		REPLACE_ALL = 1,
	};

	// Not reconstructed yet. Copies at most aiCount characters (cdecl, ret 0, 0xCB10B0). Its eax is the CRT's strncpy
	// result, or aiCount when it is zero; no caller reads it.
	static void strNcpy(char* dst, const char* src, int32_t n);
	// The first c in string, or null (cdecl, ret 0, 0xCB1140, a jump to the CRT's strchr).
	static const char* strChr(const char* haystack, int32_t needle);
	// The last c in string, or null (cdecl, ret 0, 0xCB1150, a jump to the CRT's strrchr).
	static const char* strRchr(const char* haystack, int32_t needle);
	static char toLower(char c);
	static char* strCpy(char* dst, const char* src);
	static int32_t strCasecmp(const char* a, const char* b);
	static int32_t strLen(const char* src);
	static float atof(const char* in);
	static char* strDup(const char* apcString, hkMemoryAllocator& apAllocator);
	// A copy of apcString from the calling thread's heap.
	static char* strDup(const char* apcString);
	// A copy of at most aiMaxLen characters of apcString from apAllocator.
	static char* strNdup(const char* apcString, int32_t aiMaxLen, hkMemoryAllocator& apAllocator);
	// A copy of at most aiMaxLen characters of apcString from the calling thread's heap.
	static char* strNdup(const char* apcString, int32_t aiMaxLen);
	// Frees a string from the calling thread's heap, as strDup made it.
	static void strFree(char* s);
	static int32_t strCmp(const char* a, const char* b);
	// Copies aiSize bytes: the exe's whole body is `jmp memcpy` (0xCB1260), which leaves the destination in eax.
	static void* memCpy(void* dst, const void* src, int32_t n);

	// The SDK's inline copy of numWords dwords (hkString.inl); the exe inlines it (rep movsd).
	static void memCpy4(void* dst, const void* src, int32_t numWords) {
		const uint32_t* src32 = static_cast<const uint32_t*>(src);
		uint32_t* dst32 = static_cast<uint32_t*>(dst);
		for (int32_t i = 0; i < numWords; i++) {
			*(dst32++) = *(src32++);
		}
	}

	// The SDK's inline fill of numWords dwords with value (hkString.inl); the exe inlines it (rep stosd).
	static void memSet4(void* dst, const int32_t value, int32_t numWords) {
		uint32_t* dst32 = static_cast<uint32_t*>(dst);
		for (int32_t i = numWords - 1; i >= 0; i--) {
			*dst32 = static_cast<uint32_t>(value);
			dst32++;
		}
	}

	// The SDK's inline copy of numQuads 16-byte blocks as dwords (hkString.inl); none when numQuads is not positive.
	static void memCpy16(void* dst, const void* src, int32_t numQuads) {
		const uint32_t* src32 = static_cast<const uint32_t*>(src);
		uint32_t* dst32 = static_cast<uint32_t*>(dst);
		for (int32_t i = 0; i < numQuads; i++) {
			dst32[0] = src32[0];
			dst32[1] = src32[1];
			dst32[2] = src32[2];
			dst32[3] = src32[3];
			dst32 += 4;
			src32 += 4;
		}
	}

	// The SDK's inline copy of numQuads 16-byte blocks as dwords (hkString.inl); at least one block is copied.
	static void memCpy16NonEmpty(void* dst, const void* src, int32_t numQuads) {
		const uint32_t* src32 = static_cast<const uint32_t*>(src);
		uint32_t* dst32 = static_cast<uint32_t*>(dst);
		do {
			dst32[0] = src32[0];
			dst32[1] = src32[1];
			dst32[2] = src32[2];
			dst32[3] = src32[3];
			dst32 += 4;
			src32 += 4;
		} while (--numQuads > 0);
	}

	// Copies aiSize bytes, the ranges allowed to overlap: the exe's whole body is `jmp
	// memmove` (0xEC7230; cdecl, ret 0, 0xCB1270; IDA: j::memcpy_0_0); no caller reads the eax it leaves.
	static void memMove(void* dst, const void* src, int32_t n);
	// Not reconstructed yet. Fills nBytes bytes at apDest with c: in the exe a jump into the C run time's memset
	// (cdecl, 0xCB1280); no caller reads the eax it leaves.
	static void memSet(void* dst, int32_t c, int32_t n);
	// The exe returns the hkBool through a hidden pointer, so endsWith takes it as its first argument.
	static hkBool* endsWith(hkBool* arResult, const char* apcString, const char* apcEnding);
	// Havok SDK signature (hkBase.lib, tied by reloc).
	static int32_t strNcmp(const char* a, const char* b, int32_t n);
	// Havok SDK signature (hkBase.lib, tied by reloc); hkargs is the SDK's hk_va_list, a va_list.
	static int32_t vsnprintf(char* buf, int32_t len, const char* fmt, va_list hkargs);
	// The upper-case of a lower-case letter, else the character.
	static char toUpper(char c);
	// Formats into apcBuffer, writing at most aiMaxLen characters; returns the length the text needs.
	static int32_t snprintf(char* buf, int32_t len, const char* fmt, ...);
	// Formats into apcBuffer without a bound.
	static int32_t sprintf(char* buf, const char* fmt, ...);
	// Case-insensitive compare of at most aiMaxLen characters; the lowered characters compare signed.
	static int32_t strNcasecmp(const char* a, const char* b, int32_t n);
	// Lower-cases apcString in place and returns it.
	static char* strLwr(char* s);
	// Upper-cases apcString in place and returns it.
	static char* strUpr(char* s);
	// The first occurrence of apcSubString in apcString, or null.
	static const char* strStr(const char* haystack, const char* needle);
	// Appends the index of every occurrence of apcSubString in apcString to arOffsets: the first one always, the later
	// ones only for REPLACE_ALL. arResult says whether there was one at all.
	static hkBool* findAllOccurrences(hkBool* arResult, const char* apcString, const char* apcSubString,
		hkArray<int32_t>& arOffsets, ReplaceType eReplaceType);
	// Inferred type: alloc: hkMemoryAllocator* from 0xcb13ce (hkMemoryRouter::m_heap (result of
	// hkMemoryRouter::getInstance))
	static void strFree_00CB1190(char* s, hkMemoryAllocator& alloc);
	static hkBool* beginsWith(hkBool* apResult, const char* a, const char* b);
	static int32_t lastIndexOf(const char* str, char c);
	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	static int32_t memCmp(const void* buf1, const void* buf2, int32_t n);
	// The SDK's copy of size bytes, 16 at a time (hkString.inl). The exe keeps its instances out of line, so only
	// their specializations are defined.
	template <int32_t size> static void memCpy16(void* dst, const void* src);
	// Only reads String (std::strtoul); the SDK's atoi takes a const
	// char*, and detectFormat passes an hkStringPtr's cString().
	// Inferred type: return: hkClassMember::Type: returned from hkClassMember::getSubtypeOf at 0xd86eab
	static int32_t atoi(const char* String, int32_t Radix);
};
// The SDK's body: size / 16 blocks of four dwords through memCpy16NonEmpty, which the exe's copy (0xCF8690) inlines.
template <int32_t size> void hkString::memCpy16(void* dst, const void* src) {
	hkString::memCpy16NonEmpty(dst, src, size / 16);
}
// hkStringPtr's comparisons (the SDK's inline bodies, hkStringPtr.inl); here, since they need hkString.
inline int32_t hkStringPtr::compareTo(const char* s) const {
	const char* t = cString();
	if (t && s) {
		return hkString::strCmp(t, s);
	}
	return t ? 1 : (s ? -1 : 0);
}

inline int32_t hkStringPtr::operator==(const char* cptr) const {
	return compareTo(cptr) == 0;
}

inline int32_t hkStringPtr::operator!=(const char* s) const {
	return compareTo(s) != 0;
}

inline int32_t hkStringPtr::startsWith(const char* s) const {
	hkBool result;
	return *hkString::beginsWith(&result, cString(), s);
}

inline int32_t hkStringPtr::endsWith(const char* s) const {
	hkBool result;
	return *hkString::endsWith(&result, cString(), s);
}
