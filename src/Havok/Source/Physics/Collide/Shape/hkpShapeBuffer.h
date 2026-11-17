#pragma once

#include <stdint.h>

// The size of the stack buffer a shape container builds a child shape in (the SDK's hkpShapeBuffer, 16-byte
// aligned).
enum hkpBufferSize : int32_t {
    HK_SHAPE_BUFFER_SIZE = 512,
};
