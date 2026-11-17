#pragma once

#include <cstddef>

// A zero size is raised to 1; 4-byte aligned (event NI_MALLOC).
void* _NiMalloc(size_t astSizeInBytes);
// Null is ignored (event NI_FREE).
void _NiFree(void* pvMem);
// Realloc on the external allocator: frees on a zero size, allocates on a null block.
void* _NiExternalRealloc(void* pvMem, size_t astSizeInBytes);
// As _NiExternalRealloc, aligned to astAlignment.
void* _NiExternalAlignedRealloc(void* pvMem, size_t astSizeInBytes, size_t stAlignment);
// Frees a block from the aligned allocator; null is ignored.
void _NiAlignedFree(void* pvMem);
// Frees a block from the external aligned allocator; null is ignored (event NI_EXTERNAL_ALIGNEDFREE).
void _NiExternalAlignedFree(void* pvMem);
// A zero size is raised to 1; aligned to astAlignment.
void* _NiAlignedMalloc(size_t stSizeInBytes, size_t stAlignment);
// A zero size is raised to 1; aligned to astAlignment on the external allocator (event NI_EXTERNAL_ALIGNEDMALLOC).
void* _NiExternalAlignedMalloc(size_t stSizeInBytes, size_t stAlignment);
// Frees a block from the external allocator; null is ignored (event NI_EXTERNAL_FREE).
void _NiExternalFree(void* pvMem);

// Gamebryo heap entry points for plain blocks (no constructor or destructor runs); objects use NiMemObject's new and
// delete. All go through NiMemManager's allocator.
