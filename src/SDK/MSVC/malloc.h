#pragma once

#include <cstddef>
#include <malloc.h>

// The Microsoft-only function of VC9's <malloc.h> the game calls. The Windows CRT declares and defines _msize itself
// (in its <malloc.h>, included above); a second definition would be a second definition of the CRT's own C function,
// so on Windows the CRT's is used, and elsewhere it is defined here.
#if !defined(_WIN32)

// _msize, 0xECD31F: the size of a C heap block. glibc's malloc_usable_size is the block's usable size, which can
// exceed the size asked for; VC9's is the size asked for (HeapSize).
inline size_t _msize(void* apMemory) {
    return ::malloc_usable_size(apMemory);
}

#endif
