#pragma once

#include <cstddef>

// Root of Gamebryo's heap-allocated classes; in the engine it routes their new/delete to the engine heap.
class NiMemObject {
public:
	// The engine's NiNew macro. A zero size is raised to 1; the block comes from NiMemManager's allocator (event
	// NI_OPER_NEW). The engine's new-expressions test the result for null, hence noexcept.
	static void* operator new(size_t astSizeInBytes) noexcept;

	// Null is ignored.
	static void operator delete(void* apvMem, size_t astElementSize);

	// Like operator new, but the block is allocated as an array (event NI_OPER_NEW_ARRAY); a zero size is raised to 1.
	static void* operator new[](size_t astSizeInBytes) noexcept;

	// Null is ignored; stElementSize is not read.
	static void operator delete[](void* pvMem, size_t stElementSize);

	// Placement forms, for objects built in storage the caller allocated (hidden by the operators above otherwise).
	static void* operator new(size_t, void* apvMem) noexcept { return apvMem; }
	static void operator delete(void*, void*) noexcept {}
};
