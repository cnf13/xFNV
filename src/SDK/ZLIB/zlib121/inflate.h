#pragma once

#include "Reconstruction.hpp"

#include "SDK/ZLIB/zlib121/inftrees.h"

// The modes inflate's state machine is in (zlib 1.2.1 inflate.h's inflate_mode; the PDB's enumerators, in order).
enum inflate_mode : int32_t {
	HEAD = 0,
	FLAGS,
	TIME,
	OS,
	EXLEN,
	EXTRA,
	NAME,
	COMMENT,
	HCRC,
	DICTID,
	DICT,
	TYPE,
	TYPEDO,
	STORED,
	COPY,
	TABLE,
	LENLENS,
	CODELENS,
	LEN,
	LENEXT,
	DIST,
	DISTEXT,
	MATCH,
	LIT,
	CHECK,
	LENGTH,
	DONE,
	BAD,
	MEM,
	SYNC,
};

// inflate's stream state (zlib 1.2.1 inflate.h). The PDB's inflate_state has this exact layout and sizeof 7080; the
// exe's inflateInit2_ allocates that many bytes (0xB43EEE pushes 0x1BA8) and writes wrap at +0x8, wbits at +0x1C and
// window at +0x2C, and fixedtables (0xB45BD0) writes lencode at +0x44, distcode at +0x48, lenbits at +0x4C and
// distbits at +0x50. A caller may provide the state itself (TESFile::DecompressCurrentForm keeps one on the stack
// and points z_stream_s::state at it).
struct inflate_state {
	inflate_mode mode;
	int32_t last;
	int32_t wrap;
	int32_t havedict;
	int32_t flags;
	uint32_t check;
	uint32_t total;
	uint32_t wbits;
	uint32_t wsize;
	uint32_t whave;
	uint32_t write;
	uint8_t* window;
	uint32_t hold;
	uint32_t bits;
	uint32_t length;
	uint32_t offset;
	uint32_t extra;
	const code* lencode;
	const code* distcode;
	uint32_t lenbits;
	uint32_t distbits;
	uint32_t ncode;
	uint32_t nlen;
	uint32_t ndist;
	uint32_t have;
	code* next;
	uint16_t lens[320];
	uint16_t work[288];
	code codes[ENOUGH];
};

struct z_stream_s;

// inflate.c's file-local helpers (the PDB's names), called by inflate (0xB44000) as well.
// Points the state at the fixed Huffman code tables (cdecl, 0xB45BD0).
void fixedtables(inflate_state* state);
// Copies the output written since out bytes were available into the sliding window, allocating it on first use;
// 1 when that allocation fails.
int32_t updatewindow(z_stream_s* strm, uint32_t out);
// Looks in buf[0..len-1] for the 00 00 FF FF of an empty stored block, *have the bytes matched so far; returns the
// bytes looked at.
uint32_t syncsearch(uint32_t* have, uint8_t* buf, uint32_t len);

// IDA name; not in the PDB. The exe's ZLIB_VERSION string ("1.2.1"), whose first character inflateInit2_ compares
// with the caller's.
using my_version_t = const char[6];
extern my_version_t my_version;
