#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkTypeTraits.h"

#include <new>

class hkMemoryAllocator;
struct hkpRootCdPoint;
class hkpShape;

template <typename T> class hkArrayBase;

// The array operations hkArray calls when it grows or shrinks; the element type is passed as its size.
class hkArrayUtil {
public:
	// Grows an array's storage to aiReqElem elements of aiSizeElem bytes.
	static void _reserve(hkMemoryAllocator& mem, void* array, int32_t numElem, int32_t sizeElem);

	// The same call for an array of T, through a pointer to the allocator: every hkArrayBase<T> is passed as the
	// untyped array to the one _reserve above.
	template <typename T>
	static void _reserve(hkMemoryAllocator* mem, hkArrayBase<T>* array, int32_t numElem, int32_t sizeElem) {
		_reserve(*mem, array, numElem, sizeElem);
	}

	// Grows an array to twice its size, or to a single element when it is empty, and returns its new capacity. array is
	// any hkArrayBase<T>, passed as void*.
	static int32_t _reserveMore(hkMemoryAllocator& mem, void* array, int32_t sizeElem);
	// Shrinks an array's storage to aiNumElems elements, moving them to apDest when one is given. apArray is any
	// hkArrayBase<T>, as in the SDK (void* array).
	static void _reduce(hkMemoryAllocator& mem, void* array, int32_t sizeElem, char* inplaceMem,
		int32_t requestedCapacity);

	// Destroys n elements at t; the class version, an empty loop for elements without a destructor. Each instantiation
	// in use gets an explicit specialization.
	template <typename T> static void destruct(T* t, int32_t n, hkTraitBool<0>);
	// Copy-constructs n elements at t from tcopy; the class version.
	template <typename T> static void construct(T* t, int32_t n, const T& tcopy, hkTraitBool<0>);

	// The plain-data versions of destruct and construct: empty.
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

	// The class versions of construct, constructWithCopy and constructWithArray (and destruct above) run each element's
	// C++ constructor or destructor. Where an element type's class has none (it uses static Construct/Destruct
	// instead), that type gets an explicit specialization and any other use fails to link; otherwise the generic bodies
	// follow the class.
	template <typename T> static void construct(T* t, int32_t n, hkTypeIsClass);
	template <typename T> static void constructWithCopy(T* t, int32_t n, const T& tcopy, hkTypeIsClass);
	template <typename T> static void constructWithArray(T* t, int32_t n, const T* tcopy, hkTypeIsClass);
};

// The class versions: each element's own constructor, copy constructor or destructor is called.
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
