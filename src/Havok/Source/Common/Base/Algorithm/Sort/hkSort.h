#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

class hkShapeKeyPair;
class hkpCollidable;
class hkpEntity;
struct hkpMoppCompilerPrimitive;
struct hkpRootCdPoint;
class hkpSimulationIsland;
struct hkpWorldRayCastOutput;

namespace hkAlgorithm {

// Orders two elements with their operator<. The result is Havok's hkBool32, a plain int: the exe's copy for
// hkpRootCdPoint (0x9B3080) zero-extends its byte into all of eax.
template <typename T> class less {
public:
	int32_t operator()(const T& a, const T& b) { return a < b; }
};

// quickSortRecursive's comparison: the comparator's own result.
template <typename L, typename A, typename B> inline decltype(auto) lessThan(L& cmpLess, A& a, B& b) {
	return cmpLess(a, b);
}

// The exe's comparators that return their hkBool through a hidden result pointer (the free-list allocator's
// _comparePointers, the island and entity comparators): the sort passes one and reads the result it points to.
template <typename R, typename P, typename A, typename B> inline R lessThan(R* (*&cmpLess)(R*, P, P), A& a, B& b) {
	R result;
	return *cmpLess(&result, a, b);
}

// Sorts pArr[d..h] in place (inclusive bounds): Hoare partition around the middle element, recursing on the left part
// and looping on the right one. The exe compiles one copy per element type and comparator (cdecl).
template <typename T, typename L> void quickSortRecursive(T* pArr, int32_t d, int32_t h, L cmpLess) {
	for (;;) {
		int32_t i = d;
		int32_t j = h;
		const T str = pArr[(d + h) >> 1];
		do {
			// Natively the hidden-result comparators are called with a result slot (lessThan above); the hybrid's
			// instances with one are the exe's own copies, declared below.
			while (lessThan(cmpLess, pArr[i], str)) {
				i++;
			}
			while (lessThan(cmpLess, str, pArr[j])) {
				j--;
			}
			if (i > j) {
				break;
			}
			if (i != j) {
				T tmp = pArr[i];
				pArr[i] = pArr[j];
				pArr[j] = tmp;
			}
			i++;
			j--;
		} while (i <= j);
		if (d < j) {
			quickSortRecursive(pArr, d, j, cmpLess);
		}
		if (i >= h) {
			return;
		}
		d = i;
	}
}
// The SDK's quickSortRecursive as the exe compiled it for the phantoms' collidables, with hkpAabbPhantom::OrderByUid
// inlined: sorts apArr[aiLow..aiHigh] in place (inclusive bounds) by broad-phase id (cdecl, ret 0, 0xCF2ED0). The
// comparator (an empty class) travels in a 4-byte stack slot of which the callers write only the low byte, so it is
// that byte here: an empty class argument carries no bytes in C++. eax is left holding aiHigh.
// hkpSimpleShapePhantom's details (one collidable pointer each) use the same copy: its overload is declared in
// hkpSimpleShapePhantom.h.
int32_t quickSort(hkpCollidable** apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);

// The SDK's quickSortRecursive<hkShapeKeyPair, hkAlgorithm::less<hkShapeKeyPair>> as the exe compiled it, with the
// pair's operator< inlined: sorts apArr[aiLow..aiHigh] in place (inclusive bounds) by key A, then key B (cdecl, ret 0,
// 0xD36520). The comparator travels in a 4-byte stack slot (the empty class and its padding; the caller writes only
// its low byte) that the recursive call passes on whole, so it is a word here. eax is left holding the last right
// index, or the last recursive call's eax.
int32_t quickSortRecursive(hkShapeKeyPair* apArr, int32_t aiLow, int32_t aiHigh, int32_t aiLess);

// The SDK's quickSortRecursive<hkpMoppCompilerPrimitive, hkAlgorithm::less<hkpMoppCompilerPrimitive>> as the exe
// compiled it, with the primitive's operator< (its extent's minimum) inlined: sorts apArr[aiLow..aiHigh] in place
// (inclusive bounds) by the extents' minimum (cdecl, ret 0, 0xE0AB60). The comparator (an empty class) travels in a
// 4-byte stack slot of which the caller writes only the low byte, passed on to the recursive call, so it is that byte
// here, as quickSort's aiLess: an empty class argument carries no bytes in C++. Defined in hkpMoppDefaultSplitter.cpp.
void quickSortRecursive(hkpMoppCompilerPrimitive* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);

// The SDK's quickSortRecursive<hkpWorldRayCastOutput, hkAlgorithm::less<hkpWorldRayCastOutput>> as the exe compiled
// it, with the hit's operator< (its m_hitFraction) inlined: sorts apArr[aiLow..aiHigh] in place (inclusive bounds) by
// hit fraction (cdecl, ret 0, 0xC852C0). The comparator (an empty class) travels in a 4-byte stack slot of which the
// caller writes only the low byte, passed on to the recursive call, so it is that byte here, as quickSort's aiLess: an
// empty class argument carries no bytes in C++. Defined in hkpAllRayHitCollector.cpp, as in the PDB.
void quickSortRecursive(hkpWorldRayCastOutput* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);

// The SDK's quickSortRecursive<hkReal, hkAlgorithm::less<hkReal>> as the exe compiled it: sorts apArr[aiLow..aiHigh]
// in place (inclusive bounds), ascending (cdecl, ret 0, 0xE50F10). The comparator (an empty class) travels in a 4-byte
// stack slot of which the caller writes only the low byte, passed on to the recursive call, so it is that byte here,
// as quickSort's aiLess. Defined in hkaWavelet.cpp, as in the PDB.
void quickSortRecursive(float* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess);

} // namespace hkAlgorithm
