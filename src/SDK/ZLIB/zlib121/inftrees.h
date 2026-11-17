#pragma once

#include "Reconstruction.hpp"

// zlib 1.2.1 inftrees.h: one entry of inflate's decoding tables. op is the operation (literal, table link, length or
// distance base with extra bits, end of block, invalid code), bits the bits this code uses, val its value or offset.
struct code {
	uint8_t op;
	uint8_t bits;
	uint16_t val;
};

// ENOUGH: zlib 1.2.1 inftrees.h's maximum size of the dynamic code tables, in code entries (inflate_state::codes).
// The exe's inflate_state has room for exactly this many: 0x1BA8 is the 1320 bytes before codes plus 1440 * 4.
static constexpr uint32_t ENOUGH = 1440;

// The kind of table inflate_table builds.
enum codetype {
	CODES = 0,
	LENS = 1,
	DISTS = 2,
};

// Builds the decoding table for the codes lengths lens[0..codes-1] at *table, advancing *table past it and setting
// *bits to its root size; work is scratch of at least codes entries. 0 on success, 1 when the table would not fit in
// ENOUGH entries, -1 for an over-subscribed or incomplete set of lengths.
int32_t inflate_table(codetype type, uint16_t* lens, uint32_t codes, code** table, uint32_t* bits,
	uint16_t* work);
