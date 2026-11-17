#pragma once

// The program's guard-page heap (FNVD_GUARD_ALLOC, docs/ENVIRONMENT.md): no game code. An allocation of at
// least kGuardMinSize bytes (DebugHeap.cpp) sits at the end of its own run of pages, with a no-access page after it
// and a header before it (size, allocating thread, return addresses); a freed run is decommitted and its addresses are
// not handed out again until the allocator has gone round the whole reserved region. A smaller allocation is the C
// runtime's malloc with a header and a tail canary, checked at free. An access violation is described in
// fnvd_debugheap.log (the game's folder): the block the faulting address or a register points into, who allocated
// it and who freed it.

#include <stddef.h>

namespace fnvd::debugheap {

// aAlign: a power of two.
void* Allocate(size_t aSize, size_t aAlign);
// Null is ignored; a pointer this heap did not hand out goes to the C runtime's free.
void Free(void* apMem);
// The size asked for.
size_t Size(const void* apMem);
// A new block, the old contents copied up to the smaller size, the old block freed.
void* Reallocate(void* apMem, size_t aSize, size_t aAlign);

}  // namespace fnvd::debugheap
