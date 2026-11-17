#pragma once

#include "Reconstruction.hpp"

// rt: runtime support the game relies on. Here the VC9 compiler's array construction and destruction helpers the exe
// calls (stdcall): the `eh vector constructor iterator' and `eh vector destructor iterator' of the CRT's eh vector
// sources. They have no C++ identifier; the names here spell IDA's with underscores. They go when the arrays are
// built with new[].
namespace rt {

// The callback type of the three iterators, the exe CRT's thiscall `void (*)(void*)`: the constructor or destructor
// of any item type, its result ignored. FNVD_ORIGINAL_FUNCTION constants passed to them have it.
using EhVectorCallback = void (*)(void*);

// The copy constructor callback of the copy iterator, the exe CRT's thiscall `void (*)(void*, void*)`: the copy
// constructor of any item type (this, then the source item), its result ignored.
using EhVectorCopyCallback = void (*)(void*, void*);

// PDB name `vector copy constructor iterator' (no C++ identifier); the name here spells the CRT's with underscores.
// Not reconstructed yet: the CRT's `eh vector copy constructor iterator', 0xECAEBB (stdcall, ret 0x18): runs
// apfnCopyConstructor (thiscall) on aiCount items of astSize bytes from apArray, each from the matching item of
// apSource, and on an exception destroys the items already built with apfnDestructor.
inline void eh_vector_copy_constructor_iterator(void* apArray, const void* apSource, size_t astSize, int32_t aiCount,
	EhVectorCopyCallback apfnCopyConstructor, EhVectorCallback apfnDestructor)
{
	char* pItems = static_cast<char*>(apArray);
	char* pSources = const_cast<char*>(static_cast<const char*>(apSource));
	int32_t iBuilt = 0;
	try {
		for (; iBuilt < aiCount; ++iBuilt) {
			apfnCopyConstructor(pItems + iBuilt * astSize, pSources + iBuilt * astSize);
		}
	} catch (...) {
		// The items already built are destroyed, last to first, and the exception goes on.
		while (iBuilt-- > 0) {
			apfnDestructor(pItems + iBuilt * astSize);
		}
		throw;
	}
}

// KEEP: the forwarder stays (0x401050). An exe caller reads ecx after the call, relying on the body leaving it intact,
// which clang does not guarantee; the constructors it runs are exe thiscall code passed as an address.
// `eh vector constructor iterator', 0x401050 (stdcall, ret 0x10), the compiler helper that
// runs the constructor apfnConstructor (thiscall) on aiCount items of astSize bytes from apArray.
void eh_vector_constructor_iterator(void* apArray, size_t astSize, int32_t aiCount,
	EhVectorCallback apfnConstructor);

// Not reconstructed yet: the CRT's `eh vector constructor iterator', 0xEC782F (stdcall), the form with a destructor:
// runs apfnConstructor (thiscall) on aiCount items of astSize bytes from apArray, and on an exception destroys the
// items already built with apfnDestructor.
// Inferred type: aiCount: int32_t: signed uses from 0xec784c
inline void eh_vector_constructor_iterator(void* apArray, size_t astSize, int32_t aiCount,
	EhVectorCallback apfnConstructor, EhVectorCallback apfnDestructor)
{
	char* pItems = static_cast<char*>(apArray);
	int32_t iBuilt = 0;
	try {
		for (; iBuilt < aiCount; ++iBuilt) {
			apfnConstructor(pItems + iBuilt * astSize);
		}
	} catch (...) {
		// The items already built are destroyed, last to first, and the exception goes on.
		while (iBuilt-- > 0) {
			apfnDestructor(pItems + iBuilt * astSize);
		}
		throw;
	}
}

// Not reconstructed yet: the CRT's `eh vector destructor iterator', 0xEC5FCE (stdcall): runs apfnDestructor
// (thiscall) on aiCount items of astSize bytes from apArray, last to first.
inline void eh_vector_destructor_iterator(void* apArray, size_t astSize, int32_t aiCount,
	EhVectorCallback apfnDestructor)
{
	char* pItem = static_cast<char*>(apArray) + aiCount * astSize;
	int32_t iLeft = aiCount;
	try {
		while (iLeft-- > 0) {
			pItem -= astSize;
			apfnDestructor(pItem);
		}
	} catch (...) {
		// A destructor threw: the items before it are still destroyed, last to first, and the exception goes on.
		while (iLeft-- > 0) {
			pItem -= astSize;
			apfnDestructor(pItem);
		}
		throw;
	}
}

} // namespace rt

// Not named in the PDB or IDA; name chosen here. A copy of VC9's `vector destructor iterator' (stdcall, ret 0x10,
// 0xA26FA0): runs apfnDestructor (thiscall, the element in ecx) on aiCount elements of auiSize bytes from the last
// to the first. No caller reads eax.
void __ehvec_dtor(void* apArray, uint32_t auiSize, int32_t aiCount, rt::EhVectorCallback apfnDestructor);
