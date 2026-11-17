#pragma once

#include <cstdint>

// The array operations hkSmallArray calls when it grows; the element type is passed to them as its size. The storage
// comes from the memory router's per-thread heap allocator, not from an allocator the caller names.
class hkSmallArrayUtil {
public:
    // Grows an array's storage to aiReqElem elements of aiSizeElem bytes and returns its new capacity and flags.
    // apArray is any hkSmallArray<T>, passed as void*.
    static int16_t _reserve(void* array, int32_t numElem, int32_t sizeElem);
    // Grows an array to twice its size, or to a single element when it is empty, and returns its new capacity and
    // flags. apArray is any hkSmallArray<T>, as for _reserve.
    static int16_t _reserveMore(void* array, int32_t sizeElem);
};
