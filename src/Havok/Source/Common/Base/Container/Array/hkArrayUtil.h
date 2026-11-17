#pragma once

#include "Reconstruction.hpp"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkTypeTraits.h"

#include <new>

template <typename T> class hkArrayBase;
class hkMemoryAllocator;
struct hkpRootCdPoint;
class hkpShape;

// The array operations hkArray calls when it grows or shrinks; the element type is passed as its size.
class hkArrayUtil {
public:
	// Grows an array's storage to aiReqElem elements of aiSizeElem bytes.
	static void _reserve(hkMemoryAllocator& mem, void* array, int32_t numElem, int32_t sizeElem);

	// The same call for an array of T, through a pointer to the allocator: the exe passes every hkArrayBase<T> as
	// the untyped array to the one _reserve above (0xC908C0).
	template <typename T>
	static void _reserve(hkMemoryAllocator* mem, hkArrayBase<T>* array, int32_t numElem, int32_t sizeElem) {
		_reserve(*mem, array, numElem, sizeElem);
	}

	// Grows an array to twice its size, or to a single element when it is empty, and returns its new capacity.
	// apArray is any hkArrayBase<T>: the SDK's void* array, as the exe passes it.
	static int32_t _reserveMore(hkMemoryAllocator& mem, void* array, int32_t sizeElem);
	// Shrinks an array's storage to aiNumElems elements, moving them to apDest when one is given. apArray is any
	// hkArrayBase<T>, as in the SDK (void* array).
	static void _reduce(hkMemoryAllocator& mem, void* array, int32_t sizeElem, char* inplaceMem,
		int32_t requestedCapacity);

	// Destroys n elements at t; the class version, an empty loop for elements without a destructor. Out of line per
	// element type in the exe (cdecl): each instantiation src uses gets an explicit specialization forwarding to it.
	template <typename T> static void destruct(T* t, int32_t n, hkTraitBool<0>);
	// Copy-constructs n elements at t from tcopy; the class version. Out of line per element type, like
	// destruct.
	template <typename T> static void construct(T* t, int32_t n, const T& tcopy, hkTraitBool<0>);

	// The plain-data versions of destruct and construct: empty. The exe makes the calls out of line per element type
	// (cdecl), to its shared empty function 0x40FBE0.
	template <typename T> static void destruct(T* t, int32_t n, hkTraitBool<1>) {}

	template <typename T> static void construct(T* t, int32_t n, hkTraitBool<1>) {}

	// The plain-data copies: tcopy into each of the n elements at t, and the n elements of tcopy into t.
	template <typename T> static void constructWithCopy(T* t, int32_t n, const T& tcopy, hkTypeIsPod) {
		for (int32_t i = 0; i < n; ++i) {
			t[i] = tcopy;
		}
	}

	template <typename T> static void constructWithArray(T* t, int32_t n, const T* tcopy, hkTypeIsPod) {
		for (int32_t i = 0; i < n; ++i) {
			t[i] = tcopy[i];
		}
	}

	// The class versions of construct, constructWithCopy and constructWithArray (and destruct above): the SDK's run
	// each element's C++ constructor or destructor. The hybrid's classes have none until parity (theirs are the static
	// Construct/Destruct), so there an element type that needs one gets an explicit specialization and any other use
	// fails to link; the standalone build's generic bodies follow the class.
	template <typename T> static void construct(T* t, int32_t n, hkTypeIsClass);
	template <typename T> static void constructWithCopy(T* t, int32_t n, const T& tcopy, hkTypeIsClass);
	template <typename T> static void constructWithArray(T* t, int32_t n, const T* tcopy, hkTypeIsClass);
};

// The SDK's class versions (hkArrayUtil.inl): each element's own constructor, copy constructor or destructor. The
// standalone build's classes have real ones; the hybrid's have only their Construct/Destruct statics.
template <typename T> void hkArrayUtil::destruct(T* t, int32_t n, hkTraitBool<0>) {
	for (int32_t i = 0; i < n; ++i) {
		t[i].~T();
	}
}

template <typename T> void hkArrayUtil::construct(T* t, int32_t n, const T& tcopy, hkTraitBool<0>) {
	for (int32_t i = 0; i < n; ++i) {
		new (t + i) T(tcopy);
	}
}

template <typename T> void hkArrayUtil::construct(T* t, int32_t n, hkTypeIsClass) {
	for (int32_t i = 0; i < n; ++i) {
		new (t + i) T();
	}
}

template <typename T> void hkArrayUtil::constructWithCopy(T* t, int32_t n, const T& tcopy, hkTypeIsClass) {
	for (int32_t i = 0; i < n; ++i) {
		new (t + i) T(tcopy);
	}
}

template <typename T> void hkArrayUtil::constructWithArray(T* t, int32_t n, const T* tcopy, hkTypeIsClass) {
	for (int32_t i = 0; i < n; ++i) {
		new (t + i) T(tcopy[i]);
	}
}
// After hkArrayUtil, which hkArray's inline members call: a file including this header alone still gets hkArray.
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
