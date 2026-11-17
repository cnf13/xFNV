#pragma once

#include <stdint.h>

// The steps of memCpyBackwards, one per copy width.
void memCopyBackwards8(const void* src, void* dst, uint32_t nbytes);
void memCopyBackwards4(void* dst, const void* src, uint32_t nbytes);
// The 2-byte step.
void memCopyBackwards2(void* dst, const void* src, uint32_t nbytes);
void memCopyBackwards1(const void* src, uint32_t nbytes, void* dst);

// Havok's memory copy helpers.
class hkMemUtil {
public:
	// The integer type an NBYTES-aligned copy moves at a time: the specializations below for 1, 2, 4 and 8 bytes, and
	// for a larger alignment the 8-byte one, through halving.
	template <uint32_t NBYTES> struct TypeFromAlign : public TypeFromAlign<NBYTES / 2> {};

	// Copies one ELEMENTSIZE-byte element aligned to ALIGN bytes from src to dst.
	template <int32_t ELEMENTSIZE, int32_t ALIGN> static void memCpyOneAligned(void* dst, const void* src);
	static void memCpy(void* dst, const void* src, int32_t nbytes);

	// Copies nbytes from src to dst ELEMENTALIGN bytes at a time, front to back (hkArray's removeAtAndCopy calls it).
	template <int32_t ELEMENTALIGN> static void memCpy(void* dst, const void* src, int32_t n) {
		typedef typename TypeFromAlign<ELEMENTALIGN>::type CopyType;
		for (int32_t i = 0, j = 0; i < n; i += sizeof(CopyType), ++j) {
			static_cast<CopyType*>(dst)[j] = static_cast<const CopyType*>(src)[j];
		}
	}

	// Copies nbytes from src to dst in the direction that is safe for overlapping ranges.
	static void memMove(void* dst, const void* src, int32_t nbytes);
	// Copies nbytes from src to dst from the end down, the order for ranges that overlap with
	// dst above src.
	static void memCpyBackwards(void* dst, const void* src, int32_t nbytes);
	// Sets n bytes at dst to c, by calling the CRT's memset with the same arguments.
	static void memSet(void* dst, const int32_t c, int32_t n);
};

template <> struct hkMemUtil::TypeFromAlign<1> {
	typedef char type;
};
template <> struct hkMemUtil::TypeFromAlign<2> {
	typedef int16_t type;
};
template <> struct hkMemUtil::TypeFromAlign<4> {
	typedef int32_t type;
};
template <> struct hkMemUtil::TypeFromAlign<8> {
	typedef int64_t type;
};
// Generic body for instantiations without a specialization: copies the element's ALIGN-byte words one at a time
// (hkArray's removeAt calls it).
template <int32_t ELEMENTSIZE, int32_t ALIGN> inline void hkMemUtil::memCpyOneAligned(void* dst, const void* src) {
	typedef typename TypeFromAlign<ALIGN>::type CopyType;
	uint32_t i = 0;
	do {
		static_cast<CopyType*>(dst)[i] = static_cast<const CopyType*>(src)[i];
	} while (++i < ELEMENTSIZE / sizeof(CopyType));
}
