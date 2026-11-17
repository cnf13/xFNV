#pragma once

// FNVD_SYSTEM_ALLOC's pass-through (docs/ENVIRONMENT.md): the no-exe program's game allocators (MemoryManager,
// ScrapHeap, Havok's router allocators) call these in place of their pools, heaps and arenas. The C runtime's heap,
// or with FNVD_GUARD_ALLOC the guard-page heap of DebugHeap.h. Declarations only, so that src files include no
// Windows header through it.

#include <stddef.h>

namespace fnvd::sysalloc {

// malloc, free, realloc, _msize.
void* Allocate(size_t aSize);
void Free(void* apMem);
void* Reallocate(void* apMem, size_t aSize);
size_t Size(const void* apMem);

// _aligned_malloc and _aligned_free (aAlign: a power of two): a pair of their own, never mixed with the four above.
void* AlignedAllocate(size_t aSize, size_t aAlign);
void AlignedFree(void* apMem);

}  // namespace fnvd::sysalloc
