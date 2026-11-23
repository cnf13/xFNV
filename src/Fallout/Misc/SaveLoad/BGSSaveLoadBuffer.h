#pragma once

#include <cstdint>

// The size of a save-game buffer's items.
enum SIZE_TYPE : int32_t {
    SIZE_CHAR = 0,
    SIZE_SHORT = 1,
    SIZE_INT = 2,
    SIZE_COUNT = 3,
};

// A save-game byte buffer: the pointer to its bytes.
class BGSSaveLoadBuffer {
public:
    char* pBuffer;
};
