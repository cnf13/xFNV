#pragma once

#include <cstdint>

// The game's boolean: one byte, 0 (false) or 1 (true). The enumerators would be the C++ keywords false and true, so
// they are spelled with an e prefix here.
enum eBool : uint8_t {
	eFalse = 0x0,
	eTrue = 0x1,
};
