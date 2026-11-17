#pragma once

#include <stdint.h>

// What the binary tagfile reader and writer share.
namespace hkBinaryTagfile {
// The tag that starts each record of a binary tagfile (values from the SDK header).
enum Tags : int32_t {
	TAG_NONE = 0,
	TAG_FILE_INFO = 1,
	TAG_METADATA = 2,
	TAG_OBJECT = 3,
	TAG_OBJECT_REMEMBER = 4,
	TAG_OBJECT_BACKREF = 5,
	TAG_OBJECT_NULL = 6,
};

// Returns n with its bytes reversed, swapped through a local byte/word union.
inline uint32_t convertEndianU32(uint32_t n) {
	union fmtU32 {
		uint8_t b[4];
		uint32_t v;
	};
	union fmtU32 fDataIn, fDataOut;

	fDataIn.v = n;
	fDataOut.b[0] = fDataIn.b[3];
	fDataOut.b[1] = fDataIn.b[2];
	fDataOut.b[2] = fDataIn.b[1];
	fDataOut.b[3] = fDataIn.b[0];
	return fDataOut.v;
}
uint8_t* isBinaryMagic(uint8_t* a1, int32_t a2, int32_t a3);
}
