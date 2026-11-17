// SystemAlloc.h's pass-through: the C runtime's aligned heap (Platform.h), or the guard-page heap under
// FNVD_GUARD_ALLOC (CMakeLists.txt defines it on this file).

#include "SystemAlloc.h"

#include "DebugHeap.h"
#include "Platform.h"

namespace fnvd::sysalloc {

#if FNVD_GUARD_ALLOC
// The game heap's alignment: 16 (bodies keep hkVector4s in game-heap blocks and read them with movaps).
constexpr size_t kAlign = 16;

void* Allocate(size_t aSize)
{
	return debugheap::Allocate(aSize, kAlign);
}

void Free(void* apMem)
{
	debugheap::Free(apMem);
}

void* Reallocate(void* apMem, size_t aSize)
{
	return debugheap::Reallocate(apMem, aSize, kAlign);
}

size_t Size(const void* apMem)
{
	return debugheap::Size(apMem);
}

void* AlignedAllocate(size_t aSize, size_t aAlign)
{
	return debugheap::Allocate(aSize, aAlign);
}

void AlignedFree(void* apMem)
{
	debugheap::Free(apMem);
}
#else
// The game heap's alignment, 16 (malloc gives 8 on 32-bit): aligned blocks, freed, grown and measured as such.
constexpr size_t kAlign = 16;

void* Allocate(size_t aSize)
{
	return debugheap::platform::AlignedAllocate(aSize, kAlign);
}

void Free(void* apMem)
{
	debugheap::platform::AlignedFree(apMem);
}

void* Reallocate(void* apMem, size_t aSize)
{
	return debugheap::platform::AlignedReallocate(apMem, aSize, kAlign);
}

size_t Size(const void* apMem)
{
	return debugheap::platform::AlignedSize(const_cast<void*>(apMem), kAlign);
}

void* AlignedAllocate(size_t aSize, size_t aAlign)
{
	return debugheap::platform::AlignedAllocate(aSize, aAlign);
}

void AlignedFree(void* apMem)
{
	debugheap::platform::AlignedFree(apMem);
}
#endif

}  // namespace fnvd::sysalloc
